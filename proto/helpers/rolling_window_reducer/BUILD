# RollingWindowReducer proto definitions.

package(default_visibility = ["//visibility:public"])

load("@io_bazel_rules_go//proto:def.bzl", "go_proto_library")

licenses(["notice"])

proto_library(
    name = "rolling_window_reducer_proto",
    srcs = ["rolling_window_reducer.proto"],
    deps = ["//spec/proto:mako_proto"],
)

cc_proto_library(
    name = "rolling_window_reducer_cc_proto",
    deps = [":rolling_window_reducer_proto"],
)

go_proto_library(
    name = "rolling_window_reducer_go_proto",
    importpath = "github.com/google/mako/proto/helpers/rolling_window_reducer/rolling_window_reducer_go_proto",
    proto = ":rolling_window_reducer_proto",
    deps = ["//spec/proto:mako_go_proto"],
)
