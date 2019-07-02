#ifndef TESTING_PERFORMANCE_PERFGATE_CLIENTS_CXX_AGGREGATOR_THREADSAFE_RUNNING_STATS_H_
#define TESTING_PERFORMANCE_PERFGATE_CLIENTS_CXX_AGGREGATOR_THREADSAFE_RUNNING_STATS_H_

#include <memory>
#include <string>
#include <vector>

#include "internal/cxx/pgmath.h"
#include "absl/synchronization/mutex.h"

namespace mako {
namespace aggregator {

class ThreadsafeRunningStats {
 public:
  explicit ThreadsafeRunningStats(const int max_sample_size):
    // We only need a Random instance when max_sample_size > 0. They're somewhat
    // heavyweight, so if max_sample_size <= 0, don't bother creating it.
    random_(max_sample_size > 0 ? new mako::internal::Random : nullptr),
    rs_(mako::internal::RunningStats::Config{
      max_sample_size, random_.get()}) {}

  std::string AddVector(const std::vector<double>& values) {
    absl::MutexLock l(&mutex_);
    return rs_.AddVector(values);
  }

  mako::internal::RunningStats::Result Count() const {
    return rs_.Count();
  }

  mako::internal::RunningStats::Result Min() const {
    return rs_.Min();
  }

  mako::internal::RunningStats::Result Max() const {
    return rs_.Max();
  }

  mako::internal::RunningStats::Result Mean() const {
    return rs_.Mean();
  }

  mako::internal::RunningStats::Result Median() {
    absl::MutexLock l(&mutex_);
    return rs_.Median();
  }

  mako::internal::RunningStats::Result Stddev() const {
    return rs_.Stddev();
  }

  mako::internal::RunningStats::Result Mad() {
    absl::MutexLock l(&mutex_);
    return rs_.Mad();
  }

  mako::internal::RunningStats::Result Percentile(double pct) {
    absl::MutexLock l(&mutex_);
    return rs_.Percentile(pct);
  }

 private:
  // Used to synchronize calls that modify rs_. The const methods are safe to
  // call concurrently, so no synchronization is needed there.
  absl::Mutex mutex_;
  std::unique_ptr<mako::internal::Random> random_;
  mako::internal::RunningStats rs_;
};
}  // namespace aggregator
}  // namespace mako
#endif  // TESTING_PERFORMANCE_PERFGATE_CLIENTS_CXX_AGGREGATOR_THREADSAFE_RUNNING_STATS_H_
