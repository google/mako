package mako

import (
	"context"
	"net/http"
	"net/http/httptest"
	"testing"

	"google3/base/go/gflag"
	"github.com/golang/protobuf/proto"
	pgpb "github.com/google/mako/spec/proto/mako_go_proto"
)

func TestBadHostname(t *testing.T) {
	mux := http.NewServeMux()

	mux.HandleFunc("/storage/benchmark-info/create", func(w http.ResponseWriter, r *http.Request) {
		response := &pgpb.CreationResponse{Status: &pgpb.Status{Code: pgpb.Status_SUCCESS.Enum()}, Key: proto.String("ABCDE")}
		data, _ := proto.Marshal(response)
		w.Write(data)
	})
	s := httptest.NewServer(mux)
	defer s.Close()

	// TODO(b/124472003): Remove this when we fix our HTTP Client's handling of Expect: 100-continue.
	gflag.Set("mako_internal_disable_expect_100_continue", "false")

	gflag.Set("mako_auth", "false")
	m, err := NewWithHostname(s.URL)
	if err != nil {
		t.Fatalf("NewWithHostname returned unexpected error: %v", err)
	}

	resp, err := m.CreateBenchmarkInfo(context.Background(), &pgpb.BenchmarkInfo{})
	if err != nil {
		t.Fatalf("Unexpected error from CreateBenchmarkInfo call: %v", err)
	}
	if resp.GetKey() != "ABCDE" {
		t.Fatalf("Got CreationResponse (%+v), expected key=ABCDE", resp)
	}
}

func TestGetHostname(t *testing.T) {
	m, err := New()
	if err != nil {
		t.Fatalf("New returned unexpected error: %v", err)
	}
	want := "makoperf.appspot.com"
	if got := m.GetHostname(context.Background()); got != want {
		t.Errorf("mako.GetHostname() got %v; want %v", got, want)
	}
}
