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

# Fail on any error.
set -e
# Display commands being run.
set -x

# Find the script's directory.
DIR="${BASH_SOURCE%/*}"
if [[ ! -d "$DIR" ]]; then DIR="$PWD"; fi

# Include dependencies.
. "$DIR/utils.sh"

set_bazel

some_changed=false
while read -r sourcepath destpath; do
  if ! cmp -s "${sourcepath}" "${destpath}"; then
    some_changed=true
    echo "WARNING: Generated proto Go file has changed!"
    echo "- Old version:${sourcepath}"
    echo "- New version:${destpath}"
    echo "- Command to see diff: diff ${sourcepath} ${destpath}"
    echo
  fi
done <<<$(generate_go_proto_files)

if ${some_changed}; then
  echo "==========================="
  printf "ERROR: Some generated proto Go files are out of date! This means"
  printf "the source proto files have been modified without the generated"
  printf "files being refreshed.\n"
  exit 1
fi

echo "All generated proto Go files are okay!"
