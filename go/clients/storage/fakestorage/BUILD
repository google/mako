# Go storage mako library
package(default_visibility = ["//visibility:public"])

licenses(["notice"])

load("@io_bazel_rules_go//go:def.bzl", "go_library", "go_test")

go_library(
    name = "fakestorage",
    srcs = ["fake_storage.go"],
    importpath = "github.com/google/mako/go/clients/storage/fakestorage",
    deps = [
        "//cxx/clients/storage/go:fakestorage_wrap",
        "//go/internal/wrappedstorage",
        "//spec/proto:mako_go_proto",
        "@com_github_golang_protobuf//proto:go_default_library",
    ],
)

go_test(
    name = "fakestorage_test",
    srcs = ["fake_storage_test.go"],
    embed = [":fakestorage"],
    importpath = "github.com/google/mako/go/clients/storage/fakestorage_test",
    deps = [
        "//spec/go:mako",
        "//spec/proto:mako_go_proto",
        "@com_github_golang_protobuf//proto:go_default_library",
    ],
)
