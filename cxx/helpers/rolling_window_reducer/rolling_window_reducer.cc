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
#include "cxx/internal/utils/cleanup.h"
#include "helpers/cxx/status/canonical_errors.h"
#include "helpers/cxx/status/status.h"
#include "helpers/cxx/status/statusor.h"
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

Status ProcessFileData(
    absl::string_view file_path,
    const std::vector<std::unique_ptr<RollingWindowReducer>>& reducers,
    FileIO* file_io) {
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
      for (std::size_t i = 0; i < reducers.size(); i++) {
        const Status status = reducers[i]->AddPoints(points_to_process);
        if (!status.ok()) {
          return Annotate(status,
                          absl::StrFormat("adding points to reducer %d", i));
        }
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
    for (std::size_t i = 0; i < reducers.size(); i++) {
      const Status status = reducers[i]->AddPoints(points_to_process);
      if (!status.ok()) {
        return Annotate(status,
                        absl::StrFormat("adding points to reducer %d", i));
      }
    }
  }

  return OkStatus();
}
}  // namespace

StatusOr<google::protobuf::RepeatedPtrField<SamplePoint>>
RollingWindowReducer::ReduceImpl(absl::Span<absl::string_view> file_paths,
                                 const std::vector<RWRConfig>& configs,
                                 FileIO* file_io) {
  if (file_io == nullptr) {
    return InvalidArgumentError("FileIO pointer invalid.");
  }

  // Create RWR instances for each RWRConfig in configs
  std::vector<std::unique_ptr<RollingWindowReducer>> reducers;
  for (std::size_t i = 0; i < configs.size(); i++) {
    auto reducer_or = RollingWindowReducer::New(configs[i]);
    if (!reducer_or.ok()) {
      return Annotate(reducer_or.status(),
                      absl::StrFormat("creating reducer for config %d", i));
    }

    reducers.push_back(std::move(reducer_or).value());
  }

  // Loop through all files
  for (auto file_path : file_paths) {
    if (!file_io->Open(std::string(file_path), FileIO::AccessMode::kRead)) {
      return Annotate(UnknownError(file_io->Error()), "opening file");
    }

    const auto close_file =
        mako::internal::MakeCleanup([file_io] { file_io->Close(); });

    const Status status = ProcessFileData(file_path, reducers, file_io);
    if (!status.ok()) {
      return Annotate(status, absl::StrFormat("processing file: %s",
                                              file_path));
    }
  }

  // Collect resulting data from each RWR instance
  google::protobuf::RepeatedPtrField<SamplePoint> output;
  for (std::size_t i = 0; i < reducers.size(); i++) {
    const Status status = reducers[i]->CompleteImpl(&output);
    if (!status.ok()) {
      return Annotate(status, absl::StrFormat("completing reducer %d", i));
    }
  }

  return output;
}


static mako::internal::Random* GetRandom() {
  static mako::internal::Random* random = new mako::internal::Random;
  return random;
}

StatusOr<std::unique_ptr<RollingWindowReducer>> RollingWindowReducer::New(
    const RWRConfig& config) {
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

  int max_sample_size = 0;

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

    if (config.has_max_sample_size()) {
      max_sample_size = config.max_sample_size();
    } else {
      max_sample_size = -1;
    }
  }
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

  // absl::make_unique can't call a private constructor.
  return absl::WrapUnique(new RollingWindowReducer(
      config, max_sample_size, max_sample_size > 0 ? GetRandom() : nullptr,
      std::move(error_matcher)));
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

Status RollingWindowReducer::AddPoints(const RWRAddPointsInput& input) {
  if (window_operation_ == RWRConfig::ERROR_COUNT) {
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
      // Add the point to all relevant windows if it contains the metric
      // value key we are interested in
      for (const auto& metric_value : point.metric_value_list()) {
        if (input_metric_keys_.count(metric_value.value_key()) ||
            (window_operation_ == RWRConfig::RATIO_SUM &&
             denominator_input_metric_keys_.count(metric_value.value_key()))) {
          AddPointToAllContainingWindows(point.input_value(),
                                         metric_value.value(),
                                         metric_value.value_key());
        }
      }
    }
  }

  return OkStatus();
}

bool RollingWindowReducer::IsMatch(absl::string_view error_message) {
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

  // No point processed, return an empty list
  if (window_data_.empty()) {
    return OkStatus();
  }

  if (zero_for_empty_window_) {
    // Loop through all possible windows locations
    for (int curr_window_index = min_window_index_;
         curr_window_index <= max_window_index_; curr_window_index++) {
      // Get window's value
      double value_to_set = 0;
      const auto window_iter = window_data_.find(curr_window_index);
      if (window_iter != window_data_.end()) {
        value_to_set =
            window_iter->second.GetWindowValue(window_operation_, percentile_);
      }
      if (window_operation_ == RWRConfig::RATIO_SUM) {
        const auto denominator_iter =
            window_data_denominator_.find(curr_window_index);
        if (denominator_iter != window_data_denominator_.end()) {
          double denominator = denominator_iter->second.GetWindowValue(
              window_operation_, percentile_);
          value_to_set /= denominator;
        } else {
          value_to_set = 0;
        }
      }

      // Create sample point at middle of window
      SamplePoint* sp = output->Add();
      sp->set_input_value(WindowLocation(curr_window_index));
      KeyedValue* kv = sp->add_metric_value_list();
      kv->set_value_key(output_metric_key_);
      kv->set_value(value_to_set);
    }
  } else {
    // Only loop through all windows in window_data_ map
    for (auto& window : window_data_) {
      int curr_window_index = window.first;

      // If computing RATIO_SUM, then only use the data point if both numerator
      // and denominator windows contain valid data in the current window.
      if (window_operation_ == RWRConfig::RATIO_SUM &&
          window_data_denominator_.find(curr_window_index) ==
              window_data_denominator_.end()) {
        continue;
      }

      double value_to_set =
          window.second.GetWindowValue(window_operation_, percentile_);
      if (window_operation_ == RWRConfig::RATIO_SUM) {
        const auto denominator_iter =
            window_data_denominator_.find(curr_window_index);
        double denominator = denominator_iter->second.GetWindowValue(
            window_operation_, percentile_);
        value_to_set /= denominator;
      }

      //  Create sample point at window location
      SamplePoint* sp = output->Add();
      sp->set_input_value(WindowLocation(curr_window_index));
      KeyedValue* kv = sp->add_metric_value_list();
      kv->set_value_key(output_metric_key_);
      kv->set_value(value_to_set);
    }
  }

  return OkStatus();
}

double RollingWindowReducer::HighestWindowLoc(double value) {
  return std::floor((value + window_size_ / 2.0) / step_size_) * step_size_;
}

int RollingWindowReducer::WindowIndex(double value) {
  return static_cast<int>(std::lround((value - base_window_loc_) / step_size_));
}

double RollingWindowReducer::WindowLocation(int window_index) {
  return window_index * step_size_ + base_window_loc_;
}

void RollingWindowReducer::AddPointToAllContainingWindows(
    double x_val, double y_val, const std::string& value_key) {
  auto index_bounds = updateWindowIndexBounds(x_val);

  std::vector<absl::node_hash_map<int, WindowDataProcessor>*>
      window_data_selectors;
  if (input_metric_keys_.count(value_key)) {
    window_data_selectors.push_back(&window_data_);
  }
  if (window_operation_ == RWRConfig::RATIO_SUM &&
      denominator_input_metric_keys_.count(value_key)) {
    window_data_selectors.push_back(&window_data_denominator_);
  }
  for (auto& window_data_selector : window_data_selectors) {
    // Give point to window's DataProcessor
    for (int curr_window_index = index_bounds.low;
         curr_window_index <= index_bounds.high; curr_window_index++) {
      auto insert_result = window_data_selector->insert(
          {curr_window_index, WindowDataProcessor(running_stats_config_)});
      insert_result.first->second.AddPoint(y_val);
    }
  }
}

void RollingWindowReducer::AddErrorToAllContainingWindows(double x_val) {
  auto index_bounds = updateWindowIndexBounds(x_val);

  // Give point to window's DataProcessor
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
RollingWindowReducer::WindowIndexBounds
RollingWindowReducer::updateWindowIndexBounds(double sample_x_val) {
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

void RollingWindowReducer::WindowDataProcessor::AddPoint(double point) {
  running_stats_.Add(point);
}

void RollingWindowReducer::WindowDataProcessor::AddError() { ++error_count_; }

double RollingWindowReducer::WindowDataProcessor::GetWindowValue(
    RWRConfig_WindowOperation window_operation, double percent) {
  RunningStats::Result result;

  switch (window_operation) {
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
      result = running_stats_.Percentile(percent);
      break;
    default:
      LOG(FATAL) << "Unknown window operation type: "
                 << RWRConfig_WindowOperation_Name(window_operation);
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
