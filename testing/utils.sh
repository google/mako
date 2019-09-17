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

# The utils in this file assume that the calling script has ensured the CWD is
# the root of the Mako repo.

# Returns two paths on each line: <src> <dst>
# where <src> is the bazel-generated source file, and <dst> is the directory
# path in the repo where that file should be copied.
generate_go_proto_files() {
  bazel query 'kind("go_proto_library rule", //...)' 2>"./bazel.err" | while read target; do
    bazel build $target

    # Break the target into its base path and the target name by using ':' as a
    # separator.
    IFS=':' read -r target_path target_name <<< "${target}"

    # Bazel has no way to directly get the 'importpath' field, so we ask it to
    # export the query output to XML and then parse it.
    xml=$(bazel query $target --output xml | grep importpath)
    import_path=$([[ ${xml} =~ \<string\ name=\"importpath\"\ value=\"(.*)\"\/\> ]]; echo ${BASH_REMATCH[1]})

    # This is the path to the generated file.
    genfile=$(ls bazel-genfiles/${target_path}/*/${target_name}%/${import_path}/*.pb.go)

    # Figure out where to copy it.
    filename=$(basename ${genfile})
    dest_path="${import_path#'github.com/google/mako/'}/${filename}"

    echo "${genfile} ${dest_path}"
  done
}
