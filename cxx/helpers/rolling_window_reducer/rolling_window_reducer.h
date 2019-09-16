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

// Mako Rolling Window Reducer.
//
// For detailed information on usage, see proto at:
// https://github.com/google/mako/blob/master/proto/helpers/rolling_window_reducer/rolling_window_reducer.proto

#ifndef CXX_HELPERS_ROLLING_WINDOW_REDUCER_ROLLING_WINDOW_REDUCER_H_
#define CXX_HELPERS_ROLLING_WINDOW_REDUCER_ROLLING_WINDOW_REDUCER_H_

#include <limits>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "src/google/protobuf/repeated_field.h"
#include "absl/container/node_hash_map.h"
#include "absl/strings/string_view.h"
#include "cxx/helpers/status/status.h"
#include "cxx/helpers/status/statusor.h"
#include "cxx/internal/pgmath.h"
#include "cxx/spec/fileio.h"
#include "proto/helpers/rolling_window_reducer/rolling_window_reducer.pb.h"
#include "spec/proto/mako.pb.h"
#include "re2/re2.h"

namespace mako {
namespace helpers {

class RollingWindowReducer {
 public:

  // Create a RollingWindowReducer based on the supplied config, returning an
  // error if the config is invalid.
  static StatusOr<std::unique_ptr<RollingWindowReducer> > New(
      const RWRConfig& config);

  // A legacy version of New that returns null on error, for use by SWIG. Please
  // use the above version elsewhere.
  static std::unique_ptr<RollingWindowReducer> Create(
      const mako::helpers::RWRConfig& config);

  // Called multiple times to process all data points
  // The RWRAddPointsInput protobuf should pack large numbers of SamplePoints
  // (as many as can fit in memory).
  Status AddPoints(const mako::helpers::RWRAddPointsInput& input);

  // Called once all points are processed
  // Fills in point_list with SamplePoints with:
  // x-val = middle of window
  // y-val = window values
  // metric key = output_metric_key
  Status Complete(mako::helpers::RWRCompleteOutput* output);

  // Exposed for SWIG which doesn't work with mako::helpers::Status
  std::string StringAddPoints(
      const mako::helpers::RWRAddPointsInput& input);
  std::string StringComplete(mako::helpers::RWRCompleteOutput* output);

 private:
  RollingWindowReducer(const RWRConfig& config, int max_sample_size,
                       mako::internal::Random* random,
                       std::unique_ptr<RE2> error_matcher)
      : output_metric_key_(config.output_metric_key()),
        window_operation_(config.window_operation()),
        window_size_(config.window_size()),
        step_size_(config.window_size() / config.steps_per_window()),
        steps_per_window_(config.steps_per_window()),
        zero_for_empty_window_(config.zero_for_empty_window()),
        max_sample_size_(max_sample_size),
        percentile_(config.percentile_milli() / 100000.0),
        base_window_loc_(std::numeric_limits<double>::max()),
        min_window_index_(std::numeric_limits<int>::max()),
        max_window_index_(std::numeric_limits<int>::min()),
        error_matcher_(std::move(error_matcher)) {
    for (const auto& key : config.input_metric_keys()) {
      input_metric_keys_.insert(key);
    }
    for (const auto& key : config.denominator_input_metric_keys()) {
      denominator_input_metric_keys_.insert(key);
    }
    for (const auto& key : config.error_sampler_name_inputs()) {
      error_sampler_name_inputs_.insert(key);
    }

    running_stats_config_.max_sample_size = max_sample_size_;
    running_stats_config_.random = random;
  }

  // Returns whether a sampler error message matches the error_matcher set in
  // the config. Will always return true if no error_matcher was set.
  bool IsMatch(absl::string_view error_message);

  // Returns the highest possible window location (window locations must
  // be evenly divisible by the step size) which would contain the point
  double HighestWindowLoc(double value);

  // The first point that is processed is used as the "base_window_loc_"
  // and has index 0. All future points are relative to this index.
  //
  // Returns an index for a window based on the window starting location.
  int WindowIndex(double value);

  static StatusOr<google::protobuf::RepeatedPtrField<SamplePoint> > ReduceImpl(
      absl::Span<absl::string_view> file_paths,
      const std::vector<RWRConfig>& configs, FileIO* file_io);
  Status CompleteImpl(google::protobuf::RepeatedPtrField<SamplePoint>* output);

  // Struct to store the low and high index bounds of a particular x value
  struct WindowIndexBounds {
    double low, high;
  };

  // Updates the base window index and window bounds with the sample X value. It
  // then returns the indices of the lower and upper windows that contain the X
  // value.
  WindowIndexBounds updateWindowIndexBounds(double sample_x_val);

  // Returns a window starting location based on the index.
  double WindowLocation(int window_index);

  // Given a point, this function gives the point to all relevant
  // WindowDataProcessors
  void AddPointToAllContainingWindows(double x_val, double y_val,
                                      const std::string& value_key);

  // Given an error, this function gives the error to all relevant
  // WindowDataProcessors
  void AddErrorToAllContainingWindows(double x_val);

  // Class to keep track of sum and count for each window. Provides the sum,
  // mean, or count "window value" depending on the user's WindowOperation
  // Important to keep this class's size small since an instance of it
  // will be created for each window
  class WindowDataProcessor {
   public:
    explicit WindowDataProcessor(
        const mako::internal::RunningStats::Config& config);

    // Add point to window
    void AddPoint(double point);

    // Add error to window
    void AddError();

    // Get current value of window.
    // percent only needed when operation is PERCENTILE.
    double GetWindowValue(RWRConfig_WindowOperation window_operation,
                          double percent);

   private:
    mako::internal::RunningStats running_stats_;
    int64_t error_count_;
  };

  // Construction parameters
  std::set<std::string> input_metric_keys_;
  // When computing window of a Ratio, remember which metric is the denominator.
  std::set<std::string> denominator_input_metric_keys_;
  // Sampler names to pull errors from
  std::set<std::string> error_sampler_name_inputs_;
  std::string output_metric_key_;
  RWRConfig_WindowOperation window_operation_;
  double window_size_;
  double step_size_;
  int steps_per_window_;
  bool zero_for_empty_window_;
  int max_sample_size_;
  double percentile_;   // only for percentile operation
  mako::internal::RunningStats::Config running_stats_config_;

  // Used to map window indices to window locations
  double base_window_loc_;

  // Useful in looping through all windows
  double min_window_index_;
  double max_window_index_;

  // Maps window index to window data (stored in WindowDataProcessor instances)
  absl::node_hash_map<int, WindowDataProcessor> window_data_;
  // For use when computing RATIO, keep a separate window data for the
  // denominator.
  absl::node_hash_map<int, WindowDataProcessor> window_data_denominator_;
  // Regex specified in RWRConfig.error_matcher. Used to match against error
  // text to determine if that error should be counted for ERROR_COUNT
  // operations.
  std::unique_ptr<RE2> error_matcher_;

  // friend function so that it can live in a separate visibility-restricted
  // build rule but still have access to private methods / fields.
  friend Status Reduce(
      absl::string_view file_path, const std::vector<RWRConfig>& configs,
      FileIO* file_io);
};


}  // namespace helpers
}  // namespace mako

#endif  // CXX_HELPERS_ROLLING_WINDOW_REDUCER_ROLLING_WINDOW_REDUCER_H_
