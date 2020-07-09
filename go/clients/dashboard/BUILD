# Go dashboard mako library
package(default_visibility = ["//visibility:public"])

licenses(["notice"])

load("@io_bazel_rules_go//go:def.bzl", "go_library", "go_test")

go_library(
    name = "standarddashboard",
    srcs = ["standard_dashboard.go"],
    importpath = "github.com/google/mako/go/clients/dashboard/standarddashboard",
    deps = [
        "//go/spec:mako",
        "//spec/proto:mako_go_proto",
        "@com_github_golang_glog//:go_default_library",
    ],
)

go_test(
    name = "standarddashboard_test",
    size = "small",
    srcs = ["standard_dashboard_test.go"],
    embed = [":standarddashboard"],
    importpath = "github.com/google/mako/go/clients/dashboard/standarddashboard",
    deps = [
        "//spec/proto:mako_go_proto",
        "@com_github_golang_protobuf//proto:go_default_library",
    ],
)
