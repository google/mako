#include "helpers/cxx/status/status.h"
#include <string>

#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"

namespace mako {
namespace helpers {

Status OkStatus() { return Status(); }

std::string StatusToString(const Status& status) {
  return absl::StrCat(StatusCodeToString(status.code()), ": ",
                      status.message());
}

bool HasErrorCode(const Status& status, StatusCode code) {
  return status.code() == code;
}

}  // namespace helpers
}  // namespace mako

