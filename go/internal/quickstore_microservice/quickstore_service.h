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
//
// QuickstoreService exposes an RPC interface for running Quickstore
#ifndef GO_INTERNAL_QUICKSTORE_MICROSERVICE_QUICKSTORE_SERVICE_H_
#define GO_INTERNAL_QUICKSTORE_MICROSERVICE_QUICKSTORE_SERVICE_H_

#include <functional>
#include <memory>

#include "cxx/helpers/status/statusor.h"
#include "cxx/internal/queue_ifc.h"
#include "cxx/spec/storage.h"
#include "go/internal/quickstore_microservice/proto/quickstore.grpc.pb.h"
#include "go/internal/quickstore_microservice/proto/quickstore.pb.h"
#include "proto/internal/mako_internal.pb.h"

namespace mako {
namespace internal {
namespace quickstore_microservice {

class QuickstoreService : public Quickstore::Service {
 public:
  // Exposed for testing
  explicit QuickstoreService(
      mako::internal::QueueInterface<bool>* shutdown_queue,
      std::function<std::unique_ptr<mako::Storage>(absl::string_view)>
          storage_factory)
      : shutdown_queue_(shutdown_queue), storage_factory_(storage_factory) {}
  ~QuickstoreService() override {}

  static mako::helpers::StatusOr<std::unique_ptr<QuickstoreService>> Create(
      const std::string& default_host,
      mako::internal::QueueInterface<bool>* shutdown_queue);

  grpc::Status Init(grpc::ServerContext* context, const InitInput* request,
                    InitOutput* response) override;

  grpc::Status Store(grpc::ServerContext* context, const StoreInput* request,
                     StoreOutput* response) override;

  grpc::Status ShutdownMicroservice(grpc::ServerContext* context,
                                    const ShutdownInput* request,
                                    ShutdownOutput* response) override;

 private:
  bool initialized_ = false;
  mako::internal::QueueInterface<bool>* shutdown_queue_;

  // The storage factory creates a storage instance from a hostname parameter
  // specifying the target Mako host. Empty hostname parameters may be passed
  // and must be supported.
  std::function<std::unique_ptr<mako::Storage>(absl::string_view)>
      storage_factory_;

  // Storage isn't populated until Init(), or until the first Store() call in
  // the case that Init is skipped.
  std::unique_ptr<mako::Storage> storage_;
};

}  // namespace quickstore_microservice
}  // namespace internal
}  // namespace mako
#endif  // GO_INTERNAL_QUICKSTORE_MICROSERVICE_QUICKSTORE_SERVICE_H_
