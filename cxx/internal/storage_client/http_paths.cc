#include "cxx/internal/storage_client/http_paths.h"

#include "absl/strings/string_view.h"

namespace mako {
namespace internal {

const absl::string_view kCreateProjectInfoPath = "/storage/project-info/create";
const absl::string_view kUpdateProjectInfoPath = "/storage/project-info/update";
const absl::string_view kGetProjectInfoPath = "/storage/project-info/get";
const absl::string_view kQueryProjectInfoPath = "/storage/project-info/query";
const absl::string_view kCreateBenchmarkPath = "/storage/benchmark-info/create";
const absl::string_view kQueryBenchmarkPath = "/storage/benchmark-info/query";
const absl::string_view kModificationBenchmarkPath =
    "/storage/benchmark-info/update";
const absl::string_view kDeleteBenchmarkPath = "/storage/benchmark-info/delete";
const absl::string_view kCountBenchmarkPath = "/storage/benchmark-info/count";
const absl::string_view kCreateRunInfoPath = "/storage/run-info/create";
const absl::string_view kQueryRunInfoPath = "/storage/run-info/query";
const absl::string_view kModificationRunInfoPath = "/storage/run-info/update";
const absl::string_view kDeleteRunInfoPath = "/storage/run-info/delete";
const absl::string_view kCountRunInfoPath = "/storage/run-info/count";
const absl::string_view kCreateSampleBatchPath = "/storage/sample-batch/create";
const absl::string_view kQuerySampleBatchPath = "/storage/sample-batch/query";
const absl::string_view kDeleteSampleBatchPath = "/storage/sample-batch/delete";
const absl::string_view kSudoPath = "/storage/sudo";
const absl::string_view kAlertInfoCreatePath = "/storage/alert-info/create";
const absl::string_view kAlertInfoUpdatePath = "/storage/alert-info/update";
const absl::string_view kAlertInfoQueryPath = "/storage/alert-info/query";
const absl::string_view kModificationProjectSummaryPath =
    "/storage/project-summary/update";
const absl::string_view kModificationProjectSummaryBatchPath =
    "/storage/project-summary/update-batch";
const absl::string_view kCleanupProjectSummaryPath =
    "/storage/project-summary/cleanup";
const absl::string_view kGetProjectSummaryPath = "/storage/project-summary/get";
const absl::string_view kProjectInfoInternalUpdatePath =
    "/storage/project-info-internal/update";
const absl::string_view kProjectInfoInternalGetPath =
    "/storage/project-info-internal/get";

}  // namespace internal
}  // namespace mako
