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
#include "cxx/internal/analyzer_common.h"

#include <functional>

#include "glog/logging.h"
#include "spec/proto/mako.pb.h"
#include "absl/strings/str_cat.h"
#include "cxx/helpers/status/canonical_errors.h"
#include "cxx/helpers/status/status.h"
#include "cxx/helpers/status/statusor.h"
#include "cxx/internal/filter_utils.h"

namespace mako {
namespace internal {

helpers::StatusOr<std::vector<RunData>> ExtractDataAndRemoveEmptyResults(
    const DataFilter& data_filter,
    const std::vector<const RunBundle*>& sorted_run_bundles) {
  std::vector<RunData> data;
  // Optimize for the case where there are not many empty results.
  data.reserve(sorted_run_bundles.size());
  for (const RunBundle* run_bundle : sorted_run_bundles) {
    const RunInfo* run = &run_bundle->run_info();
    std::vector<const SampleBatch*> unused;
    std::vector<DataPoint> results;
    std::string err =
        mako::internal::ApplyFilter(run_bundle->benchmark_info(), *run,
                                          unused, data_filter, false, &results);
    if (!err.empty()) {
      return helpers::UnknownError(
          absl::StrCat("Run data extraction failed for run_key(",
                       run->run_key(), "): ", err));
    }
    if (results.empty()) {
      VLOG(1) << "No run data found for run key: " << run->run_key();
      continue;
    } else if (results.size() != 1) {
      return helpers::UnknownError(
          absl::StrCat("Run data extraction failed to get one value, got : ",
                       results.size()));
    }
    data.push_back(RunData{run, results[0].y_value});
  }
  return data;
}

std::vector<const RunBundle*> SortRunBundles(const AnalyzerInput& input,
                                       const RunOrder& run_order) {
  std::vector<const RunBundle*> run_bundles;
  for (const RunBundle& bundle : input.historical_run_list()) {
    run_bundles.push_back(&bundle);
  }

  std::function<bool(const RunBundle*, const RunBundle*)> sort_function;
  switch (run_order) {
    case RunOrder::UNSPECIFIED:
    case RunOrder::TIMESTAMP:
    default:
      sort_function = [](const RunBundle* a, const RunBundle* b) {
        return a->run_info().timestamp_ms() < b->run_info().timestamp_ms();
      };
      break;
    case RunOrder::BUILD_ID:
      sort_function = [](const RunBundle* a, const RunBundle* b) {
        return a->run_info().build_id() < b->run_info().build_id();
      };
      break;
  }
  std::sort(run_bundles.begin(), run_bundles.end(), sort_function);
  run_bundles.push_back(&input.run_to_be_analyzed());
  return run_bundles;
}

}  // namespace internal
}  // namespace mako
