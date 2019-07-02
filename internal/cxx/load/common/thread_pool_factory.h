#ifndef TESTING_PERFORMANCE_PERFGATE_INTERNAL_CXX_LOAD_COMMON_THREAD_POOL_FACTORY_H_
#define TESTING_PERFORMANCE_PERFGATE_INTERNAL_CXX_LOAD_COMMON_THREAD_POOL_FACTORY_H_

#include "internal/cxx/load/common/thread_pool.h"

namespace mako {
namespace internal {

using ::mako::threadpool_internal::ThreadPool;

std::unique_ptr<ThreadPool> CreateThreadPool(int num_threads);

}  // namespace internal
}  // namespace mako

#endif  // TESTING_PERFORMANCE_PERFGATE_INTERNAL_CXX_LOAD_COMMON_THREAD_POOL_FACTORY_H_
