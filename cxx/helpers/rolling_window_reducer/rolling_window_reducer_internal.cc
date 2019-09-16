// Copyright 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "cxx/helpers/rolling_window_reducer/rolling_window_reducer_internal.h"

#include "absl/strings/str_format.h"
#include "cxx/helpers/rolling_window_reducer/rolling_window_reducer.h"
#include "cxx/internal/utils/cleanup.h"
#include "helpers/cxx/status/canonical_errors.h"

namespace mako {
namespace helpers {

Status Reduce(absl::string_view file_path,
                                    const std::vector<RWRConfig>& configs,
                                    FileIO* file_io) {
  auto status_or_points = RollingWindowReducer::ReduceImpl(
      absl::MakeSpan(&file_path, 1), configs, file_io);
  if (!status_or_points.ok()) {
    return std::move(status_or_points).status();
  }

  if (!file_io->Open(std::string(file_path), FileIO::AccessMode::kAppend)) {
    return Annotate(UnknownError(file_io->Error()), "opening file");
  }
  const auto close_file =
      mako::internal::MakeCleanup([file_io] { file_io->Close(); });

  for (auto& point : std::move(status_or_points).value()) {
    mako::SampleRecord record;
    *record.mutable_sample_point() = std::move(point);
    if (!file_io->Write(record)) {
      return Annotate(UnknownError(file_io->Error()), "writing record");
    }
  }
  return OkStatus();
}
}  // namespace helpers
}  // namespace mako
