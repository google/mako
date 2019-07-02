#ifndef TESTING_PERFORMANCE_PERFGATE_SPEC_CXX_MOCK_ANALYZER_H_
#define TESTING_PERFORMANCE_PERFGATE_SPEC_CXX_MOCK_ANALYZER_H_

#include "gmock/gmock.h"
#include "spec/cxx/analyzer.h"

namespace mako {
class MockAnalyzer : public Analyzer {
 public:
  MOCK_METHOD2(ConstructHistoricQuery,
               bool(const mako::AnalyzerHistoricQueryInput& query_input,
                    mako::AnalyzerHistoricQueryOutput* query_output));
  MOCK_METHOD2(Analyze, bool(const mako::AnalyzerInput& analyzer_input,
                             mako::AnalyzerOutput* analyzer_output));
  MOCK_METHOD0(analyzer_type, std::string());
  MOCK_METHOD0(analyzer_name, std::string());

 private:
  bool DoAnalyze(const mako::AnalyzerInput& analyzer_input,
                 mako::AnalyzerOutput* analyzer_output) override {
    return true;
  }
};
}  // namespace mako

#endif  // TESTING_PERFORMANCE_PERFGATE_SPEC_CXX_MOCK_ANALYZER_H_
