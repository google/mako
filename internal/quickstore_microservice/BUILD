load("@io_bazel_rules_docker//cc:image.bzl", "cc_image")

package(default_visibility = ["//:internal"])

licenses(["notice"])

cc_library(
    name = "quickstore_service",
    srcs = ["quickstore_service.cc"],
    hdrs = ["quickstore_service.h"],
    deps = [
        "//clients/cxx/storage:mako_client",
        "//helpers/cxx/quickstore/internal:store",
        "//helpers/cxx/status:statusor",
        "//internal/cxx:queue_ifc",
        "//internal/proto:mako_internal_cc_proto",
        "//internal/quickstore_microservice/proto:quickstore_cc_grpc_proto",
        "//internal/quickstore_microservice/proto:quickstore_cc_proto",
        "//spec/cxx:storage",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "quickstore_service_test",
    srcs = ["quickstore_service_test.cc"],
    linkstatic = 1,  # TODO(b/138956597) Fix linker error requiring this.
    deps = [
        ":quickstore_service",
        "//clients/cxx/storage:fake_google3_storage",
        "//helpers/proto/quickstore:quickstore_cc_proto",
        "//internal/cxx:queue",
        "//internal/grpc:grpc++_gce",
        "//internal/quickstore_microservice/proto:quickstore_cc_proto",
        "//spec/proto:mako_cc_proto",
        "//testing/cxx:protocol-buffer-matchers",
        "@com_google_absl//absl/memory",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_binary(
    name = "quickstore_microservice_mako",
    srcs = ["main.cc"],
    deps = [
        ":quickstore_service",
        "//internal/cxx:queue",
        "//internal/grpc:grpc++_gce",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/strings",
        "@com_google_glog//:glog",
    ],
)

# See https://github.com/google/mako/blob/master/docs/BUILDING.md#microservice-docker-image
cc_image(
    name = "quickstore_microservice_mako_image",
    args = ["--addr=0.0.0.0:9813"],
    binary = ":quickstore_microservice_mako",
)
