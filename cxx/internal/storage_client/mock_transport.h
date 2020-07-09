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
#ifndef CXX_INTERNAL_STORAGE_CLIENT_MOCK_TRANSPORT_H_
#define CXX_INTERNAL_STORAGE_CLIENT_MOCK_TRANSPORT_H_

#include "src/google/protobuf/message.h"
#include "gmock/gmock.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "cxx/helpers/status/status.h"
#include "cxx/internal/storage_client/transport.h"
#include "proto/internal/mako_internal.pb.h"
#include "proto/internal/storage_client/storage.pb.h"
#include "spec/proto/mako.pb.h"

namespace mako {
namespace internal {

class MockTransport : public mako::internal::StorageTransport {
 public:
  MOCK_METHOD(helpers::Status, Connect, (), (override));
  MOCK_METHOD(helpers::Status, CreateProjectInfo,
              (absl::Duration deadline, const CreateProjectInfoRequest& request,
               mako::CreationResponse*),
              (override));
  MOCK_METHOD(helpers::Status, UpdateProjectInfo,
              (absl::Duration deadline, const UpdateProjectInfoRequest& request,
               mako::ModificationResponse*),
              (override));
  MOCK_METHOD(helpers::Status, GetProjectInfo,
              (absl::Duration deadline, const GetProjectInfoRequest& request,
               mako::ProjectInfoGetResponse*),
              (override));

  MOCK_METHOD(helpers::Status, QueryProjectInfo,
              (absl::Duration deadline, const QueryProjectInfoRequest& request,
               mako::ProjectInfoQueryResponse*),
              (override));
  MOCK_METHOD(helpers::Status, CreateBenchmarkInfo,
              (absl::Duration deadline,
               const CreateBenchmarkInfoRequest& request,
               mako::CreationResponse*),
              (override));
  MOCK_METHOD(helpers::Status, UpdateBenchmarkInfo,
              (absl::Duration deadline,
               const UpdateBenchmarkInfoRequest& request,
               mako::ModificationResponse*),
              (override));
  MOCK_METHOD(helpers::Status, QueryBenchmarkInfo,
              (absl::Duration deadline,
               const QueryBenchmarkInfoRequest& request,
               mako::BenchmarkInfoQueryResponse*),
              (override));
  MOCK_METHOD(helpers::Status, DeleteBenchmarkInfo,
              (absl::Duration deadline,
               const DeleteBenchmarkInfoRequest& request,
               mako::ModificationResponse*),
              (override));
  MOCK_METHOD(helpers::Status, CountBenchmarkInfo,
              (absl::Duration deadline,
               const CountBenchmarkInfoRequest& request,
               mako::CountResponse*),
              (override));
  MOCK_METHOD(helpers::Status, CreateRunInfo,
              (absl::Duration deadline, const CreateRunInfoRequest& request,
               mako::CreationResponse*),
              (override));
  MOCK_METHOD(helpers::Status, UpdateRunInfo,
              (absl::Duration deadline, const UpdateRunInfoRequest& request,
               mako::ModificationResponse*),
              (override));
  MOCK_METHOD(helpers::Status, QueryRunInfo,
              (absl::Duration deadline, const QueryRunInfoRequest& request,
               mako::RunInfoQueryResponse*),
              (override));
  MOCK_METHOD(helpers::Status, DeleteRunInfo,
              (absl::Duration deadline, const DeleteRunInfoRequest& request,
               mako::ModificationResponse*),
              (override));
  MOCK_METHOD(helpers::Status, CountRunInfo,
              (absl::Duration deadline, const CountRunInfoRequest& request,
               mako::CountResponse*),
              (override));
  MOCK_METHOD(helpers::Status, CreateSampleBatch,
              (absl::Duration deadline, const CreateSampleBatchRequest& request,
               mako::CreationResponse*),
              (override));
  MOCK_METHOD(helpers::Status, QuerySampleBatch,
              (absl::Duration deadline, const QuerySampleBatchRequest& request,
               mako::SampleBatchQueryResponse*),
              (override));
  MOCK_METHOD(helpers::Status, DeleteSampleBatch,
              (absl::Duration deadline, const DeleteSampleBatchRequest& request,
               mako::ModificationResponse*),
              (override));

  MOCK_METHOD(absl::Duration, last_call_server_elapsed_time, (),
              (const, override));
  MOCK_METHOD(void, set_client_tool_tag, (absl::string_view), (override));
  MOCK_METHOD(std::string, GetHostname, (), (override));
};

}  // namespace internal
}  // namespace mako

#endif  // CXX_INTERNAL_STORAGE_CLIENT_MOCK_TRANSPORT_H_
