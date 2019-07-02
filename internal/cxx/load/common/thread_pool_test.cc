#include "internal/cxx/load/common/thread_pool.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/synchronization/blocking_counter.h"
#include "absl/synchronization/mutex.h"

namespace mako {
namespace threadpool_internal {
namespace {

using ::testing::Eq;

TEST(ThreadPoolTest, SimpleCounter) {
  absl::Mutex mutex;
  int count = 0;
  int num_threads = 5;
  int iterations = 100;

  absl::BlockingCounter block(num_threads);

  ThreadPool pool(num_threads);

  auto work = [&count, &mutex, &block, iterations]() {
    for (int i=0; i < iterations; ++i) {
      absl::MutexLock lock(&mutex);
      count += 1;
    }
    block.DecrementCount();
  };

  for (int i=0; i < num_threads; ++i) {
    pool.Schedule(work);
  }
  pool.StartWorkers();
  block.Wait();
  EXPECT_THAT(count, Eq(num_threads * iterations));
}

}  // namespace
}  // namespace threadpool_internal
}  // namespace mako
