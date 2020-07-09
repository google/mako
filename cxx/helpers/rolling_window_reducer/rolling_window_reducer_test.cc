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
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "cxx/clients/fileio/memory_fileio.h"
#include "cxx/helpers/rolling_window_reducer/rolling_window_reducer_internal.h"
#include "cxx/helpers/status/canonical_errors.h"
#include "cxx/helpers/status/status_matchers.h"
#include "cxx/testing/protocol-buffer-matchers.h"
#include "proto/helpers/rolling_window_reducer/rolling_window_reducer.pb.h"
#include "spec/proto/mako.pb.h"

namespace mako {
namespace helpers {
namespace {

using ::testing::Contains;
using ::testing::DescribeMatcher;
using ::testing::HasSubstr;
using ::testing::Matches;
using ::testing::Not;
using ::testing::UnorderedElementsAreArray;

using SamplePointMatcher = ::testing::Matcher<const SamplePoint&>;
using KeyedValueMatcher = ::testing::Matcher<const KeyedValue&>;

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
    const std::string& output_metric_key,
    RWRConfig_WindowOperation window_operation, double window_size,
    int steps_per_window, bool zero_for_empty_window,
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
    double x_val,
    const std::vector<std::pair<std::string, double>>& value_pairs) {
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
    const std::string& sampler_name, const std::vector<double>& values) {
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
    const std::vector<std::string>& metric_keys,
    const std::vector<double>& x_vals,
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

// TODO(b/141503378): Delete this and use testing::DoubleNear.
MATCHER_P(HelperDoubleNear, expected,
          absl::StrCat("is approximately ", expected)) {
  if (std::isnan(expected) && std::isnan(arg)) {
    return true;
  }
  if (std::isinf(expected) && std::isinf(arg)) {
    // Verify signage matches (-inf != +inf)
    return (expected * arg) == std::numeric_limits<double>::infinity();
  }
  double diff = expected - arg;
  return (diff < kEpsilon && diff > -kEpsilon);
}

MATCHER_P2(KeyedValueIs, key, value,
           absl::StrCat("has key ", DescribeMatcher<std::string>(key),
                        " and value ", DescribeMatcher<double>(value))) {
  *result_listener << "has key = " << arg.value_key()
                   << " and value = " << arg.value();
  return Matches(key)(arg.value_key()) && Matches(value)(arg.value());
}

MATCHER_P2(SamplePointFullMatch, x_val, y_key_and_val,
           absl::StrCat("is a SamplePoint whose input_value ",
                        DescribeMatcher<double>(x_val),
                        " and has an output point that ",
                        DescribeMatcher<const KeyedValue&>(y_key_and_val))) {
  if (!Matches(x_val)(arg.input_value())) {
    *result_listener << "whose input_value is " << arg.input_value();
    return false;
  }
  auto y_list_match = ::testing::ElementsAre(y_key_and_val);
  if (!Matches(y_list_match)(arg.metric_value_list())) {
    *result_listener << "whose metric_value_list is "
                     << ::testing::PrintToString(arg.metric_value_list());
    return false;
  }
  return true;
}

SamplePointMatcher SamplePointWithX(double x_val) {
  return SamplePointFullMatch(HelperDoubleNear(x_val), ::testing::_);
}
SamplePointMatcher SamplePointWithY(double y_val) {
  return SamplePointFullMatch(
      ::testing::_, KeyedValueIs(::testing::_, HelperDoubleNear(y_val)));
}
SamplePointMatcher SamplePointWithXY(double x_val, double y_val) {
  return SamplePointFullMatch(
      HelperDoubleNear(x_val),
      KeyedValueIs(::testing::_, HelperDoubleNear(y_val)));
}
SamplePointMatcher ExactSamplePoint(double x_val, absl::string_view metric_key,
                                    double y_val) {
  return SamplePointFullMatch(
      HelperDoubleNear(x_val),
      KeyedValueIs(metric_key, HelperDoubleNear(y_val)));
}

::testing::Matcher<const google::protobuf::RepeatedPtrField<SamplePoint>&> ContainsPoints(
    std::vector<SamplePointMatcher> expected_points) {
  return ::testing::IsSupersetOf(expected_points);
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
  EXPECT_THAT(comp_output.point_list(), ContainsPoints({
                                            SamplePointWithXY(1.0, 10),
                                            SamplePointWithXY(1.1, 15),
                                            SamplePointWithXY(2.0, 20),
                                        }));
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
  EXPECT_THAT(comp_output.point_list(), ContainsPoints({
                                            SamplePointWithXY(1.0, 10),
                                            SamplePointWithXY(1.1, 30),
                                            SamplePointWithXY(2.0, 20),
                                        }));
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
  EXPECT_THAT(comp_output.point_list(), ContainsPoints({
                                            SamplePointWithXY(1.0, 1),
                                            SamplePointWithXY(1.1, 2),
                                            SamplePointWithXY(2.0, 1),
                                        }));
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
  EXPECT_THAT(comp_output.point_list(), Not(Contains(SamplePointWithX(0.5))));
  EXPECT_THAT(comp_output.point_list(), ContainsPoints({
                                            SamplePointWithXY(0.6, 1),
                                            SamplePointWithXY(1.0, 1),
                                            SamplePointWithXY(1.1, 3),
                                            SamplePointWithXY(1.4, 3),
                                            SamplePointWithXY(1.5, 3),
                                            SamplePointWithXY(2.0, 3),
                                            SamplePointWithXY(2.1, 1),
                                            SamplePointWithXY(2.5, 1),
                                        }));
  EXPECT_THAT(comp_output.point_list(), Not(Contains(SamplePointWithX(2.6))));
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
  EXPECT_THAT(comp_output.point_list(), ContainsPoints({
                                            SamplePointWithXY(1, 14),
                                            SamplePointWithXY(1.6, 16),
                                            SamplePointWithXY(1.7, 18),
                                            SamplePointWithXY(2, 20),
                                        }));
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
  EXPECT_THAT(point_list_1.point_list(), ContainsPoints({
                                             SamplePointWithXY(0.5, 10),
                                             SamplePointWithXY(0.6, 0),
                                             SamplePointWithXY(2.0, 0),
                                             SamplePointWithXY(2.5, 0),
                                             SamplePointWithXY(2.6, 20),
                                             SamplePointWithXY(3.5, 20),
                                         }));

  // window size set to 10.
  auto sb_5 = HelperCreateRWRInstance(
      {kInputKey}, {}, kOutputKey, RWRConfig::SUM, 10, kStepsPerWindow, true);
  ASSERT_NE(nullptr, sb_5);

  ASSERT_OK(sb_5->AddPoints(
      HelperCreateRWRAddPointsInput(kInputKey, {{0, 10}, {3, 20}})));
  RWRCompleteOutput point_list_5;
  ASSERT_OK(sb_5->Complete(&point_list_5));
  EXPECT_THAT(point_list_5.point_list(), ContainsPoints({
                                             SamplePointWithXY(5, 30),
                                             SamplePointWithXY(6, 20),
                                             SamplePointWithXY(7, 20),
                                         }));
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
    EXPECT_THAT(point_list_05.point_list(), Contains(SamplePointWithX(i)));
  }

  auto sb_20 = HelperCreateRWRInstance({kInputKey}, {}, kOutputKey,
                                       RWRConfig::SUM, 10, 5, true);
  ASSERT_NE(nullptr, sb_20);

  ASSERT_OK(sb_20->AddPoints(
      HelperCreateRWRAddPointsInput(kInputKey, {{0, 10}, {3, 20}})));
  RWRCompleteOutput point_list_20;
  ASSERT_OK(sb_20->Complete(&point_list_20));
  EXPECT_THAT(point_list_20.point_list(),
              ContainsPoints({SamplePointWithX(0), SamplePointWithX(2),
                              SamplePointWithX(4), SamplePointWithX(6),
                              SamplePointWithX(8)}));
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
    EXPECT_THAT(output.point_list(), Contains(SamplePointWithX(i)));
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
    EXPECT_THAT(output.point_list(), Contains(SamplePointWithXY(i, 40)));
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
  EXPECT_THAT(
      output.point_list(),
      UnorderedElementsAreArray({
          SamplePointWithXY(0.0, 0.25),
          SamplePointWithXY(0.5, 0.25),
          SamplePointWithXY(1.0, 0.5),
          SamplePointWithXY(1.5, 0.5),
          SamplePointWithXY(2.0, std::numeric_limits<double>::infinity()),
          SamplePointWithXY(2.5, std::numeric_limits<double>::infinity()),
          SamplePointWithXY(3.0, 1),
          SamplePointWithXY(3.5, 2.0 / 3.0),
          SamplePointWithXY(4.0, 0.5),
      }));
}

// TODO(b/141503378): Div-by-zero is UB
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
  EXPECT_THAT(output.point_list(), UnorderedElementsAreArray(
                                       {SamplePointWithXY(0.0, std::nan(""))}));
}

// TODO(b/141503378): Div-by-zero is UB
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

  EXPECT_THAT(
      output.point_list(),
      UnorderedElementsAreArray(
          {SamplePointWithXY(0, std::numeric_limits<double>::infinity()),
           SamplePointWithXY(3, -std::numeric_limits<double>::infinity())}));
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
  EXPECT_THAT(output.point_list(), Contains(SamplePointWithXY(-1.2, 0.25)));
  EXPECT_THAT(output.point_list(), Contains(SamplePointWithXY(-0.9, 0.25)));
  EXPECT_THAT(output.point_list(), Contains(SamplePointWithXY(-0.6, 0.25)));
  // The following data points contain x = {0, 1} points in the window
  EXPECT_THAT(output.point_list(), Contains(SamplePointWithXY(-0.3, 0.375)));
  EXPECT_THAT(output.point_list(), Contains(SamplePointWithXY(0.0, 0.375)));
  EXPECT_THAT(output.point_list(), Contains(SamplePointWithXY(0.3, 0.375)));
  // The following data points contain x = {0, 1, 2} points in the window
  EXPECT_THAT(output.point_list(), Contains(SamplePointWithXY(0.6, 0.5)));
  EXPECT_THAT(output.point_list(), Contains(SamplePointWithXY(0.9, 0.5)));
  EXPECT_THAT(output.point_list(), Contains(SamplePointWithXY(1.2, 0.5)));
  EXPECT_THAT(output.point_list(), Contains(SamplePointWithXY(1.5, 0.5)));
  // The following data points contain x = {1, 2, 3} points in the window
  EXPECT_THAT(output.point_list(), Contains(SamplePointWithXY(1.8, 5.0 / 6.0)));
  // The following data points contain x = {1, 2, 3, 3.5} points in the window
  EXPECT_THAT(output.point_list(), Contains(SamplePointWithXY(2.1, 0.7)));
  EXPECT_THAT(output.point_list(), Contains(SamplePointWithXY(2.4, 0.7)));
  // The following data points contain x = {2, 3, 3.5} points in the window
  EXPECT_THAT(output.point_list(), Contains(SamplePointWithXY(2.7, 5.0 / 6.0)));
  EXPECT_THAT(output.point_list(), Contains(SamplePointWithXY(3.0, 5.0 / 6.0)));
  EXPECT_THAT(output.point_list(), Contains(SamplePointWithXY(3.3, 5.0 / 6.0)));
  // The following data points contain x = {3, 3.5} points in the window
  EXPECT_THAT(output.point_list(), Contains(SamplePointWithXY(3.6, 2.0 / 3.0)));
  EXPECT_THAT(output.point_list(), Contains(SamplePointWithXY(3.9, 2.0 / 3.0)));
  EXPECT_THAT(output.point_list(), Contains(SamplePointWithXY(4.2, 2.0 / 3.0)));
  EXPECT_THAT(output.point_list(), Contains(SamplePointWithXY(4.5, 2.0 / 3.0)));
  // The following data points contain x = {3.5} points in the window
  EXPECT_THAT(output.point_list(), Contains(SamplePointWithXY(4.8, 0.5)));
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
  EXPECT_THAT(output.point_list(), UnorderedElementsAreArray({
                                       SamplePointWithXY(0.0, 0.25),
                                       SamplePointWithXY(0.5, 0.25),
                                       SamplePointWithXY(1.0, 0),
                                       SamplePointWithXY(1.5, 0),
                                       SamplePointWithXY(2.0, 0),
                                       SamplePointWithXY(2.5, 0),
                                       SamplePointWithXY(3.0, 0),
                                       SamplePointWithXY(3.5, 0),
                                       SamplePointWithXY(4.0, 0),
                                       SamplePointWithXY(4.5, 0),
                                       SamplePointWithXY(5.0, 2.0 / 3.0),
                                       SamplePointWithXY(5.5, 4.0 / 3.0),
                                       SamplePointWithXY(6.0, 0),
                                       SamplePointWithXY(6.5, 0),
                                       SamplePointWithXY(7.0, 0),
                                       SamplePointWithXY(7.5, 0),
                                   }));
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
  EXPECT_THAT(output.point_list(), UnorderedElementsAreArray({
                                       SamplePointWithXY(0.0, 0.25),
                                       SamplePointWithXY(0.5, 0.25),
                                       SamplePointWithXY(5.0, 2.0 / 3.0),
                                       SamplePointWithXY(5.5, 4.0 / 3.0),
                                   }));
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
    EXPECT_THAT(output.point_list(), Contains(SamplePointWithXY(i, 0.5)));
  }
}

TEST(RollingWindowReducerTest, RatesWithScalingFactor) {
  RWRConfig base_config;
  base_config.add_input_metric_keys(kInputKey);
  base_config.set_window_operation(RWRConfig::SUM);
  base_config.set_zero_for_empty_window(true);
  RWRConfig config1 = base_config;
  config1.set_output_metric_key("window1");
  config1.set_window_size(1);
  config1.set_steps_per_window(2);  // output point every 0.5
  config1.set_output_scaling_factor(1);
  RWRConfig config2 = base_config;
  config2.set_output_metric_key("window2");
  config2.set_window_size(2);
  config2.set_steps_per_window(4);  // output point every 0.5
  config2.set_output_scaling_factor(0.5);
  RWRConfig config05 = base_config;
  config05.set_output_metric_key("window05");
  config05.set_window_size(0.5);
  config05.set_steps_per_window(1);  // output point every 0.5
  config05.set_output_scaling_factor(2);

  auto reducer_or =
      RollingWindowReducer::NewMerged({config1, config2, config05});
  ASSERT_OK(reducer_or);
  auto rwr = std::move(reducer_or).value();

  ASSERT_OK(rwr->AddPoints(HelperCreateRWRAddPointsInput(
      kInputKey, {{0, 1}, {0.5, 1}, {1, 1}, {1.5, 1}, {2, 2}, {3, 1}})));

  // X-axis: 0.0  0.5  1.0  1.5  2.0  2.5  3.0  3.5  4.0
  //          |----+----|----+----|----+----|----+----|
  // wsize=1       [   1@   )[   2@   )[   3@   )
  //                    [   1@5  )[   2@5  )
  //          |----+----|----+----|----+----|----+----|
  // wsize=2  [        1@        )[        3@      )
  //                    [        2@      )
  //                         [        2@5     )
  //          |----+----|----+----|----+----|----+----|
  // wsize=.5    [ @ )[ @ )[ @ )[ @ )[ @ )[ @ )[ @ )[ @ )
  //          |----+----|----+----|----+----|----+----|
  // Points:  1    1    1    1    2    0    1    0    0   // w/added zeroes
  RWRCompleteOutput output;
  ASSERT_OK(rwr->Complete(&output));
  // Initially there is a y=1 point every half-second, maintained long enough to
  // span all window sizes. All scales should agree that rate at x=1 is 2.
  EXPECT_THAT(output.point_list(),
              ContainsPoints({ExactSamplePoint(1, "window1", 2),
                              ExactSamplePoint(1, "window2", 2),
                              ExactSamplePoint(1, "window05", 2)}));
  // At x=2 there's a y=2 value, but then there's no point at x=2.5.
  // window2 sees both and averages back to rate=2.
  // window1 sees the spike but not the drop and reports rate=3.
  // window05 has only the y=2 point and reports rate=4.
  EXPECT_THAT(output.point_list(),
              ContainsPoints({ExactSamplePoint(2, "window1", 3),
                              ExactSamplePoint(2, "window2", 2),
                              ExactSamplePoint(2, "window05", 4)}));
  // etc
  EXPECT_THAT(output.point_list(),
              ContainsPoints({ExactSamplePoint(2.5, "window1", 2),
                              ExactSamplePoint(2.5, "window2", 2),
                              ExactSamplePoint(2.5, "window05", 0),
                              ExactSamplePoint(3, "window1", 1),
                              ExactSamplePoint(3, "window2", 1.5),
                              ExactSamplePoint(3, "window05", 2)}));
}

TEST(RollingWindowReducerTest, ArbitraryScalingFactor) {
  RWRConfig config;
  config.add_input_metric_keys(kInputKey);
  config.set_zero_for_empty_window(true);
  config.set_window_size(kWindowSize);
  config.set_steps_per_window(kStepsPerWindow);
  std::vector<RWRConfig> configs;
  config.set_output_metric_key("mean");
  config.set_window_operation(RWRConfig::MEAN);
  config.set_output_scaling_factor(1);
  configs.push_back(config);
  config.set_output_metric_key("meanx10");
  config.set_output_scaling_factor(10);
  configs.push_back(config);
  config.set_output_metric_key("count");
  config.set_window_operation(RWRConfig::COUNT);
  config.set_output_scaling_factor(1);
  configs.push_back(config);
  config.set_output_metric_key("halfcount");
  config.set_output_scaling_factor(0.5);
  configs.push_back(config);
  config.set_output_metric_key("median");
  config.set_window_operation(RWRConfig::PERCENTILE);
  config.set_percentile_milli(50000);
  config.set_output_scaling_factor(1);
  configs.push_back(config);
  config.set_output_metric_key("medianx3");
  config.set_output_scaling_factor(3);
  configs.push_back(config);

  auto reducer_or = RollingWindowReducer::NewMerged({configs});
  ASSERT_OK(reducer_or);
  auto rwr = std::move(reducer_or).value();

  ASSERT_OK(rwr->AddPoints(
      HelperCreateRWRAddPointsInput(kInputKey, {{0, 0}, {0, 1}, {0, 5}})));
  RWRCompleteOutput output;
  ASSERT_OK(rwr->Complete(&output));
  EXPECT_THAT(output.point_list(),
              ContainsPoints({ExactSamplePoint(0, "mean", 2),
                              ExactSamplePoint(0, "meanx10", 20),
                              ExactSamplePoint(0, "count", 3),
                              ExactSamplePoint(0, "halfcount", 1.5),
                              ExactSamplePoint(0, "median", 1),
                              ExactSamplePoint(0, "medianx3", 3)}));
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
  EXPECT_THAT(comp_output.point_list(), Not(Contains(SamplePointWithY(0))));
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
  EXPECT_THAT(comp_output.point_list(), Contains(SamplePointWithY(0)));
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


TEST(RollingWindowReducerTest, MergeAfterPercentile) {
  RWRConfig base_config;
  base_config.add_input_metric_keys(kInputKey);
  base_config.set_zero_for_empty_window(true);
  RWRConfig config1 = base_config;
  config1.set_output_metric_key("median");
  config1.set_window_size(1);
  config1.set_window_operation(RWRConfig::PERCENTILE);
  config1.set_percentile_milli(50000);
  config1.set_steps_per_window(2);  // output point every 0.5
  RWRConfig config2 = base_config;
  config2.set_output_metric_key("sum");
  config2.set_window_size(1);
  config2.set_window_operation(RWRConfig::SUM);
  config2.set_steps_per_window(2);  // output point every 0.5

  auto reducer_or =
      RollingWindowReducer::NewMerged({config1, config2});
  ASSERT_OK(reducer_or);
  auto rwr = std::move(reducer_or).value();

  ASSERT_OK(rwr->AddPoints(HelperCreateRWRAddPointsInput(
      kInputKey, {{0, 1}, {0.5, 1}, {1, 1}, {1.5, 1}, {2, 2}, {3, 1}})));

  // X-axis: 0.0  0.5  1.0  1.5  2.0  2.5  3.0  3.5  4.0
  //          |----+----|----+----|----+----|----+----|
  // wsize=1       [   1@   )[   2@   )[   3@   )
  //                    [   1@5  )[   2@5  )
  //          |----+----|----+----|----+----|----+----|
  // Points:  1    1    1    1    2         1
  RWRCompleteOutput output;
  ASSERT_OK(rwr->Complete(&output));
  // At x=1, the window is [1, 1] so the median is 1 and the sum 2.
  EXPECT_THAT(output.point_list(),
              ContainsPoints({ExactSamplePoint(1, "median", 1),
                              ExactSamplePoint(1, "sum", 2)}));
  // At x=2, the window is [1, 2] so the median is 1.5 and the sum 3.
  EXPECT_THAT(output.point_list(),
              ContainsPoints({ExactSamplePoint(2, "median", 1.5),
                              ExactSamplePoint(2, "sum", 3)}));
  // At x=2.5 the window is [2] so the median is 2 and the sum is 2.
  // At x = 3 the window is [1] so the median is 1 and the sum is 1.
  EXPECT_THAT(output.point_list(),
              ContainsPoints({ExactSamplePoint(2.5, "median", 2),
                              ExactSamplePoint(2.5, "sum", 2),
                              ExactSamplePoint(3, "median", 1),
                              ExactSamplePoint(3, "sum", 1)}));
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
    LOG(INFO) << record.ShortDebugString();
    for (const KeyedValue& kv : record.sample_point().metric_value_list()) {
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
  // Points every 0.25 starting at 0 and going to 2, with y-val = 10.
  // Regardless of steps_per_window we should:
  //   * never get a COUNT more than 4.
  //   * MEAN should always be 10.
  //   * never get a SUM more than 40.
  const double window_size = 1;
  struct OpAndExpectations {
    RWRConfig::WindowOperation window_op;
    double max_value;
    double value_at_0, value_at_1, value_at_2;
  } ops_to_test[] = {{RWRConfig::COUNT, 4, 2, 4, 3},
                     {RWRConfig::SUM, 40, 20, 40, 30},
                     {RWRConfig::MEAN, 10, 10, 10, 10}};

  for (const auto& op_test : ops_to_test) {
    const auto window_op = op_test.window_op;
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
          ASSERT_OK(r->AddPoints(
              HelperCreateRWRAddPointsInput(kInputKey, {{i, 10}})));
        }

        RWRCompleteOutput output;
        ASSERT_OK(r->Complete(&output));

        // 3 because we are always creating points at 0, 1, 2
        ASSERT_EQ(steps_per_window * 3, output.point_list_size());

        for (const auto& sp : output.point_list()) {
          LOG(INFO) << sp.ShortDebugString();
          ASSERT_EQ(1, sp.metric_value_list_size());
          ASSERT_LE(sp.metric_value_list(0).value(), op_test.max_value);
        }
        ASSERT_THAT(output.point_list(),
                    ContainsPoints({SamplePointWithXY(0, op_test.value_at_0),
                                    SamplePointWithXY(1, op_test.value_at_1),
                                    SamplePointWithXY(2, op_test.value_at_2)}));
      }
    }
  }
}

}  // namespace
}  // namespace helpers
}  // namespace mako
