//
// For more information about Mako see: go/mako.
//
#ifndef TESTING_PERFORMANCE_PERFGATE_SPEC_CXX_DOWNSAMPLER_H_
#define TESTING_PERFORMANCE_PERFGATE_SPEC_CXX_DOWNSAMPLER_H_

#include <string>
#include <memory>

#include "spec/cxx/fileio.h"
#include "spec/proto/mako.pb.h"

namespace mako {

// An abstract class which describes the Mako C++ Downsampler interface.
//
// The Downsampler class is used to downsample user data to fit within Mako
// storage limits. See go/mako-storage#limits for more info on these limits.
//
// See implementing classes for detailed description on usage. See below for
// details on individual functions.
class Downsampler {
 public:
  // Set the FileIO implementation that is used to read samples.
  virtual void SetFileIO(std::unique_ptr<FileIO> fileio) = 0;

  // Perform downsampling to ensure data is suitable for storage system.
  // Returned std::string contains error message, if empty then operation was
  // successful.
  virtual std::string Downsample(
      const mako::DownsamplerInput& downsampler_input,
      mako::DownsamplerOutput* downsampler_output) = 0;

  virtual ~Downsampler() {}
};

}  // namespace mako

#endif  // TESTING_PERFORMANCE_PERFGATE_SPEC_CXX_DOWNSAMPLER_H_
