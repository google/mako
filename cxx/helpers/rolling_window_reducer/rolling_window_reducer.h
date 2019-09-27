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
#include <utility>
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

  // Create a merged RollingWindowReducer based on the supplied configs,
  // returning an error if any config is invalid. The returned reducer is
  // equivalent to having a collection of reducers for each individual config,
  // but may be more efficient.
  //
  // The following examples give the same result:
  //   auto rwr_merge = RollingWindowReducer::New({config1, config2});
  //   CHECK_OK(rwr_merge->AddPoints(input));
  //   CHECK_OK(rwr_merge->Complete(&output));
  // -- or --
  //   auto r1 = RWR::New(config1);     auto r2 = RWR::New(config2);
  //   CHECK_OK(r1->AddPoints(input));  CHECK_OK(r2->AddPoints(input));
  //   CHECK_OK(r1->Complete(&output)); CHECK_OK(r2->Complete(&output));
  static StatusOr<std::unique_ptr<RollingWindowReducer> > NewMerged(
      const std::vector<RWRConfig>& configs);

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
  RollingWindowReducer() {}

  // The CLIF Python adapter gets confused unless we explicitly delete these.
  // On the other hand, SWIG cannot parse the "= delete" syntax.
#ifndef SWIG
  RollingWindowReducer(const RollingWindowReducer&) = delete;
  RollingWindowReducer& operator=(const RollingWindowReducer&) = delete;
#endif  // #ifndef SWIG

  // Merge a config into a compatable subreducer, or create a new subreducer for
  // it. Returns an error if the config is not valid.
  Status AddConfig(const RWRConfig& config);

  static StatusOr<google::protobuf::RepeatedPtrField<SamplePoint> > ReduceImpl(
      absl::Span<absl::string_view> file_paths,
      const std::vector<RWRConfig>& configs, FileIO* file_io);
  Status CompleteImpl(google::protobuf::RepeatedPtrField<SamplePoint>* output);

  struct OutputConfig {
    explicit OutputConfig(const RWRConfig& config)
        : metric_key(config.output_metric_key()),
          window_operation(config.window_operation()),
          percentile(config.percentile_milli() / 100000.0),
          scaling_factor(config.output_scaling_factor()),
          zero_for_empty_window(config.zero_for_empty_window()) {}

    std::string metric_key;
    RWRConfig::WindowOperation window_operation;
    double percentile;  // only for percentile operation
    double scaling_factor;
    bool zero_for_empty_window;
  };

  // Class to keep track of metric values in a specific window.
  // Important to keep this class's size small since an instance of it
  // will be created for each window
  class WindowDataProcessor {
   public:
    explicit WindowDataProcessor(
        const mako::internal::RunningStats::Config& config);

    void AddPoint(double point) { running_stats_.Add(point); }
    void AddError() { ++error_count_; }

    // Get current value of window.
    double GetWindowValue(const OutputConfig& output_config);

   private:
    mako::internal::RunningStats running_stats_;
    int64_t error_count_;
  };

  // Each Subreducer keeps track of the windows for the metric(s) for a
  // particular RWRConfig. It may be the case that the Subreducers for two
  // configs would always contain the same data; we call these "similar", and
  // only create one object per set of similar configs.
  class Subreducer {
   public:
    Subreducer(const RWRConfig& config, int max_sample_size,
               std::unique_ptr<RE2> error_matcher);

    void AddPoints(const mako::helpers::RWRAddPointsInput& input);
    void Complete(google::protobuf::RepeatedPtrField<SamplePoint>* output);

    // Compare another config to this reducer's config. If the two configs are
    // "similar", merge other_config into this reducer and return true. If the
    // other config is not similar, does nothing and returns false.
    // May only be called before points are added.
    bool TryMergeSimilarConfig(const RWRConfig& other_config);

   private:
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

    struct WindowIndexBounds {
      int low, high;  // inclusive
    };

    // Updates the base window index and window bounds with the sample X value.
    // It then returns the indices of the lower and upper windows that contain
    // the X value.
    WindowIndexBounds UpdateWindowIndexBounds(double sample_x_val);

    // Returns a window starting location based on the index.
    double WindowLocation(int window_index);

    // Given a point, this function gives the point to all relevant
    // WindowDataProcessors. No-op if value_key is not an input_metric_key or
    // denominator_metric_key.
    void AddPointToAllContainingWindows(double x_val, double y_val,
                                        const std::string& value_key);

    // Given an error, this function gives the error to all relevant
    // WindowDataProcessors
    void AddErrorToAllContainingWindows(double x_val);

    // Append the SamplePoint(s) for the window at the given index. An empty
    // window has no WindowDataProcessor and is represented by null.
    void AppendOutputPointsForWindow(
        int window_index, WindowDataProcessor* window,
        google::protobuf::RepeatedPtrField<SamplePoint>* output);

    // Construction parameters
    std::set<std::string> input_metric_keys_;
    // When computing window of a Ratio, remember which metric is the
    // denominator.
    std::set<std::string> denominator_input_metric_keys_;
    // Sampler names to pull errors from
    std::set<std::string> error_sampler_name_inputs_;
    double window_size_;
    double step_size_;
    int steps_per_window_;
    mako::internal::RunningStats::Config running_stats_config_;
    std::vector<OutputConfig> output_configs_;

    // Used to map window indices to window locations
    double base_window_loc_;

    // Useful in looping through all windows
    int min_window_index_;
    int max_window_index_;

    // Maps window index to window data (stored in WindowDataProcessor
    // instances)
    absl::node_hash_map<int, WindowDataProcessor> window_data_;
    // For use when computing RATIO, keep a separate window data for the
    // denominator.
    absl::node_hash_map<int, WindowDataProcessor> window_data_denominator_;
    // Regex specified in RWRConfig.error_matcher. Used to match against error
    // text to determine if that error should be counted for ERROR_COUNT
    // operations.
    std::unique_ptr<RE2> error_matcher_;
  };

  // At least one subreducer, and at most one subreducer per config.
  std::vector<std::unique_ptr<Subreducer> > subreducers_;

  // friend function so that it can live in a separate visibility-restricted
  // build rule but still have access to private methods / fields.
  friend Status Reduce(
      absl::string_view file_path, const std::vector<RWRConfig>& configs,
      FileIO* file_io);
};


}  // namespace helpers
}  // namespace mako

#endif  // CXX_HELPERS_ROLLING_WINDOW_REDUCER_ROLLING_WINDOW_REDUCER_H_
