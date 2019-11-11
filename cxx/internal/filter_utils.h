// Copyright 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// see the license for the specific language governing permissions and
// limitations under the license.

// Provides a high level library to filter data from Mako protobuffers.
#ifndef CXX_INTERNAL_FILTER_UTILS_H_
#define CXX_INTERNAL_FILTER_UTILS_H_

#include <ostream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "spec/proto/mako.pb.h"

namespace mako {
namespace internal {

struct DataPoint {
  explicit DataPoint(double x, double y) : x_value(x), y_value(y) {}

  bool operator==(const DataPoint& other) const {
    return x_value == other.x_value && y_value == other.y_value;
  }
  // The X-value for this data point.
  // This will be either RunInfo.timestamp_ms or SamplePoint.input_value
  // depending on the DataFilter.
  double x_value = 0;

  // The Y-value for this data point.
  // This will be either an aggregate value or metric value depending on the
  // DataFilter.
  double y_value = 0;
};

// Function used to sort vectors of DataPoints, sorting first by the x value and
// then by the y value.
// Inline comparison operator is not defined so sorting requires the comparison
// function be explicitly supplied.
inline bool CompareDataPoint(const DataPoint& a, const DataPoint& b) {
  return std::tie(a.x_value, a.y_value) < std::tie(b.x_value, b.y_value);
}

inline std::ostream& operator<<(std::ostream& stream, const DataPoint& point) {
  return stream << "{" << point.x_value << ", " << point.y_value << "}";
}

// ApplyFilter applies the passed DataFilter to the RunInfo/SampleBatches
// and returns the data as vector<pair<double, double>>.
//
// If the DataFilter specifies an aggregate, custom aggregate, benchmark score
// or error value a single pair will be returned from the RunInfo. The single
// pair will contain the RunInfo.timestamp_ms as the first value and the
// aggregate as the second (eg. [[RunInfo.timestamp_ms, X]]).
//
// If the DataFilter specifies a metric, data will be parsed from the
// supplied SampleBatches. Data from the ignore regions specified by the RunInfo
// will be stripped. Data will be returned as multiple x,y pairs, with the input
// SampleBatch.SamplePoint.input_value as the first element (x). For example:
// [[input_value1, metric_value1], [input_value2, metric_value2], etc.]
//
// By default no sorting of the data will take place. If sorting is requested
// values will be sorted by x value (eg. SampleBatch.input_values) in
// increasing order before being returned.
//
// If the data specified by the DataFilter cannot be found and
// DataFilter.ignore_missing_data = false then a string containing the error is
// returned. If the data is missing and DataFilter.ignore_missing_data = true,
// then an empty string is returned and the result vector is empty.
//
std::string ApplyFilter(const mako::RunInfo& run_info,
    const std::vector<const mako::SampleBatch*>& sample_batches,
    const mako::DataFilter& data_filter, bool sort_data,
    std::vector<DataPoint>* results);

// a helper that allows us to pass in iterators for other types of containers of
// SampleBatch pointers... for example,
// google::protobuf::RepeatedPtrField<SampleBatch>::pointer_begin/end
template <typename SampleBatchIt>
std::string ApplyFilter(const mako::RunInfo& run_info,
                   SampleBatchIt sample_batches_begin,
                   SampleBatchIt sample_batches_end,
                   const mako::DataFilter& data_filter, bool sort_data,
                   std::vector<DataPoint>* results) {
  std::vector<const mako::SampleBatch*> sample_batches(sample_batches_begin,
                                                           sample_batches_end);

  return ApplyFilter(run_info, sample_batches, data_filter, sort_data, results);
}

}  // namespace internal
}  // namespace mako

#endif  // CXX_INTERNAL_FILTER_UTILS_H_
