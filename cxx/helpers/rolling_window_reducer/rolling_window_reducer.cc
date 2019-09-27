// Copyright 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "cxx/helpers/rolling_window_reducer/rolling_window_reducer.h"

#include <cmath>
#include <cstddef>
#include <utility>

#include "glog/logging.h"
#include "src/google/protobuf/repeated_field.h"
#include "absl/container/node_hash_map.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_format.h"
#include "absl/types/span.h"
#include "cxx/helpers/status/canonical_errors.h"
#include "cxx/helpers/status/status.h"
#include "cxx/helpers/status/statusor.h"
#include "cxx/internal/utils/cleanup.h"
#include "proto/helpers/rolling_window_reducer/rolling_window_reducer.pb.h"
#include "spec/proto/mako.pb.h"

namespace mako {
namespace helpers {
namespace {
using mako::internal::RunningStats;

// Reduce function buffer size.
// Maximum number of metric values per AddPoint call.
constexpr int kMaxBufferSize = 100000;

constexpr char kNoError[] = "";

static std::string ToMakoError(const Status& status) {
  return status.ok() ? kNoError : StatusToString(status);
}

Status ProcessFileData(absl::string_view file_path,
                       RollingWindowReducer* reducer, FileIO* file_io) {
  RWRAddPointsInput points_to_process;
  int buffer_size = 0;

  // Add each sample point in file to points_to_process
  SampleRecord sample_record;
  while (file_io->Read(&sample_record)) {
    if (sample_record.has_sample_point()) {
      *points_to_process.add_point_list() = sample_record.sample_point();
      buffer_size += sample_record.sample_point().metric_value_list_size();
    }
    if (sample_record.has_sample_error()) {
      *points_to_process.add_error_list() = sample_record.sample_error();
      buffer_size += sample_record.sample_error().ByteSizeLong();
    }

    // Once the points_to_process buffer is filled, call AddPoints
    // for each RWR instance and reset the buffer
    if (buffer_size >= kMaxBufferSize) {
      VLOG(1) << "Size of AddPointsInput Proto to be passed for file "
              << file_path << ": "
              << points_to_process.ByteSizeLong() << " bytes";

      const Status status = reducer->AddPoints(points_to_process);
      if (!status.ok()) {
        return status;
      }
      points_to_process.Clear();
      buffer_size = 0;
    }
  }

  // Make sure read return false to indicate EOF
  if (!file_io->ReadEOF()) {
    return Annotate(UnknownError(file_io->Error()), "ReadEOF");
  }

  // If any points in buffer, add them to each reducer instance
  if (buffer_size != 0) {
    VLOG(1) << "Size of AddPointsInput Proto to be passed for file "
            << file_path << ": "
            << points_to_process.ByteSizeLong() << " bytes";
    const Status status = reducer->AddPoints(points_to_process);
    if (!status.ok()) {
      return status;
    }
  }

  return OkStatus();
}

static mako::internal::Random* GetRandom() {
  static mako::internal::Random* random = new mako::internal::Random;
  return random;
}

std::set<std::string> ToStringSet(const google::protobuf::RepeatedPtrField<std::string>& list) {
  std::set<std::string> set;
  for (const auto& key : list) {
    set.insert(key);
  }
  return set;
}

int EffectiveMaxSampleSize(const RWRConfig& config) {
  if (config.window_operation() != RWRConfig::PERCENTILE) {
    return 0;  // do not keep samples unless percentile
  } else if (config.has_max_sample_size()) {
    return config.max_sample_size();
  } else {
    return -1;  // no maximum size
  }
}

// Note: Doesn't check if 'error_matcher' compiles to a valid RE2.
Status ValidateConfig(const RWRConfig& config) {
  // Validate config protobuf
  if (config.window_operation() == RWRConfig::ERROR_COUNT &&
      config.input_metric_keys_size() > 0) {
    return InvalidArgumentError(
        "RWRConfig should not have any input_metric_keys when using an "
        "error (ERROR_*) window operation.");
  }

  if (config.window_operation() == RWRConfig::ERROR_COUNT &&
      config.error_sampler_name_inputs_size() == 0) {
    return InvalidArgumentError(
        "RWRConfig must provide at least one error_sampler_name_inputs when "
        "using an error (ERROR_*) window operation.");
  }

  if (config.window_operation() != RWRConfig::ERROR_COUNT &&
      config.error_sampler_name_inputs_size() > 0) {
    return InvalidArgumentError(
        "RWRConfig should not have any error_sampler_name_inputs "
        "when using a non-error (!= ERROR_*) window operation.");
  }

  if (config.window_operation() != RWRConfig::ERROR_COUNT &&
      config.input_metric_keys_size() == 0) {
    return InvalidArgumentError(
        "RWRConfig must specify at least one input_metric_key.");
  }

  if (config.window_operation() == RWRConfig::RATIO_SUM &&
      config.denominator_input_metric_keys_size() == 0) {
    return InvalidArgumentError(
        "RWRConfig must provide at least one denominator_input_metric_keys "
        "when using a ratio (RATIO_*) window operation.");
  }

  if (config.window_operation() != RWRConfig::RATIO_SUM &&
      config.denominator_input_metric_keys_size() > 0) {
    return InvalidArgumentError(
        "RWRConfig should not have any denominator_input_metric_keys "
        "when using a non-ratio (!= RATIO_*) window operation.");
  }

  if (!config.has_output_metric_key() || config.output_metric_key().empty()) {
    return InvalidArgumentError("RWRConfig must specify output_metric_key.");
  }

  if (!config.has_window_operation()) {
    return InvalidArgumentError("RWRConfig must specify window_operation.");
  }

  if (!config.has_window_size() || config.window_size() <= 0) {
    return InvalidArgumentError("RWRConfig must specify window_size.");
  }

  // proto contains a default value.
  if (config.steps_per_window() <= 0) {
    return InvalidArgumentError("RWRConfig must specify steps_per_window.");
  }

  if (!config.has_zero_for_empty_window()) {
    return InvalidArgumentError(
        "RWRConfig must specify zero_for_empty_window.");
  }
  if (config.output_scaling_factor() == 0) {
    return InvalidArgumentError(
        "RWRConfig cannot have output_scaling_factor=0.");
  }

  if (config.window_operation() == RWRConfig::PERCENTILE) {
    if (!config.has_percentile_milli()) {
      return InvalidArgumentError(
          "RWRConfig must specify percentile_milli when window_operation is "
          "PERCENTILE.");
    }

    if (config.percentile_milli() <= 0) {
      return InvalidArgumentError(
          "RWRConfig should specify percentile_milli that is >= 0");
    }
  }

  return OkStatus();
}

// Preserve test expectations for denominator=zero.
// TODO(b/141503378): Fix behavior and remove this workaround.
double DivByZero(double numerator) {
  if (numerator == 0) {
    return std::nan("");
  } else {
    return numerator > 0 ? std::numeric_limits<double>::infinity()
                         : -std::numeric_limits<double>::infinity();
  }
}

}  // namespace

StatusOr<google::protobuf::RepeatedPtrField<SamplePoint>>
RollingWindowReducer::ReduceImpl(absl::Span<absl::string_view> file_paths,
                                 const std::vector<RWRConfig>& configs,
                                 FileIO* file_io) {
  if (file_io == nullptr) {
    return InvalidArgumentError("FileIO pointer invalid.");
  }

  auto reducer_or = RollingWindowReducer::NewMerged(configs);
  if (!reducer_or.ok()) {
    return reducer_or.status();
  }
  RollingWindowReducer* reducer = reducer_or.value().get();

  // Loop through all files
  for (auto file_path : file_paths) {
    if (!file_io->Open(std::string(file_path), FileIO::AccessMode::kRead)) {
      return Annotate(UnknownError(file_io->Error()), "opening file");
    }

    const auto close_file =
        mako::internal::MakeCleanup([file_io] { file_io->Close(); });

    const Status status = ProcessFileData(file_path, reducer, file_io);
    if (!status.ok()) {
      return Annotate(status, absl::StrFormat("processing file: %s",
                                              file_path));
    }
  }

  google::protobuf::RepeatedPtrField<SamplePoint> output;
  const Status status = reducer->CompleteImpl(&output);
  if (!status.ok()) {
    return status;
  }
  return output;
}

// static
StatusOr<std::unique_ptr<RollingWindowReducer>> RollingWindowReducer::New(
    const RWRConfig& config) {
  return NewMerged({config});
}

StatusOr<std::unique_ptr<RollingWindowReducer>> RollingWindowReducer::NewMerged(
    const std::vector<RWRConfig>& configs) {
  std::unique_ptr<RollingWindowReducer> reducer(new RollingWindowReducer);
  for (std::size_t i = 0; i < configs.size(); i++) {
    auto status = reducer->AddConfig(configs[i]);
    if (!status.ok()) {
      return Annotate(status,
                      absl::StrFormat("creating reducer for config %d", i));
    }
  }
  return reducer;
}

Status RollingWindowReducer::AddConfig(const RWRConfig& config) {
  auto validation = ValidateConfig(config);
  if (!validation.ok()) {
    return validation;
  }
  // Try to merge the config into each existing subreducer. The number of
  // RWRConfigs will be tiny compared to the number of input points, so it
  // shouldn't matter if we do a naive O(n^2) match.
  for (auto& subreducer : subreducers_) {
    if (subreducer->TryMergeSimilarConfig(config)) {
      VLOG(1) << "Merging config: " << config.ShortDebugString();
      return OkStatus();
    }
  }

  VLOG(1) << "Creating new subreducer: " << config.ShortDebugString();
  int max_sample_size = EffectiveMaxSampleSize(config);
  std::unique_ptr<RE2> error_matcher;
  if (config.window_operation() == RWRConfig::ERROR_COUNT &&
      !config.error_matcher().empty()) {
    error_matcher = absl::make_unique<RE2>(config.error_matcher());
    if (!error_matcher->ok()) {
      return InvalidArgumentError(
          "Compiling error_matcher returned an error: " +
          error_matcher->error());
    }
  }
  subreducers_.push_back(absl::make_unique<Subreducer>(
      config, max_sample_size, std::move(error_matcher)));
  return OkStatus();
}

// static
std::unique_ptr<RollingWindowReducer> RollingWindowReducer::Create(
    const RWRConfig& config) {
  auto reducer_or = New(config);
  if (!reducer_or.ok()) {
    LOG(ERROR) << "Failed to create reducer: " << reducer_or.status();
    return nullptr;
  }

  return std::move(reducer_or.value());
}

RollingWindowReducer::Subreducer::Subreducer(const RWRConfig& config,
                                             int max_sample_size,
                                             std::unique_ptr<RE2> error_matcher)
    : window_size_(config.window_size()),
      step_size_(config.window_size() / config.steps_per_window()),
      steps_per_window_(config.steps_per_window()),
      base_window_loc_(std::numeric_limits<double>::max()),
      min_window_index_(std::numeric_limits<int>::max()),
      max_window_index_(std::numeric_limits<int>::min()),
      error_matcher_(std::move(error_matcher)) {
  input_metric_keys_ = ToStringSet(config.input_metric_keys());
  denominator_input_metric_keys_ =
      ToStringSet(config.denominator_input_metric_keys());
  error_sampler_name_inputs_ = ToStringSet(config.error_sampler_name_inputs());

  running_stats_config_.max_sample_size = max_sample_size;
  if (max_sample_size > 0) {
    running_stats_config_.random = GetRandom();
  }
  output_configs_.push_back(OutputConfig(config));
}

bool RollingWindowReducer::Subreducer::TryMergeSimilarConfig(
    const RWRConfig& other_config) {
  if (other_config.window_size() != window_size_ ||
      other_config.steps_per_window() != steps_per_window_) {
    VLOG(3) << " cannot merge: different window sizes";
    return false;
  }
  if (error_matcher_ || !other_config.error_matcher().empty()) {
    VLOG(3) << " cannot merge: config has error matcher regex";
    return false;
  }
  if (input_metric_keys_ != ToStringSet(other_config.input_metric_keys())) {
    VLOG(3) << " cannot merge: different input_metric_keys";
    return false;
  }
  if (denominator_input_metric_keys_ !=
      ToStringSet(other_config.denominator_input_metric_keys())) {
    VLOG(3) << " cannot merge: different denominator_input_metric_keys";
    return false;
  }
  if (error_sampler_name_inputs_ !=
      ToStringSet(other_config.error_sampler_name_inputs())) {
    VLOG(3) << " cannot merge: different error_sampler_name_inputs";
    return false;
  }

  VLOG(2) << "All input fields match, adding output metric to reducer";

  output_configs_.push_back(OutputConfig(other_config));

  // Increase max_sample_size to match the new config, if needed.
  int other_max_sample_size = EffectiveMaxSampleSize(other_config);
  if (other_max_sample_size < 0) {
    running_stats_config_.max_sample_size = -1;
  } else if (other_max_sample_size > running_stats_config_.max_sample_size) {
    running_stats_config_.max_sample_size = other_max_sample_size;
    if (running_stats_config_.random == nullptr) {
      running_stats_config_.random = GetRandom();
    }
  }

  return true;
}

Status RollingWindowReducer::AddPoints(const RWRAddPointsInput& input) {
  for (auto& subreducer : subreducers_) {
    subreducer->AddPoints(input);
  }
  return OkStatus();
}

void RollingWindowReducer::Subreducer::AddPoints(
    const RWRAddPointsInput& input) {
  // Since the config(s) passed validation, input_metric_keys_ is empty iff the
  // window_operation is ERROR_COUNT.
  if (input_metric_keys_.empty()) {
    // Loop through all SampleErrors
    for (const auto& error : input.error_list()) {
      if (error_sampler_name_inputs_.count(error.sampler_name()) &&
          IsMatch(error.error_message())) {
        AddErrorToAllContainingWindows(error.input_value());
      }
    }
  } else {
    // Loop through all SamplePoints
    for (const auto& point : input.point_list()) {
      // Add the point to all relevant windows if it contains a metric value key
      // we are interested in
      for (const auto& metric_value : point.metric_value_list()) {
          AddPointToAllContainingWindows(point.input_value(),
                                         metric_value.value(),
                                         metric_value.value_key());
      }
    }
  }
}

bool RollingWindowReducer::Subreducer::IsMatch(
    absl::string_view error_message) {
  return !error_matcher_ || RE2::PartialMatch(error_message, *error_matcher_);
}

Status RollingWindowReducer::Complete(
    mako::helpers::RWRCompleteOutput* output) {
  if (output == nullptr) {
    return InvalidArgumentError("RWRCompleteOutput pointer invalid.");
  }
  return CompleteImpl(output->mutable_point_list());
}

Status RollingWindowReducer::CompleteImpl(
    google::protobuf::RepeatedPtrField<SamplePoint>* output) {
  if (output == nullptr) {
    return InvalidArgumentError("RepeatedPtrField pointer invalid.");
  }
  for (auto& subreducer : subreducers_) {
    subreducer->Complete(output);
  }
  return OkStatus();
}

void RollingWindowReducer::Subreducer::AppendOutputPointsForWindow(
    int window_index, WindowDataProcessor* window,
    google::protobuf::RepeatedPtrField<SamplePoint>* output) {
  for (const auto& output_config : output_configs_) {
    if (!window && !output_config.zero_for_empty_window) {
      continue;
    }

    double value_to_set = window ? window->GetWindowValue(output_config) : 0;

    if (output_config.window_operation == RWRConfig::RATIO_SUM) {
      auto& denominator_windows = window_data_denominator_;
      auto iter = denominator_windows.find(window_index);
      if (iter != denominator_windows.end()) {
        double denominator = iter->second.GetWindowValue(output_config);
        // The tests check for "proper" div-by-zero behavior; this is a bug, but
        // for now we preserve existing behavior.
        // Note that this is inconsistent with zero_for_empty_window.
        // TODO(b/141503378): Fix this.
        if (denominator == 0) {
          value_to_set = DivByZero(value_to_set);
        } else {
          value_to_set /= denominator;
        }
      } else if (!output_config.zero_for_empty_window) {
        continue;
      } else {
        value_to_set = 0;
      }
    }
    value_to_set *= output_config.scaling_factor;

    // Create sample point at middle of window
    SamplePoint* sp = output->Add();
    sp->set_input_value(WindowLocation(window_index));
    KeyedValue* kv = sp->add_metric_value_list();
    kv->set_value_key(output_config.metric_key);
    kv->set_value(value_to_set);
  }
}

void RollingWindowReducer::Subreducer::Complete(
    google::protobuf::RepeatedPtrField<SamplePoint>* output) {
  // No point processed, return an empty list
  if (window_data_.empty()) {
    return;
  }

  bool care_about_empty_windows = false;
  for (const auto& config : output_configs_) {
    care_about_empty_windows |= config.zero_for_empty_window;
  }

  if (care_about_empty_windows) {
    // Loop through all possible windows locations
    for (int curr_window_index = min_window_index_;
         curr_window_index <= max_window_index_; curr_window_index++) {
      const auto window_iter = window_data_.find(curr_window_index);
      WindowDataProcessor* window =
          window_iter != window_data_.end() ? &window_iter->second : nullptr;
      AppendOutputPointsForWindow(curr_window_index, window, output);
    }
  } else {
    // Only loop through all windows in window_data_ map
    for (auto& indexed_window : window_data_) {
      int curr_window_index = indexed_window.first;
      WindowDataProcessor* window = &indexed_window.second;
      AppendOutputPointsForWindow(curr_window_index, window, output);
    }
  }
}

double RollingWindowReducer::Subreducer::HighestWindowLoc(double value) {
  return std::floor((value + window_size_ / 2.0) / step_size_) * step_size_;
}

int RollingWindowReducer::Subreducer::WindowIndex(double value) {
  return static_cast<int>(std::lround((value - base_window_loc_) / step_size_));
}

double RollingWindowReducer::Subreducer::WindowLocation(int window_index) {
  return window_index * step_size_ + base_window_loc_;
}

void RollingWindowReducer::Subreducer::AddPointToAllContainingWindows(
    double x_val, double y_val, const std::string& value_key) {
  const bool is_primary_metric = input_metric_keys_.count(value_key);
  const bool is_denominator_metric =
      denominator_input_metric_keys_.count(value_key);

  if (!is_primary_metric && !is_denominator_metric) return;

  auto index_bounds = UpdateWindowIndexBounds(x_val);

  for (int curr_window_index = index_bounds.low;
       curr_window_index <= index_bounds.high; curr_window_index++) {
    if (is_primary_metric) {
      auto insert_result = window_data_.insert(
          {curr_window_index, WindowDataProcessor(running_stats_config_)});
      insert_result.first->second.AddPoint(y_val);
    }
    if (is_denominator_metric) {
      auto insert_result = window_data_denominator_.insert(
          {curr_window_index, WindowDataProcessor(running_stats_config_)});
      insert_result.first->second.AddPoint(y_val);
    }
  }
}

void RollingWindowReducer::Subreducer::AddErrorToAllContainingWindows(
    double x_val) {
  auto index_bounds = UpdateWindowIndexBounds(x_val);

  for (int curr_window_index = index_bounds.low;
       curr_window_index <= index_bounds.high; curr_window_index++) {
    auto insert_result = window_data_.insert(
        {curr_window_index, WindowDataProcessor(running_stats_config_)});
    insert_result.first->second.AddError();
  }
}

// Updates the base window index and window bounds with the sample X value. It
// then returns the indices of the lower and upper windows that contain the X
// value.
RollingWindowReducer::Subreducer::WindowIndexBounds
RollingWindowReducer::Subreducer::UpdateWindowIndexBounds(double sample_x_val) {
  // Take note of the first point we encounter, this will serve as the
  // base window location (all future points will be stored relative to
  // this location)
  if (base_window_loc_ == std::numeric_limits<double>::max()) {
    base_window_loc_ = HighestWindowLoc(sample_x_val);
  }

  // Get the index of the highest and lowest window this point belongs to
  double high_window_loc = HighestWindowLoc(sample_x_val);
  double low_window_loc = high_window_loc - window_size_;
  WindowIndexBounds bounds;
  bounds.high = WindowIndex(high_window_loc);
  // Get the lowest window index this point belongs to
  // + 1 because we are (...range...] and we want to exclude the leftmost
  // window.
  bounds.low = WindowIndex(low_window_loc) + 1;

  // update min and max window indices so that we can easily loop through all
  // windows in the future
  if (bounds.high > max_window_index_) {
    max_window_index_ = bounds.high;
  }
  if (bounds.low < min_window_index_) {
    min_window_index_ = bounds.low;
  }

  // return the lower and upper windows that contain the point in question
  return bounds;
}

RollingWindowReducer::WindowDataProcessor::WindowDataProcessor(
    const RunningStats::Config& config)
    : running_stats_(config), error_count_(0) {}

double RollingWindowReducer::WindowDataProcessor::GetWindowValue(
    const OutputConfig& output_config) {
  RunningStats::Result result;

  switch (output_config.window_operation) {
    case RWRConfig::RATIO_SUM:
      // The ratio in RATIO_SUM is calculated outside of the individual
      // WindowDataProcessor value calculation, which only provides the SUM
      // values for both the numerator and denominator of the RATIO calculation.
      ABSL_FALLTHROUGH_INTENDED;
    case RWRConfig::SUM:
      result = running_stats_.Sum();
      break;
    case RWRConfig::MEAN:
      result = running_stats_.Mean();
      break;
    case RWRConfig::COUNT:
      result = running_stats_.Count();
      break;
    case RWRConfig::ERROR_COUNT:
      result.value = error_count_;
      break;
    case RWRConfig::PERCENTILE:
      result = running_stats_.Percentile(output_config.percentile);
      break;
    default:
      LOG(FATAL) << "Unknown window operation type: "
                 << RWRConfig_WindowOperation_Name(
                        output_config.window_operation);
      return 0;
  }

  if (!result.error.empty()) {
    LOG(WARNING) << "RunningStats::Result has error: " << result.error;
    return 0;
  }

  return result.value;
}

std::string RollingWindowReducer::StringAddPoints(
    const mako::helpers::RWRAddPointsInput& input) {
  return ToMakoError(AddPoints(input));
}

std::string RollingWindowReducer::StringComplete(
    mako::helpers::RWRCompleteOutput* output) {
  return ToMakoError(Complete(output));
}

}  // namespace helpers
}  // namespace mako
