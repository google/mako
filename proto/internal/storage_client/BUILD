load("@io_bazel_rules_go//proto:def.bzl", "go_proto_library")

package(default_visibility = ["//:internal"])

licenses(["notice"])

proto_library(
    name = "storage",
    srcs = ["storage.proto"],
    deps = [
        "//spec/proto:mako_proto",
    ],
)

cc_proto_library(
    name = "storage_cc_proto",
    deps = [":storage"],
)

go_proto_library(
    name = "storage_go_proto",
    importpath = "github.com/google/mako/proto/internal/storage_client/storage_go_proto",
    proto = ":storage",
    deps = ["//spec/proto:mako_go_proto"],
)
