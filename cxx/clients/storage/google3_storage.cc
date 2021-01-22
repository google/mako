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

#include <array>
#include <cstring>
#include <utility>
#include <vector>

#include "glog/logging.h"
#include "absl/container/flat_hash_map.h"
#include "absl/flags/flag.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "cxx/internal/proto_validation.h"
#include "cxx/internal/storage_client/retrying_storage_request.h"
#include "cxx/internal/storage_client/transport.h"
#include "proto/internal/mako_internal.pb.h"
#include "proto/internal/storage_client/storage.pb.h"
#include "spec/proto/mako.pb.h"

ABSL_FLAG(
    std::string, mako_internal_storage_host, "",
    "If set, overrides the storage host which was passed to constructor.");

ABSL_FLAG(
    std::string, mako_client_tool_tag, "",
    "Allows clients to identify their workload. If this is not set we will "
    "use the google3_environment_collector to generate a tool tag based on "
    "the build target. This data is used for understanding usage patterns.");

ABSL_FLAG(
    std::string, mako_internal_sudo_run_as, "",
    "If set, runs the command as the specified identity. Should be of the "
    "form: user@google.com or group@prod.google.com. The server will check "
    "whether the caller has permission to use this feature.");

ABSL_FLAG(bool, mako_internal_force_trace, false,
          "Force a stackdriver trace on the server for all storage requests.");

ABSL_FLAG(std::string, mako_internal_test_pass_id_override, "",
          "If set, overrides the test_pass_id set by a user or the Mako "
          "framework. Useful for frameworks such as Chamber that need to group "
          "runs. Note this is only applied on RunInfo creation/update. If "
          "provided along with the mako_internal_test_pass_id_override "
          "environment variable, this will take precedence (the envrionment "
          "variable will be ignored).");

ABSL_FLAG(
    std::vector<std::string>, mako_internal_additional_tags, {},
    "Additional tags to attach to all created RunInfos.  Note that these tags "
    "are only added on RunInfo creation/update. Be aware of tag limits "
    "(go/mako-limits) when using this flag - the number of tags in the "
    "original RunInfo plus those added via this flag must not exceed the "
    "limit! If provided along with the mako_internal_additional_tags "
    "environment variable, this will take precedence (the envrionment variable "
    "will be ignored).");

namespace mako {
namespace google3_storage {
namespace {

using ::mako_internal::SudoStorageRequest;

constexpr char kNoError[] = "";
constexpr char kMakoStorageServer[] = "mako.dev";

// NOTE: Total time may exceed this by up to kRPCDeadline + kMaxSleep.
constexpr absl::Duration kDefaultOperationTimeout = absl::Minutes(3);
// Min and max amount of time we sleep between storage request retries.
constexpr absl::Duration kMinSleep = absl::Seconds(1);
constexpr absl::Duration kMaxSleep = absl::Seconds(30);

constexpr int kMetricValueCountMax = 50000;
constexpr int kSampleErrorCountMax = 1000;
constexpr int kBatchSizeMax = 1000000;

std::string ResolveClientToolTag() {
  std::string client_tool_tag = absl::GetFlag(FLAGS_mako_client_tool_tag);
  if (client_tool_tag.empty()) {
    if (client_tool_tag.empty()) {
      client_tool_tag = "unknown";
    }
  }
  return client_tool_tag;
}

template <typename Request, typename Response>
bool RetryingStorageRequest(
    Request* request, Response* response, internal::StorageTransport* transport,
    helpers::Status (internal::StorageTransport::*method)(absl::Duration,
                                                          const Request&,
                                                          Response*),
    internal::StorageRetryStrategy* retry_strategy,
    absl::string_view telemetry_action) {
  const std::string& run_as =
      absl::GetFlag(FLAGS_mako_internal_sudo_run_as);
  if (!run_as.empty()) {
    request->mutable_request_options()->set_sudo_run_as(run_as);
  }

  return internal::RetryingStorageRequest(*request, response, transport, method,
                                          retry_strategy, telemetry_action);
}

template <typename Request, typename Response>
bool RetryingStorageQuery(Request* request, Response* response,
                          internal::StorageTransport* transport,
                          helpers::Status (internal::StorageTransport::*method)(
                              absl::Duration, const Request&, Response*),
                          internal::StorageRetryStrategy* retry_strategy,
                          absl::string_view telemetry_action) {
  bool success = RetryingStorageRequest(
      request, response, transport, method, retry_strategy, telemetry_action);
  // Ensure that query responses always have the cursor set.
  // This matches the behavior of the server, but ensures it works even when
  // different transports are used.
  if (!response->has_cursor()) {
    response->set_cursor("");
  }
  return success;
}

template <typename Request, typename Response>
bool UploadRunInfo(Request* request, internal::StorageTransport* transport,
                   helpers::Status (internal::StorageTransport::*method)(
                       absl::Duration, const Request&, Response*),
                   Response* response,
                   internal::StorageRetryStrategy* retry_strategy,
                   absl::string_view telemetry_action) {
  // Look for mako_internal_additional_tags in both flags and environment
  // variables. If found in both places, prefer the value from the flags.
  std::vector<std::string> additional_tags =
      absl::GetFlag(FLAGS_mako_internal_additional_tags);
  if (additional_tags.empty()) {
    additional_tags = absl::StrSplit(absl::NullSafeStringView(std::getenv(
                                         "MAKO_INTERNAL_ADDITIONAL_TAGS")),
                                     ',', absl::SkipWhitespace());
  }
  // Look for mako_internal_test_pass_id_override in both flags and
  // environment variables. If found in both places, prefer the value from the
  // flags.
  std::string test_pass_id_override =
      absl::GetFlag(FLAGS_mako_internal_test_pass_id_override);
  if (test_pass_id_override.empty()) {
    test_pass_id_override = std::string(absl::NullSafeStringView(
        std::getenv("MAKO_INTERNAL_TEST_PASS_ID_OVERRIDE")));
  }
  if (additional_tags.empty() && test_pass_id_override.empty()) {
    return RetryingStorageRequest(request, response, transport, method,
                                  retry_strategy, telemetry_action);
  }
  // add any additional tags requested by flag, ensuring that all tags are
  // unique and ordered as given using a std::set (vs an absl::flat_hash_set,
  // which is unordered)
  std::vector<std::string> tags(request->payload().tags().begin(),
                                request->payload().tags().end());
  std::set<std::string> unique_tags(request->payload().tags().begin(),
                                    request->payload().tags().end());
  for (const std::string& tag : additional_tags) {
    absl::string_view no_whitespace_tag = absl::StripAsciiWhitespace(tag);
    LOG(INFO) << "Adding new tag " << no_whitespace_tag << " to run";
    auto res = unique_tags.insert(std::string(no_whitespace_tag));
    if (res.second) {
      tags.emplace_back(no_whitespace_tag);
    }
  }
  request->mutable_payload()->clear_tags();
  for (const auto& tag : tags) {
    *request->mutable_payload()->add_tags() = std::string(tag);
  }
  // TODO(b/136285571) reference this limit from some common location where it
  // is defined (instead of redefining it here)
  int tag_limit = 20;
  if (request->payload().tags_size() > tag_limit) {
    std::string err_msg =
        "This run has too many tags; cannot add it to mako storage!";
    LOG(ERROR) << err_msg;
    response->mutable_status()->set_fail_message(err_msg);
    response->mutable_status()->set_code(mako::Status::FAIL);
    return false;
  }

  if (!test_pass_id_override.empty()) {
    LOG(INFO) << "Overriding test pass id for run: changing "
              << request->payload().test_pass_id() << " to "
              << test_pass_id_override << ".";
    request->mutable_payload()->set_test_pass_id(test_pass_id_override);
  }
  return RetryingStorageRequest(request, response, transport, method,
                                retry_strategy, telemetry_action);
}

}  // namespace

Storage::Storage(
    std::unique_ptr<mako::internal::StorageTransport> transport)
    : Storage(std::move(transport),
              absl::make_unique<mako::internal::StorageBackoff>(
                  kDefaultOperationTimeout, kMinSleep, kMaxSleep)) {}

Storage::Storage(
    std::unique_ptr<mako::internal::StorageTransport> transport,
    std::unique_ptr<mako::internal::StorageRetryStrategy> retry_strategy)
    : transport_(std::move(transport)),
      retry_strategy_(std::move(retry_strategy)) {
  transport_->set_client_tool_tag(ResolveClientToolTag());
}

bool Storage::CreateProjectInfo(const ProjectInfo& project_info,
                                mako::CreationResponse* creation_response) {
  constexpr absl::string_view telemetry_action = "Storage.CreateProjectInfo";
  internal::CreateProjectInfoRequest request;
  *request.mutable_payload() = project_info;
  return RetryingStorageRequest(
      &request, creation_response, transport_.get(),
      &mako::internal::StorageTransport::CreateProjectInfo,
      retry_strategy_.get(), telemetry_action);
}

bool Storage::UpdateProjectInfo(const ProjectInfo& project_info,
                                mako::ModificationResponse* mod_response) {
  constexpr absl::string_view telemetry_action = "Storage.UpdateProjectInfo";
  internal::UpdateProjectInfoRequest request;
  *request.mutable_payload() = project_info;
  return RetryingStorageRequest(
      &request, mod_response, transport_.get(),
      &mako::internal::StorageTransport::UpdateProjectInfo,
      retry_strategy_.get(), telemetry_action);
}

bool Storage::GetProjectInfo(const mako::ProjectInfo& project_info,
                             mako::ProjectInfoGetResponse* get_response) {
  constexpr absl::string_view telemetry_action = "Storage.GetProjectInfo";
  internal::GetProjectInfoRequest request;
  *request.mutable_payload() = project_info;
  return RetryingStorageRequest(
      &request, get_response, transport_.get(),
      &mako::internal::StorageTransport::GetProjectInfo,
      retry_strategy_.get(), telemetry_action);
}

bool Storage::GetProjectInfoByName(
    const std::string& project_name,
    mako::ProjectInfoGetResponse* get_response) {
  mako::ProjectInfo project_info;
  project_info.set_project_name(project_name);
  return GetProjectInfo(project_info, get_response);
}

bool Storage::QueryProjectInfo(
    const mako::ProjectInfoQuery& project_info_query,
    mako::ProjectInfoQueryResponse* query_response) {
  constexpr absl::string_view telemetry_action = "Storage.QueryProjectInfo";
  internal::QueryProjectInfoRequest request;
  *request.mutable_payload() = project_info_query;
  return RetryingStorageQuery(
      &request, query_response, transport_.get(),
      &mako::internal::StorageTransport::QueryProjectInfo,
      retry_strategy_.get(), telemetry_action);
}

bool Storage::CreateBenchmarkInfo(const BenchmarkInfo& benchmark_info,
                                  CreationResponse* creation_response) {
  constexpr absl::string_view telemetry_action = "Storage.CreateBenchmarkInfo";
  internal::CreateBenchmarkInfoRequest request;
  *request.mutable_payload() = benchmark_info;
  return RetryingStorageRequest(
      &request, creation_response, transport_.get(),
      &mako::internal::StorageTransport::CreateBenchmarkInfo,
      retry_strategy_.get(), telemetry_action);
}

bool Storage::UpdateBenchmarkInfo(const BenchmarkInfo& benchmark_info,
                                  ModificationResponse* mod_response) {
  constexpr absl::string_view telemetry_action = "Storage.UpdateBenchmarkInfo";
  internal::UpdateBenchmarkInfoRequest request;
  *request.mutable_payload() = benchmark_info;
  return RetryingStorageRequest(
      &request, mod_response, transport_.get(),
      &mako::internal::StorageTransport::UpdateBenchmarkInfo,
      retry_strategy_.get(), telemetry_action);
}

bool Storage::QueryBenchmarkInfo(const BenchmarkInfoQuery& benchmark_info_query,
                                 BenchmarkInfoQueryResponse* query_response) {
  constexpr absl::string_view telemetry_action = "Storage.QueryBenchmarkInfo";
  internal::QueryBenchmarkInfoRequest request;
  *request.mutable_payload() = benchmark_info_query;
  return RetryingStorageQuery(
      &request, query_response, transport_.get(),
      &mako::internal::StorageTransport::QueryBenchmarkInfo,
      retry_strategy_.get(), telemetry_action);
}

bool Storage::DeleteBenchmarkInfo(
    const BenchmarkInfoQuery& benchmark_info_query,
    ModificationResponse* mod_response) {
  constexpr absl::string_view telemetry_action = "Storage.DeleteBenchmarkInfo";
  internal::DeleteBenchmarkInfoRequest request;
  *request.mutable_payload() = benchmark_info_query;
  return RetryingStorageRequest(
      &request, mod_response, transport_.get(),
      &mako::internal::StorageTransport::DeleteBenchmarkInfo,
      retry_strategy_.get(), telemetry_action);
}

bool Storage::CountBenchmarkInfo(const BenchmarkInfoQuery& benchmark_info_query,
                                 CountResponse* count_response) {
  constexpr absl::string_view telemetry_action = "Storage.CountBenchmarkInfo";
  internal::CountBenchmarkInfoRequest request;
  *request.mutable_payload() = benchmark_info_query;
  return RetryingStorageRequest(
      &request, count_response, transport_.get(),
      &mako::internal::StorageTransport::CountBenchmarkInfo,
      retry_strategy_.get(), telemetry_action);
}

bool Storage::CreateRunInfo(const RunInfo& run_info,
                            CreationResponse* creation_response) {
  constexpr absl::string_view telemetry_action = "Storage.CreateRunInfo";
  internal::CreateRunInfoRequest request;
  *request.mutable_payload() = run_info;
  return UploadRunInfo(&request, transport_.get(),
                       &mako::internal::StorageTransport::CreateRunInfo,
                       creation_response, retry_strategy_.get(),
                       telemetry_action);
}

bool Storage::UpdateRunInfo(const RunInfo& run_info,
                            ModificationResponse* mod_response) {
  constexpr absl::string_view telemetry_action = "Storage.UpdateRunInfo";
  internal::UpdateRunInfoRequest request;
  *request.mutable_payload() = run_info;
  return UploadRunInfo(&request, transport_.get(),
                       &mako::internal::StorageTransport::UpdateRunInfo,
                       mod_response, retry_strategy_.get(), telemetry_action);
}

bool Storage::QueryRunInfo(const RunInfoQuery& run_info_query,
                           RunInfoQueryResponse* query_response) {
  constexpr absl::string_view telemetry_action = "Storage.QueryRunInfo";
  internal::QueryRunInfoRequest request;
  *request.mutable_payload() = run_info_query;
  return RetryingStorageQuery(
      &request, query_response, transport_.get(),
      &mako::internal::StorageTransport::QueryRunInfo,
      retry_strategy_.get(), telemetry_action);
}

bool Storage::DeleteRunInfo(const RunInfoQuery& run_info_query,
                            ModificationResponse* mod_response) {
  constexpr absl::string_view telemetry_action = "Storage.DeleteRunInfo";
  internal::DeleteRunInfoRequest request;
  *request.mutable_payload() = run_info_query;
  return RetryingStorageRequest(
      &request, mod_response, transport_.get(),
      &mako::internal::StorageTransport::DeleteRunInfo,
      retry_strategy_.get(), telemetry_action);
}

bool Storage::CountRunInfo(const RunInfoQuery& run_info_query,
                           CountResponse* count_response) {
  constexpr absl::string_view telemetry_action = "Storage.CountRunInfo";
  internal::CountRunInfoRequest request;
  *request.mutable_payload() = run_info_query;
  return RetryingStorageRequest(
      &request, count_response, transport_.get(),
      &mako::internal::StorageTransport::CountRunInfo,
      retry_strategy_.get(), telemetry_action);
}

bool Storage::CreateSampleBatch(const SampleBatch& sample_batch,
                                CreationResponse* creation_response) {
  constexpr absl::string_view telemetry_action = "Storage.CreateSampleBatch";
  // Make a copy every time to keep code simpler and readable. Cost of copy in
  // the absolute worst case (50000 metrics) (~4ms) is small
  // compared to the cost of the request to the server (~200ms).
  internal::CreateSampleBatchRequest request;
  *request.mutable_payload() = sample_batch;
  for (auto& point : *request.mutable_payload()->mutable_sample_point_list()) {
    if (point.aux_data_size() > 0) {
      LOG_FIRST_N(WARNING, 1)
          << "Attempting to create a SampleBatch which contains SamplePoints "
             "with "
             "Aux Data. Aux Data is not displayed on the server, and should be "
             "stripped out before being sent as not to take up valuable space. "
             "This normally happens in the default Downsampler. If your "
             "Mako "
             "test is using the default downsampler and you are seeing this "
             "message, please file a bug at go/mako-bug.";
      internal::StripAuxData(&point);
    }
  }
  return RetryingStorageRequest(
      &request, creation_response, transport_.get(),
      &mako::internal::StorageTransport::CreateSampleBatch,
      retry_strategy_.get(), telemetry_action);
}

bool Storage::QuerySampleBatch(const SampleBatchQuery& sample_batch_query,
                               SampleBatchQueryResponse* query_response) {
  constexpr absl::string_view telemetry_action = "Storage.QuerySampleBatch";
  internal::QuerySampleBatchRequest request;
  *request.mutable_payload() = sample_batch_query;
  return RetryingStorageQuery(
      &request, query_response, transport_.get(),
      &mako::internal::StorageTransport::QuerySampleBatch,
      retry_strategy_.get(), telemetry_action);
}

bool Storage::DeleteSampleBatch(const SampleBatchQuery& sample_batch_query,
                                ModificationResponse* mod_response) {
  constexpr absl::string_view telemetry_action = "Storage.DeleteSampleBatch";
  internal::DeleteSampleBatchRequest request;
  *request.mutable_payload() = sample_batch_query;
  return RetryingStorageRequest(
      &request, mod_response, transport_.get(),
      &mako::internal::StorageTransport::DeleteSampleBatch,
      retry_strategy_.get(), telemetry_action);
}

std::string Storage::GetMetricValueCountMax(int* metric_count_max) {
  *metric_count_max = kMetricValueCountMax;
  return kNoError;
}

std::string Storage::GetSampleErrorCountMax(int* sample_error_max) {
  *sample_error_max = kSampleErrorCountMax;
  return kNoError;
}

std::string Storage::GetBatchSizeMax(int* batch_size_max) {
  *batch_size_max = kBatchSizeMax;
  return kNoError;
}

std::string Storage::GetHostname() {
  if (hostname_.has_value()) {
    return *hostname_;
  }
  return transport_->GetHostname();
}

std::string ApplyHostnameFlagOverrides(const std::string& hostname) {
  const std::string hostname_override =
      absl::GetFlag(FLAGS_mako_internal_storage_host);
  if (!hostname_override.empty()) {
    LOG(WARNING) << "Overriding constructor-supplied hostname of '" << hostname
                 << "' with flag value '" << hostname_override << "'";
    return hostname_override;
  }
  return hostname;
}
}  // namespace google3_storage
}  // namespace mako
