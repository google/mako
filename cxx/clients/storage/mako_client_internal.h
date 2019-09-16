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
#ifndef CXX_CLIENTS_STORAGE_MAKO_CLIENT_INTERNAL_H_
#define CXX_CLIENTS_STORAGE_MAKO_CLIENT_INTERNAL_H_

#include <memory>

#include "absl/strings/string_view.h"
#include "cxx/clients/storage/google3_storage.h"
#include "cxx/clients/storage/mako_client.h"
#include "cxx/helpers/status/status.h"
#include "cxx/helpers/status/statusor.h"

namespace mako {
namespace internal {

// A simpler NewMakoClient function to SWIG.
mako::google3_storage::Storage* NewMakoClient(absl::string_view hostname,
                                                  std::string* error) {
    return mako::NewMakoClient(hostname).release();
}

// A NewMakoClient adapter for CLIF. unique_ptr causes ambiguities,
// see https://stackoverflow.com/a/53480559 for details.
mako::helpers::StatusOr<
    std::shared_ptr<mako::google3_storage::Storage> /* spacer for swig*/>
NewMakoClientClif(absl::string_view hostname) {
  return {mako::NewMakoClient(hostname)};
}

}  // namespace internal
}  // namespace mako

#endif  // CXX_CLIENTS_STORAGE_MAKO_CLIENT_INTERNAL_H_
