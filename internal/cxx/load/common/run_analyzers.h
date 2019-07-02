#ifndef TESTING_PERFORMANCE_PERFGATE_INTERNAL_CXX_LOAD_COMMON_RUN_ANALYZERS_H_
#define TESTING_PERFORMANCE_PERFGATE_INTERNAL_CXX_LOAD_COMMON_RUN_ANALYZERS_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "internal/proto/mako_internal.pb.h"
#include "spec/cxx/analyzer.h"
#include "spec/cxx/dashboard.h"
#include "spec/cxx/storage.h"
#include "spec/proto/mako.pb.h"

namespace mako {
namespace internal {

// Runs the provided analyzers against the provided BenchmarkInfo and RunInfo,
// using the provided mako::Storage to look up any other required data.
//
// If attach_e_divisive_regressions_to_changepoints is true, any regressions
// reported by an E-Divisive analyzer are attached to the corresponding
// changepoints. See
// cs/symbol:mako::e_divisive::Analyzer::AttachRegressionsToChangepoints
// for more information.
//
// dashboard is an optional parameter. If dashboard is not null, then we will
// use the provided dashboard to provide analysis visualization links with the
// Dashboard::VisualizeAnalysis() method for each analyzer that has a
// regression.
std::string RunAnalyzers(const mako::BenchmarkInfo& benchmark_info,
                    const mako::RunInfo& run_info,
                    const std::vector<mako::SampleBatch>& sample_batches,
                    bool attach_e_divisive_regressions_to_changepoints,
                    mako::Storage* storage, mako::Dashboard* dashboard,
                    const std::vector<mako::Analyzer*>& analyzers,
                    mako::TestOutput* test_output);
}  // namespace internal
}  // namespace mako

#endif  // TESTING_PERFORMANCE_PERFGATE_INTERNAL_CXX_LOAD_COMMON_RUN_ANALYZERS_H_
