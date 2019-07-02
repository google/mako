#ifndef TESTING_PERFORMANCE_PERFGATE_SPEC_CXX_DASHBOARD_H_
#define TESTING_PERFORMANCE_PERFGATE_SPEC_CXX_DASHBOARD_H_

#include <string>

#include "spec/proto/mako.pb.h"

namespace mako {

// Dashboard interface used to generate links to charts.
class Dashboard {
 public:
  // Generates a URL to an aggregate chart.
  // Args:
  //   input: configuration data, see proto for details
  //   link: output URL
  // Returns:
  //   Error details or empty std::string for no error
  virtual std::string AggregateChart(
      const mako::DashboardAggregateChartInput& input, std::string* link) const;

  // Generates a URL to a run chart.
  // Args:
  //   input: configuration data, see proto for details
  //   link: output URL
  // Returns:
  //   Error details or empty std::string for no error
  virtual std::string RunChart(const mako::DashboardRunChartInput& input,
                          std::string* link) const;

  // Generates a URL to a compare aggregate chart, which is an aggregate
  // chart showing data across multiple benchmarks.
  // Args:
  //   input: configuration data, see proto for details
  //   link: output URL
  // Returns:
  //   Error details or empty std::string for no error
  virtual std::string CompareAggregateChart(
      const mako::DashboardCompareAggregateChartInput& input,
      std::string* link) const;

  // Generates a URL to a compare run chart, which is a run chart showing
  // data across multiple runs.
  // Args:
  //   input: configuration data, see proto for details
  //   link: output URL
  // Returns:
  //   Error details or empty std::string for no error
  virtual std::string CompareRunChart(
      const mako::DashboardCompareRunChartInput& input, std::string* link) const;

  // Generates a URL to a visualization of an analyzer execution, which is
  // a place showing how the analyzer operated on the run and any historical
  // runs.
  // Args:
  //   input: configuration data, see proto for details
  //   link: output URL
  // Returns:
  //   Error details or empty std::string for no error
  virtual std::string VisualizeAnalysis(
      const mako::DashboardVisualizeAnalysisInput& input,
      std::string* link) const;

  virtual ~Dashboard() {}
};

}  // namespace mako

#endif  // TESTING_PERFORMANCE_PERFGATE_SPEC_CXX_DASHBOARD_H_
