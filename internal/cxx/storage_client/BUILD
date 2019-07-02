# C++ Internal storage helpers for mako.
package(default_visibility = ["//:internal"])

cc_library(
    name = "retry_strategy",
    hdrs = ["retry_strategy.h"],
    deps = [
        "//internal/cxx:backoff",
        "//internal/cxx:clock",
        "@com_google_absl//absl/time",
        "@com_google_glog//:glog",
    ],
)

cc_test(
    name = "retry_strategy_test",
    srcs = ["retry_strategy_test.cc"],
    deps = [
        ":retry_strategy",
        "//internal/cxx:clock_mock",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "transport",
    hdrs = ["transport.h"],
    deps = [
        "//helpers/cxx/status",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "retrying_storage_request",
    hdrs = ["retrying_storage_request.h"],
    deps = [
        ":retry_strategy",
        ":transport",
        "//helpers/cxx/status",
        "//internal/proto:mako_internal_cc_proto",
        "//spec/proto:mako_cc_proto",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_glog//:glog",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "mock_transport",
    testonly = 1,
    hdrs = ["mock_transport.h"],
    deps = [
        ":transport",
        "//helpers/cxx/status",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest",
        "@com_google_protobuf//:protobuf",
    ],
)
