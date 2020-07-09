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
#ifndef CXX_SPEC_MOCK_STORAGE_H_
#define CXX_SPEC_MOCK_STORAGE_H_

#include "gmock/gmock.h"
#include "cxx/spec/storage.h"
#include "spec/proto/mako.pb.h"

namespace mako {

class MockStorage : public mako::Storage {
 public:
  MOCK_METHOD(bool, CreateBenchmarkInfo,
              (const mako::BenchmarkInfo&, mako::CreationResponse*),
              (override));
  MOCK_METHOD(bool, UpdateBenchmarkInfo,
              (const mako::BenchmarkInfo&, mako::ModificationResponse*),
              (override));
  MOCK_METHOD(bool, QueryBenchmarkInfo,
              (const mako::BenchmarkInfoQuery&,
               mako::BenchmarkInfoQueryResponse*),
              (override));
  MOCK_METHOD(bool, DeleteBenchmarkInfo,
              (const mako::BenchmarkInfoQuery&,
               mako::ModificationResponse*),
              (override));
  MOCK_METHOD(bool, CountBenchmarkInfo,
              (const mako::BenchmarkInfoQuery&, mako::CountResponse*),
              (override));

  MOCK_METHOD(bool, CreateRunInfo,
              (const mako::RunInfo&, mako::CreationResponse*),
              (override));
  MOCK_METHOD(bool, UpdateRunInfo,
              (const mako::RunInfo&, mako::ModificationResponse*),
              (override));
  MOCK_METHOD(bool, QueryRunInfo,
              (const mako::RunInfoQuery&, mako::RunInfoQueryResponse*),
              (override));
  MOCK_METHOD(bool, DeleteRunInfo,
              (const mako::RunInfoQuery&, mako::ModificationResponse*),
              (override));
  MOCK_METHOD(bool, CountRunInfo,
              (const mako::RunInfoQuery&, mako::CountResponse*),
              (override));

  MOCK_METHOD(bool, CreateSampleBatch,
              (const mako::SampleBatch&, mako::CreationResponse*),
              (override));
  MOCK_METHOD(bool, QuerySampleBatch,
              (const mako::SampleBatchQuery&,
               mako::SampleBatchQueryResponse*),
              (override));
  MOCK_METHOD(bool, DeleteSampleBatch,
              (const mako::SampleBatchQuery&,
               mako::ModificationResponse*),
              (override));

  MOCK_METHOD(std::string, GetMetricValueCountMax, (int*), (override));
  MOCK_METHOD(std::string, GetSampleErrorCountMax, (int*), (override));
  MOCK_METHOD(std::string, GetBatchSizeMax, (int*), (override));

  MOCK_METHOD(std::string, GetHostname, (), (override));
};

}  // namespace mako

#endif  // CXX_SPEC_MOCK_STORAGE_H_
