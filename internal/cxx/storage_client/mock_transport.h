#ifndef TESTING_PERFORMANCE_PERFGATE_INTERNAL_CXX_STORAGE_CLIENT_MOCK_TRANSPORT_H_
#define TESTING_PERFORMANCE_PERFGATE_INTERNAL_CXX_STORAGE_CLIENT_MOCK_TRANSPORT_H_

#include "src/google/protobuf/message.h"
#include "gmock/gmock.h"
#include "internal/cxx/storage_client/transport.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "helpers/cxx/status/status.h"

namespace mako {
namespace internal {

class MockTransport : public mako::internal::StorageTransport {
 public:
  MOCK_METHOD0(Connect, helpers::Status());
  MOCK_METHOD4(Call, helpers::Status(absl::string_view path,
                                     const google::protobuf::Message& request,
                                     absl::Duration deadline,
                                     google::protobuf::Message* response));
  MOCK_CONST_METHOD0(last_call_server_elapsed_time, absl::Duration());
  MOCK_METHOD1(set_client_tool_tag, void(absl::string_view));
  MOCK_METHOD0(GetHostname, std::string());
};

}  // namespace internal
}  // namespace mako

#endif  // TESTING_PERFORMANCE_PERFGATE_INTERNAL_CXX_STORAGE_CLIENT_MOCK_TRANSPORT_H_
