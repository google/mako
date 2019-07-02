#ifndef TESTING_PERFORMANCE_PERFGATE_SPEC_CXX_MOCK_STORAGE_H_
#define TESTING_PERFORMANCE_PERFGATE_SPEC_CXX_MOCK_STORAGE_H_

#include "gmock/gmock.h"
#include "spec/cxx/storage.h"
#include "spec/proto/mako.pb.h"

namespace mako {

class MockStorage : public mako::Storage {
 public:
  MOCK_METHOD2(CreateBenchmarkInfo, bool(const mako::BenchmarkInfo&,
                                         mako::CreationResponse*));
  MOCK_METHOD2(UpdateBenchmarkInfo, bool(const mako::BenchmarkInfo&,
                                         mako::ModificationResponse*));
  MOCK_METHOD2(QueryBenchmarkInfo, bool(const mako::BenchmarkInfoQuery&,
                                        mako::BenchmarkInfoQueryResponse*));
  MOCK_METHOD2(DeleteBenchmarkInfo, bool(const mako::BenchmarkInfoQuery&,
                                         mako::ModificationResponse*));
  MOCK_METHOD2(CountBenchmarkInfo, bool(const mako::BenchmarkInfoQuery&,
                                        mako::CountResponse*));

  MOCK_METHOD2(CreateRunInfo,
               bool(const mako::RunInfo&, mako::CreationResponse*));
  MOCK_METHOD2(UpdateRunInfo,
               bool(const mako::RunInfo&, mako::ModificationResponse*));
  MOCK_METHOD2(QueryRunInfo, bool(const mako::RunInfoQuery&,
                                  mako::RunInfoQueryResponse*));
  MOCK_METHOD2(DeleteRunInfo, bool(const mako::RunInfoQuery&,
                                   mako::ModificationResponse*));
  MOCK_METHOD2(CountRunInfo,
               bool(const mako::RunInfoQuery&, mako::CountResponse*));

  MOCK_METHOD2(CreateSampleBatch,
               bool(const mako::SampleBatch&, mako::CreationResponse*));
  MOCK_METHOD2(QuerySampleBatch, bool(const mako::SampleBatchQuery&,
                                      mako::SampleBatchQueryResponse*));
  MOCK_METHOD2(DeleteSampleBatch, bool(const mako::SampleBatchQuery&,
                                       mako::ModificationResponse*));

  MOCK_METHOD1(GetMetricValueCountMax, std::string(int*));
  MOCK_METHOD1(GetSampleErrorCountMax, std::string(int*));
  MOCK_METHOD1(GetBatchSizeMax, std::string(int*));

  MOCK_METHOD0(GetHostname, std::string());
};

}  // namespace mako

#endif  // TESTING_PERFORMANCE_PERFGATE_SPEC_CXX_MOCK_STORAGE_H_
