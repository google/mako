package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "threshold_analyzer",
    srcs = ["threshold_analyzer.cc"],
    hdrs = ["threshold_analyzer.h"],
    deps = [
        ":util",
        "//cxx/internal:analyzer_common",
        "//cxx/internal:filter_utils",
        "//proto/clients/analyzers:threshold_analyzer_cc_proto",
        "//spec/cxx:analyzer",
        "//spec/proto:mako_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_glog//:glog",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "threshold_analyzer_test",
    size = "small",
    srcs = ["threshold_analyzer_test.cc"],
    deps = [
        ":threshold_analyzer",
        "//cxx/testing:protocol-buffer-matchers",
        "//proto/clients/analyzers:threshold_analyzer_cc_proto",
        "//spec/proto:mako_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "util",
    srcs = ["util.cc"],
    hdrs = ["util.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//spec/proto:mako_cc_proto",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_test(
    name = "util_test",
    srcs = ["util_test.cc"],
    deps = [
        ":util",
        "//spec/proto:mako_cc_proto",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "utest_analyzer",
    srcs = ["utest_analyzer.cc"],
    hdrs = ["utest_analyzer.h"],
    deps = [
        ":util",
        "//cxx/internal:filter_utils",
        "//cxx/internal:pgmath",
        "//proto/clients/analyzers:utest_analyzer_cc_proto",
        "//spec/cxx:analyzer",
        "//spec/proto:mako_cc_proto",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/strings",
        "@com_google_glog//:glog",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "utest_analyzer_test",
    size = "small",
    srcs = ["utest_analyzer_test.cc"],
    deps = [
        ":utest_analyzer",
        "//proto/clients/analyzers:utest_analyzer_cc_proto",
        "//spec/proto:mako_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_glog//:glog",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "window_deviation",
    srcs = ["window_deviation.cc"],
    hdrs = ["window_deviation.h"],
    deps = [
        ":util",
        "//cxx/internal:analyzer_common",
        "//cxx/internal:filter_utils",
        "//cxx/internal:pgmath",
        "//helpers/cxx/status",
        "//proto/clients/analyzers:window_deviation_cc_proto",
        "//spec/cxx:analyzer",
        "//spec/proto:mako_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:span",
        "@com_google_glog//:glog",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "window_deviation_test",
    size = "small",
    srcs = ["window_deviation_test.cc"],
    deps = [
        ":window_deviation",
        "//cxx/testing:protocol-buffer-matchers",
        "//proto/clients/analyzers:window_deviation_cc_proto",
        "//spec/proto:mako_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_glog//:glog",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)
