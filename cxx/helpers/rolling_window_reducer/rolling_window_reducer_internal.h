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
#ifndef CXX_HELPERS_ROLLING_WINDOW_REDUCER_ROLLING_WINDOW_REDUCER_INTERNAL_H_
#define CXX_HELPERS_ROLLING_WINDOW_REDUCER_ROLLING_WINDOW_REDUCER_INTERNAL_H_

#include "absl/strings/string_view.h"
#include "cxx/spec/fileio.h"
#include "helpers/cxx/status/status.h"
#include "helpers/cxx/status/statusor.h"
#include "proto/helpers/rolling_window_reducer/rolling_window_reducer.pb.h"
#include "spec/proto/mako.pb.h"
namespace mako {
namespace helpers {

// Uses user provided FileIO instance to read sample points from file_path and
// feeds them to a RollingWindowReducer instance configured based on user
// provided RWRConfigs. Resulting data is appended back to file_path.
// Intended for use by Quickstore. If you think you have a valid use case for
// this functionit, please reach out to us at
// https://github.com/google/mako/issues
Status Reduce(absl::string_view file_path,
              const std::vector<RWRConfig>& configs, FileIO* file_io);

}  // namespace helpers
}  // namespace mako

#endif  // CXX_HELPERS_ROLLING_WINDOW_REDUCER_ROLLING_WINDOW_REDUCER_INTERNAL_H_
