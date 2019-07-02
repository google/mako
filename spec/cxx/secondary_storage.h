//
// For more information about Mako see: go/mako.
//
#ifndef TESTING_PERFORMANCE_PERFGATE_SPEC_CXX_SECONDARY_STORAGE_H_
#define TESTING_PERFORMANCE_PERFGATE_SPEC_CXX_SECONDARY_STORAGE_H_

#include <string>

#include "spec/proto/mako.pb.h"

namespace mako {

// Provides a way to write the results of a run to an alternate storage system.
//
// Mako provides primary storage for performance data collected in a
// Mako test. SecondaryStorage provides a way to also store that data in
// another system. Mako writes to secondary storage when writing to primary
// storage.
class SecondaryStorage {
 public:
  virtual ~SecondaryStorage() {}

  // Write all the data accumulated during a run to storage.
  //
  // Mako will provide this with SecondaryStorageWriteInput, which contains
  // details about the benchmark, the run, and all sample data (aggregates and
  // raw measurements).
  //
  // Implementations may also expect a pointer to a SecondaryStorageWriteOutput.
  // This should be set with the outcome of write operation and can also provide
  // links, references, or identifiers that enable later lookup of this data in
  // the secondary storage system by users.
  //
  // Returns a boolean representing whether the operation was successful or not.
  // Error details may be found in SecondaryStorageWriteOutput.status.
  virtual bool Write(const SecondaryStorageWriteInput& input,
                     SecondaryStorageWriteOutput* output) = 0;

  // Max number of metric values that can be saved per run.
  // String contains error message, or empty if successful.
  virtual std::string GetMetricValueCountMax(int* metric_value_count_max) = 0;

  // Max number of errors that can be saved per run.
  // String contains error message, or empty if successful.
  virtual std::string GetSampleErrorCountMax(int* sample_error_max) = 0;

  // Max binary size (in base 10 bytes, eg 1MB == 1,000,000) of a SampleBatch.
  // String contains error message, or empty if successful.
  virtual std::string GetBatchSizeMax(int* batch_size_max) = 0;
};

}  // namespace mako

#endif  // TESTING_PERFORMANCE_PERFGATE_SPEC_CXX_SECONDARY_STORAGE_H_
