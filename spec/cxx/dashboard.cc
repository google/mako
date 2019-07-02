#include "spec/cxx/dashboard.h"

namespace mako {

namespace {
constexpr char kNeedsImplementedStr[] =
    "This dashboard method is not supported by this implementation.";
}  // namespace

std::string Dashboard::AggregateChart(
    const mako::DashboardAggregateChartInput& input, std::string* link) const {
  return kNeedsImplementedStr;
}
std::string Dashboard::RunChart(const mako::DashboardRunChartInput& input,
                           std::string* link) const {
  return kNeedsImplementedStr;
}
std::string Dashboard::CompareAggregateChart(
    const mako::DashboardCompareAggregateChartInput& input,
    std::string* link) const {
  return kNeedsImplementedStr;
}
std::string Dashboard::CompareRunChart(
    const mako::DashboardCompareRunChartInput& input, std::string* link) const {
  return kNeedsImplementedStr;
}
std::string Dashboard::VisualizeAnalysis(
    const mako ::DashboardVisualizeAnalysisInput& input,
    std::string* link) const {
  return kNeedsImplementedStr;
}

}  // namespace mako
