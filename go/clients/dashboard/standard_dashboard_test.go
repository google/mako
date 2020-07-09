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
package standarddashboard

import (
	"testing"

	"github.com/golang/protobuf/proto"
	pgpb "github.com/google/mako/spec/proto/mako_go_proto"
)

func TestAggregateMissingBenchmarkKey(t *testing.T) {
	if _, err := New().AggregateChart(&pgpb.DashboardAggregateChartInput{}); err == nil {
		t.Error("AggregateChart() expected err; got nil")
	}
}

func TestAggregateChartSimple(t *testing.T) {
	in := &pgpb.DashboardAggregateChartInput{BenchmarkKey: proto.String("123")}
	url, err := New().AggregateChart(in)
	if err != nil {
		t.Fatalf("AggregateChart(%v) err: %v", in, err)
	}
	want := "https://mako.dev/benchmark?benchmark_key=123"
	if u := url.String(); u != want {
		// Add newlines before strings for easy visual comparison
		t.Errorf("AggregateChart(%v) got\n%q; want\n%q", in, u, want)
	}
}

func TestAggregateChartComplex(t *testing.T) {
	_ = &pgpb.DataFilter{DataType: pgpb.DataFilter_METRIC_AGGREGATE_MIN.Enum(), ValueKey: proto.String("y")}
	in := &pgpb.DashboardAggregateChartInput{
		BenchmarkKey: proto.String("123"),
		Tags:         []string{"a=b"},
		ValueSelections: []*pgpb.DataFilter{
			{DataType: pgpb.DataFilter_METRIC_AGGREGATE_MIN.Enum(), ValueKey: proto.String("y")},
			{DataType: pgpb.DataFilter_METRIC_AGGREGATE_MAX.Enum(), ValueKey: proto.String("y")},
			{DataType: pgpb.DataFilter_METRIC_AGGREGATE_MEAN.Enum(), ValueKey: proto.String("y")},
			{DataType: pgpb.DataFilter_METRIC_AGGREGATE_MEDIAN.Enum(), ValueKey: proto.String("y")},
			{DataType: pgpb.DataFilter_METRIC_AGGREGATE_STDDEV.Enum(), ValueKey: proto.String("y")},
			{DataType: pgpb.DataFilter_METRIC_AGGREGATE_PERCENTILE.Enum(), ValueKey: proto.String("y"), PercentileMilliRank: proto.Int32(1000)},
			{DataType: pgpb.DataFilter_CUSTOM_AGGREGATE.Enum(), ValueKey: proto.String("c")},
			{DataType: pgpb.DataFilter_BENCHMARK_SCORE.Enum()},
			{DataType: pgpb.DataFilter_ERROR_COUNT.Enum()},
		},
		MinTimestampMs: proto.Float64(9999999999999.9),
		MaxTimestampMs: proto.Float64(5555555555555.5),
		MaxRuns:        proto.Int32(7),
	}
	url, err := New().AggregateChart(in)
	if err != nil {
		t.Fatalf("AggregateChart(%v) err: %v", in, err)
	}
	// Query params are sorted by key
	want := "https://mako.dev/benchmark?" +
		"benchmark-score=1" +
		"&benchmark_key=123" +
		"&error-count=1" +
		"&maxruns=7" +
		"&tag=a%3Db" +
		"&tmax=5555555555555" +
		"&tmin=9999999999999" +
		"&~c=1" +
		"&~y=min" +
		"&~y=max" +
		"&~y=mean" +
		"&~y=median" +
		"&~y=stddev" +
		"&~y=p1000"
	if u := url.String(); u != want {
		// Add newlines before strings for easy visual comparison
		t.Errorf("AggregateChart(%v) got\n%q; want\n%q", in, u, want)
	}
}

func TestRunChartMissingRunKey(t *testing.T) {
	if _, err := New().RunChart(&pgpb.DashboardRunChartInput{}); err == nil {
		t.Error("RunChart() expected err; got nil")
	}
}

func TestRunChartSimple(t *testing.T) {
	in := &pgpb.DashboardRunChartInput{RunKey: proto.String("123")}
	url, err := New().RunChart(in)
	if err != nil {
		t.Fatalf("RunChart(%v) err: %v", in, err)
	}
	want := "https://mako.dev/run?run_key=123"
	if u := url.String(); u != want {
		// Add newlines before strings for easy visual comparison
		t.Errorf("RunChart(%v) got\n%q; want\n%q", in, u, want)
	}
}

func TestRunChartComplex(t *testing.T) {
	in := &pgpb.DashboardRunChartInput{
		RunKey:     proto.String("123"),
		MetricKeys: []string{"a", "b"},
	}
	url, err := New().RunChart(in)
	if err != nil {
		t.Fatalf("RunChart(%v) err: %v", in, err)
	}
	want := "https://mako.dev/run?run_key=123&~a=1&~b=1"
	if u := url.String(); u != want {
		t.Errorf("RunChart(%v) got %q; want %q", in, u, want)
	}
}

func TestCompareAggregateChartMissingSeries(t *testing.T) {
	if _, err := New().CompareAggregateChart(&pgpb.DashboardCompareAggregateChartInput{}); err == nil {
		t.Error("CompareAggregateChart() expected err; got nil")
	}
}

func TestCompareAggregateChartInvalidSeries(t *testing.T) {
	validSeries := &pgpb.DashboardCompareAggregateChartInput_Series{
		SeriesLabel: proto.String("yL"), BenchmarkKey: proto.String("123"),
		ValueSelection: &pgpb.DataFilter{
			DataType: pgpb.DataFilter_METRIC_AGGREGATE_MEAN.Enum(),
			ValueKey: proto.String("y"),
		},
	}

	// Validate no errors when use valid series
	in := &pgpb.DashboardCompareAggregateChartInput{
		SeriesList: []*pgpb.DashboardCompareAggregateChartInput_Series{validSeries},
	}
	if _, err := New().CompareAggregateChart(in); err != nil {
		t.Errorf("CompareAggregateChart(%v) err: %v", in, err)
	}

	// When missing Label, error occurs
	cp := proto.Clone(validSeries).(*pgpb.DashboardCompareAggregateChartInput_Series)
	cp.SeriesLabel = nil
	in = &pgpb.DashboardCompareAggregateChartInput{
		SeriesList: []*pgpb.DashboardCompareAggregateChartInput_Series{cp},
	}
	if _, err := New().CompareAggregateChart(in); err == nil {
		t.Errorf("CompareAggregateChart(%v) expected error", in)
	}

	// When missing benchmark_key, error occurs
	cp = proto.Clone(validSeries).(*pgpb.DashboardCompareAggregateChartInput_Series)
	cp.BenchmarkKey = nil
	in = &pgpb.DashboardCompareAggregateChartInput{
		SeriesList: []*pgpb.DashboardCompareAggregateChartInput_Series{cp},
	}
	if _, err := New().CompareAggregateChart(in); err == nil {
		t.Errorf("CompareAggregateChart(%v) expected error", in)
	}

	// When missing value_selection, error occurs
	cp = proto.Clone(validSeries).(*pgpb.DashboardCompareAggregateChartInput_Series)
	cp.ValueSelection = nil
	in = &pgpb.DashboardCompareAggregateChartInput{
		SeriesList: []*pgpb.DashboardCompareAggregateChartInput_Series{cp},
	}
	if _, err := New().CompareAggregateChart(in); err == nil {
		t.Errorf("CompareAggregateChart(%v) expected error", in)
	}
}

func TestCompareAggregateChart(t *testing.T) {
	in := &pgpb.DashboardCompareAggregateChartInput{
		SeriesList: []*pgpb.DashboardCompareAggregateChartInput_Series{
			{SeriesLabel: proto.String("yL"), BenchmarkKey: proto.String("123"),
				ValueSelection: &pgpb.DataFilter{
					DataType: pgpb.DataFilter_METRIC_AGGREGATE_MEAN.Enum(), ValueKey: proto.String("y")},
				Tags: []string{"a=b"},
			},
			{SeriesLabel: proto.String("score"), BenchmarkKey: proto.String("456"),
				ValueSelection: &pgpb.DataFilter{
					DataType: pgpb.DataFilter_BENCHMARK_SCORE.Enum()}},
			{SeriesLabel: proto.String("err"), BenchmarkKey: proto.String("456"),
				ValueSelection: &pgpb.DataFilter{DataType: pgpb.DataFilter_ERROR_COUNT.Enum()}},
		},
		MinTimestampMs: proto.Float64(9999999999999.9),
		MaxTimestampMs: proto.Float64(5555555555555.5),
		MaxRuns:        proto.Int32(7),
	}
	url, err := New().CompareAggregateChart(in)
	if err != nil {
		t.Fatalf("CompareAggregateChart(%v) err: %v", in, err)
	}
	// Query params are sorted by key
	want := "https://mako.dev/cmpagg?" +
		"0b=123" +
		"&0n=yL" +
		"&0t=a%3Db" +
		"&0~y=mean" +
		"&1b=456" +
		"&1n=score" +
		"&1s=1" +
		"&2b=456" +
		"&2e=1" +
		"&2n=err" +
		"&maxruns=7" +
		"&tmax=5555555555555" +
		"&tmin=9999999999999"
	if u := url.String(); u != want {
		// Add newlines before strings for easy visual comparison
		t.Errorf("CompareAggregateChart(%v) got\n%q; want\n%q", in, u, want)
	}
}

func TestCompareRunChartMissingRunKey(t *testing.T) {
	if _, err := New().CompareRunChart(&pgpb.DashboardCompareRunChartInput{}); err == nil {
		t.Error("CompareRunChart() expected err; got nil")
	}
}

func TestCompareRunChartComplex(t *testing.T) {
	in := &pgpb.DashboardCompareRunChartInput{
		RunKeys:    []string{"123"},
		MetricKeys: []string{"y"},
	}
	url, err := New().CompareRunChart(in)
	if err != nil {
		t.Fatalf("CompareRunChart(%v) err: %v", in, err)
	}
	want := "https://mako.dev/cmprun?0r=123&~y=1"
	if u := url.String(); u != want {
		// Add newlines before strings for easy visual comparison
		t.Errorf("CompareRunChart(%v) got\n%q; want\n%q", in, u, want)
	}
}

func TestVisualizeAnalysis(t *testing.T) {
	in := &pgpb.DashboardVisualizeAnalysisInput{
		RunKey: proto.String("test_run_key"),
	}
	url, err := New().VisualizeAnalysis(in)
	if err != nil {
		t.Fatalf("VisualizeAnalysis(%v) err: %v", in, err)
	}
	want := "https://mako.dev/analysis-results?run_key=test_run_key"
	if u := url.String(); u != want {
		// Add newlines before strings for easy visual comparison
		t.Errorf("VisualizeAnalysis(%v) got\n%q; want\n%q", in, u, want)
	}
}

func TestVisualizeAnalysisWithAnalysisKey(t *testing.T) {
	in := &pgpb.DashboardVisualizeAnalysisInput{
		RunKey:      proto.String("test_run_key"),
		AnalysisKey: proto.String("test_analysis_key"),
	}
	url, err := New().VisualizeAnalysis(in)
	if err != nil {
		t.Fatalf("VisualizeAnalysis(%v) err: %v", in, err)
	}
	want := "https://mako.dev/analysis-results?run_key=test_run_key#analysistest_analysis_key"
	if u := url.String(); u != want {
		// Add newlines before strings for easy visual comparison
		t.Errorf("VisualizeAnalysis(%v) got\n%q; want\n%q", in, u, want)
	}
}
