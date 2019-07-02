#include "internal/cxx/load/common/thread_pool_factory.h"

#include "absl/memory/memory.h"

namespace mako {
namespace internal {

std::unique_ptr<ThreadPool> CreateThreadPool(int num_threads) {
  return absl::make_unique<ThreadPool>(
      num_threads);
}

}  // namespace internal
}  // namespace mako
