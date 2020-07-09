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
#ifndef CXX_INTERNAL_STORAGE_CLIENT_TRANSPORT_H_
#define CXX_INTERNAL_STORAGE_CLIENT_TRANSPORT_H_

#include "src/google/protobuf/message.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "cxx/helpers/status/status.h"
#include "proto/internal/mako_internal.pb.h"
#include "proto/internal/storage_client/storage.pb.h"
#include "spec/proto/mako.pb.h"

namespace mako {
namespace internal {

// `mako::google3_storage::Storage` uses a `StorageTransport` to communicate
// with the server.
//
// Implementations should be thread-safe as per go/thread-safe.
class StorageTransport {
 public:
  virtual ~StorageTransport() = default;

  // Do whatever connection is necessary for the transport. Will be called at
  // least once before Call is called. Calls to Connect() after a successful
  // connection should be no-ops.
  //
  // mako::helpers::StatusCode::kFailedPrecondition indicates an error that
  // is not retryable.
  virtual helpers::Status Connect() = 0;

  virtual void set_client_tool_tag(absl::string_view) = 0;

  // Returns the number of seconds that the last operation took (according to
  // the server). This is exposed for tests of this library to use and should
  // not otherwise be relied on. This is also not guaranteed to be correct in
  // multi-threaded situations.
  virtual absl::Duration last_call_server_elapsed_time() const = 0;

  // The hostname backing the Storage implementation.
  // Returns a URL without the trailing slash.
  virtual std::string GetHostname() = 0;

  // For all the methods below, implementations must follow this pattern:
  //
  // Return Status codes other than mako::helpers::StatusCode::kOk indicate
  // a transport-layer error (e.g. failed to send an RPC). Calls that result in
  // Storage API layer errors (e.g. fetching a project that doesn't exist) must
  // return  mako::helpers::StatusCode::kOk and must specify the error via
  // the response message.
  //
  // mako::helpers::StatusCode::kFailedPrecondition indicates an error that
  // is not retryable.

  virtual helpers::Status CreateProjectInfo(
      absl::Duration deadline, const CreateProjectInfoRequest& request,
      mako::CreationResponse* response) = 0;

  virtual helpers::Status UpdateProjectInfo(
      absl::Duration deadline, const UpdateProjectInfoRequest& request,
      mako::ModificationResponse* response) = 0;

  virtual helpers::Status GetProjectInfo(
      absl::Duration deadline, const GetProjectInfoRequest& request,
      mako::ProjectInfoGetResponse* response) = 0;


  virtual helpers::Status QueryProjectInfo(
      absl::Duration deadline, const QueryProjectInfoRequest& request,
      mako::ProjectInfoQueryResponse* response) = 0;

  virtual helpers::Status CreateBenchmarkInfo(
      absl::Duration deadline, const CreateBenchmarkInfoRequest& request,
      mako::CreationResponse* response) = 0;

  virtual helpers::Status UpdateBenchmarkInfo(
      absl::Duration deadline, const UpdateBenchmarkInfoRequest& request,
      mako::ModificationResponse* response) = 0;

  virtual helpers::Status QueryBenchmarkInfo(
      absl::Duration deadline, const QueryBenchmarkInfoRequest& request,
      mako::BenchmarkInfoQueryResponse* response) = 0;

  virtual helpers::Status DeleteBenchmarkInfo(
      absl::Duration deadline, const DeleteBenchmarkInfoRequest& request,
      mako::ModificationResponse* response) = 0;

  virtual helpers::Status CountBenchmarkInfo(
      absl::Duration deadline, const CountBenchmarkInfoRequest& request,
      mako::CountResponse* response) = 0;

  virtual helpers::Status CreateRunInfo(
      absl::Duration deadline, const CreateRunInfoRequest& request,
      mako::CreationResponse* response) = 0;

  virtual helpers::Status UpdateRunInfo(
      absl::Duration deadline, const UpdateRunInfoRequest& request,
      mako::ModificationResponse* response) = 0;

  virtual helpers::Status QueryRunInfo(
      absl::Duration deadline, const QueryRunInfoRequest& request,
      mako::RunInfoQueryResponse* response) = 0;

  virtual helpers::Status DeleteRunInfo(
      absl::Duration deadline, const DeleteRunInfoRequest& request,
      mako::ModificationResponse* response) = 0;

  virtual helpers::Status CountRunInfo(absl::Duration deadline,
                                       const CountRunInfoRequest& request,
                                       mako::CountResponse* response) = 0;

  virtual helpers::Status CreateSampleBatch(
      absl::Duration deadline, const CreateSampleBatchRequest& request,
      mako::CreationResponse* response) = 0;

  virtual helpers::Status QuerySampleBatch(
      absl::Duration deadline, const QuerySampleBatchRequest& request,
      mako::SampleBatchQueryResponse* response) = 0;

  virtual helpers::Status DeleteSampleBatch(
      absl::Duration deadline, const DeleteSampleBatchRequest& request,
      mako::ModificationResponse* response) = 0;
};

}  // namespace internal
}  // namespace mako

#endif  // CXX_INTERNAL_STORAGE_CLIENT_TRANSPORT_H_
