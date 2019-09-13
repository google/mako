# Go storage mako library
package(default_visibility = ["//visibility:public"])

licenses(["notice"])

load("@io_bazel_rules_go//go:def.bzl", "go_library")

go_library(
    name = "g3storage",
    srcs = ["google3_storage.go"],
    importpath = "github.com/google/mako/go/clients/storage/g3storage",
    deps = [
        "//cxx/clients/storage/go:g3storage_wrap",
        "//go/internal/wrappedstorage",
    ],
)
