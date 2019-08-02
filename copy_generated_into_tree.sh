#!/bin/bash
#
# Copyright 2019 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


mkdir -p ./spec/proto/mako_go_proto/ && cp -rf ./bazel-genfiles/spec/proto/linux_amd64_stripped/mako_go_proto%/github.com/google/mako/spec/proto/mako_go_proto/mako.pb.go  ./spec/proto/mako_go_proto/mako.pb.go
mkdir -p ./helpers/proto/quickstore/quickstore_go_proto/ && cp -rf ./bazel-genfiles/helpers/proto/quickstore/linux_amd64_stripped/quickstore_go_proto%/github.com/google/mako/helpers/proto/quickstore/quickstore_go_proto/quickstore.pb.go ./helpers/proto/quickstore/quickstore_go_proto/quickstore.pb.go
mkdir -p ./internal/quickstore_microservice/proto/quickstore_go_proto/ && cp -f ./bazel-genfiles/internal/quickstore_microservice/proto/linux_amd64_stripped/quickstore_go_grpc_pb%/github.com/google/mako/internal/quickstore_microservice/proto/quickstore_go_grpc_pb/quickstore.pb.go ./internal/quickstore_microservice/proto/quickstore_go_proto/
mkdir -p ./clients/proto/analyzers/threshold_analyzer_go_proto && cp -f bazel-genfiles/clients/proto/analyzers/linux_amd64_stripped/threshold_analyzer_go_proto%/github.com/google/mako/clients/proto/analyzers/threshold_analyzer_go_proto/threshold_analyzer.pb.go ./clients/proto/analyzers/threshold_analyzer_go_proto
mkdir -p ./clients/proto/analyzers/utest_analyzer_go_proto && cp -f bazel-genfiles/clients/proto/analyzers/linux_amd64_stripped/utest_analyzer_go_proto%/github.com/google/mako/clients/proto/analyzers/utest_analyzer_go_proto/utest_analyzer.pb.go ./clients/proto/analyzers/utest_analyzer_go_proto
mkdir -p ./clients/proto/analyzers/window_deviation_go_proto && cp -f bazel-genfiles/clients/proto/analyzers/linux_amd64_stripped/window_deviation_go_proto%/github.com/google/mako/clients/proto/analyzers/window_deviation_go_proto/window_deviation.pb.go ./clients/proto/analyzers/window_deviation_go_proto
