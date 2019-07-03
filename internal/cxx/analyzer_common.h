#ifndef TESTING_PERFORMANCE_PERFGATE_INTERNAL_CXX_ANALYZER_COMMON_H_
#define TESTING_PERFORMANCE_PERFGATE_INTERNAL_CXX_ANALYZER_COMMON_H_

#include "spec/proto/mako.pb.h"
#include "helpers/cxx/status/statusor.h"

namespace mako {
namespace internal {

// TODO(b/136286920): add unit tests for these methods. Currently they are
// relying on the unit tests for WDA and E-divisive analyzers for correctness
// verification.

struct RunData {
  // Does not own the run
  const RunInfo* run;
  double value;
};

helpers::StatusOr<std::vector<RunData>> ExtractDataAndRemoveEmptyResults(
    const DataFilter& data_filter,
    const std::vector<const RunInfo*>& sorted_runs);

std::vector<const RunInfo*> SortRuns(const AnalyzerInput& input,
                                     const RunOrder& run_order);

}  // namespace internal
}  // namespace mako

#endif  // TESTING_PERFORMANCE_PERFGATE_INTERNAL_CXX_ANALYZER_COMMON_H_
