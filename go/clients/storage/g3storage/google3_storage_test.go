package g3storage

import (
	"context"
	"testing"
)

func TestNewGetHostname(t *testing.T) {
	want := "mako.dev"
	if got := New().GetHostname(context.Background()); got != want {
		t.Errorf("GetHostname() got %v; want %v", got, want)
	}
}

func TestNewWithHostnameGetHostname(t *testing.T) {
	want := "example.com"
	if got := NewWithHostname(want).GetHostname(context.Background()); got != want {
		t.Errorf("GetHostname() got %v; want %v", got, want)
	}
}
