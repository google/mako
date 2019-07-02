#ifndef TESTING_PERFORMANCE_PERFGATE_HELPERS_CXX_QUICKSTORE_INTERNAL_STORE_H_
#define TESTING_PERFORMANCE_PERFGATE_HELPERS_CXX_QUICKSTORE_INTERNAL_STORE_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "clients/cxx/dashboard/standard_dashboard.h"
#include "helpers/proto/quickstore/quickstore.pb.h"
#include "spec/cxx/aggregator.h"
#include "spec/cxx/downsampler.h"
#include "spec/cxx/fileio.h"
#include "spec/cxx/storage.h"
#include "spec/proto/mako.pb.h"

namespace mako {
namespace helpers {
namespace quickstore {
namespace internal {

mako::helpers::quickstore::QuickstoreOutput Save(
    const mako::helpers::quickstore::QuickstoreInput& input,
    const std::vector<mako::SamplePoint>& points,
    const std::vector<mako::SampleError>& errors,
    const std::vector<mako::KeyedValue>& run_aggregates,
    const std::vector<std::string>& aggregate_value_keys,
    const std::vector<std::string>& aggregate_types,
    const std::vector<double>& aggregate_values);

mako::helpers::quickstore::QuickstoreOutput SaveWithStorage(
    mako::Storage* storage,
    const mako::helpers::quickstore::QuickstoreInput& input,
    const std::vector<mako::SamplePoint>& points,
    const std::vector<mako::SampleError>& errors,
    const std::vector<mako::KeyedValue>& run_aggregates,
    const std::vector<std::string>& aggregate_value_keys,
    const std::vector<std::string>& aggregate_types,
    const std::vector<double>& aggregate_values);

///// FOR TESTING /////
class InternalQuickstore {
 public:
  InternalQuickstore(mako::Storage* s, std::unique_ptr<mako::FileIO> f,
                     std::unique_ptr<mako::Aggregator> a,
                     std::unique_ptr<mako::Downsampler> d,
                     const QuickstoreInput& i,
                     const std::vector<mako::SamplePoint>& p,
                     const std::vector<mako::SampleError>& e,
                     const std::vector<mako::KeyedValue>& ra,
                     const std::vector<std::string>& avk,
                     const std::vector<std::string>& at,
                     const std::vector<double>& av)
      : storage_(s),
        dashboard_(s->GetHostname()),
        fileio_(std::move(f)),
        aggregator_(std::move(a)),
        downsampler_(std::move(d)),
        input_(i),
        points_(p),
        errors_(e),
        run_aggregates_(ra),
        aggregate_value_keys_(avk),
        aggregate_types_(at),
        aggregate_values_(av) {}
  ~InternalQuickstore() {}
  QuickstoreOutput Save();

 protected:
  std::string UpdateRunAggregates();
  std::string QueryBenchmarkInfo();
  std::string CreateAndUpdateRunInfo();
  std::string UpdateMetricAggregates();
  std::string WriteSampleFile();
  std::string Aggregate();
  std::string Downsample();
  std::string Analyze();
  std::string WriteToStorage();
  std::string UpdateRunInfoTags();
  QuickstoreOutput Complete();

 private:
  mako::Storage* storage_;
  // We currently don't support overriding this. Leaving this as the descendant
  // class type so it is more obvious that this would need more work to support
  // arbitrary mako::Dashboard implementations.
  mako::standard_dashboard::Dashboard dashboard_;
  std::unique_ptr<mako::FileIO> fileio_;
  std::unique_ptr<mako::Aggregator> aggregator_;
  std::unique_ptr<mako::Downsampler> downsampler_;
  const QuickstoreInput& input_;
  const std::vector<mako::SamplePoint>& points_;
  const std::vector<mako::SampleError>& errors_;
  const std::vector<mako::KeyedValue>& run_aggregates_;
  const std::vector<std::string>& aggregate_value_keys_;
  const std::vector<std::string>& aggregate_types_;
  const std::vector<double>& aggregate_values_;
  std::string tmp_dir_;
  mako::BenchmarkInfo benchmark_info_;
  mako::RunInfo run_info_;
  mako::SampleFile sample_file_;
  std::vector<mako::SampleBatch> sample_batches_;
};

}  // namespace internal
}  // namespace quickstore
}  // namespace helpers
}  // namespace mako

#endif  // TESTING_PERFORMANCE_PERFGATE_HELPERS_CXX_QUICKSTORE_INTERNAL_STORE_H_
