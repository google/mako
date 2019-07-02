#ifndef TESTING_PERFORMANCE_PERFGATE_CLIENTS_CXX_DASHBOARD_STANDARD_DASHBOARD_H_
#define TESTING_PERFORMANCE_PERFGATE_CLIENTS_CXX_DASHBOARD_STANDARD_DASHBOARD_H_

#include <string>

#include "spec/cxx/dashboard.h"
#include "spec/proto/mako.pb.h"
#include "absl/strings/string_view.h"

namespace mako {
namespace standard_dashboard {

// Dashboard implementation for google3. See interface for docs.
class Dashboard : public mako::Dashboard {
 public:
  Dashboard();
  // Provided hostname must not have a trailing slash.
  // TODO(b/128004122): Use a URL type to make comparisons smarter and less
  // error prone.
  explicit Dashboard(absl::string_view hostname);

  std::string AggregateChart(
      const mako::DashboardAggregateChartInput& input,
      std::string* link) const override;

  std::string RunChart(
      const mako::DashboardRunChartInput& input,
      std::string* link) const override;

  std::string CompareAggregateChart(
      const mako::DashboardCompareAggregateChartInput& input,
      std::string* link) const override;

  std::string CompareRunChart(
      const mako::DashboardCompareRunChartInput& input,
      std::string* link) const override;

  std::string VisualizeAnalysis(
      const mako::DashboardVisualizeAnalysisInput& input,
      std::string* link) const override;

 private:
  std::string hostname_;
};

}  // namespace standard_dashboard
}  // namespace mako

#endif  // TESTING_PERFORMANCE_PERFGATE_CLIENTS_CXX_DASHBOARD_STANDARD_DASHBOARD_H_
