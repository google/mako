// Provides a high level library to filter data from Mako protobuffers.
#ifndef TESTING_PERFORMANCE_PERFGATE_INTERNAL_CXX_FILTER_UTILS_H_
#define TESTING_PERFORMANCE_PERFGATE_INTERNAL_CXX_FILTER_UTILS_H_

#include <string>
#include <utility>
#include <vector>

#include "spec/proto/mako.pb.h"

namespace mako {
namespace internal {

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
    std::vector<std::pair<double, double>>* results);

// a helper that allows us to pass in iterators for other types of containers of
// SampleBatch pointers... for example,
// google::protobuf::RepeatedPtrField<SampleBatch>::pointer_begin/end
template <typename SampleBatchIt>
std::string ApplyFilter(const mako::RunInfo& run_info,
                   SampleBatchIt sample_batches_begin,
                   SampleBatchIt sample_batches_end,
                   const mako::DataFilter& data_filter, bool sort_data,
                   std::vector<std::pair<double, double>>* results) {
  std::vector<const mako::SampleBatch*> sample_batches(sample_batches_begin,
                                                           sample_batches_end);

  return ApplyFilter(run_info, sample_batches, data_filter, sort_data, results);
}

}  // namespace internal
}  // namespace mako

#endif  // TESTING_PERFORMANCE_PERFGATE_INTERNAL_CXX_FILTER_UTILS_H_
