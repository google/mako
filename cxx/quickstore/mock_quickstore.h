// Copyright 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// see the license for the specific language governing permissions and
// limitations under the license.
#ifndef CXX_QUICKSTORE_MOCK_QUICKSTORE_H_
#define CXX_QUICKSTORE_MOCK_QUICKSTORE_H_

#include <map>
#include <string>

#include "gmock/gmock.h"
#include "cxx/quickstore/quickstore.h"
#include "proto/quickstore/quickstore.pb.h"

namespace mako {
namespace quickstore {

class MockQuickstore : public Quickstore {
 public:
  explicit MockQuickstore(const std::string& benchmark_key)
      : Quickstore(benchmark_key) {}
  explicit MockQuickstore(
      const mako::quickstore::QuickstoreInput& input)
      : Quickstore(input) {}

  MOCK_METHOD(std::string, AddSamplePoint, (const mako::SamplePoint& point),
              (override));
  MOCK_METHOD(std::string, AddSamplePoint,
              (double xval, (const std::map<std::string, double>& yvals)),
              (override));

  MOCK_METHOD(std::string, AddError,
              (double xval, const std::string& error_msg), (override));
  MOCK_METHOD(std::string, AddError, (const mako::SampleError&),
              (override));

  MOCK_METHOD(std::string, AddRunAggregate,
              (const std::string& value_key, double value), (override));
  MOCK_METHOD(std::string, AddMetricAggregate,
              (const std::string& value_key, const std::string& aggregate_type,
               double value),
              (override));
  MOCK_METHOD(mako::quickstore::QuickstoreOutput, Store, (),
              (override));
};

}  // namespace quickstore
}  // namespace mako

#endif  // CXX_QUICKSTORE_MOCK_QUICKSTORE_H_
