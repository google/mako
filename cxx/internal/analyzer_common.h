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
#ifndef CXX_INTERNAL_ANALYZER_COMMON_H_
#define CXX_INTERNAL_ANALYZER_COMMON_H_

#include "spec/proto/mako.pb.h"
#include "cxx/helpers/status/statusor.h"

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
    const std::vector<const RunBundle*>& sorted_run_bundles);

std::vector<const RunBundle*> SortRunBundles(const AnalyzerInput& input,
                                       const RunOrder& run_order);

}  // namespace internal
}  // namespace mako

#endif  // CXX_INTERNAL_ANALYZER_COMMON_H_
