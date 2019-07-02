#ifndef TESTING_PERFORMANCE_PERFGATE_INTERNAL_CXX_STORAGE_CLIENT_TRANSPORT_H_
#define TESTING_PERFORMANCE_PERFGATE_INTERNAL_CXX_STORAGE_CLIENT_TRANSPORT_H_

#include "src/google/protobuf/message.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "helpers/cxx/status/status.h"

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
  // util::error::FAILED_PRECONDITION indicates an error that is not retryable
  // (as per the litmus test described at
  // http://google3/util/task/codes.proto?l=91-101&rcl=182095077).
  virtual helpers::Status Connect() = 0;

  virtual void set_client_tool_tag(absl::string_view) = 0;

  // Make the specified call. Return Status codes other than util::error::OK
  // indicate a transport-layer error (e.g. failed to send an RPC). Storage API
  // errors will be returned via the response (with a Status of
  // util::error::OK).
  //
  // util::error::FAILED_PRECONDITION indicates an error that is not retryable
  // (as per the litmus test described at
  // http://google3/util/task/codes.proto?l=91-101&rcl=182095077).
  virtual helpers::Status Call(absl::string_view path,
                               const google::protobuf::Message& request,
                               absl::Duration deadline,
                               google::protobuf::Message* response) = 0;

  // Returns the number of seconds that the last RPC call took (according to the
  // server). This is exposed for tests of this library to use and should not
  // otherwise be relied on. This is also not guaranteed to be correct in
  // multi-threaded situations.
  virtual absl::Duration last_call_server_elapsed_time() const = 0;

  // The hostname backing this Storage implementation.
  // Returns a URL without the trailing slash.
  virtual std::string GetHostname() = 0;
};

}  // namespace internal
}  // namespace mako

#endif  // TESTING_PERFORMANCE_PERFGATE_INTERNAL_CXX_STORAGE_CLIENT_TRANSPORT_H_
