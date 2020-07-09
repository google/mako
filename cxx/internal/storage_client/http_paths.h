// The HTTP paths that make up the HTTP storage API exposed by the Mako
// storage server.
#ifndef CXX_INTERNAL_STORAGE_CLIENT_HTTP_PATHS_H_
#define CXX_INTERNAL_STORAGE_CLIENT_HTTP_PATHS_H_

#include "absl/base/attributes.h"
#include "absl/strings/string_view.h"

namespace mako {
namespace internal {

ABSL_CONST_INIT extern const absl::string_view kCreateProjectInfoPath;
ABSL_CONST_INIT extern const absl::string_view kUpdateProjectInfoPath;
ABSL_CONST_INIT extern const absl::string_view kGetProjectInfoPath;
ABSL_CONST_INIT extern const absl::string_view kQueryProjectInfoPath;
ABSL_CONST_INIT extern const absl::string_view kCreateBenchmarkPath;
ABSL_CONST_INIT extern const absl::string_view kQueryBenchmarkPath;
ABSL_CONST_INIT extern const absl::string_view kModificationBenchmarkPath;
ABSL_CONST_INIT extern const absl::string_view kDeleteBenchmarkPath;
ABSL_CONST_INIT extern const absl::string_view kCountBenchmarkPath;
ABSL_CONST_INIT extern const absl::string_view kCreateRunInfoPath;
ABSL_CONST_INIT extern const absl::string_view kQueryRunInfoPath;
ABSL_CONST_INIT extern const absl::string_view kModificationRunInfoPath;
ABSL_CONST_INIT extern const absl::string_view kDeleteRunInfoPath;
ABSL_CONST_INIT extern const absl::string_view kCountRunInfoPath;
ABSL_CONST_INIT extern const absl::string_view kCreateSampleBatchPath;
ABSL_CONST_INIT extern const absl::string_view kQuerySampleBatchPath;
ABSL_CONST_INIT extern const absl::string_view kDeleteSampleBatchPath;
ABSL_CONST_INIT extern const absl::string_view kSudoPath;
ABSL_CONST_INIT extern const absl::string_view kAlertInfoCreatePath;
ABSL_CONST_INIT extern const absl::string_view kAlertInfoUpdatePath;
ABSL_CONST_INIT extern const absl::string_view kAlertInfoQueryPath;
ABSL_CONST_INIT extern const absl::string_view kModificationProjectSummaryPath;
ABSL_CONST_INIT extern const absl::string_view
    kModificationProjectSummaryBatchPath;
ABSL_CONST_INIT extern const absl::string_view kCleanupProjectSummaryPath;
ABSL_CONST_INIT extern const absl::string_view kGetProjectSummaryPath;
ABSL_CONST_INIT extern const absl::string_view kProjectInfoInternalUpdatePath;
ABSL_CONST_INIT extern const absl::string_view kProjectInfoInternalGetPath;

}  // namespace internal
}  // namespace mako

#endif  // CXX_INTERNAL_STORAGE_CLIENT_HTTP_PATHS_H_
