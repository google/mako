load("@io_bazel_rules_go//proto:def.bzl", "go_proto_library")

package(default_visibility = ["//:internal"])

proto_library(
    name = "quickstore_proto",
    srcs = ["quickstore.proto"],
    deps = [
        "//helpers/proto/quickstore:quickstore_proto",
        "//spec/proto:mako_proto",
    ],
)

go_proto_library(
    name = "quickstore_go_proto",
    compilers = ["@io_bazel_rules_go//proto:go_grpc"],
    importpath = "github.com/google/mako/internal/quickstore_microservice/proto/quickstore_go_proto",
    proto = ":quickstore_proto",
    deps = [
        "//helpers/proto/quickstore:quickstore_go_proto",
        "//spec/proto:mako_go_proto",
    ],
)

go_proto_library(
    name = "quickstore_go_grpc_pb",
    compilers = ["@io_bazel_rules_go//proto:go_grpc"],
    importpath = "github.com/google/mako/internal/quickstore_microservice/proto/quickstore_go_grpc_pb",
    proto = ":quickstore_proto",
    deps = [
        "//helpers/proto/quickstore:quickstore_go_proto",
        "//spec/proto:mako_go_proto",
    ],
)
