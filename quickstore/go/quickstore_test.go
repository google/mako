package quickstore

import (
	"context"
	"errors"
	"strings"
	"testing"

	"github.com/golang/protobuf/proto"
	"github.com/google/mako/clients/go/storage/fakestorage"
	qpb "github.com/google/mako/quickstore/quickstore_go_proto"
	pgpb "github.com/google/mako/spec/proto/mako_go_proto"
)

type fakeSaver struct {
	// Copy of what was passed into Save()
	Input              qpb.QuickstoreInput
	SamplePoints       []*pgpb.SamplePoint
	SampleErrors       []*pgpb.SampleError
	RunAggregates      []*pgpb.KeyedValue
	MetricAggValueKeys []string
	MetricAggTypes     []string
	MetricAggValues    []float64

	// What will be returned from Save()
	SaveOutput qpb.QuickstoreOutput
	SaveErr    error
}

func (f *fakeSaver) Save(input *qpb.QuickstoreInput,
	samplePoints []*pgpb.SamplePoint,
	sampleErrors []*pgpb.SampleError,
	runAggregates []*pgpb.KeyedValue,
	metricAggValueKeys []string,
	metricAggTypes []string,
	metricAggValues []float64) (qpb.QuickstoreOutput, error) {

	f.Input = *input
	f.SamplePoints = samplePoints
	f.SampleErrors = sampleErrors
	f.RunAggregates = runAggregates
	f.MetricAggValueKeys = metricAggValueKeys
	f.MetricAggTypes = metricAggTypes
	f.MetricAggValues = metricAggValues
	return f.SaveOutput, f.SaveErr
}

func newQuickstore() (*Quickstore, *fakeSaver) {
	q := &Quickstore{}
	f := &fakeSaver{}
	q.saverImpl = f
	return q, f
}

func TestFatalError(t *testing.T) {
	expectedError := errors.New("something went wrong")
	q, f := newQuickstore()
	f.SaveErr = expectedError
	if _, err := q.Store(); err != expectedError {
		t.Errorf("got err: %v; want: %v", err, expectedError)
	}
}

func TestAnalysisFail(t *testing.T) {
	out := qpb.QuickstoreOutput{Status: qpb.QuickstoreOutput_ANALYSIS_FAIL.Enum()}
	q, f := newQuickstore()
	f.SaveOutput = out
	out, err := q.Store()
	if err == nil {
		t.Errorf("want err")
	}
	if out.GetStatus() != qpb.QuickstoreOutput_ANALYSIS_FAIL {
		t.Errorf("got status: %s; want ANALYSIS_FAIL", out.GetStatus())
	}
}

func TestData(t *testing.T) {
	q, f := newQuickstore()

	q.BenchmarkKey = "abc"

	// Add data
	err := q.AddSamplePoint(12345, map[string]float64{"m1": 1})
	if err != nil {
		t.Errorf("AddSamplePoint err: %v", err)
	}
	err = q.AddError(67890, "Some error")
	if err != nil {
		t.Errorf("AddError err: %v", err)
	}
	err = q.AddRunAggregate("c1", 3)
	if err != nil {
		t.Errorf("AddRunAggregate err: %v", err)
	}
	err = q.AddMetricAggregate("m31", "m32", 100)
	if err != nil {
		t.Errorf("AddMetricAggregat err: %v", err)
	}

	// Store
	if _, err := q.Store(); err != nil {
		t.Fatalf("Store err: %v", err)
	}

	// Validate QuickstoreInput
	if i := f.Input.GetBenchmarkKey(); "abc" != i {
		t.Errorf("QuickstoreInput.BenchmarkKey: %s; want 'abc'", i)
	}

	// Validate sample point
	if l := len(f.SamplePoints); l != 1 {
		t.Fatalf("SamplePoints: %d; want 1", l)
	}
	if i := f.SamplePoints[0].GetInputValue(); 12345 != i {
		t.Errorf("SamplePoint.InputValue: %d; want 12345", i)
	}
	if l := len(f.SamplePoints[0].MetricValueList); l != 1 {
		t.Fatalf("SamplePoints.MetricValueList: %d; want 1", l)
	}
	if i := f.SamplePoints[0].MetricValueList[0].GetValueKey(); i != "m1" {
		t.Errorf("SamplePoints.MetricValueList.ValueKey: %s; want 'm1'", i)
	}

	// Validate sample error
	if l := len(f.SampleErrors); l != 1 {
		t.Fatalf("SampleErrors: %d; want 1", l)
	}
	if i := f.SampleErrors[0].GetInputValue(); 67890 != i {
		t.Errorf("SampleErrors.InputValue: %d; want 67890", i)
	}
	if i := f.SampleErrors[0].GetErrorMessage(); "Some error" != i {
		t.Errorf("SampleErrors.InputValue: %d; want 'Some error'", i)
	}

	// Validate run aggregate
	if l := len(f.RunAggregates); l != 1 {
		t.Fatalf("RunAggregates: %d; want 1", l)
	}
	if i := f.RunAggregates[0].GetValueKey(); "c1" != i {
		t.Errorf("RunAggregates.ValueKey: %s; want 'c1'", i)
	}

	// Validate metric aggregates
	if l := len(f.MetricAggValueKeys); l != 1 {
		t.Fatalf("MetricAggValueKeys: %d; want 1", l)
	}
	if i := f.MetricAggValueKeys[0]; "m31" != i {
		t.Errorf("MetricAggValueKeys[0]: %s; want 'm31'", i)
	}
	if l := len(f.MetricAggTypes); l != 1 {
		t.Fatalf("MetricAggTypes: %d; want 1", l)
	}
	if i := f.MetricAggTypes[0]; "m32" != i {
		t.Errorf("MetricAggTypes[0]: %s; want 'm32'", i)
	}
	if l := len(f.MetricAggValues); l != 1 {
		t.Fatalf("MetricAggValues: %d; want 1", l)
	}
	if i := f.MetricAggValues[0]; 100 != i {
		t.Errorf("MetricAggValues[0]: %f; want 100", i)
	}
}

// When providing no benchmark key the SWIG call will fail before any production
// servers are contacted. This validates that SWIG library can be called with the
// correct args.
func TestNoFakeSaver(t *testing.T) {
	q := Quickstore{}
	err := q.AddSamplePoint(123, map[string]float64{"m1": 1})
	if err != nil {
		t.Fatalf("AddSamplePoint err: %v", err)
	}
	out, err := q.Store()
	if err == nil {
		t.Fatal("want Store() err")
	}
	if s := out.GetStatus(); s != qpb.QuickstoreOutput_ERROR {
		t.Errorf("got status: %s; want ERROR", s)
	}
}

func TestBadQuickstoreInput(t *testing.T) {
	_, err := New(&qpb.QuickstoreInput{})
	if err == nil || !strings.Contains(err.Error(), "benchmark_key") {
		t.Fatalf("got New()=_,%v, want err with 'benchmark_key'", err)
	}
}

func TestCreateRun(t *testing.T) {
	s := fakestorage.New()
	s.FakeClear()
	s.FakeStageBenchmarks([]*pgpb.BenchmarkInfo{
		{
			BenchmarkKey:  proto.String("abcde"),
			BenchmarkName: proto.String("b"),
			ProjectName:   proto.String("p"),
			OwnerList:     []string{"o"},
			InputValueInfo: &pgpb.ValueInfo{
				ValueKey: proto.String("t"),
				Label:    proto.String("time"),
			},
		},
	})
	input := &qpb.QuickstoreInput{BenchmarkKey: proto.String("abcde"), Tags: []string{"atag"}}
	qs, err := NewWithStorage(input, s)
	if err != nil {
		t.Fatalf("NewWithStorage(input=%+v) failed with %v", input, err)
	}

	if err := qs.AddSamplePoint(0, map[string]float64{"a": 123}); err != nil {
		t.Fatalf("AddSamplePoint error: %v", err)
	}
	if _, err := qs.Store(); err != nil {
		t.Fatalf("Store error: %v", err)
	}
	resp, err := s.QueryRunInfo(context.Background(), &pgpb.RunInfoQuery{BenchmarkKey: proto.String("abcde")})
	if err != nil {
		t.Fatalf("QueryRunInfo error: %v", err)
	}
	if len(resp.GetRunInfoList()) != 1 || len(resp.GetRunInfoList()[0].GetTags()) != 1 || resp.GetRunInfoList()[0].GetTags()[0] != "atag" {
		t.Fatalf("Got %+v, expected response with RunInfoList with a single run with tag \"atag\"", resp)
	}
}
