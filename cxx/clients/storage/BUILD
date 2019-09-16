# Google3 implementation of mako storage interface.
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

cc_library(
    name = "google3_storage",
    srcs = ["google3_storage.cc"],
    hdrs = ["google3_storage.h"],
    deps = [
        "//cxx/internal:proto_validation",
        "//cxx/internal/storage_client:retry_strategy",
        "//cxx/internal/storage_client:retrying_storage_request",
        "//cxx/internal/storage_client:transport",
        "//cxx/spec:storage",
        "//proto/internal:mako_internal_cc_proto",
        "//spec/proto:mako_cc_proto",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_glog//:glog",
    ],
)

cc_library(
    name = "base_storage",
    hdrs = ["base_storage_client.h"],
    visibility = ["//:internal"],
    deps = [
        ":google3_storage",
    ],
)

cc_library(
    name = "fake_google3_storage",
    srcs = ["fake_google3_storage.cc"],
    hdrs = ["fake_google3_storage.h"],
    deps = [
        "//cxx/internal:proto_validation",
        "//cxx/spec:storage",
        "//spec/proto:mako_cc_proto",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/synchronization",
        "@com_google_glog//:glog",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "fake_google3_storage_test",
    size = "small",
    srcs = ["fake_google3_storage_test.cc"],
    deps = [
        ":fake_google3_storage",
        "//cxx/testing:protocol-buffer-matchers",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "mako_client",
    srcs = ["mako_client.cc"],
    hdrs = ["mako_client.h"],
    deps = [
        ":base_storage",
        ":google3_storage",
        "//cxx/internal/storage_client:google_oauth_fetcher",
        "//cxx/internal/storage_client:http_transport",
        "//helpers/cxx/status:statusor",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "mako_client_internal",
    hdrs = ["mako_client_internal.h"],
    visibility = [
        "//cxx/clients/storage/go:__pkg__",
        "//cxx/clients/storage/python:__pkg__",
    ],
    deps = [
        ":mako_client",
        "//cxx/clients/storage:google3_storage",
        "//helpers/cxx/status",
        "//helpers/cxx/status:statusor",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "mako_client_test",
    srcs = ["mako_client_test.cc"],
    data = [
    ],
    deps = [
        ":google3_storage",
        ":mako_client",
        "//cxx/internal/storage_client:google_oauth_fetcher",
        "//cxx/internal/storage_client:http_transport",
        "//cxx/internal/storage_client:oauth_token_provider",
        "//cxx/internal/storage_client:transport",
        "@com_google_absl//absl/flags:flag",
        "@com_google_googletest//:gtest_main",
    ],
)
