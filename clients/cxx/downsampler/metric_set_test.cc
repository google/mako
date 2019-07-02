#include "clients/cxx/downsampler/metric_set.h"

#include "gtest/gtest.h"

namespace mako {
namespace downsampler {

TEST(MetricSetTest, ConstructorSampleError) {
  mako::SampleError e;
  e.set_sampler_name("Sampler");
  MetricSet ms(&e);
  EXPECT_EQ(ms.key, "Sampler");
  EXPECT_EQ(ms.slot_count, 1);
}

TEST(MetricSetTest, ConstructorSamplePoint) {
  mako::SamplePoint p;
  mako::KeyedValue* kv = p.add_metric_value_list();
  kv->set_value_key("m3");
  kv = p.add_metric_value_list();
  kv->set_value_key("m1");
  kv = p.add_metric_value_list();
  kv->set_value_key("m2");
  MetricSet ms(&p);
  EXPECT_EQ(ms.key, "m1,m2,m3");
  EXPECT_EQ(ms.slot_count, 3);
}

TEST(MetricSetTest, ConstructorSamplePointDuplicateMetrics) {
  mako::SamplePoint p;
  mako::KeyedValue* kv = p.add_metric_value_list();
  kv->set_value_key("m3");
  kv = p.add_metric_value_list();
  kv->set_value_key("m1");
  kv = p.add_metric_value_list();
  kv->set_value_key("m2");
  kv = p.add_metric_value_list();
  kv->set_value_key("m1");
  MetricSet ms(&p);
  EXPECT_EQ(ms.key, "m1,m1,m2,m3");
  EXPECT_EQ(ms.slot_count, 4);
}

TEST(MetricSetTest, MetricSetEquals) {
  mako::SamplePoint p;
  mako::KeyedValue* kv = p.add_metric_value_list();
  kv->set_value_key("m3");
  EXPECT_EQ(MetricSet(&p), MetricSet(&p));
}

TEST(MetricSetTest, MetricSetNotEquals) {
  mako::SamplePoint p1;
  mako::KeyedValue* kv = p1.add_metric_value_list();
  kv->set_value_key("m3");

  mako::SamplePoint p2;
  kv = p2.add_metric_value_list();
  kv->set_value_key("m2");
  EXPECT_NE(MetricSet(&p1), MetricSet(&p2));
}

TEST(MetricSetTest, HashMetricSet) {
  mako::SamplePoint p;
  mako::KeyedValue* kv = p.add_metric_value_list();
  kv->set_value_key("m3");
  EXPECT_EQ(std::hash<std::string>()("m3"), HashMetricSet()(MetricSet(&p)));
}
}  // namespace downsampler
}  // namespace mako
