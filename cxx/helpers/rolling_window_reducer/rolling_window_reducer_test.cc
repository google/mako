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
#include "cxx/helpers/rolling_window_reducer/rolling_window_reducer.h"

#include <iostream>
#include <utility>

#include "glog/logging.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/strings/str_format.h"
#include "cxx/clients/fileio/memory_fileio.h"
#include "cxx/helpers/rolling_window_reducer/rolling_window_reducer_internal.h"
#include "helpers/cxx/status/canonical_errors.h"
#include "helpers/cxx/status/status_matchers.h"
#include "proto/helpers/rolling_window_reducer/rolling_window_reducer.pb.h"
#include "spec/proto/mako.pb.h"

namespace mako {
namespace helpers {
namespace {

using ::testing::HasSubstr;

constexpr char kInputKey[] = "InputKey";
constexpr char kOutputKey[] = "OutputKey";
constexpr char kSamplerName[] = "SamplerName";
constexpr char kSamplerNameTwo[] = "SamplerNameTwo";
constexpr double kWindowSize = 1;
constexpr int kStepsPerWindow = 10;
constexpr double kEpsilon = std::numeric_limits<double>::epsilon() * 1000000;

std::unique_ptr<RollingWindowReducer> HelperCreateRWRInstance(
    std::vector<std::string> input_metric_keys,
    std::vector<std::string> error_sampler_name_inputs,
    const std::string& output_metric_key,
    RWRConfig_WindowOperation window_operation, double window_size,
    int steps_per_window, bool zero_for_empty_window,
    int percentile_milli = 0) {
  RWRConfig config;
  for (const auto& k : input_metric_keys) {
    *config.add_input_metric_keys() = k;
  }
  for (const auto& k : error_sampler_name_inputs) {
    *config.add_error_sampler_name_inputs() = k;
  }
  config.set_output_metric_key(output_metric_key);
  config.set_window_operation(window_operation);
  config.set_window_size(window_size);
  config.set_steps_per_window(steps_per_window);
  config.set_zero_for_empty_window(zero_for_empty_window);
  config.set_percentile_milli(percentile_milli);
  return RollingWindowReducer::Create(config);
}

std::unique_ptr<RollingWindowReducer> HelperCreateRWRRatioInstance(
    std::vector<std::string> input_metric_keys,
    std::vector<std::string> denominator_input_metric_keys,
    const std::string& output_metric_key, RWRConfig_WindowOperation window_operation,
    double window_size, int steps_per_window, bool zero_for_empty_window,
    int percentile_milli = 0) {
  RWRConfig config;
  for (const auto& k : input_metric_keys) {
    *config.add_input_metric_keys() = k;
  }
  for (const auto& k : denominator_input_metric_keys) {
    *config.add_denominator_input_metric_keys() = k;
  }
  config.set_output_metric_key(output_metric_key);
  config.set_window_operation(window_operation);
  config.set_window_size(window_size);
  config.set_steps_per_window(steps_per_window);
  config.set_zero_for_empty_window(zero_for_empty_window);
  config.set_percentile_milli(percentile_milli);
  return RollingWindowReducer::Create(config);
}

SamplePoint HelperCreateSamplePoint(
    double x_val, const std::vector<std::pair<std::string, double>>& value_pairs) {
  SamplePoint sp;
  sp.set_input_value(x_val);
  for (const auto& val_pair : value_pairs) {
    KeyedValue* kv = sp.add_metric_value_list();
    kv->set_value_key(val_pair.first);
    kv->set_value(val_pair.second);
  }
  return sp;
}

RWRAddPointsInput HelperCreateRWRAddPointsInput(
    std::string metric_key,
    const std::vector<std::pair<double, double>>& value_pairs) {
  RWRAddPointsInput input;
  for (const auto& val_pair : value_pairs) {
    *input.add_point_list() = HelperCreateSamplePoint(
        val_pair.first, {{metric_key, val_pair.second}});
  }
  return input;
}

RWRAddPointsInput HelperCreateRWRAddPointsInputWithErrors(
    const std::string& sampler_name,
    const std::vector<double>& values) {
  RWRAddPointsInput input;
  for (const double value : values) {
    SampleError se;
    se.set_input_value(value);
    se.set_sampler_name(sampler_name);
    *input.add_error_list() = se;
  }
  return input;
}

// Create a SamplePoint for each element in x_vals containing all metric_keys.
// value_tuples is a list of the values to use in each SamplePoint.
// We match metric_keys to x_vals to value_tuples using the index into each
// vector.
RWRAddPointsInput HelperCreateRWRAddPointsInputs(
    const std::vector<std::string>& metric_keys, const std::vector<double>& x_vals,
    const std::vector<std::vector<double>>& value_tuples) {
  RWRAddPointsInput input;
  EXPECT_EQ(x_vals.size(), value_tuples.size());
  for (std::size_t x_val_index = 0; x_val_index < x_vals.size();
       x_val_index++) {
    const auto& value_tuple = value_tuples[x_val_index];
    EXPECT_EQ(value_tuple.size(), metric_keys.size());
    std::vector<std::pair<std::string, double>> value_pairs;
    for (std::size_t i = 0; i < metric_keys.size(); i++) {
      value_pairs.push_back(std::make_pair(metric_keys[i], value_tuple[i]));
    }
    *input.add_point_list() =
        HelperCreateSamplePoint(x_vals[x_val_index], value_pairs);
  }
  return input;
}

bool HelperDoubleNear(double expected, double actual) {
  if (std::isnan(expected) && std::isnan(actual)) {
    return true;
  }
  if (std::isinf(expected) && std::isinf(actual)) {
    // Verify signage matches (-inf != +inf)
    return (expected * actual) == std::numeric_limits<double>::infinity();
  }
  double diff = expected - actual;
  return (diff < kEpsilon && diff > -kEpsilon);
}

bool HelperFoundPointWithXY(double x_val, double y_val,
                            const RWRCompleteOutput& output) {
  for (const auto& point : output.point_list()) {
    if (HelperDoubleNear(x_val, point.input_value())) {
      for (const auto& kv : point.metric_value_list()) {
        if (HelperDoubleNear(y_val, kv.value())) {
          return true;
        }
      }
    }
  }

  return false;
}

bool HelperFoundPointWithX(double x_val, const RWRCompleteOutput& output) {
  for (const auto& point : output.point_list()) {
    if (HelperDoubleNear(x_val, point.input_value())) {
      if (point.metric_value_list_size() != 0) {
        return true;
      }
    }
  }

  return false;
}

bool HelperFoundPointWithY(double y_val, const RWRCompleteOutput& output) {
  for (const auto& point : output.point_list()) {
    for (const auto& kv : point.metric_value_list()) {
      if (HelperDoubleNear(y_val, kv.value())) {
        return true;
      }
    }
  }

  return false;
}

TEST(RollingWindowReducerTest, InvalidInput) {
  RWRConfig config;
  *config.add_input_metric_keys() = kInputKey;
  config.set_output_metric_key(kOutputKey);
  config.set_window_operation(RWRConfig::MEAN);
  config.set_window_size(kWindowSize);
  config.set_steps_per_window(kStepsPerWindow);
  config.set_zero_for_empty_window(true);

  LOG(INFO) << "Missing input_metric_key test case:";
  config.clear_input_metric_keys();
  EXPECT_THAT(
      RollingWindowReducer::New(config),
      StatusIs(StatusCode::kInvalidArgument, HasSubstr("input_metric_key")));
  *config.add_input_metric_keys() = kInputKey;

  LOG(INFO) << "Missing output_metric_key test case:";
  config.clear_output_metric_key();
  EXPECT_THAT(
      RollingWindowReducer::New(config),
      StatusIs(StatusCode::kInvalidArgument, HasSubstr("output_metric_key")));
  config.set_output_metric_key(kOutputKey);

  LOG(INFO) << "Missing window_operation test case:";
  config.clear_window_operation();
  EXPECT_THAT(
      RollingWindowReducer::New(config),
      StatusIs(StatusCode::kInvalidArgument, HasSubstr("window_operation")));
  config.set_window_operation(RWRConfig::MEAN);

  LOG(INFO) << "Missing window_size test case:";
  config.clear_window_size();
  EXPECT_THAT(
      RollingWindowReducer::New(config),
      StatusIs(StatusCode::kInvalidArgument, HasSubstr("window_size")));
  LOG(INFO) << "Negative window_size test case:";
  config.set_window_size(-1);
  EXPECT_THAT(
      RollingWindowReducer::New(config),
      StatusIs(StatusCode::kInvalidArgument, HasSubstr("window_size")));
  LOG(INFO) << "Zero window_size test case:";
  config.set_window_size(0);
  EXPECT_THAT(
      RollingWindowReducer::New(config),
      StatusIs(StatusCode::kInvalidArgument, HasSubstr("window_size")));
  config.set_window_size(kWindowSize);

  LOG(INFO) << "Negative steps_per_window test case:";
  config.set_steps_per_window(-1);
  EXPECT_THAT(
      RollingWindowReducer::New(config),
      StatusIs(StatusCode::kInvalidArgument, HasSubstr("steps_per_window")));
  LOG(INFO) << "Zero steps_per_window test case:";
  config.set_steps_per_window(0);
  EXPECT_THAT(
      RollingWindowReducer::New(config),
      StatusIs(StatusCode::kInvalidArgument, HasSubstr("steps_per_window")));
  config.set_steps_per_window(kStepsPerWindow);

  LOG(INFO) << "Missing zero_for_empty_window test case:";
  config.clear_zero_for_empty_window();
  EXPECT_THAT(RollingWindowReducer::New(config),
              StatusIs(StatusCode::kInvalidArgument,
                       HasSubstr("zero_for_empty_window")));
  config.set_zero_for_empty_window(false);

  LOG(INFO)
      << "Missing denominator input keys for RATIO_SUM operation test case:";
  config.set_window_operation(RWRConfig::RATIO_SUM);
  EXPECT_THAT(
      RollingWindowReducer::New(config),
      StatusIs(StatusCode::kInvalidArgument, HasSubstr("denominator")));
  config.set_window_operation(RWRConfig::MEAN);

  LOG(INFO) << "Reject denominator input keys when not using RATIO_SUM "
               "operation test case:";
  *config.add_denominator_input_metric_keys() = "denom_key";
  EXPECT_THAT(
      RollingWindowReducer::New(config),
      StatusIs(StatusCode::kInvalidArgument, HasSubstr("denominator")));
  config.clear_denominator_input_metric_keys();

  LOG(INFO) << "Illegal input_metric_key with ERROR_COUNT operation test case:";
  config.set_window_operation(RWRConfig::ERROR_COUNT);
  EXPECT_THAT(
      RollingWindowReducer::New(config),
      StatusIs(StatusCode::kInvalidArgument, HasSubstr("input_metric_key")));
  LOG(INFO) << "Missing error_sampler_name with ERROR_COUNT operation test "
               "case:";
  config.clear_input_metric_keys();
  EXPECT_THAT(
      RollingWindowReducer::New(config),
      StatusIs(StatusCode::kInvalidArgument, HasSubstr("error_sampler_name")));
  LOG(INFO) << "Correct configuration with ERROR_COUNT operation test case:";
  config.add_error_sampler_name_inputs(kSamplerName);
  EXPECT_OK(RollingWindowReducer::New(config));
  config.clear_error_sampler_name_inputs();
  *config.add_input_metric_keys() = kInputKey;
  config.set_window_operation(RWRConfig::MEAN);

  LOG(INFO)
      << "Invalid error_matcher regex with non-ERROR_COUNT operation test case";
  config.set_window_operation(RWRConfig::MEAN);
  config.set_error_matcher("((");
  EXPECT_OK(RollingWindowReducer::New(config));

  LOG(INFO)
      << "Invalid error_matcher regex with ERROR_COUNT operation test case";
  config.clear_input_metric_keys();
  config.add_error_sampler_name_inputs(kSamplerName);
  config.set_window_operation(RWRConfig::ERROR_COUNT);
  config.set_error_matcher("((");
  EXPECT_THAT(
      RollingWindowReducer::New(config),
      StatusIs(StatusCode::kInvalidArgument, HasSubstr("error_matcher")));

  LOG(INFO) << "Valid error_matcher regex with ERROR_COUNT operation test case";
  config.set_window_operation(RWRConfig::ERROR_COUNT);
  config.set_error_matcher("aaa");
  EXPECT_OK(RollingWindowReducer::New(config));
}

TEST(RollingWindowReducerTest, NoInputEmptyOutput) {
  auto sb =
      HelperCreateRWRInstance({kInputKey}, {}, kOutputKey, RWRConfig::MEAN,
                              kWindowSize, kStepsPerWindow, true);
  ASSERT_NE(nullptr, sb);

  RWRCompleteOutput comp_output;
  ASSERT_OK(sb->Complete(&comp_output));
  EXPECT_EQ(0, comp_output.point_list_size());
}

TEST(RollingWindowReducerTest, NullOutputOnComplete) {
  auto sb =
      HelperCreateRWRInstance({kInputKey}, {}, kOutputKey, RWRConfig::MEAN,
                              kWindowSize, kStepsPerWindow, true);
  ASSERT_NE(nullptr, sb);
  RWRCompleteOutput* output = nullptr;
  EXPECT_THAT(sb->Complete(output), StatusIs(StatusCode::kInvalidArgument));
}

TEST(RollingWindowReducerTest, ExtractCorrectData) {
  auto sb =
      HelperCreateRWRInstance({kInputKey}, {}, kOutputKey, RWRConfig::MEAN,
                              kWindowSize, kStepsPerWindow, true);
  ASSERT_NE(nullptr, sb);

  RWRAddPointsInput input;
  *input.add_point_list() = HelperCreateSamplePoint(0, {{kInputKey, 10}});
  *input.add_point_list() =
      HelperCreateSamplePoint(0, {{"NoGoodKey", -900000}});
  ASSERT_OK(sb->AddPoints(input));
  RWRCompleteOutput comp_output;
  ASSERT_OK(sb->Complete(&comp_output));

  for (const auto& point : comp_output.point_list()) {
    for (const auto& kv : point.metric_value_list()) {
      EXPECT_EQ(10, kv.value());
    }
  }
}

TEST(RollingWindowReducerTest, OutputKeysCorrect) {
  auto sb =
      HelperCreateRWRInstance({kInputKey}, {}, kOutputKey, RWRConfig::MEAN,
                              kWindowSize, kStepsPerWindow, true);
  ASSERT_NE(nullptr, sb);

  ASSERT_OK(sb->AddPoints(HelperCreateRWRAddPointsInput(kInputKey, {{0, 10}})));
  RWRCompleteOutput comp_output;
  ASSERT_OK(sb->Complete(&comp_output));

  for (const auto& point : comp_output.point_list()) {
    for (const auto& kv : point.metric_value_list()) {
      EXPECT_EQ(kOutputKey, kv.value_key());
    }
  }
}

TEST(RollingWindowReducerTest, NoOverlap) {
  auto sb =
      HelperCreateRWRInstance({kInputKey}, {}, kOutputKey, RWRConfig::MEAN,
                              kWindowSize, kStepsPerWindow, false);
  ASSERT_NE(nullptr, sb);

  // Points are exactly the windowSize away, so their 'windows' should never
  // overlap. We'll only get points with values 10, or 20.
  ASSERT_OK(sb->AddPoints(HelperCreateRWRAddPointsInput(
      kInputKey, {{1, 10}, {1 + kWindowSize, 20}})));
  RWRCompleteOutput comp_output;
  ASSERT_OK(sb->Complete(&comp_output));

  for (const auto& sp : comp_output.point_list()) {
    EXPECT_EQ(1, sp.metric_value_list_size());
    const auto& p = sp.metric_value_list(0);
    EXPECT_TRUE(p.value() == 10 || p.value() == 20) << "Unexpected value: "
                                                    << p.value();
  }
}

TEST(RollingWindowReducerTest, MeanCorrect) {
  auto sb =
      HelperCreateRWRInstance({kInputKey}, {}, kOutputKey, RWRConfig::MEAN,
                              kWindowSize, kStepsPerWindow, false);
  ASSERT_NE(nullptr, sb);

  ASSERT_OK(sb->AddPoints(
      HelperCreateRWRAddPointsInput(kInputKey, {{1, 10}, {1.5, 20}})));
  RWRCompleteOutput comp_output;
  ASSERT_OK(sb->Complete(&comp_output));

  // 1.0 is still 10.0 because [ ... range ... )
  EXPECT_TRUE(HelperFoundPointWithXY(1.0, 10, comp_output));
  EXPECT_TRUE(HelperFoundPointWithXY(1.1, 15, comp_output));
  EXPECT_TRUE(HelperFoundPointWithXY(2, 20, comp_output));
}

TEST(RollingWindowReducerTest, SumCorrect) {
  auto sb = HelperCreateRWRInstance({kInputKey}, {}, kOutputKey, RWRConfig::SUM,
                                    kWindowSize, kStepsPerWindow, true);
  ASSERT_NE(nullptr, sb);

  ASSERT_OK(sb->AddPoints(
      HelperCreateRWRAddPointsInput(kInputKey, {{1, 10}, {1.5, 20}})));
  RWRCompleteOutput comp_output;
  ASSERT_OK(sb->Complete(&comp_output));

  // 1.0 is still 10.0 because [ ... range ... )
  EXPECT_TRUE(HelperFoundPointWithXY(1, 10, comp_output));
  EXPECT_TRUE(HelperFoundPointWithXY(1.1, 30, comp_output));
  EXPECT_TRUE(HelperFoundPointWithXY(2, 20, comp_output));
}

TEST(RollingWindowReducerTest, CountCorrect) {
  auto sb =
      HelperCreateRWRInstance({kInputKey}, {}, kOutputKey, RWRConfig::COUNT,
                              kWindowSize, kStepsPerWindow, false);
  ASSERT_NE(nullptr, sb);

  ASSERT_OK(sb->AddPoints(
      HelperCreateRWRAddPointsInput(kInputKey, {{1, 10}, {1.5, 20}})));
  RWRCompleteOutput comp_output;
  ASSERT_OK(sb->Complete(&comp_output));

  // 1 is still 1 because [ ... range ... )
  EXPECT_TRUE(HelperFoundPointWithXY(1, 1, comp_output));
  EXPECT_TRUE(HelperFoundPointWithXY(1.1, 2, comp_output));
  EXPECT_TRUE(HelperFoundPointWithXY(2, 1, comp_output));
}

TEST(RollingWindowReducerTest, ErrorCountCorrect) {
  auto sb = HelperCreateRWRInstance({}, {kSamplerName, kSamplerNameTwo},
                                    kOutputKey, RWRConfig::ERROR_COUNT,
                                    kWindowSize, kStepsPerWindow, false);
  ASSERT_NE(nullptr, sb);

  ASSERT_OK(sb->AddPoints(
      HelperCreateRWRAddPointsInputWithErrors(kSamplerName, {1, 1.5})));
  ASSERT_OK(sb->AddPoints(
      HelperCreateRWRAddPointsInputWithErrors("ExludedSamplerName", {1, 1.5})));
  ASSERT_OK(sb->AddPoints(
      HelperCreateRWRAddPointsInputWithErrors(kSamplerNameTwo, {1.5, 2})));
  RWRCompleteOutput comp_output;
  ASSERT_OK(sb->Complete(&comp_output));

  // 1 is still 1 because [ ... range ... )
  EXPECT_FALSE(HelperFoundPointWithX(0.5, comp_output));
  EXPECT_TRUE(HelperFoundPointWithXY(0.6, 1, comp_output));
  EXPECT_TRUE(HelperFoundPointWithXY(1.0, 1, comp_output));
  EXPECT_TRUE(HelperFoundPointWithXY(1.1, 3, comp_output));
  EXPECT_TRUE(HelperFoundPointWithXY(1.4, 3, comp_output));
  EXPECT_TRUE(HelperFoundPointWithXY(1.5, 3, comp_output));
  EXPECT_TRUE(HelperFoundPointWithXY(2.0, 3, comp_output));
  EXPECT_TRUE(HelperFoundPointWithXY(2.1, 1, comp_output));
  EXPECT_TRUE(HelperFoundPointWithXY(2.5, 1, comp_output));
  EXPECT_FALSE(HelperFoundPointWithX(2.6, comp_output));
}

TEST(RollingWindowReducerTest, PercentileCorrect) {
  auto sb = HelperCreateRWRInstance({kInputKey}, {}, kOutputKey,
                                    RWRConfig::PERCENTILE, kWindowSize,
                                    kStepsPerWindow, false, 50000);
  ASSERT_NE(nullptr, sb);

  ASSERT_OK(sb->AddPoints(HelperCreateRWRAddPointsInput(
      kInputKey, {{1, 10}, {1.1, 14}, {1.2, 16}, {1.5, 20}})));
  RWRCompleteOutput comp_output;
  ASSERT_OK(sb->Complete(&comp_output));

  comp_output.PrintDebugString();

  EXPECT_TRUE(HelperFoundPointWithXY(1, 14, comp_output));
  EXPECT_TRUE(HelperFoundPointWithXY(1.6, 16, comp_output));
  EXPECT_TRUE(HelperFoundPointWithXY(1.7, 18, comp_output));
  EXPECT_TRUE(HelperFoundPointWithXY(2, 20, comp_output));
}

TEST(RollingWindowReducerTest, WindowSizeCheck) {
  // steps per window set to 1.
  auto sb_1 = HelperCreateRWRInstance({kInputKey}, {}, kOutputKey,
                                      RWRConfig::SUM, 1, kStepsPerWindow, true);
  ASSERT_NE(nullptr, sb_1);

  ASSERT_OK(sb_1->AddPoints(
      HelperCreateRWRAddPointsInput(kInputKey, {{0, 10}, {3, 20}})));
  RWRCompleteOutput point_list_1;
  ASSERT_OK(sb_1->Complete(&point_list_1));

  EXPECT_TRUE(HelperFoundPointWithXY(0.5, 10, point_list_1));
  EXPECT_TRUE(HelperFoundPointWithXY(0.6, 0, point_list_1));
  EXPECT_TRUE(HelperFoundPointWithXY(2.0, 0, point_list_1));
  EXPECT_TRUE(HelperFoundPointWithXY(2.5, 0, point_list_1));
  EXPECT_TRUE(HelperFoundPointWithXY(2.6, 20, point_list_1));
  EXPECT_TRUE(HelperFoundPointWithXY(3.5, 20, point_list_1));

  // window size set to 10.
  auto sb_5 = HelperCreateRWRInstance(
      {kInputKey}, {}, kOutputKey, RWRConfig::SUM, 10, kStepsPerWindow, true);
  ASSERT_NE(nullptr, sb_5);

  ASSERT_OK(sb_5->AddPoints(
      HelperCreateRWRAddPointsInput(kInputKey, {{0, 10}, {3, 20}})));
  RWRCompleteOutput point_list_5;
  ASSERT_OK(sb_5->Complete(&point_list_5));

  EXPECT_TRUE(HelperFoundPointWithXY(5, 30, point_list_5));
  EXPECT_TRUE(HelperFoundPointWithXY(6, 20, point_list_5));
  EXPECT_TRUE(HelperFoundPointWithXY(7, 20, point_list_5));
}

TEST(RollingWindowReducerTest, StepsPerWindowCheck) {
  auto sb_05 = HelperCreateRWRInstance({kInputKey}, {}, kOutputKey,
                                       RWRConfig::SUM, 1, 2, true);
  ASSERT_NE(nullptr, sb_05);

  ASSERT_OK(sb_05->AddPoints(
      HelperCreateRWRAddPointsInput(kInputKey, {{0, 10}, {3, 20}})));
  RWRCompleteOutput point_list_05;
  ASSERT_OK(sb_05->Complete(&point_list_05));

  for (double i = 0; i <= 3.5; i += 0.5) {
    EXPECT_TRUE(HelperFoundPointWithX(i, point_list_05));
  }

  auto sb_20 = HelperCreateRWRInstance({kInputKey}, {}, kOutputKey,
                                       RWRConfig::SUM, 10, 5, true);
  ASSERT_NE(nullptr, sb_20);

  ASSERT_OK(sb_20->AddPoints(
      HelperCreateRWRAddPointsInput(kInputKey, {{0, 10}, {3, 20}})));
  RWRCompleteOutput point_list_20;
  ASSERT_OK(sb_20->Complete(&point_list_20));

  EXPECT_TRUE(HelperFoundPointWithX(0, point_list_20));
  EXPECT_TRUE(HelperFoundPointWithX(2, point_list_20));
  EXPECT_TRUE(HelperFoundPointWithX(4, point_list_20));
  EXPECT_TRUE(HelperFoundPointWithX(6, point_list_20));
  EXPECT_TRUE(HelperFoundPointWithX(8, point_list_20));
}

TEST(RollingWindowReducerTest, MultipleInputKeys) {
  std::string input_key_1 = "k1";
  std::string input_key_2 = "k2";

  auto rwr = HelperCreateRWRInstance({input_key_1, input_key_2}, {}, kOutputKey,
                                     RWRConfig::SUM, 1, 2, true);
  ASSERT_NE(nullptr, rwr);

  // input_key_1 has two points at x=0, and x=1
  ASSERT_OK(rwr->AddPoints(
      HelperCreateRWRAddPointsInput(input_key_1, {{0, 10}, {1, 20}})));

  // input_key_2 has two points at x=2, and x=3
  ASSERT_OK(rwr->AddPoints(
      HelperCreateRWRAddPointsInput(input_key_2, {{2, 30}, {3, 40}})));

  RWRCompleteOutput output;
  ASSERT_OK(rwr->Complete(&output));

  for (double i = 0.5; i <= 3.5; i += 0.5) {
    EXPECT_TRUE(HelperFoundPointWithX(i, output));
  }
}

// Verify that when providing multiple input keys, and sample points contain
// multiple metrics instead of different metrics being contained in different
// sample points, we generate correct results with RWR.
TEST(RollingWindowReducerTest, MultipleInputKeysSameSamplePoint) {
  std::string input_key_1 = "k1";
  std::string input_key_2 = "k2";
  std::string input_key_3 = "k3";

  auto rwr = HelperCreateRWRInstance({input_key_1, input_key_2}, {}, kOutputKey,
                                     RWRConfig::SUM, 1, 2, true);
  ASSERT_NE(nullptr, rwr);

  const std::vector<std::vector<double>> value_tuples = {
      {10, 30, 100}, {20, 20, 100}, {0, 40, 100}, {30, 10, 100}};
  ASSERT_OK(rwr->AddPoints(HelperCreateRWRAddPointsInputs(
      {input_key_1, input_key_2, input_key_3}, {0, 1, 2, 3}, value_tuples)));

  RWRCompleteOutput output;
  ASSERT_OK(rwr->Complete(&output));

  for (double i = 0.5; i <= 3.5; i += 0.5) {
    EXPECT_TRUE(HelperFoundPointWithXY(i, 40, output));
  }
}

TEST(RollingWindowReducerTest, RatioTest) {
  std::string input_key_1 = "k1";
  std::string input_key_2 = "k2";

  auto rwr =
      HelperCreateRWRRatioInstance({input_key_1}, {input_key_2}, kOutputKey,
                                   RWRConfig::RATIO_SUM, 1, 2, true);
  ASSERT_NE(nullptr, rwr);

  // input_key_1 has five sample points at x={0, 1, 2, 3, 3.5}
  ASSERT_OK(rwr->AddPoints(HelperCreateRWRAddPointsInput(
      input_key_1, {{0, 10}, {1, 20}, {2, 10}, {3, 20}, {3.5, 20}})));

  // input_key_2 has five sample points at x={0, 1, 2, 3, 3.5}
  ASSERT_OK(rwr->AddPoints(HelperCreateRWRAddPointsInput(
      input_key_2, {{0, 40}, {1, 40}, {2, 0}, {3, 20}, {3.5, 40}})));

  RWRCompleteOutput output;
  ASSERT_OK(rwr->Complete(&output));

  EXPECT_EQ(9, output.point_list_size());
  EXPECT_TRUE(HelperFoundPointWithXY(0.0, 0.25, output));
  EXPECT_TRUE(HelperFoundPointWithXY(0.5, 0.25, output));
  EXPECT_TRUE(HelperFoundPointWithXY(1.0, 0.5, output));
  EXPECT_TRUE(HelperFoundPointWithXY(1.5, 0.5, output));
  EXPECT_TRUE(HelperFoundPointWithXY(
      2.0, std::numeric_limits<double>::infinity(), output));
  EXPECT_TRUE(HelperFoundPointWithXY(
      2.5, std::numeric_limits<double>::infinity(), output));
  EXPECT_TRUE(HelperFoundPointWithXY(3.0, 1, output));
  EXPECT_TRUE(HelperFoundPointWithXY(3.5, 2.0 / 3.0, output));
  EXPECT_TRUE(HelperFoundPointWithXY(4.0, 0.5, output));
}

TEST(RollingWindowReducerTest, RatioNaN) {
  std::string input_key_1 = "k1";
  std::string input_key_2 = "k2";

  auto rwr =
      HelperCreateRWRRatioInstance({input_key_1}, {input_key_2}, kOutputKey,
                                   RWRConfig::RATIO_SUM, 1, 1, false);
  ASSERT_NE(nullptr, rwr);

  ASSERT_OK(
      rwr->AddPoints(HelperCreateRWRAddPointsInput(input_key_1, {{0, 0}})));

  ASSERT_OK(
      rwr->AddPoints(HelperCreateRWRAddPointsInput(input_key_2, {{0, 0}})));

  RWRCompleteOutput output;
  ASSERT_OK(rwr->Complete(&output));

  EXPECT_EQ(1, output.point_list_size());
  EXPECT_TRUE(HelperFoundPointWithXY(0.0, std::nan(""), output));
}

TEST(RollingWindowReducerTest, RatioInf) {
  std::string input_key_1 = "k1";
  std::string input_key_2 = "k2";

  auto rwr =
      HelperCreateRWRRatioInstance({input_key_1}, {input_key_2}, kOutputKey,
                                   RWRConfig::RATIO_SUM, 1, 1, false);
  ASSERT_NE(nullptr, rwr);

  ASSERT_OK(rwr->AddPoints(
      HelperCreateRWRAddPointsInput(input_key_1, {{0, 1}, {3, -1}})));

  ASSERT_OK(rwr->AddPoints(
      HelperCreateRWRAddPointsInput(input_key_2, {{0, 0}, {3, 0}})));

  RWRCompleteOutput output;
  ASSERT_OK(rwr->Complete(&output));

  EXPECT_EQ(2, output.point_list_size());

  EXPECT_TRUE(HelperFoundPointWithXY(
          0, std::numeric_limits<double>::infinity(), output));
  EXPECT_TRUE(HelperFoundPointWithXY(
          3, -std::numeric_limits<double>::infinity(), output));
}

TEST(RollingWindowReducerTest, RatioTestWideWindow) {
  std::string input_key_1 = "k1";
  std::string input_key_2 = "k2";

  // double window_size, int steps_per_window, bool zero_for_empty_window, int
  // percentile_milli = 0
  auto rwr =
      HelperCreateRWRRatioInstance({input_key_1}, {input_key_2}, kOutputKey,
                                   RWRConfig::RATIO_SUM, 3, 10, true);
  ASSERT_NE(nullptr, rwr);

  // input_key_1 has five sample points at x={0, 1, 2, 3, 3.5}
  ASSERT_OK(rwr->AddPoints(HelperCreateRWRAddPointsInput(
      input_key_1, {{0, 10}, {1, 20}, {2, 10}, {3, 20}, {3.5, 20}})));

  // input_key_2 has five sample points at x={0, 1, 2, 3, 3.5}
  ASSERT_OK(rwr->AddPoints(HelperCreateRWRAddPointsInput(
      input_key_2, {{0, 40}, {1, 40}, {2, 0}, {3, 20}, {3.5, 40}})));

  RWRCompleteOutput output;
  ASSERT_OK(rwr->Complete(&output));

  EXPECT_EQ(21, output.point_list_size());
  // Note: window size extends 1.5 on both sides from the window center point

  // The following data points only contain x = {0} points in the window
  EXPECT_TRUE(HelperFoundPointWithXY(-1.2, 0.25, output));
  EXPECT_TRUE(HelperFoundPointWithXY(-0.9, 0.25, output));
  EXPECT_TRUE(HelperFoundPointWithXY(-0.6, 0.25, output));
  // The following data points contain x = {0, 1} points in the window
  EXPECT_TRUE(HelperFoundPointWithXY(-0.3, 0.375, output));
  EXPECT_TRUE(HelperFoundPointWithXY(0.0, 0.375, output));
  EXPECT_TRUE(HelperFoundPointWithXY(0.3, 0.375, output));
  // The following data points contain x = {0, 1, 2} points in the window
  EXPECT_TRUE(HelperFoundPointWithXY(0.6, 0.5, output));
  EXPECT_TRUE(HelperFoundPointWithXY(0.9, 0.5, output));
  EXPECT_TRUE(HelperFoundPointWithXY(1.2, 0.5, output));
  EXPECT_TRUE(HelperFoundPointWithXY(1.5, 0.5, output));
  // The following data points contain x = {1, 2, 3} points in the window
  EXPECT_TRUE(HelperFoundPointWithXY(1.8, 5.0 / 6.0, output));
  // The following data points contain x = {1, 2, 3, 3.5} points in the window
  EXPECT_TRUE(HelperFoundPointWithXY(2.1, 0.7, output));
  EXPECT_TRUE(HelperFoundPointWithXY(2.4, 0.7, output));
  // The following data points contain x = {2, 3, 3.5} points in the window
  EXPECT_TRUE(HelperFoundPointWithXY(2.7, 5.0 / 6.0, output));
  EXPECT_TRUE(HelperFoundPointWithXY(3.0, 5.0 / 6.0, output));
  EXPECT_TRUE(HelperFoundPointWithXY(3.3, 5.0 / 6.0, output));
  // The following data points contain x = {3, 3.5} points in the window
  EXPECT_TRUE(HelperFoundPointWithXY(3.6, 2.0 / 3.0, output));
  EXPECT_TRUE(HelperFoundPointWithXY(3.9, 2.0 / 3.0, output));
  EXPECT_TRUE(HelperFoundPointWithXY(4.2, 2.0 / 3.0, output));
  EXPECT_TRUE(HelperFoundPointWithXY(4.5, 2.0 / 3.0, output));
  // The following data points contain x = {3.5} points in the window
  EXPECT_TRUE(HelperFoundPointWithXY(4.8, 0.5, output));
}

TEST(RollingWindowReducerTest, RatioSparseDataZeroForEmptyWindowTrue) {
  std::string input_key_1 = "k1";
  std::string input_key_2 = "k2";

  auto rwr =
      HelperCreateRWRRatioInstance({input_key_1}, {input_key_2}, kOutputKey,
                                   RWRConfig::RATIO_SUM, 1, 2, true);
  ASSERT_NE(nullptr, rwr);

  // input_key_1 has four sample points at x={0, 3, 5, 5.5}
  ASSERT_OK(rwr->AddPoints(
      HelperCreateRWRAddPointsInput(input_key_1, {{0, 10},
                                                  // Missing 1 and 2
                                                  {3, 10},
                                                  {5, 20},
                                                  {5.5, 20}})));

  // input_key_2 has five sample points at x={0, 1, 5, 6.5, 7}
  ASSERT_OK(rwr->AddPoints(HelperCreateRWRAddPointsInput(
      input_key_2, {{0, 40}, {1, 40}, {5, 30}, {6.5, 20}, {7, 40}})));

  RWRCompleteOutput output;
  ASSERT_OK(rwr->Complete(&output));

  EXPECT_EQ(16, output.point_list_size());
  EXPECT_TRUE(HelperFoundPointWithXY(0.0, 0.25, output));
  EXPECT_TRUE(HelperFoundPointWithXY(0.5, 0.25, output));
  EXPECT_TRUE(HelperFoundPointWithXY(1.0, 0, output));
  EXPECT_TRUE(HelperFoundPointWithXY(1.5, 0, output));
  EXPECT_TRUE(HelperFoundPointWithXY(2.0, 0, output));
  EXPECT_TRUE(HelperFoundPointWithXY(2.5, 0, output));
  EXPECT_TRUE(HelperFoundPointWithXY(3.0, 0, output));
  EXPECT_TRUE(HelperFoundPointWithXY(3.5, 0, output));
  EXPECT_TRUE(HelperFoundPointWithXY(4.0, 0, output));
  EXPECT_TRUE(HelperFoundPointWithXY(4.5, 0, output));
  EXPECT_TRUE(HelperFoundPointWithXY(5.0, 2.0 / 3.0, output));
  EXPECT_TRUE(HelperFoundPointWithXY(5.5, 4.0 / 3.0, output));
  EXPECT_TRUE(HelperFoundPointWithXY(6.0, 0, output));
  EXPECT_TRUE(HelperFoundPointWithXY(6.5, 0, output));
  EXPECT_TRUE(HelperFoundPointWithXY(7.0, 0, output));
  EXPECT_TRUE(HelperFoundPointWithXY(7.5, 0, output));
}

TEST(RollingWindowReducerTest, RatioSparseDataZeroForEmptyWindowFalse) {
  std::string input_key_1 = "k1";
  std::string input_key_2 = "k2";

  auto rwr =
      HelperCreateRWRRatioInstance({input_key_1}, {input_key_2}, kOutputKey,
                                   RWRConfig::RATIO_SUM, 1, 2, false);
  ASSERT_NE(nullptr, rwr);

  // input_key_1 has four sample points at x={0, 3, 5, 5.5}
  ASSERT_OK(rwr->AddPoints(
      HelperCreateRWRAddPointsInput(input_key_1, {{0, 10},
                                                  // Missing 1 and 2
                                                  {3, 10},
                                                  {5, 20},
                                                  {5.5, 20}})));

  // input_key_2 has five sample points at x={0, 1, 5, 6.5, 7}
  ASSERT_OK(rwr->AddPoints(HelperCreateRWRAddPointsInput(
      input_key_2, {{0, 40}, {1, 40}, {5, 30}, {6.5, 20}, {7, 40}})));

  RWRCompleteOutput output;
  ASSERT_OK(rwr->Complete(&output));

  EXPECT_EQ(4, output.point_list_size());
  EXPECT_TRUE(HelperFoundPointWithXY(0.0, 0.25, output));
  EXPECT_TRUE(HelperFoundPointWithXY(0.5, 0.25, output));
  EXPECT_TRUE(HelperFoundPointWithXY(5.0, 2.0 / 3.0, output));
  EXPECT_TRUE(HelperFoundPointWithXY(5.5, 4.0 / 3.0, output));
}

TEST(RollingWindowReducerTest, RatioMultipleDenominatorInputKeys) {
  std::string input_key_1 = "successes";
  std::string input_key_2 = "errors";
  std::string input_key_3 = "non_related_metric";

  auto rwr = HelperCreateRWRRatioInstance(
      {input_key_1}, {input_key_1, input_key_2}, kOutputKey,
      RWRConfig::RATIO_SUM, 1, 2, true);
  ASSERT_NE(nullptr, rwr);

  const std::vector<std::vector<double>> value_tuples = {
      {10, 10, 100}, {20, 20, 100}, {30, 30, 100}, {40, 40, 100}};
  ASSERT_OK(rwr->AddPoints(HelperCreateRWRAddPointsInputs(
      {input_key_1, input_key_2, input_key_3}, {0, 1, 2, 3}, value_tuples)));

  RWRCompleteOutput output;
  ASSERT_OK(rwr->Complete(&output));

  for (double i = 0.5; i <= 3.5; i += 0.5) {
    EXPECT_TRUE(HelperFoundPointWithXY(i, 0.5, output));
  }
}

TEST(RollingWindowReducerTest, ZeroIfEmptyFalse) {
  auto sb =
      HelperCreateRWRInstance({kInputKey}, {}, kOutputKey, RWRConfig::MEAN,
                              kWindowSize, kStepsPerWindow, false);
  ASSERT_NE(nullptr, sb);

  ASSERT_OK(sb->AddPoints(
      HelperCreateRWRAddPointsInput(kInputKey, {{1, 10}, {4, 20}})));
  RWRCompleteOutput comp_output;
  ASSERT_OK(sb->Complete(&comp_output));

  EXPECT_FALSE(HelperFoundPointWithY(0, comp_output));
}

TEST(RollingWindowReducerTest, ZeroIfEmptyTrue) {
  auto sb =
      HelperCreateRWRInstance({kInputKey}, {}, kOutputKey, RWRConfig::MEAN,
                              kWindowSize, kStepsPerWindow, true);
  ASSERT_NE(nullptr, sb);

  ASSERT_OK(sb->AddPoints(
      HelperCreateRWRAddPointsInput(kInputKey, {{1, 10}, {4, 20}})));
  RWRCompleteOutput comp_output;
  ASSERT_OK(sb->Complete(&comp_output));

  EXPECT_TRUE(HelperFoundPointWithY(0, comp_output));
}

// // Useful for testing this code against preexisting data
// TEST(RollingWindowReducerTest, MessWithSomeoneElsesData) {
//   auto binner = HelperCreateRWRInstance(
//       {"psgetms"}, {}, kOutputKey, RWRConfig_WindowOperation_MEAN, 1000, 5,
//       false);
//   ASSERT_NE(nullptr, binner);
//
//   mako::google3_storage::Storage pgs;
//   SampleBatchQuery request;
//   request.set_run_key("4747370399006720");
//   // request.set_run_key("6038603729731584");
//   SampleBatchQueryResponse response;
//   pgs.QuerySampleBatch(request, &response);
//
//   RWRAddPointsInput points_to_process;
//
//   for (const auto& sb : response.sample_batch_list()) {
//     for (const auto& sp : sb.sample_point_list()) {
//       *points_to_process.add_point_list() = sp;
//     }
//   }
//
//   long long start_time = ::absl::ToUnixMillis(::absl::Now());
//
//   int mult_factor = 500;
//   for (int i = 0; i < mult_factor; i++) {
//     binner->AddPoints(points_to_process);
//   }
//   RWRCompleteOutput comp_output;
//   binner->Complete(&comp_output);
//
//   LOG(INFO) << "POINT COUNT: "
//             << points_to_process.point_list_size() * mult_factor;
//   LOG(INFO) << "TIME TAKEN: "
//             << ::absl::ToUnixMillis(::absl::Now()) - start_time;
//   HelperPrintData(comp_output, 1456776330000);
// }

void WriteFile(absl::string_view file_name, const RWRAddPointsInput& input,
               mako::FileIO* file_io) {
  ASSERT_TRUE(file_io->Open(std::string(file_name), FileIO::AccessMode::kWrite))
      << file_io->Error();
  for (const SamplePoint& sp : input.point_list()) {
    SampleRecord sr;
    *sr.mutable_sample_point() = sp;
    ASSERT_TRUE(file_io->Write(sr)) << file_io->Error();
  }
  for (const SampleError& se : input.error_list()) {
    SampleRecord sr;
    *sr.mutable_sample_error() = se;
    ASSERT_TRUE(file_io->Write(sr)) << file_io->Error();
  }
  file_io->Close();
}


TEST(RollingWindowReducerTest, ReduceAppendsToFileTest) {
  const std::string error_count_key = "error_count";

  RWRConfig config;
  *config.add_error_sampler_name_inputs() = kSamplerName;
  config.set_output_metric_key(error_count_key);
  config.set_window_operation(RWRConfig::ERROR_COUNT);
  config.set_window_size(kWindowSize);
  config.set_steps_per_window(kStepsPerWindow);
  config.set_zero_for_empty_window(true);

  RWRAddPointsInput input = HelperCreateRWRAddPointsInputWithErrors(
      kSamplerName, {0, 10});
  absl::string_view file_path = "file";
  mako::memory_fileio::FileIO file_io;
  WriteFile(file_path, input, &file_io);
  EXPECT_OK(Reduce(file_path, {config}, &file_io));

  bool has_key = false;
  ASSERT_TRUE(file_io.Open(std::string(file_path), mako::FileIO::kRead))
      << file_io.Error();
  mako::SampleRecord record;
  while (file_io.Read(&record)) {
    LOG(INFO) << record.DebugString();
    for (const KeyedValue& kv : record.sample_point().metric_value_list()) {
      LOG(INFO) << "error count test; value_key: " << kv.value_key();
      if (kv.value_key() == error_count_key) {
        has_key = true;
        break;
      }
    }
  }
  EXPECT_TRUE(file_io.ReadEOF()) << file_io.Error();
  EXPECT_TRUE(has_key);
}

TEST(RollingWindowReducerTest, SanityCheck) {
  // Simple test which is easy to rationalize.
  // Points every 0.25 starting at 0 and going to 2, with y-val = 1.
  // Regardless of steps_per_window we should:
  //   * never get a COUNT more than 4.
  //   * MEAN should always be 1.
  //   * never get a SUM more than 4.
  double window_size = 1;
  for (const auto& window_op :
       {RWRConfig::COUNT, RWRConfig::SUM, RWRConfig::MEAN}) {
    for (double steps_per_window : {1.0, 2.0, 4.0}) {
      for (bool zero_if_empty : {false, true}) {
        LOG(INFO) << "-----------";
        LOG(INFO) << "Window Op: " << RWRConfig_WindowOperation_Name(window_op);
        LOG(INFO) << "Steps Per Window: " << steps_per_window;
        LOG(INFO) << "Zero If Empty: " << zero_if_empty;

        auto r = HelperCreateRWRInstance({kInputKey}, {}, kOutputKey, window_op,
                                         window_size, steps_per_window,
                                         zero_if_empty);
        ASSERT_NE(nullptr, r);

        for (double i = 0; i <= 2; i += 0.25) {
          ASSERT_OK(
              r->AddPoints(HelperCreateRWRAddPointsInput(kInputKey, {{i, 1}})));
        }

        RWRCompleteOutput output;
        ASSERT_OK(r->Complete(&output));

        double max_value = -1;

        // 3 because we are always creating points at 0, 1, 2
        ASSERT_EQ(steps_per_window * 3, output.point_list_size());

        for (const auto& sp : output.point_list()) {
          LOG(INFO) << sp.input_value() << ","
                    << sp.metric_value_list(0).value();
          ASSERT_EQ(1, sp.metric_value_list_size());
          if (sp.metric_value_list(0).value() > max_value) {
            max_value = sp.metric_value_list(0).value();
          }
        }

        switch (window_op) {
          case RWRConfig::SUM:
            ABSL_FALLTHROUGH_INTENDED;
          case RWRConfig::COUNT:
            ASSERT_TRUE(HelperFoundPointWithXY(0, 2, output));
            ASSERT_TRUE(HelperFoundPointWithXY(2, 3, output));
            ASSERT_TRUE(HelperFoundPointWithXY(1, 4, output));
            ASSERT_EQ(4, max_value);
            break;
          case RWRConfig::MEAN:
            ASSERT_TRUE(HelperFoundPointWithXY(0, 1, output));
            ASSERT_TRUE(HelperFoundPointWithXY(2, 1, output));
            ASSERT_TRUE(HelperFoundPointWithXY(1, 1, output));
            ASSERT_EQ(1, max_value);
            break;
          case RWRConfig::PERCENTILE:
            ASSERT_EQ(1, 0);
            break;
          default:
            FAIL() << "case not expected: " << window_op;
            break;
        }
      }
    }
  }
}

}  // namespace
}  // namespace helpers
}  // namespace mako
