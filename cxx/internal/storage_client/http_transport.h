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

// HTTP/Oauth2 version of Mako (go/mako) storage communication.
//
#ifndef CXX_INTERNAL_STORAGE_CLIENT_HTTP_TRANSPORT_H_
#define CXX_INTERNAL_STORAGE_CLIENT_HTTP_TRANSPORT_H_

#include <memory>
#include <string>
#include <utility>

#include "glog/logging.h"
#include "src/google/protobuf/message.h"
#include "absl/flags/flag.h"
#include "absl/memory/memory.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "cxx/helpers/status/status.h"
#include "cxx/internal/storage_client/http_client.h"
#include "cxx/internal/storage_client/oauth_token_provider.h"
#include "cxx/internal/storage_client/transport.h"
#include "proto/internal/mako_internal.pb.h"

extern absl::Flag<bool> FLAGS_mako_internal_disable_expect_100_continue;

namespace mako {
namespace internal {

// This transport relies on using HTTP to communicate with the server, instead
// of Google-specific transports.
//
// If an OAuthTokenProvider instance is injected, OAuth2 authentication will be
// passed by setting the token returned by the OAuthTokenProvider in an
// "Authorization: Bearer <token>" request header.
//
// This class is thread-safe.
class HttpTransport : public StorageTransport {
 public:
  // Constructs an HttpTransport that uses the default OAuthTokenProvider
  // (MetadataOAuthFetcher).
  explicit HttpTransport(absl::string_view host);

  // Constructs an HttpTransport that will use the provided OAuthTokenProvider.
  // The OAuthTokenProvider must be thread-safe (go/thread-safe).
  explicit HttpTransport(absl::string_view host,
                         std::unique_ptr<OAuthTokenProvider> token_provider);

  // Constructs an HttpTransport that will use the provided OAuthTokenProvider
  // and the provided HttpClientInterface.
  // The OAuthTokenProvider must be thread-safe (go/thread-safe).
  explicit HttpTransport(absl::string_view host,
                         std::unique_ptr<OAuthTokenProvider> token_provider,
                         std::unique_ptr<HttpClientInterface> http_client)
      : host_(host),
        token_provider_(std::move(token_provider)),
        client_(std::move(http_client)) {}

  // Constructs an HttpTransport that will use the provided OAuthTokenProvider,
  // the provided HttpClientInterface, and overrides the HTTP Client's CA cert
  // path.
  //
  // The OAuthTokenProvider must be thread-safe (go/thread-safe).
  HttpTransport(absl::string_view host,
                std::unique_ptr<OAuthTokenProvider> token_provider,
                absl::string_view ca_certificate_path)
      : host_(host),
        token_provider_(std::move(token_provider)),
        client_(absl::make_unique<HttpClient>(ca_certificate_path)) {}

  helpers::Status Connect() override;

  void set_client_tool_tag(absl::string_view) override;

  void use_local_gae_server(bool use_local_gae_server);

  helpers::Status CreateProjectInfo(
      absl::Duration deadline, const CreateProjectInfoRequest& request,
      mako::CreationResponse* response) override;

  helpers::Status UpdateProjectInfo(
      absl::Duration deadline, const UpdateProjectInfoRequest& request,
      mako::ModificationResponse* response) override;

  helpers::Status GetProjectInfo(
      absl::Duration deadline, const GetProjectInfoRequest& request,
      mako::ProjectInfoGetResponse* response) override;

  helpers::Status QueryProjectInfo(
      absl::Duration deadline, const QueryProjectInfoRequest& request,
      mako::ProjectInfoQueryResponse* response) override;

  helpers::Status CreateBenchmarkInfo(
      absl::Duration deadline, const CreateBenchmarkInfoRequest& request,
      mako::CreationResponse* response) override;

  helpers::Status UpdateBenchmarkInfo(
      absl::Duration deadline, const UpdateBenchmarkInfoRequest& request,
      mako::ModificationResponse* response) override;

  helpers::Status QueryBenchmarkInfo(
      absl::Duration deadline, const QueryBenchmarkInfoRequest& request,
      mako::BenchmarkInfoQueryResponse* response) override;

  helpers::Status DeleteBenchmarkInfo(
      absl::Duration deadline, const DeleteBenchmarkInfoRequest& request,
      mako::ModificationResponse* response) override;

  helpers::Status CountBenchmarkInfo(
      absl::Duration deadline, const CountBenchmarkInfoRequest& request,
      mako::CountResponse* response) override;

  helpers::Status CreateRunInfo(absl::Duration deadline,
                                const CreateRunInfoRequest& request,
                                mako::CreationResponse* response) override;

  helpers::Status UpdateRunInfo(
      absl::Duration deadline, const UpdateRunInfoRequest& request,
      mako::ModificationResponse* response) override;

  helpers::Status QueryRunInfo(
      absl::Duration deadline, const QueryRunInfoRequest& request,
      mako::RunInfoQueryResponse* response) override;

  helpers::Status DeleteRunInfo(
      absl::Duration deadline, const DeleteRunInfoRequest& request,
      mako::ModificationResponse* response) override;

  helpers::Status CountRunInfo(absl::Duration deadline,
                               const CountRunInfoRequest& request,
                               mako::CountResponse* response) override;

  helpers::Status CreateSampleBatch(
      absl::Duration deadline, const CreateSampleBatchRequest& request,
      mako::CreationResponse* response) override;

  helpers::Status QuerySampleBatch(
      absl::Duration deadline, const QuerySampleBatchRequest& request,
      mako::SampleBatchQueryResponse* response) override;

  helpers::Status DeleteSampleBatch(
      absl::Duration deadline, const DeleteSampleBatchRequest& request,
      mako::ModificationResponse* response) override;

  // TODO(b/73734783): Remove this.
  absl::Duration last_call_server_elapsed_time() const override {
    LOG(FATAL) << "Not implemented";
    return absl::ZeroDuration();
  }

  // Fetches the token provider.
  OAuthTokenProvider* token_provider() { return token_provider_.get(); }

  // The hostname backing this Storage implementation.
  // Returns a URL without the trailing slash.
  std::string GetHostname() override { return host_; }

 private:
  // Sends a POST request to `path` on the server, serializing the `request`
  // into the HTTP body. The server's response body will be deserialized into
  // `response`.
  helpers::Status Call(absl::string_view path, const google::protobuf::Message& request,
                       absl::Duration timeout, google::protobuf::Message* response);

  const std::string host_;
  std::string client_tool_tag_ = "unknown";
  const std::unique_ptr<OAuthTokenProvider> token_provider_;
  std::unique_ptr<HttpClientInterface> client_;
  bool use_local_gae_server_ = false;
  std::string ca_authority_path_;
};

}  // namespace internal
}  // namespace mako

#endif  // CXX_INTERNAL_STORAGE_CLIENT_HTTP_TRANSPORT_H_
