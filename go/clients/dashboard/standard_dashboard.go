// Copyright 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// see the license for the specific language governing permissions and
// limitations under the license.

/*
Package standarddashboard provides the standard implementation of the Mako Dashboard interface.
*/
package standarddashboard

import (
	"errors"
	"fmt"
	"net/url"
	"strconv"

	log "github.com/golang/glog"
	"github.com/google/mako/go/spec/mako"
	pgpb "github.com/google/mako/spec/proto/mako_go_proto"
)

const (
	hostURL    = "mako.dev"
	hostScheme = "https"
	noSeriesID = -1
)

type dashboard struct{}

// New returns a ready to use instance
func New() mako.Dashboard {
	return new(dashboard)
}

func dataFilterToQueryParam(dataFilter *pgpb.DataFilter, seriesID int) (string, string, error) {
	seriesStr := ""
	if seriesID != noSeriesID {
		seriesStr = strconv.Itoa(seriesID)
	}
	switch dataFilter.GetDataType() {
	case pgpb.DataFilter_METRIC_AGGREGATE_MIN:
		return seriesStr + "~" + dataFilter.GetValueKey(), "min", nil
	case pgpb.DataFilter_METRIC_AGGREGATE_MAX:
		return seriesStr + "~" + dataFilter.GetValueKey(), "max", nil
	case pgpb.DataFilter_METRIC_AGGREGATE_MEAN:
		return seriesStr + "~" + dataFilter.GetValueKey(), "mean", nil
	case pgpb.DataFilter_METRIC_AGGREGATE_MEDIAN:
		return seriesStr + "~" + dataFilter.GetValueKey(), "median", nil
	case pgpb.DataFilter_METRIC_AGGREGATE_STDDEV:
		return seriesStr + "~" + dataFilter.GetValueKey(), "stddev", nil
	case pgpb.DataFilter_METRIC_AGGREGATE_PERCENTILE:
		return seriesStr + "~" + dataFilter.GetValueKey(), fmt.Sprintf("p%d", dataFilter.GetPercentileMilliRank()), nil
	case pgpb.DataFilter_CUSTOM_AGGREGATE:
		return seriesStr + "~" + dataFilter.GetValueKey(), "1", nil
	case pgpb.DataFilter_BENCHMARK_SCORE:
		if seriesID == noSeriesID {
			return "benchmark-score", "1", nil
		}
		return seriesStr + "s", "1", nil
	case pgpb.DataFilter_ERROR_COUNT:
		if seriesID == noSeriesID {
			return "error-count", "1", nil
		}
		return seriesStr + "e", "1", nil
	default:
		return "", "", fmt.Errorf("unknown DataFilter.data_type: %+v", dataFilter)
	}
}

// AggregateChart generates a link to an aggregate chart.
// See mako/spec/go/dashboard.go for full documentation.
func (d dashboard) AggregateChart(in *pgpb.DashboardAggregateChartInput) (url.URL, error) {

	if in.GetBenchmarkKey() == "" {
		return url.URL{}, errors.New("DashboardAggregateChartInput.benchmark_key empty or missing")
	}
	queryParams := url.Values{}
	queryParams.Add("benchmark_key", in.GetBenchmarkKey())
	for _, dataFilter := range in.GetValueSelections() {
		k, v, err := dataFilterToQueryParam(dataFilter, noSeriesID)
		if err != nil {
			log.Error(err)
			return url.URL{}, err
		}
		queryParams.Add(k, v)
	}
	for _, tag := range in.GetTags() {
		queryParams.Add("tag", tag)
	}
	if in.MinTimestampMs != nil {
		queryParams.Add("tmin", strconv.Itoa(int(in.GetMinTimestampMs())))
	}
	if in.MaxTimestampMs != nil {
		queryParams.Add("tmax", strconv.Itoa(int(in.GetMaxTimestampMs())))
	}
	if in.MaxRuns != nil {
		queryParams.Add("maxruns", strconv.Itoa(int(in.GetMaxRuns())))
	}
	return url.URL{
		Scheme:   hostScheme,
		Host:     hostURL,
		Path:     "benchmark",
		RawQuery: queryParams.Encode()}, nil
}

// RunChart generates a link to a run chart.
// See mako/spec/go/dashboard.go for full documentation.
func (d dashboard) RunChart(in *pgpb.DashboardRunChartInput) (url.URL, error) {
	if in.GetRunKey() == "" {
		return url.URL{}, errors.New("DashboardRunChartInput.run_key empty or missing")
	}
	queryParams := url.Values{}
	queryParams.Add("run_key", in.GetRunKey())
	for _, mKey := range in.GetMetricKeys() {
		queryParams.Add("~"+mKey, "1")
	}
	return url.URL{
		Scheme:   hostScheme,
		Host:     hostURL,
		Path:     "run",
		RawQuery: queryParams.Encode()}, nil
}

// CompareAggregateChart generates a link to a compare aggregate chart, which is an aggregate
// chart showing data across multiple benchamrks.
// See mako/spec/go/dashboard.go for full documentation.
func (d dashboard) CompareAggregateChart(in *pgpb.DashboardCompareAggregateChartInput) (url.URL, error) {
	if len(in.GetSeriesList()) == 0 {
		return url.URL{}, errors.New("DashboardCompareAggregateChartInput.series_list empty")
	}
	queryParams := url.Values{}
	for seriesIdx, series := range in.GetSeriesList() {
		seriesIdxStr := strconv.Itoa(seriesIdx)
		if series.GetSeriesLabel() == "" {
			return url.URL{}, fmt.Errorf("DashboardCompareAggregateChartInput.series_list[%d].series_label empty or missing", seriesIdx)
		}
		if series.GetBenchmarkKey() == "" {
			return url.URL{}, fmt.Errorf("DashboardCompareAggregateChartInput.series_list[%d].benchmark_key empty or missing", seriesIdx)
		}
		if series.ValueSelection == nil {
			return url.URL{}, fmt.Errorf("DashboardCompareAggregateChartInput.series_list[%d].value_selection missing", seriesIdx)
		}
		queryParams.Add(seriesIdxStr+"n", series.GetSeriesLabel())
		queryParams.Add(seriesIdxStr+"b", series.GetBenchmarkKey())
		k, v, err := dataFilterToQueryParam(series.GetValueSelection(), seriesIdx)
		if err != nil {
			err := fmt.Errorf("CompareAggregateChartInput.series_list[%d].value_selection err: %v", seriesIdx, err)
			log.Error(err)
			return url.URL{}, err
		}
		queryParams.Add(k, v)
		for _, tag := range series.GetTags() {
			queryParams.Add(seriesIdxStr+"t", tag)
		}
	}
	if in.MinTimestampMs != nil {
		queryParams.Add("tmin", strconv.Itoa(int(in.GetMinTimestampMs())))
	}
	if in.MaxTimestampMs != nil {
		queryParams.Add("tmax", strconv.Itoa(int(in.GetMaxTimestampMs())))
	}
	if in.MaxRuns != nil {
		queryParams.Add("maxruns", strconv.Itoa(int(in.GetMaxRuns())))
	}
	return url.URL{
		Scheme:   hostScheme,
		Host:     hostURL,
		Path:     "cmpagg",
		RawQuery: queryParams.Encode()}, nil
}

// CompareRunChart generates a link to a compare run chart, which is a run chart showing
// data across multiple runs.
// See mako/spec/go/dashboard.go for full documentation.
func (d dashboard) CompareRunChart(in *pgpb.DashboardCompareRunChartInput) (url.URL, error) {
	if len(in.GetRunKeys()) == 0 {
		return url.URL{}, errors.New("DashboardCompareRunChartInput.run_keys missing")
	}

	queryParams := url.Values{}
	for seriesIdx, runKey := range in.GetRunKeys() {
		queryParams.Add(strconv.Itoa(seriesIdx)+"r", runKey)
	}
	for _, metricKey := range in.GetMetricKeys() {
		queryParams.Add("~"+metricKey, "1")
	}
	return url.URL{
		Scheme:   hostScheme,
		Host:     hostURL,
		Path:     "cmprun",
		RawQuery: queryParams.Encode()}, nil
}

func (d dashboard) VisualizeAnalysis(in *pgpb.DashboardVisualizeAnalysisInput) (url.URL, error) {
	queryParams := url.Values{}
	queryParams.Add("run_key", in.GetRunKey())
	fragment := ""
	if in.GetAnalysisKey() != "" {
		fragment = "analysis" + in.GetAnalysisKey()
	}
	return url.URL{
		Scheme:   hostScheme,
		Host:     hostURL,
		Path:     "analysis-results",
		RawQuery: queryParams.Encode(),
		Fragment: fragment}, nil
}
