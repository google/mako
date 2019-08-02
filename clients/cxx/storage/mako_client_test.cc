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
#include "clients/cxx/storage/mako_client.h"
#include "devtools/build/runtime/get_runfiles_dir.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "clients/cxx/storage/google3_storage.h"
#include "internal/cxx/storage_client/google_oauth_fetcher.h"
#include "internal/cxx/storage_client/http_transport.h"
#include "internal/cxx/storage_client/http_over_rpc_transport.h"
#include "internal/cxx/storage_client/oauth_token_provider.h"
#include "internal/cxx/storage_client/transport.h"
#include "absl/flags/flag.h"

ABSL_DECLARE_FLAG(bool, mako_auth);
ABSL_DECLARE_FLAG(std::string, mako_auth_service_account);
ABSL_DECLARE_FLAG(bool, mako_auth_force_adc);
ABSL_DECLARE_FLAG(bool, mako_internal_auth_testuser_ok);

namespace mako {
namespace {

OAuthTokenProvider* GetTokenProvider(BaseStorageClient* client) {
  return down_cast<internal::HttpTransport*>(client->transport())
      ->token_provider();
}

TEST(MakoStorageTest, HttpOverRpcAuth) {
  FlagSaver flag_saver;
  // Getting a token will fail under the test user, but the class will be
  // constructed without failure.
  absl::SetFlag(&FLAGS_mako_internal_auth_testuser_ok, true);
  auto client = NewMakoClient("probablynotarealurl2534547342.com").value();
  EXPECT_NE(
      dynamic_cast<internal::HttpOverRpcStorageTransport*>(client->transport()),
      nullptr);
}

TEST(MakoStorageTest, NoAuth) {
  FlagSaver flag_saver;
  absl::SetFlag(&FLAGS_mako_auth, false);
  auto client = NewMakoClient().value();
  EXPECT_EQ(GetTokenProvider(client.get()), nullptr);
}

TEST(MakoStorageTest, ADCAuth) {
  FlagSaver flag_saver;
  absl::SetFlag(&FLAGS_mako_auth_force_adc, true);
  auto client = NewMakoClient("yahoo.com").value();
  EXPECT_NE(dynamic_cast<internal::GoogleOAuthFetcher*>(
                GetTokenProvider(client.get())),
            nullptr);
}

TEST(MakoStorageTest, NoHostnameProvided) {
  FlagSaver flag_saver;
  // Getting a token will fail under the test user, but the class will be
  // constructed without failure.
  absl::SetFlag(&FLAGS_mako_internal_auth_testuser_ok, true);
  auto client = NewMakoClient().value();
  EXPECT_EQ(client->GetHostname(), "makoperf.appspot.com");
}

TEST(MakoStorageTest, HostnameProvided) {
  const std::string host = "http://example.com";
  FlagSaver flag_saver;
  // Getting a token will fail under the test user, but the class will be
  // constructed without failure.
  absl::SetFlag(&FLAGS_mako_internal_auth_testuser_ok, true);
  auto client = NewMakoClient(host).value();
  EXPECT_EQ(client->GetHostname(), host);
}

TEST(MakoStoragetest, HostnameOverride) {
  const std::string hostname_override = "http://hostname_override.com";
  const std::string host = "http://example.com";
  FlagSaver flag_saver;
  // Getting a token will fail under the test user, but the class will be
  // constructed without failure.
  absl::SetFlag(&FLAGS_mako_internal_auth_testuser_ok, true);
  absl::SetFlag(&FLAGS_mako_internal_storage_host, hostname_override);
  auto client = NewMakoClient(host).value();
  EXPECT_EQ(client->GetHostname(), hostname_override);
}

}  // namespace
}  // namespace mako

int main(int argc, char* argv[]) {
  // TODO(b/123657925): Avoid GoogleOAuthFetcher check-failing if there are no
  // Application Default Credentials. setenv should be called before GoogleInit,
  // per go/cpp-primer#registration.
  std::string key_path = devtools_build::GetDataDependencyFilepath(
      "google3/testing/performance/mako/clients/go/fileio/"
      "mako-gcsfio-tester-key.json");
  setenv("GOOGLE_APPLICATION_CREDENTIALS", key_path.data(), 0);
  InitGoogle(argv[0], &argc, &argv, true);
  return RUN_ALL_TESTS();
}
