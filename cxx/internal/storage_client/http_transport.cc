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

#include "cxx/internal/storage_client/http_transport.h"

#include <string>
#include <vector>

#include "src/google/protobuf/text_format.h"
#include "absl/flags/flag.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "cxx/helpers/status/canonical_errors.h"
#include "cxx/helpers/status/status.h"
#include "cxx/helpers/status/statusor.h"
#include "cxx/internal/storage_client/http_paths.h"
#include "cxx/internal/storage_client/url.h"
#include "proto/internal/mako_internal.pb.h"

// TODO(b/124472003): Remove this when we fix our HTTP Client's handling of
// Expect: 100-continue.
ABSL_FLAG(bool, mako_internal_disable_expect_100_continue, true,
          "Disable libcurl's 'Expect: 100-Continue' feature by sending an "
          "'Expect:' header. Some servers don't understand 'Expect:' -- in "
          "those cases, set this flag to 'false'");

namespace mako {
namespace internal {
namespace {

constexpr int kPayloadLogCharLimit = 1000;

// Cookie for non admin user on a GAE dev/hermetic server
const char kGaeDavAppserverCookie[] =
    "dev_appserver_login=test@example.com:False:185804764220139124118;";

helpers::StatusOr<Url> BuildUrl(absl::string_view host,
                                absl::string_view path) {
  auto parsed = Url::Parse(host);
  if (!parsed.ok()) {
    return parsed.status();
  }
  return parsed.value().WithPath(path);
}

std::string TruncatedShortDebugString(const google::protobuf::Message& message,
                                      int max_len) {
  std::string s;
  google::protobuf::TextFormat::Printer printer;
  printer.SetSingleLineMode(true);
  printer.SetExpandAny(true);
  printer.SetTruncateStringFieldLongerThan(max_len);
  printer.PrintToString(message, &s);
  return s;
}

}  // namespace

HttpTransport::HttpTransport(absl::string_view host,
                             std::unique_ptr<OAuthTokenProvider> token_provider)
    : HttpTransport(host, std::move(token_provider),
                    absl::make_unique<HttpClient>()) {}

helpers::Status HttpTransport::Connect() {
  helpers::StatusOr<Url> maybe_url = BuildUrl(host_, /*path=*/"");
  if (!maybe_url.ok()) {
    return helpers::FailedPreconditionError(
        absl::StrCat("Bad Mako Storage HttpTransport host: ",
                     maybe_url.status().message()));
  }

  return helpers::OkStatus();
}

helpers::Status HttpTransport::Call(absl::string_view orig_path,
                                    const google::protobuf::Message& request,
                                    absl::Duration timeout,
                                    google::protobuf::Message* response) {
  std::string path(orig_path);
  if (token_provider_ != nullptr) {
    path = absl::StrCat("/oauth", path);
  }

  VLOG(1) << "Making Mako Storage HttpTransport call. hostname=" << host_
          << ", path=" << path;
  VLOG(2) << "Request (possibly truncated): "
          << TruncatedShortDebugString(request, kPayloadLogCharLimit);

  helpers::StatusOr<Url> maybe_url = BuildUrl(host_, path);
  if (!maybe_url.ok()) {
    return helpers::FailedPreconditionError(absl::StrFormat(
        "Mako Storage HttpTransport failed to assemble valid "
        "URL from host %s and request path %s: %s",
        host_, path, maybe_url.status().message()));
  }
  Url url = maybe_url.value();
  VLOG(2) << "Assembled URL: " << url.ToString();

  // Request headers.
  std::vector<std::pair<std::string, std::string>> headers;

  // Mark that we're sending binary data.
  headers.push_back({"Content-type", "application/octet-stream"});
  headers.push_back({"client-tool-tag", client_tool_tag_});

  if (use_local_gae_server_) {
    headers.push_back({"Cookie:", kGaeDavAppserverCookie});
  } else if (absl::GetFlag(
                 FLAGS_mako_internal_disable_expect_100_continue)) {
    // TODO(b/124472003): Fix our HTTP Client's handling of Expect:
    // 100-continue.
    //
    // Disable 100-continue feature because the client doesn't handle it
    // correctly. This could (e.g. in the case of an http client implementation
    // that uses libcurl) result in the HTTP client NOT sending an "Expect:
    // 100-continue" header.
    headers.push_back({"Expect", ""});
  }

  // Set bearer token for OAuth2 authentication.
  if (token_provider_ != nullptr) {
    helpers::StatusOr<std::string> status_or_token =
        token_provider_->GetBearerToken();
    if (!status_or_token.ok()) {
      return status_or_token.status();
    }
    // REMINDER to future readers that you should not log this token.
    std::string token = status_or_token.value();
    if (token.empty()) {
      return helpers::UnavailableError(
          "Received an empty OAuth2 Bearer token from the configured token "
          "provider. This is not expected.");
    }
    VLOG(2) << "Successfully fetched OAuth2 bearer token. Setting in header.";
    headers.push_back({"Authorization", absl::StrCat("Bearer ", token)});
  }

  // Use the HTTP client to make the storage API request.
  helpers::StatusOr<std::string> status_or_response = client_->Post(
      url.ToString(), headers, request.SerializeAsString());

  if (!status_or_response.ok()) {
    VLOG(1) << "HttpTransport received error status from http client: "
            << status_or_response.status();
    return status_or_response.status();
  }

  std::string raw_response = std::move(status_or_response).value();
  if (!response->ParseFromString(raw_response)) {
    const std::string error = "Failed parsing response from server.";
    LOG(ERROR) << error << " First " << kPayloadLogCharLimit
               << " chars of response:\n"
               << raw_response.substr(0, kPayloadLogCharLimit) << "\n";
    // TODO(b/74948849) Maybe should be helpers::InternalError(...).
    return helpers::FailedPreconditionError(
        absl::StrCat(error, "\nCheck logs for dump of response payload."));
  }
  return helpers::OkStatus();
}

helpers::Status HttpTransport::CreateProjectInfo(
    absl::Duration deadline, const CreateProjectInfoRequest& request,
    mako::CreationResponse* response) {
  if (request.has_request_options() &&
      !request.request_options().sudo_run_as().empty()) {
    mako_internal::SudoStorageRequest sudo_request;
    sudo_request.set_run_as(request.request_options().sudo_run_as());
    sudo_request.set_type(
        mako_internal::SudoStorageRequest::CREATE_PROJECT_INFO);
    *sudo_request.mutable_project() = request.payload();
    return Call(kSudoPath, sudo_request, deadline, response);
  }
  const absl::string_view& path = kCreateProjectInfoPath;
  return Call(path, request.payload(), deadline, response);
}

helpers::Status HttpTransport::UpdateProjectInfo(
    absl::Duration deadline, const UpdateProjectInfoRequest& request,
    mako::ModificationResponse* response) {
  if (request.has_request_options() &&
      !request.request_options().sudo_run_as().empty()) {
    mako_internal::SudoStorageRequest sudo_request;
    sudo_request.set_run_as(request.request_options().sudo_run_as());
    sudo_request.set_type(
        mako_internal::SudoStorageRequest::UPDATE_PROJECT_INFO);
    *sudo_request.mutable_project() = request.payload();
    return Call(kSudoPath, sudo_request, deadline, response);
  }
  const absl::string_view& path = kUpdateProjectInfoPath;
  return Call(path, request.payload(), deadline, response);
}

helpers::Status HttpTransport::GetProjectInfo(
    absl::Duration deadline, const GetProjectInfoRequest& request,
    mako::ProjectInfoGetResponse* response) {
  if (request.has_request_options() &&
      !request.request_options().sudo_run_as().empty()) {
    mako_internal::SudoStorageRequest sudo_request;
    sudo_request.set_run_as(request.request_options().sudo_run_as());
    sudo_request.set_type(
        mako_internal::SudoStorageRequest::GET_PROJECT_INFO);
    *sudo_request.mutable_project() = request.payload();
    return Call(kSudoPath, sudo_request, deadline, response);
  }
  const absl::string_view& path = kGetProjectInfoPath;
  return Call(path, request.payload(), deadline, response);
}

helpers::Status HttpTransport::QueryProjectInfo(
    absl::Duration deadline, const QueryProjectInfoRequest& request,
    mako::ProjectInfoQueryResponse* response) {
  if (request.has_request_options() &&
      !request.request_options().sudo_run_as().empty()) {
    mako_internal::SudoStorageRequest sudo_request;
    sudo_request.set_run_as(request.request_options().sudo_run_as());
    sudo_request.set_type(
        mako_internal::SudoStorageRequest::QUERY_PROJECT_INFO);
    return Call(kSudoPath, sudo_request, deadline, response);
  }
  const absl::string_view& path = kQueryProjectInfoPath;
  return Call(path, request, deadline, response);
}

helpers::Status HttpTransport::CreateBenchmarkInfo(
    absl::Duration deadline, const CreateBenchmarkInfoRequest& request,
    mako::CreationResponse* response) {
  if (request.has_request_options() &&
      !request.request_options().sudo_run_as().empty()) {
    mako_internal::SudoStorageRequest sudo_request;
    sudo_request.set_run_as(request.request_options().sudo_run_as());
    sudo_request.set_type(
        mako_internal::SudoStorageRequest::CREATE_BENCHMARK_INFO);
    *sudo_request.mutable_benchmark() = request.payload();
    return Call(kSudoPath, sudo_request, deadline, response);
  }
  const absl::string_view& path = kCreateBenchmarkPath;
  return Call(path, request.payload(), deadline, response);
}

helpers::Status HttpTransport::UpdateBenchmarkInfo(
    absl::Duration deadline, const UpdateBenchmarkInfoRequest& request,
    mako::ModificationResponse* response) {
  if (request.has_request_options() &&
      !request.request_options().sudo_run_as().empty()) {
    mako_internal::SudoStorageRequest sudo_request;
    sudo_request.set_run_as(request.request_options().sudo_run_as());
    sudo_request.set_type(
        mako_internal::SudoStorageRequest::UPDATE_BENCHMARK_INFO);
    *sudo_request.mutable_benchmark() = request.payload();
    return Call(kSudoPath, sudo_request, deadline, response);
  }
  const absl::string_view& path = kModificationBenchmarkPath;
  return Call(path, request.payload(), deadline, response);
}

helpers::Status HttpTransport::QueryBenchmarkInfo(
    absl::Duration deadline, const QueryBenchmarkInfoRequest& request,
    mako::BenchmarkInfoQueryResponse* response) {
  if (request.has_request_options() &&
      !request.request_options().sudo_run_as().empty()) {
    mako_internal::SudoStorageRequest sudo_request;
    sudo_request.set_run_as(request.request_options().sudo_run_as());
    sudo_request.set_type(
        mako_internal::SudoStorageRequest::QUERY_BENCHMARK_INFO);
    *sudo_request.mutable_benchmark_query() = request.payload();
    return Call(kSudoPath, sudo_request, deadline, response);
  }
  const absl::string_view& path = kQueryBenchmarkPath;
  return Call(path, request.payload(), deadline, response);
}

helpers::Status HttpTransport::DeleteBenchmarkInfo(
    absl::Duration deadline, const DeleteBenchmarkInfoRequest& request,
    mako::ModificationResponse* response) {
  if (request.has_request_options() &&
      !request.request_options().sudo_run_as().empty()) {
    mako_internal::SudoStorageRequest sudo_request;
    sudo_request.set_run_as(request.request_options().sudo_run_as());
    sudo_request.set_type(
        mako_internal::SudoStorageRequest::DELETE_BENCHMARK_INFO);
    *sudo_request.mutable_benchmark_query() = request.payload();
    return Call(kSudoPath, sudo_request, deadline, response);
  }
  const absl::string_view& path = kDeleteBenchmarkPath;
  return Call(path, request.payload(), deadline, response);
}

helpers::Status HttpTransport::CountBenchmarkInfo(
    absl::Duration deadline, const CountBenchmarkInfoRequest& request,
    mako::CountResponse* response) {
  if (request.has_request_options() &&
      !request.request_options().sudo_run_as().empty()) {
    mako_internal::SudoStorageRequest sudo_request;
    sudo_request.set_run_as(request.request_options().sudo_run_as());
    sudo_request.set_type(
        mako_internal::SudoStorageRequest::COUNT_BENCHMARK_INFO);
    *sudo_request.mutable_benchmark_query() = request.payload();
    return Call(kSudoPath, sudo_request, deadline, response);
  }
  const absl::string_view& path = kCountBenchmarkPath;
  return Call(path, request.payload(), deadline, response);
}

helpers::Status HttpTransport::CreateRunInfo(
    absl::Duration deadline, const CreateRunInfoRequest& request,
    mako::CreationResponse* response) {
  if (request.has_request_options() &&
      !request.request_options().sudo_run_as().empty()) {
    mako_internal::SudoStorageRequest sudo_request;
    sudo_request.set_run_as(request.request_options().sudo_run_as());
    sudo_request.set_type(
        mako_internal::SudoStorageRequest::CREATE_RUN_INFO);
    *sudo_request.mutable_run() = request.payload();
    return Call(kSudoPath, sudo_request, deadline, response);
  }
  const absl::string_view& path = kCreateRunInfoPath;
  return Call(path, request.payload(), deadline, response);
}

helpers::Status HttpTransport::UpdateRunInfo(
    absl::Duration deadline, const UpdateRunInfoRequest& request,
    mako::ModificationResponse* response) {
  if (request.has_request_options() &&
      !request.request_options().sudo_run_as().empty()) {
    mako_internal::SudoStorageRequest sudo_request;
    sudo_request.set_run_as(request.request_options().sudo_run_as());
    sudo_request.set_type(
        mako_internal::SudoStorageRequest::UPDATE_RUN_INFO);
    *sudo_request.mutable_run() = request.payload();
    return Call(kSudoPath, sudo_request, deadline, response);
  }
  const absl::string_view& path = kModificationRunInfoPath;
  return Call(path, request.payload(), deadline, response);
}

helpers::Status HttpTransport::QueryRunInfo(
    absl::Duration deadline, const QueryRunInfoRequest& request,
    mako::RunInfoQueryResponse* response) {
  if (request.has_request_options() &&
      !request.request_options().sudo_run_as().empty()) {
    mako_internal::SudoStorageRequest sudo_request;
    sudo_request.set_run_as(request.request_options().sudo_run_as());
    sudo_request.set_type(
        mako_internal::SudoStorageRequest::QUERY_RUN_INFO);
    *sudo_request.mutable_run_query() = request.payload();
    return Call(kSudoPath, sudo_request, deadline, response);
  }
  const absl::string_view& path = kQueryRunInfoPath;
  return Call(path, request.payload(), deadline, response);
}

helpers::Status HttpTransport::DeleteRunInfo(
    absl::Duration deadline, const DeleteRunInfoRequest& request,
    mako::ModificationResponse* response) {
  if (request.has_request_options() &&
      !request.request_options().sudo_run_as().empty()) {
    mako_internal::SudoStorageRequest sudo_request;
    sudo_request.set_run_as(request.request_options().sudo_run_as());
    sudo_request.set_type(
        mako_internal::SudoStorageRequest::DELETE_RUN_INFO);
    *sudo_request.mutable_run_query() = request.payload();
    return Call(kSudoPath, sudo_request, deadline, response);
  }
  const absl::string_view& path = kDeleteRunInfoPath;
  return Call(path, request.payload(), deadline, response);
}

helpers::Status HttpTransport::CountRunInfo(absl::Duration deadline,
                                            const CountRunInfoRequest& request,
                                            mako::CountResponse* response) {
  if (request.has_request_options() &&
      !request.request_options().sudo_run_as().empty()) {
    mako_internal::SudoStorageRequest sudo_request;
    sudo_request.set_run_as(request.request_options().sudo_run_as());
    sudo_request.set_type(
        mako_internal::SudoStorageRequest::COUNT_RUN_INFO);
    *sudo_request.mutable_run_query() = request.payload();
    return Call(kSudoPath, sudo_request, deadline, response);
  }
  const absl::string_view& path = kCountRunInfoPath;
  return Call(path, request.payload(), deadline, response);
}

helpers::Status HttpTransport::CreateSampleBatch(
    absl::Duration deadline, const CreateSampleBatchRequest& request,
    mako::CreationResponse* response) {
  if (request.has_request_options() &&
      !request.request_options().sudo_run_as().empty()) {
    mako_internal::SudoStorageRequest sudo_request;
    sudo_request.set_run_as(request.request_options().sudo_run_as());
    sudo_request.set_type(
        mako_internal::SudoStorageRequest::CREATE_SAMPLE_BATCH);
    *sudo_request.mutable_batch() = request.payload();
    return Call(kSudoPath, sudo_request, deadline, response);
  }
  const absl::string_view& path = kCreateSampleBatchPath;
  return Call(path, request.payload(), deadline, response);
}

helpers::Status HttpTransport::QuerySampleBatch(
    absl::Duration deadline, const QuerySampleBatchRequest& request,
    mako::SampleBatchQueryResponse* response) {
  if (request.has_request_options() &&
      !request.request_options().sudo_run_as().empty()) {
    mako_internal::SudoStorageRequest sudo_request;
    sudo_request.set_run_as(request.request_options().sudo_run_as());
    sudo_request.set_type(
        mako_internal::SudoStorageRequest::QUERY_SAMPLE_BATCH);
    *sudo_request.mutable_batch_query() = request.payload();
    return Call(kSudoPath, sudo_request, deadline, response);
  }
  const absl::string_view& path = kQuerySampleBatchPath;
  return Call(path, request.payload(), deadline, response);
}

helpers::Status HttpTransport::DeleteSampleBatch(
    absl::Duration deadline, const DeleteSampleBatchRequest& request,
    mako::ModificationResponse* response) {
  if (request.has_request_options() &&
      !request.request_options().sudo_run_as().empty()) {
    mako_internal::SudoStorageRequest sudo_request;
    sudo_request.set_run_as(request.request_options().sudo_run_as());
    sudo_request.set_type(
        mako_internal::SudoStorageRequest::DELETE_SAMPLE_BATCH);
    *sudo_request.mutable_batch_query() = request.payload();
    return Call(kSudoPath, sudo_request, deadline, response);
  }
  const absl::string_view& path = kDeleteSampleBatchPath;
  return Call(path, request.payload(), deadline, response);
}

void HttpTransport::set_client_tool_tag(absl::string_view client_tool_tag) {
  client_tool_tag_ = std::string(client_tool_tag);
}

void HttpTransport::use_local_gae_server(bool use_local_gae_server) {
  use_local_gae_server_ = use_local_gae_server;
}

}  // namespace internal
}  // namespace mako
