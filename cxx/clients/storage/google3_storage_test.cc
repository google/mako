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
#include "cxx/clients/storage/google3_storage.h"

#include <functional>
#include <limits>

#include "src/google/protobuf/message.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "cxx/helpers/status/canonical_errors.h"
#include "cxx/helpers/status/status.h"
#include "cxx/internal/storage_client/mock_transport.h"
#include "cxx/testing/protocol-buffer-matchers.h"
#include "proto/internal/mako_internal.pb.h"
#include "proto/internal/storage_client/storage.pb.h"
#include "spec/proto/mako.pb.h"

namespace mako {
namespace google3_storage {
namespace {

using ::mako::EqualsProto;
using ::mako::internal::MockTransport;
using ::mako_internal::SudoStorageRequest;
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::DoAll;
using ::testing::HasSubstr;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SetArgPointee;

// A retry strategy that does not sleep.
class FakeRetryStrategy : public mako::internal::StorageRetryStrategy {
 public:
  FakeRetryStrategy() : FakeRetryStrategy(std::numeric_limits<int>::max()) {}
  explicit FakeRetryStrategy(int times) : times_(times) {}
  void Do(std::function<Step()> f) override {
    for (int count = 0; count < times_; count++) {
      if (f() == StorageRetryStrategy::kBreak) {
        return;
      }
    }
  }

 private:
  int times_;
};

class StorageTest : public ::testing::Test {
 protected:
  StorageTest()
      : storage_(absl::make_unique<NiceMock<MockTransport>>(),
                 absl::make_unique<FakeRetryStrategy>()) {

    EXPECT_CALL(Transport(), Connect()).Times(AnyNumber());
  }

  ~StorageTest() override {
  }

  MockTransport& Transport() {
    return *dynamic_cast<MockTransport*>(storage_.transport());
  }

  Storage storage_;
};

TEST_F(StorageTest, ProjectCreation) {
  CreationResponse mock_response;
  mock_response.set_key("foo_project");
  mock_response.mutable_status()->set_code(Status::SUCCESS);

  internal::CreateProjectInfoRequest request;
  ProjectInfo& project_info = *request.mutable_payload();
  project_info.set_project_name("foo_project");
  project_info.set_project_alias("My Foo Project");
  project_info.add_owner_list("foo@foogle.com");
  project_info.mutable_default_issue_tracker()
      ->mutable_buganizer_config()
      ->set_component_id("123456789");

  EXPECT_CALL(Transport(), CreateProjectInfo(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  CreationResponse actual_response;
  EXPECT_TRUE(storage_.CreateProjectInfo(project_info, &actual_response));
  EXPECT_THAT(actual_response, EqualsProto(mock_response));
}

TEST_F(StorageTest, ProjectUpdate) {
  ModificationResponse mock_response;
  mock_response.mutable_status()->set_code(Status::SUCCESS);
  mock_response.set_count(1);

  internal::UpdateProjectInfoRequest request;
  ProjectInfo& project_info = *request.mutable_payload();
  project_info.set_project_name("foo_project");
  project_info.set_project_alias("My Foo Project");
  project_info.add_owner_list("foo@foogle.com");
  project_info.mutable_default_issue_tracker()
      ->mutable_buganizer_config()
      ->set_component_id("123456789");

  EXPECT_CALL(Transport(), UpdateProjectInfo(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  ModificationResponse actual_response;
  EXPECT_TRUE(storage_.UpdateProjectInfo(project_info, &actual_response));
  EXPECT_THAT(actual_response, EqualsProto(mock_response));
}

TEST_F(StorageTest, ProjectGet) {
  ProjectInfoGetResponse mock_response;
  mock_response.mutable_status()->set_code(Status::SUCCESS);
  auto expected = mock_response.mutable_project_info();
  expected->set_project_name("foo_project");
  expected->set_project_alias("My Foo Project");
  expected->add_owner_list("foo@foogle.com");
  expected->mutable_default_issue_tracker()
      ->mutable_buganizer_config()
      ->set_component_id("123456789");

  internal::GetProjectInfoRequest request;
  ProjectInfo& project_info = *request.mutable_payload();
  std::string project_name = "foo_project";
  project_info.set_project_name(project_name);

  EXPECT_CALL(Transport(), GetProjectInfo(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  ProjectInfoGetResponse actual_response;
  EXPECT_TRUE(storage_.GetProjectInfoByName(project_name, &actual_response));
  EXPECT_THAT(actual_response, EqualsProto(mock_response));
}

TEST_F(StorageTest, ProjectQuery) {
  ProjectInfoQueryResponse mock_response;
  mock_response.mutable_status()->set_code(Status::SUCCESS);
  auto expected = mock_response.add_project_info_list();
  expected->set_project_name("foo");
  expected->set_project_alias("FOO");
  expected->add_owner_list("foo@foo.com");
  expected = mock_response.add_project_info_list();
  expected->set_project_name("bar");
  expected->set_project_alias("BAR");
  expected->add_owner_list("bar@bar.com");
  expected->mutable_default_issue_tracker()
      ->mutable_buganizer_config()
      ->set_component_id("123456789");
  mock_response.set_cursor("");

  internal::QueryProjectInfoRequest request;
  ProjectInfoQuery& project_info_query = *request.mutable_payload();

  EXPECT_CALL(Transport(), QueryProjectInfo(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  ProjectInfoQueryResponse response;
  EXPECT_TRUE(storage_.QueryProjectInfo(project_info_query, &response));
  EXPECT_THAT(response, EqualsProto(mock_response));
}

TEST_F(StorageTest, BenchmarkCreation) {
  CreationResponse mock_response;
  mock_response.set_key("123456");
  mock_response.mutable_status()->set_code(Status::SUCCESS);

  internal::CreateBenchmarkInfoRequest request;
  BenchmarkInfo& benchmark_info = *request.mutable_payload();
  benchmark_info.set_benchmark_name("My C++ Project");

  EXPECT_CALL(Transport(), CreateBenchmarkInfo(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  CreationResponse response;
  EXPECT_TRUE(storage_.CreateBenchmarkInfo(benchmark_info, &response));
  EXPECT_THAT(response, EqualsProto(mock_response));
}

TEST_F(StorageTest, BenchmarkQuery) {
  BenchmarkInfoQueryResponse mock_response;
  BenchmarkInfo* query_result = mock_response.add_benchmark_info_list();
  query_result->set_benchmark_key("123");
  mock_response.mutable_status()->set_code(Status::SUCCESS);
  mock_response.set_cursor("");

  internal::QueryBenchmarkInfoRequest request;
  BenchmarkInfoQuery& benchmark_info_query = *request.mutable_payload();
  benchmark_info_query.set_project_name("mako");

  EXPECT_CALL(Transport(), QueryBenchmarkInfo(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  BenchmarkInfoQueryResponse response;
  EXPECT_TRUE(storage_.QueryBenchmarkInfo(benchmark_info_query, &response));
  EXPECT_THAT(response, EqualsProto(mock_response));
}

TEST_F(StorageTest, BenchmarkUpdate) {
  ModificationResponse mock_response;
  mock_response.set_count(1);
  mock_response.mutable_status()->set_code(Status::SUCCESS);

  internal::UpdateBenchmarkInfoRequest request;
  BenchmarkInfo& benchmark_info = *request.mutable_payload();
  benchmark_info.set_benchmark_key("123");

  EXPECT_CALL(Transport(), UpdateBenchmarkInfo(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  ModificationResponse response;
  EXPECT_TRUE(storage_.UpdateBenchmarkInfo(benchmark_info, &response));
  EXPECT_THAT(response, EqualsProto(mock_response));
}

TEST_F(StorageTest, BenchmarkDelete) {
  ModificationResponse mock_response;
  mock_response.set_count(1);
  mock_response.mutable_status()->set_code(Status::SUCCESS);

  internal::DeleteBenchmarkInfoRequest request;
  BenchmarkInfoQuery& benchmark_info_query = *request.mutable_payload();
  benchmark_info_query.set_project_name("mako");

  EXPECT_CALL(Transport(), DeleteBenchmarkInfo(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  ModificationResponse response;
  EXPECT_TRUE(storage_.DeleteBenchmarkInfo(benchmark_info_query, &response));
  EXPECT_THAT(response, EqualsProto(mock_response));
}

TEST_F(StorageTest, BenchmarkCountQuery) {
  CountResponse mock_response;
  mock_response.set_count(11);
  mock_response.mutable_status()->set_code(Status::SUCCESS);

  internal::CountBenchmarkInfoRequest request;
  BenchmarkInfoQuery& benchmark_info_query = *request.mutable_payload();
  benchmark_info_query.set_project_name("mako");

  EXPECT_CALL(Transport(), CountBenchmarkInfo(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  CountResponse response;
  EXPECT_TRUE(storage_.CountBenchmarkInfo(benchmark_info_query, &response));
  EXPECT_THAT(response, EqualsProto(mock_response));
}

TEST_F(StorageTest, RunCreation) {
  CreationResponse mock_response;
  mock_response.set_key("122344");  // this is an arbitrary number
  mock_response.mutable_status()->set_code(Status::SUCCESS);

  internal::CreateRunInfoRequest request;
  RunInfo& run_info = *request.mutable_payload();
  run_info.set_benchmark_key("34566");  // this is an arbitrary number

  EXPECT_CALL(Transport(), CreateRunInfo(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  CreationResponse response;
  EXPECT_TRUE(storage_.CreateRunInfo(run_info, &response));
  EXPECT_THAT(response, EqualsProto(mock_response));
}

TEST_F(StorageTest, RunCreationAddlTags) {
  CreationResponse mock_response;
  mock_response.set_key("122344");  // this is an arbitrary number
  mock_response.mutable_status()->set_code(Status::SUCCESS);

  absl::SetFlag(&FLAGS_mako_internal_additional_tags,
                {"test1=val1", "test2=val2"});

  RunInfo run_info;
  run_info.set_benchmark_key("34566");  // this is an arbitrary number

  internal::CreateRunInfoRequest request;
  RunInfo& exp_run_info = *request.mutable_payload();
  exp_run_info.set_benchmark_key(run_info.benchmark_key());
  exp_run_info.add_tags("test1=val1");
  exp_run_info.add_tags("test2=val2");

  EXPECT_CALL(Transport(), CreateRunInfo(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  CreationResponse response;
  EXPECT_TRUE(storage_.CreateRunInfo(run_info, &response));
  EXPECT_THAT(response, EqualsProto(mock_response));
}

TEST_F(StorageTest, RunCreationAddlTagsEnv) {
  CreationResponse mock_response;
  mock_response.set_key("122344");  // this is an arbitrary number
  mock_response.mutable_status()->set_code(Status::SUCCESS);

  setenv("MAKO_INTERNAL_ADDITIONAL_TAGS", "test1=val1,test2=val2", 1);
  RunInfo run_info;
  run_info.set_benchmark_key("34566");  // this is an arbitrary number

  internal::CreateRunInfoRequest request;
  RunInfo& exp_run_info = *request.mutable_payload();
  exp_run_info.set_benchmark_key(run_info.benchmark_key());
  exp_run_info.add_tags("test1=val1");
  exp_run_info.add_tags("test2=val2");

  EXPECT_CALL(Transport(), CreateRunInfo(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  CreationResponse response;
  EXPECT_TRUE(storage_.CreateRunInfo(run_info, &response));
  EXPECT_THAT(response, EqualsProto(mock_response));
  unsetenv("MAKO_INTERNAL_ADDITIONAL_TAGS");
}

TEST_F(StorageTest, RunCreationTestPassOverride) {
  CreationResponse mock_response;
  mock_response.set_key("122344");  // this is an arbitrary number
  mock_response.mutable_status()->set_code(Status::SUCCESS);

  absl::SetFlag(&FLAGS_mako_internal_test_pass_id_override, "1234");
  RunInfo run_info;
  run_info.set_benchmark_key("34566");  // this is an arbitrary number
  run_info.set_test_pass_id("4567");

  internal::CreateRunInfoRequest request;
  RunInfo& exp_run_info = *request.mutable_payload();
  exp_run_info.set_benchmark_key(run_info.benchmark_key());
  exp_run_info.set_test_pass_id("1234");

  EXPECT_CALL(Transport(), CreateRunInfo(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  CreationResponse response;
  EXPECT_TRUE(storage_.CreateRunInfo(run_info, &response));
  EXPECT_THAT(response, EqualsProto(mock_response));
}

TEST_F(StorageTest, RunCreationTestPassOverrideEnv) {
  CreationResponse mock_response;
  mock_response.set_key("122344");  // this is an arbitrary number
  mock_response.mutable_status()->set_code(Status::SUCCESS);

  setenv("MAKO_INTERNAL_TEST_PASS_ID_OVERRIDE", "1234", 1);
  RunInfo run_info;
  run_info.set_benchmark_key("34566");  // this is an arbitrary number
  run_info.set_test_pass_id("4567");

  internal::CreateRunInfoRequest request;
  RunInfo& exp_run_info = *request.mutable_payload();
  exp_run_info.set_benchmark_key(run_info.benchmark_key());
  exp_run_info.set_test_pass_id("1234");

  EXPECT_CALL(Transport(), CreateRunInfo(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  CreationResponse response;
  EXPECT_TRUE(storage_.CreateRunInfo(run_info, &response));
  EXPECT_THAT(response, EqualsProto(mock_response));
  unsetenv("MAKO_INTERNAL_TEST_PASS_ID_OVERRIDE");
}

TEST_F(StorageTest, RunCreationTestPassOverrideEnvAndFlag) {
  CreationResponse mock_response;
  mock_response.set_key("122344");  // this is an arbitrary number
  mock_response.mutable_status()->set_code(Status::SUCCESS);

  absl::SetFlag(&FLAGS_mako_internal_test_pass_id_override, "12345");
  setenv("MAKO_INTERNAL_TEST_PASS_ID_OVERRIDE", "1234", 1);
  RunInfo run_info;
  run_info.set_benchmark_key("34566");  // this is an arbitrary number
  run_info.set_test_pass_id("4567");

  internal::CreateRunInfoRequest request;
  RunInfo& exp_run_info = *request.mutable_payload();
  exp_run_info.set_benchmark_key(run_info.benchmark_key());
  exp_run_info.set_test_pass_id("12345");

  EXPECT_CALL(Transport(), CreateRunInfo(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  CreationResponse response;
  EXPECT_TRUE(storage_.CreateRunInfo(run_info, &response));
  EXPECT_THAT(response, EqualsProto(mock_response));
  unsetenv("MAKO_INTERNAL_TEST_PASS_ID_OVERRIDE");
}

TEST_F(StorageTest, RunQuery) {
  RunInfoQueryResponse mock_response;
  RunInfo* query_result = mock_response.add_run_info_list();
  query_result->set_run_key("1234");
  mock_response.mutable_status()->set_code(Status::SUCCESS);
  mock_response.set_cursor("");

  internal::QueryRunInfoRequest request;
  RunInfoQuery& run_info_query = *request.mutable_payload();
  run_info_query.set_min_timestamp_ms(4.4);

  EXPECT_CALL(Transport(), QueryRunInfo(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  RunInfoQueryResponse response;
  EXPECT_TRUE(storage_.QueryRunInfo(run_info_query, &response));
  EXPECT_THAT(response, EqualsProto(mock_response));
}

TEST_F(StorageTest, RunUpdate) {
  ModificationResponse mock_response;
  mock_response.set_count(1);
  mock_response.mutable_status()->set_code(Status::SUCCESS);

  internal::UpdateRunInfoRequest request;
  RunInfo& run_info = *request.mutable_payload();
  run_info.set_run_key("123");

  EXPECT_CALL(Transport(), UpdateRunInfo(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  ModificationResponse response;
  EXPECT_TRUE(storage_.UpdateRunInfo(run_info, &response));
  EXPECT_THAT(response, EqualsProto(mock_response));
}

TEST_F(StorageTest, RunUpdateTestPassOverrideFlag) {
  ModificationResponse mock_response;
  mock_response.set_count(1);
  mock_response.mutable_status()->set_code(Status::SUCCESS);

  absl::SetFlag(&FLAGS_mako_internal_test_pass_id_override, "new");
  RunInfo run_info;
  run_info.set_run_key("123");
  run_info.set_test_pass_id("original");

  internal::UpdateRunInfoRequest request;
  RunInfo& exp_run_info = *request.mutable_payload();
  exp_run_info.set_run_key(run_info.run_key());
  exp_run_info.set_test_pass_id("new");

  EXPECT_CALL(Transport(), UpdateRunInfo(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  ModificationResponse response;
  EXPECT_TRUE(storage_.UpdateRunInfo(run_info, &response));
  EXPECT_THAT(response, EqualsProto(mock_response));
}

TEST_F(StorageTest, RunUpdateTestPassOverrideEnv) {
  ModificationResponse mock_response;
  mock_response.set_count(1);
  mock_response.mutable_status()->set_code(Status::SUCCESS);

  setenv("MAKO_INTERNAL_TEST_PASS_ID_OVERRIDE", "new", 1);
  RunInfo run_info;
  run_info.set_run_key("123");
  run_info.set_test_pass_id("original");

  internal::UpdateRunInfoRequest request;
  RunInfo& exp_run_info = *request.mutable_payload();
  exp_run_info.set_run_key(run_info.run_key());
  exp_run_info.set_test_pass_id("new");

  EXPECT_CALL(Transport(), UpdateRunInfo(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  ModificationResponse response;
  EXPECT_TRUE(storage_.UpdateRunInfo(run_info, &response));
  EXPECT_THAT(response, EqualsProto(mock_response));
  unsetenv("MAKO_INTERNAL_TEST_PASS_ID_OVERRIDE");
}

TEST_F(StorageTest, RunUpdateTestPassOverrideEnvAndFlag) {
  ModificationResponse mock_response;
  mock_response.set_count(1);
  mock_response.mutable_status()->set_code(Status::SUCCESS);

  absl::SetFlag(&FLAGS_mako_internal_test_pass_id_override, "new");
  setenv("MAKO_INTERNAL_TEST_PASS_ID_OVERRIDE", "newer", 1);
  RunInfo run_info;
  run_info.set_run_key("123");
  run_info.set_test_pass_id("original");

  internal::UpdateRunInfoRequest request;
  RunInfo& exp_run_info = *request.mutable_payload();
  exp_run_info.set_run_key(run_info.run_key());
  exp_run_info.set_test_pass_id("new");

  EXPECT_CALL(Transport(), UpdateRunInfo(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  ModificationResponse response;
  EXPECT_TRUE(storage_.UpdateRunInfo(run_info, &response));
  EXPECT_THAT(response, EqualsProto(mock_response));
  unsetenv("MAKO_INTERNAL_TEST_PASS_ID_OVERRIDE");
}

TEST_F(StorageTest, RunUpdateUniqueAdditionalTagsFlag) {
  ModificationResponse mock_response;
  mock_response.set_count(1);
  mock_response.mutable_status()->set_code(Status::SUCCESS);

  std::vector<std::string> flag_tags = {"tag1=val1", "tag2=val2", "tag3=val3",
                                        "tag4=val4"};
  absl::SetFlag(&FLAGS_mako_internal_additional_tags, flag_tags);
  RunInfo run_info;
  run_info.set_run_key("123");
  *run_info.add_tags() = "tag2=val3";
  *run_info.add_tags() = "tag1=val1";
  *run_info.add_tags() = "tag3=val3";
  *run_info.add_tags() = "tag5=val5";

  internal::UpdateRunInfoRequest request;
  RunInfo& exp_run_info = *request.mutable_payload();
  exp_run_info.set_run_key(run_info.run_key());
  *exp_run_info.add_tags() = "tag2=val3";
  *exp_run_info.add_tags() = "tag1=val1";
  *exp_run_info.add_tags() = "tag3=val3";
  *exp_run_info.add_tags() = "tag5=val5";
  *exp_run_info.add_tags() = "tag2=val2";
  *exp_run_info.add_tags() = "tag4=val4";

  EXPECT_CALL(Transport(), UpdateRunInfo(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  ModificationResponse response;
  EXPECT_TRUE(storage_.UpdateRunInfo(run_info, &response));
  EXPECT_THAT(response, EqualsProto(mock_response));
}

TEST_F(StorageTest, RunUpdateUniqueAdditionalTagsEnv) {
  ModificationResponse mock_response;
  mock_response.set_count(1);
  mock_response.mutable_status()->set_code(Status::SUCCESS);

  setenv("MAKO_INTERNAL_ADDITIONAL_TAGS",
         "tag1=val1,tag2=val2,tag3=val3,tag4=val4", 1);
  RunInfo run_info;
  run_info.set_run_key("123");
  *run_info.add_tags() = "tag2=val3";
  *run_info.add_tags() = "tag1=val1";
  *run_info.add_tags() = "tag3=val3";
  *run_info.add_tags() = "tag5=val5";

  internal::UpdateRunInfoRequest request;
  RunInfo& exp_run_info = *request.mutable_payload();
  exp_run_info.set_run_key(run_info.run_key());
  *exp_run_info.add_tags() = "tag2=val3";
  *exp_run_info.add_tags() = "tag1=val1";
  *exp_run_info.add_tags() = "tag3=val3";
  *exp_run_info.add_tags() = "tag5=val5";
  *exp_run_info.add_tags() = "tag2=val2";
  *exp_run_info.add_tags() = "tag4=val4";

  EXPECT_CALL(Transport(), UpdateRunInfo(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  ModificationResponse response;
  EXPECT_TRUE(storage_.UpdateRunInfo(run_info, &response));
  EXPECT_THAT(response, EqualsProto(mock_response));
  unsetenv("MAKO_INTERNAL_ADDITIONAL_TAGS");
}

TEST_F(StorageTest, RunUpdateUniqueAdditionalTagsFlagWhitespace) {
  ModificationResponse mock_response;
  mock_response.set_count(1);
  mock_response.mutable_status()->set_code(Status::SUCCESS);

  std::vector<std::string> flag_tags = {"tag1=val1 ", "   tag2=val2  \t  \n",
                                        " tag3=val3", "tag4=val4\n"};
  absl::SetFlag(&FLAGS_mako_internal_additional_tags, flag_tags);
  RunInfo run_info;
  run_info.set_run_key("123");
  *run_info.add_tags() = "tag2=val3";
  *run_info.add_tags() = "tag1=val1";
  *run_info.add_tags() = "tag3=val3";
  *run_info.add_tags() = "tag5=val5";

  internal::UpdateRunInfoRequest request;
  RunInfo& exp_run_info = *request.mutable_payload();
  exp_run_info.set_run_key(run_info.run_key());
  *exp_run_info.add_tags() = "tag2=val3";
  *exp_run_info.add_tags() = "tag1=val1";
  *exp_run_info.add_tags() = "tag3=val3";
  *exp_run_info.add_tags() = "tag5=val5";
  *exp_run_info.add_tags() = "tag2=val2";
  *exp_run_info.add_tags() = "tag4=val4";

  EXPECT_CALL(Transport(), UpdateRunInfo(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  ModificationResponse response;
  EXPECT_TRUE(storage_.UpdateRunInfo(run_info, &response));
  EXPECT_THAT(response, EqualsProto(mock_response));
}

TEST_F(StorageTest, RunUpdateUniqueAdditionalTagsEnvWhitespace) {
  ModificationResponse mock_response;
  mock_response.set_count(1);
  mock_response.mutable_status()->set_code(Status::SUCCESS);

  setenv("MAKO_INTERNAL_ADDITIONAL_TAGS",
         "tag1=val1,tag2=val2,   tag3=val3  \t \n,tag4=val4", 1);
  RunInfo run_info;
  run_info.set_run_key("123");
  *run_info.add_tags() = "tag2=val3";
  *run_info.add_tags() = "tag1=val1";
  *run_info.add_tags() = "tag3=val3";
  *run_info.add_tags() = "tag5=val5";

  internal::UpdateRunInfoRequest request;
  RunInfo& exp_run_info = *request.mutable_payload();
  exp_run_info.set_run_key(run_info.run_key());
  *exp_run_info.add_tags() = "tag2=val3";
  *exp_run_info.add_tags() = "tag1=val1";
  *exp_run_info.add_tags() = "tag3=val3";
  *exp_run_info.add_tags() = "tag5=val5";
  *exp_run_info.add_tags() = "tag2=val2";
  *exp_run_info.add_tags() = "tag4=val4";

  EXPECT_CALL(Transport(), UpdateRunInfo(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  ModificationResponse response;
  EXPECT_TRUE(storage_.UpdateRunInfo(run_info, &response));
  EXPECT_THAT(response, EqualsProto(mock_response));
  unsetenv("MAKO_INTERNAL_ADDITIONAL_TAGS");
}

TEST_F(StorageTest, RunUpdateUniqueAdditionalTagsFlagAndEnv) {
  ModificationResponse mock_response;
  mock_response.set_count(1);
  mock_response.mutable_status()->set_code(Status::SUCCESS);

  std::vector<std::string> flag_tags = {"tag1=val1", "tag2=val2", "tag3=val3",
                                        "tag4=val4"};
  absl::SetFlag(&FLAGS_mako_internal_additional_tags, flag_tags);
  setenv("MAKO_INTERNAL_ADDITIONAL_TAGS",
         "tag1=val1,tag2=val2,tag3=val3,tag6=val6", 1);
  RunInfo run_info;
  run_info.set_run_key("123");
  *run_info.add_tags() = "tag2=val3";
  *run_info.add_tags() = "tag1=val1";
  *run_info.add_tags() = "tag3=val3";
  *run_info.add_tags() = "tag5=val5";

  internal::UpdateRunInfoRequest request;
  RunInfo& exp_run_info = *request.mutable_payload();
  exp_run_info.set_run_key(run_info.run_key());
  *exp_run_info.add_tags() = "tag2=val3";
  *exp_run_info.add_tags() = "tag1=val1";
  *exp_run_info.add_tags() = "tag3=val3";
  *exp_run_info.add_tags() = "tag5=val5";
  *exp_run_info.add_tags() = "tag2=val2";
  *exp_run_info.add_tags() = "tag4=val4";

  EXPECT_CALL(Transport(), UpdateRunInfo(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  ModificationResponse response;
  EXPECT_TRUE(storage_.UpdateRunInfo(run_info, &response));
  EXPECT_THAT(response, EqualsProto(mock_response));
  unsetenv("MAKO_INTERNAL_ADDITIONAL_TAGS");
}

TEST_F(StorageTest, RunDelete) {
  ModificationResponse mock_response;
  mock_response.set_count(1);
  mock_response.mutable_status()->set_code(Status::SUCCESS);

  internal::DeleteRunInfoRequest request;
  RunInfoQuery& run_info_query = *request.mutable_payload();
  run_info_query.set_max_timestamp_ms(23.0);

  EXPECT_CALL(Transport(), DeleteRunInfo(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  ModificationResponse response;
  EXPECT_TRUE(storage_.DeleteRunInfo(run_info_query, &response));
  EXPECT_THAT(response, EqualsProto(mock_response));
}

TEST_F(StorageTest, RunCountQuery) {
  CountResponse mock_response;
  mock_response.set_count(13);
  mock_response.mutable_status()->set_code(Status::SUCCESS);

  internal::CountRunInfoRequest request;
  RunInfoQuery& run_info_query = *request.mutable_payload();
  run_info_query.set_benchmark_key("xxxx");

  EXPECT_CALL(Transport(), CountRunInfo(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  CountResponse response;
  EXPECT_TRUE(storage_.CountRunInfo(run_info_query, &response));
  EXPECT_THAT(response, EqualsProto(mock_response));
}

TEST_F(StorageTest, SampleBatchCreation) {
  CreationResponse mock_response;
  mock_response.set_key("123456");
  mock_response.mutable_status()->set_code(Status::SUCCESS);

  internal::CreateSampleBatchRequest request;
  SampleBatch& sample_batch = *request.mutable_payload();
  sample_batch.set_run_key("98787");

  EXPECT_CALL(Transport(), CreateSampleBatch(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  CreationResponse response;
  EXPECT_TRUE(storage_.CreateSampleBatch(sample_batch, &response));
  EXPECT_THAT(response, EqualsProto(mock_response));
}

TEST_F(StorageTest, SampleBatchCreationWithAuxData) {
  CreationResponse mock_response;
  mock_response.set_key("123456");
  mock_response.mutable_status()->set_code(Status::SUCCESS);

  internal::CreateSampleBatchRequest request;
  SampleBatch& sample_batch = *request.mutable_payload();
  sample_batch.set_run_key("98787");
  SamplePoint* point = sample_batch.add_sample_point_list();
  point->set_input_value(123);

  EXPECT_CALL(Transport(), CreateSampleBatch(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  SampleBatch batch_with_aux = sample_batch;
  batch_with_aux.mutable_sample_point_list(0)->mutable_aux_data()->insert(
      {"aux", "data"});

  CreationResponse response;
  EXPECT_TRUE(storage_.CreateSampleBatch(batch_with_aux, &response));
  EXPECT_THAT(response, EqualsProto(mock_response));
}

TEST_F(StorageTest, SampleBatchQuery) {
  SampleBatchQueryResponse mock_response;
  SampleBatch* query_result = mock_response.add_sample_batch_list();
  query_result->set_benchmark_key("123");
  mock_response.mutable_status()->set_code(Status::SUCCESS);
  mock_response.set_cursor("");

  internal::QuerySampleBatchRequest request;
  SampleBatchQuery& sample_batch_query = *request.mutable_payload();
  sample_batch_query.set_benchmark_key("098978");

  EXPECT_CALL(Transport(), QuerySampleBatch(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  SampleBatchQueryResponse response;
  EXPECT_TRUE(storage_.QuerySampleBatch(sample_batch_query, &response));
  EXPECT_THAT(response, EqualsProto(mock_response));
}

TEST_F(StorageTest, SampleBatchDelete) {
  ModificationResponse mock_response;
  mock_response.set_count(1);
  mock_response.mutable_status()->set_code(Status::SUCCESS);

  internal::DeleteSampleBatchRequest request;
  SampleBatchQuery& sample_batch_query = *request.mutable_payload();
  sample_batch_query.set_batch_key("109872");

  EXPECT_CALL(Transport(), DeleteSampleBatch(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  ModificationResponse response;
  EXPECT_TRUE(storage_.DeleteSampleBatch(sample_batch_query, &response));
  EXPECT_THAT(response, EqualsProto(mock_response));
}

TEST_F(StorageTest, GetMetricValueCountMax) {
  int metric_value_max;
  EXPECT_TRUE(storage_.GetMetricValueCountMax(&metric_value_max).empty());
  EXPECT_GT(metric_value_max, 0);
}

TEST_F(StorageTest, GetSampleErrorCountMax) {
  int sample_error_max;
  EXPECT_TRUE(storage_.GetSampleErrorCountMax(&sample_error_max).empty());
  EXPECT_GT(sample_error_max, 0);
}

TEST_F(StorageTest, GetBatchSizeMax) {
  int batch_size_max;
  EXPECT_TRUE(storage_.GetBatchSizeMax(&batch_size_max).empty());
  EXPECT_GT(batch_size_max, 0);
}

TEST_F(StorageTest, FailureAndRecovery) {
  CreationResponse mock_response;
  Status* status = mock_response.mutable_status();
  status->set_code(Status::FAIL);
  status->set_fail_message("Creation failed b/c a solar flare");

  EXPECT_CALL(Transport(), CreateBenchmarkInfo(_, _, _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  BenchmarkInfo benchmark_info;
  CreationResponse response;
  EXPECT_FALSE(storage_.CreateBenchmarkInfo(benchmark_info, &response));
  EXPECT_THAT(response, EqualsProto(mock_response));
}

TEST_F(StorageTest, RetryableAndNonRetryableFailures) {
  std::string error_string = "Creation failed b/c a solar flare";
  CreationResponse mock_retryable_creation_response;
  CreationResponse mock_nonretryable_creation_response;

  Status* status = mock_retryable_creation_response.mutable_status();
  status->set_code(Status::FAIL);
  status->set_fail_message(error_string);
  status->set_retry(true);

  status = mock_nonretryable_creation_response.mutable_status();
  status->set_code(Status::FAIL);
  status->set_fail_message(error_string);
  status->set_retry(false);

  EXPECT_CALL(Transport(), CreateBenchmarkInfo(_, _, _))
      .WillOnce(DoAll(SetArgPointee<2>(mock_retryable_creation_response),
                      Return(helpers::OkStatus())))
      .WillOnce(DoAll(SetArgPointee<2>(mock_retryable_creation_response),
                      Return(helpers::OkStatus())))
      .WillOnce(DoAll(SetArgPointee<2>(mock_nonretryable_creation_response),
                      Return(helpers::OkStatus())));

  BenchmarkInfo benchmark_info;
  CreationResponse response;
  EXPECT_FALSE(storage_.CreateBenchmarkInfo(benchmark_info, &response));
  EXPECT_THAT(response, EqualsProto(mock_nonretryable_creation_response));
}

TEST_F(StorageTest, RetryableAndNonRetryableTransportLevelFailures) {
  std::string error_string = "Creation failed b/c a solar flare";

  EXPECT_CALL(Transport(), CreateBenchmarkInfo(_, _, _))
      .WillOnce(Return(helpers::UnavailableError(error_string)))
      .WillOnce(Return(helpers::UnavailableError(error_string)))
      .WillOnce(Return(helpers::FailedPreconditionError(error_string)));

  BenchmarkInfo benchmark_info;
  CreationResponse response;
  EXPECT_FALSE(storage_.CreateBenchmarkInfo(benchmark_info, &response));

  EXPECT_EQ(response.status().code(), Status::FAIL);
  EXPECT_FALSE(response.status().retry());
  EXPECT_THAT(response.status().fail_message(), HasSubstr(error_string));
}

TEST_F(StorageTest, TimeoutFails) {
  storage_ = Storage(absl::make_unique<NiceMock<MockTransport>>(),
                     absl::make_unique<FakeRetryStrategy>(10));
  EXPECT_CALL(Transport(), Connect()).Times(AnyNumber());

  // First request fails, after that will succeed.
  CreationResponse failed_mock_response;
  failed_mock_response.mutable_status()->set_code(Status::FAIL);
  failed_mock_response.mutable_status()->set_retry(true);
  failed_mock_response.mutable_status()->set_fail_message("some failure");

  EXPECT_CALL(Transport(), CreateBenchmarkInfo(_, _, _))
      .Times(10)
      .WillRepeatedly(DoAll(SetArgPointee<2>(failed_mock_response),
                            Return(helpers::OkStatus())));

  CreationResponse response;
  BenchmarkInfo benchmark_info;
  EXPECT_FALSE(storage_.CreateBenchmarkInfo(benchmark_info, &response));
  EXPECT_THAT(response, EqualsProto(failed_mock_response));
}

TEST_F(StorageTest, ConnectFails) {
  storage_ = Storage(absl::make_unique<NiceMock<MockTransport>>(),
                     absl::make_unique<FakeRetryStrategy>(3));

  std::string error_string = "Connect failed for reasons";
  EXPECT_CALL(Transport(), Connect())
      .Times(3)
      .WillRepeatedly(Return(helpers::UnavailableError(error_string)));

  BenchmarkInfo benchmark_info;
  CreationResponse response;
  EXPECT_FALSE(storage_.CreateBenchmarkInfo(benchmark_info, &response));

  EXPECT_EQ(response.status().code(), Status::FAIL);
  EXPECT_TRUE(response.status().retry());
  EXPECT_THAT(response.status().fail_message(), HasSubstr(error_string));
}

TEST_F(StorageTest, Sudo) {
  absl::SetFlag(&FLAGS_mako_internal_sudo_run_as, "sudo@foogle.com");

  CreationResponse mock_response;
  mock_response.mutable_status()->set_code(Status::SUCCESS);

  internal::CreateBenchmarkInfoRequest request;
  BenchmarkInfo& benchmark_info = *request.mutable_payload();
  benchmark_info.set_benchmark_name("My C++ Project");
  request.mutable_request_options()->set_sudo_run_as("sudo@foogle.com");

  EXPECT_CALL(Transport(), CreateBenchmarkInfo(_, EqualsProto(request), _))
      .WillOnce(
          DoAll(SetArgPointee<2>(mock_response), Return(helpers::OkStatus())));

  CreationResponse response;
  EXPECT_TRUE(storage_.CreateBenchmarkInfo(benchmark_info, &response));
  EXPECT_THAT(response, EqualsProto(mock_response));
}

TEST_F(StorageTest, RunInfoQueryResponseHasCursor) {
  RunInfoQuery query;
  RunInfoQueryResponse response;
  response.mutable_status()->set_code(Status::SUCCESS);

  EXPECT_CALL(Transport(), QueryRunInfo)
      .WillOnce(DoAll(SetArgPointee<2>(response), Return(absl::OkStatus())));

  ASSERT_TRUE(storage_.QueryRunInfo(query, &response));
  EXPECT_TRUE(response.has_cursor());
}

TEST_F(StorageTest, BenchmarkInfoQueryResponseHasCursor) {
  BenchmarkInfoQuery query;
  BenchmarkInfoQueryResponse response;
  response.mutable_status()->set_code(Status::SUCCESS);

  EXPECT_CALL(Transport(), QueryBenchmarkInfo)
      .WillOnce(DoAll(SetArgPointee<2>(response), Return(absl::OkStatus())));

  ASSERT_TRUE(storage_.QueryBenchmarkInfo(query, &response));
  EXPECT_TRUE(response.has_cursor());
}

TEST_F(StorageTest, SampleBatchQueryResponseHasCursor) {
  SampleBatchQuery query;
  SampleBatchQueryResponse response;
  response.mutable_status()->set_code(Status::SUCCESS);

  EXPECT_CALL(Transport(), QuerySampleBatch)
      .WillOnce(DoAll(SetArgPointee<2>(response), Return(absl::OkStatus())));

  ASSERT_TRUE(storage_.QuerySampleBatch(query, &response));
  EXPECT_TRUE(response.has_cursor());
}

TEST_F(StorageTest, ProjectInfoQueryResponseHasCursor) {
  ProjectInfoQuery query;
  ProjectInfoQueryResponse response;
  response.mutable_status()->set_code(Status::SUCCESS);

  EXPECT_CALL(Transport(), QueryProjectInfo)
      .WillOnce(DoAll(SetArgPointee<2>(response), Return(absl::OkStatus())));

  ASSERT_TRUE(storage_.QueryProjectInfo(query, &response));
  EXPECT_TRUE(response.has_cursor());
}

}  // namespace
}  // namespace google3_storage
}  // namespace mako
