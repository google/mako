#ifndef QUICKSTORE_CXX_MOCK_QUICKSTORE_H_
#define QUICKSTORE_CXX_MOCK_QUICKSTORE_H_

#include <map>
#include <string>

#include "gmock/gmock.h"
#include "quickstore/cxx/quickstore.h"
#include "quickstore/quickstore.pb.h"

namespace mako {
namespace quickstore {

class MockQuickstore : public Quickstore {
 public:
  explicit MockQuickstore(const std::string& benchmark_key)
      : Quickstore(benchmark_key) {}
  explicit MockQuickstore(
      const mako::quickstore::QuickstoreInput& input)
      : Quickstore(input) {}

  MOCK_METHOD1(AddSamplePoint, std::string(const mako::SamplePoint& point));
  MOCK_METHOD2(AddSamplePoint,
               std::string(double xval, const std::map<std::string, double>& yvals));

  MOCK_METHOD2(AddError, std::string(double xval, const std::string& error_msg));
  MOCK_METHOD1(AddError, std::string(const mako::SampleError&));

  MOCK_METHOD2(AddRunAggregate, std::string(const std::string& value_key, double value));
  MOCK_METHOD3(AddMetricAggregate,
               std::string(const std::string& value_key, const std::string& aggregate_type,
                      double value));
  MOCK_METHOD0(Store, mako::quickstore::QuickstoreOutput());
};

}  // namespace quickstore
}  // namespace mako

#endif  // QUICKSTORE_CXX_MOCK_QUICKSTORE_H_
