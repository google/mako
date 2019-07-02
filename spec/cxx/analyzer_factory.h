#ifndef TESTING_PERFORMANCE_PERFGATE_SPEC_CXX_ANALYZER_FACTORY_H_
#define TESTING_PERFORMANCE_PERFGATE_SPEC_CXX_ANALYZER_FACTORY_H_

#include <memory>
#include <string>
#include <vector>

#include "spec/proto/mako.pb.h"
#include "spec/cxx/analyzer.h"

namespace mako {

// AnalyzerFactory interface to be implemented by test authors.
//
// An implementation is provided to the framework via LoadMain and the framework
// uses it to create and run user-configured analyzers.
class AnalyzerFactory {
 public:
  // Given the input, creates all analyzers.
  //
  // For an associated LoadMain execution, the framework will call this function
  // once.
  //
  // Args:
  //    input: Input which may or may not be needed to construct analyzers.
  //           See the AnalyzerFactoryInput proto documentation.
  //    analyzers: The output analyzers.
  //
  // Returned std::string is empty for success or an error message for failure.
  virtual std::string NewAnalyzers(
      const mako::AnalyzerFactoryInput& input,
      std::vector<std::unique_ptr<mako::Analyzer>>* analyzers) = 0;

  virtual ~AnalyzerFactory() {}
};
}  // namespace mako
#endif  // TESTING_PERFORMANCE_PERFGATE_SPEC_CXX_ANALYZER_FACTORY_H_
