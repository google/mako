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

package mako

import (
	"net/url"

	pgpb "github.com/google/mako/spec/proto/mako_go_proto"
)

// Dashboard interface used to generated links to Mako charts.
//
// Framework may make any number of calls for each LoadTest/LoadMain execution.
type Dashboard interface {
	// Generates a link to an aggregate chart.
	AggregateChart(*pgpb.DashboardAggregateChartInput) (url.URL, error)

	// Generates a link to a run chart.
	RunChart(*pgpb.DashboardRunChartInput) (url.URL, error)

	// Generates a link to a compare aggregate chart, which is an aggregate
	// chart showing data across multiple benchamrks.
	CompareAggregateChart(*pgpb.DashboardCompareAggregateChartInput) (url.URL, error)

	// Generates a link to a compare run chart, which is a run chart showing
	// data across multiple runs.
	CompareRunChart(*pgpb.DashboardCompareRunChartInput) (url.URL, error)

	// Generates a link to a visualization of regression analysis showing how
	// the analyzer interacts with the run's data (and possibly historical data
	// for the benchmark).
	VisualizeAnalysis(*pgpb.DashboardVisualizeAnalysisInput) (url.URL, error)
}
