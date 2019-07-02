//
// For more information about Mako see: go/mako.
//
#ifndef TESTING_PERFORMANCE_PERFGATE_SPEC_CXX_AGGREGATOR_H_
#define TESTING_PERFORMANCE_PERFGATE_SPEC_CXX_AGGREGATOR_H_

#include <memory>
#include <string>

#include "spec/cxx/fileio.h"
#include "spec/proto/mako.pb.h"

namespace mako {

// An abstract class which describes the Mako C++ Aggregator interface.
//
// The Aggregator class is used to calculate metric and run aggregates.
//
// See implementing classes for detailed description on usage. See below for
// details on individual functions.
class Aggregator {
 public:
  // Set the FileIO implementation that is used to read samples.
  virtual void SetFileIO(std::unique_ptr<FileIO> fileio) = 0;

  // Compute aggregates
  // Returned std::string contains error message, if empty then operation was
  // successful.
  virtual std::string Aggregate(const mako::AggregatorInput& aggregator_input,
                           mako::AggregatorOutput* aggregator_output) = 0;

  virtual ~Aggregator() {}
};

}  // namespace mako
#endif  // TESTING_PERFORMANCE_PERFGATE_SPEC_CXX_AGGREGATOR_H_
