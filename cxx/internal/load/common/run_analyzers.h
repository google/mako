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
#ifndef CXX_INTERNAL_LOAD_COMMON_RUN_ANALYZERS_H_
#define CXX_INTERNAL_LOAD_COMMON_RUN_ANALYZERS_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "cxx/spec/analyzer.h"
#include "cxx/spec/dashboard.h"
#include "cxx/spec/storage.h"
#include "proto/internal/mako_internal.pb.h"
#include "spec/proto/mako.pb.h"

namespace mako {
namespace internal {

// Runs the provided analyzers against the provided BenchmarkInfo and RunInfo,
// using the provided mako::Storage to look up any other required data.
//
// dashboard is an optional parameter. If dashboard is not null, then we will
// use the provided dashboard to provide analysis visualization links with the
// Dashboard::VisualizeAnalysis() method for each analyzer that has a
// regression.
std::string RunAnalyzers(
    const mako::BenchmarkInfo& benchmark_info,
    const mako::RunInfo& run_info,
    const std::vector<mako::SampleBatch>& sample_batches,
    bool attach_e_divisive_regressions_to_changepoints,
    mako::Storage* storage, mako::Dashboard* dashboard,
    const std::vector<mako::Analyzer*>& analyzers,
    mako::TestOutput* test_output);
}  // namespace internal
}  // namespace mako

#endif  // CXX_INTERNAL_LOAD_COMMON_RUN_ANALYZERS_H_
