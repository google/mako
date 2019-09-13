# Go storage mako library
package(default_visibility = ["//visibility:public"])

licenses(["notice"])

load("@io_bazel_rules_go//go:def.bzl", "go_library")

go_library(
    name = "mako",
    srcs = ["mako.go"],
    importpath = "github.com/google/mako/go/clients/storage/mako",
    deps = [
        "//cxx/clients/storage/go:mako_client_wrap",
        "//go/clients/storage/g3storage",
    ],
)
