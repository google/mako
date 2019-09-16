# Quickstore library for C++.
# See quickstore.h and go/mako-quickstore for documentation.
# See quickstore_example_test.cc for example usage.
package(default_visibility = ["//visibility:public"])

licenses(["notice"])

cc_library(
    name = "quickstore",
    srcs = [
        "quickstore.cc",
    ],
    hdrs = [
        "quickstore.h",
    ],
    deps = [
        "//cxx/quickstore/internal:store",
        "//cxx/spec:storage",
        "//proto/quickstore:quickstore_cc_proto",
        "//spec/proto:mako_cc_proto",
        "@com_google_glog//:glog",
    ],
)

cc_library(
    name = "mock_quickstore",
    hdrs = ["mock_quickstore.h"],
    deps = [
        ":quickstore",
        "//proto/quickstore:quickstore_cc_proto",
        "@com_google_googletest//:gtest",
    ],
)

cc_test(
    name = "quickstore_example_test",
    size = "enormous",
    srcs = ["quickstore_example_test.cc"],
    tags = [
        "examples",
        "external",
        "manual",  # so the test isn't included in wildcard queries
        "notap",  # Talks to production Mako servers.
    ],
    deps = [
        ":quickstore",
        "//cxx/clients/storage:base_storage",
        "//cxx/clients/storage:google3_storage",
        "//cxx/clients/storage:mako_client",
        "//proto/clients/analyzers:threshold_analyzer_cc_proto",
        "//proto/clients/analyzers:utest_analyzer_cc_proto",
        "//proto/clients/analyzers:window_deviation_cc_proto",
        "//proto/quickstore:quickstore_cc_proto",
        "//spec/proto:mako_cc_proto",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/time",
        "@com_google_glog//:glog",
        "@com_google_googletest//:gtest_main",
    ],
)
