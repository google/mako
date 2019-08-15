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
// See the License for the specific language governing permissions and
// limitations under the License.
#include "clients/cxx/analyzers/threshold_analyzer.h"

#include <sstream>
#include <utility>
#include <vector>

#include "glog/logging.h"
#include "src/google/protobuf/text_format.h"
#include "absl/strings/str_cat.h"
#include "clients/cxx/analyzers/util.h"
#include "internal/cxx/filter_utils.h"

namespace mako {
namespace threshold_analyzer {

using mako::AnalyzerInput;
using mako::AnalyzerOutput;
using mako::analyzers::threshold_analyzer::ThresholdAnalyzerInput;
using mako::analyzers::threshold_analyzer::ThresholdAnalyzerOutput;
using mako::analyzers::threshold_analyzer::ThresholdConfigResult;

Analyzer::Analyzer(const ThresholdAnalyzerInput& analyzer_input)
    : config_(analyzer_input) {}

bool Analyzer::ConstructHistoricQuery(
    const mako::AnalyzerHistoricQueryInput& /* query_input */,
    mako::AnalyzerHistoricQueryOutput* query_output) {
  // Threshold Analyzer doesn't need any historic output.
  query_output->mutable_status()->set_code(mako::Status_Code_SUCCESS);
  return true;
}

bool Analyzer::SetAnalyzerError(const std::string& error_message,
                                mako::AnalyzerOutput* output) {
  auto status = output->mutable_status();
  status->set_code(mako::Status_Code_FAIL);
  status->set_fail_message(
      absl::StrCat("threshold_analyzer::Analyzer Error: ", error_message));
  return false;
}

bool Analyzer::DoAnalyze(const mako::AnalyzerInput& analyzer_input,
                         mako::AnalyzerOutput* analyzer_output) {
  // Save the analyzer configuration.
  google::protobuf::TextFormat::PrintToString(config_,
                                    analyzer_output->mutable_input_config());

  if (!analyzer_input.has_run_to_be_analyzed()) {
    return SetAnalyzerError("AnalyzerInput missing run_to_be_analyzed.",
                            analyzer_output);
  }
  if (!analyzer_input.run_to_be_analyzed().has_run_info()) {
    return SetAnalyzerError("RunBundle missing run_info.", analyzer_output);
  }

  auto& run_bundle = analyzer_input.run_to_be_analyzed();
  auto& run_info = run_bundle.run_info();

  bool regression_found = false;
  ThresholdAnalyzerOutput config_out;

  for (auto& config : config_.configs()) {
    if (!config.has_data_filter()) {
      return SetAnalyzerError("ThresholdConfig missing DataFilter.",
                              analyzer_output);
    }

    if (!config.has_max() && !config.has_min()) {
      return SetAnalyzerError("ThresholdConfig must have at least max or min.",
                              analyzer_output);
    }

    std::vector<std::pair<double, double>> results;

    std::vector<const SampleBatch*> batches;
    for (const SampleBatch& sample_batch : run_bundle.batch_list()) {
      batches.push_back(&sample_batch);
    }

    auto error_string = mako::internal::ApplyFilter(
        run_info, batches, config.data_filter(), false, &results);

    if (error_string.length() > 0) {
      std::stringstream ss;
      ss << "Error attempting to retrieve data using data_filter: "
         << config.data_filter().ShortDebugString()
         << ". Error message: " << error_string;
      return SetAnalyzerError(ss.str(), analyzer_output);
    }
    if (results.size() == 0) {
      std::stringstream ss;
      ss << "Did not find any data using data_filter: "
         << config.data_filter().DebugString();
      if (config.data_filter().ignore_missing_data()) {
        // In the default case we do not emit an error.
        LOG(INFO) << ss.str() << " Ignoring missing data.";
        continue;
      }
      return SetAnalyzerError(ss.str(), analyzer_output);
    }

    double points_under_min = 0;
    double points_above_max = 0;
    double total_points = results.size();

    for (auto& pair : results) {
      if (config.has_min() && pair.second < config.min()) {
        points_under_min++;
      } else if (config.has_max() && pair.second > config.max()) {
        points_above_max++;
      }
    }

    double actual_percent_outside_range =
        (points_under_min + points_above_max) / total_points * 100;

    LOG(INFO) << "----------";
    LOG(INFO) << "Starting Threshold Config analysis";
    LOG(INFO) << "Threshold Config: " << config.ShortDebugString();
    LOG(INFO) << "Points above max: " << points_above_max;
    LOG(INFO) << "Points below min: " << points_under_min;
    LOG(INFO) << "Actual percent outliers: " << actual_percent_outside_range
              << "%";

    // Record threshold config and analysis for visualization.
    ThresholdConfigResult* result = config_out.add_config_results();
    *result->mutable_config() = config;
    result->set_percent_above_max(points_above_max / total_points * 100);
    result->set_percent_below_min(points_under_min / total_points * 100);
    result->set_metric_label(
        ::mako::analyzer_util::GetHumanFriendlyDataFilterString(
            config.data_filter(), run_bundle.benchmark_info()));
    if (results.size() == 1) {
      result->set_value_outside_threshold(results[0].second);
    }

    result->set_regression(actual_percent_outside_range >
                           config.outlier_percent_max());
    if (result->regression()) {
      LOG(INFO) << "REGRESSION found!";
      regression_found = true;
    }

    LOG(INFO) << "Analysis complete for config";
    LOG(INFO) << "----------";
  }

  analyzer_output->mutable_status()->set_code(mako::Status_Code_SUCCESS);
  analyzer_output->set_regression(regression_found);
  google::protobuf::TextFormat::PrintToString(config_out,
                                    analyzer_output->mutable_output());
  return true;
}

}  // namespace threshold_analyzer
}  // namespace mako
