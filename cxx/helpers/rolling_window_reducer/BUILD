# RollingWindowReducer

package(default_visibility = ["//visibility:public"])

licenses(["notice"])

cc_library(
    name = "rolling_window_reducer",
    srcs = ["rolling_window_reducer.cc"],
    hdrs = ["rolling_window_reducer.h"],
    deps = [
        "//helpers/cxx/status",
        "//helpers/cxx/status:statusor",
        "//internal/cxx:pgmath",
        "//internal/cxx/utils:cleanup",
        "//proto/helpers/rolling_window_reducer:rolling_window_reducer_cc_proto",
        "//spec/cxx:fileio",
        "//spec/proto:mako_cc_proto",
        "@com_google_absl//absl/container:node_hash_map",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/types:span",
        "@com_google_glog//:glog",
        "@com_google_protobuf//:protobuf",
        "@com_googlesource_code_re2//:re2",
    ],
)

cc_library(
    name = "rolling_window_reducer_internal",
    srcs = ["rolling_window_reducer_internal.cc"],
    hdrs = ["rolling_window_reducer_internal.h"],
    visibility = ["//visibility:private"],
    deps = [
        ":rolling_window_reducer",
        "//helpers/cxx/status",
        "//helpers/cxx/status:statusor",
        "//internal/cxx/utils:cleanup",
        "//proto/helpers/rolling_window_reducer:rolling_window_reducer_cc_proto",
        "//spec/cxx:fileio",
        "//spec/proto:mako_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_test(
    name = "rolling_window_reducer_test",
    size = "small",
    srcs = ["rolling_window_reducer_test.cc"],
    deps = [
        ":rolling_window_reducer",
        ":rolling_window_reducer_internal",
        "//clients/cxx/fileio:memory_fileio",
        "//helpers/cxx/status",
        "//helpers/cxx/status:status_matchers",
        "//proto/helpers/rolling_window_reducer:rolling_window_reducer_cc_proto",
        "//spec/proto:mako_cc_proto",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_glog//:glog",
        "@com_google_googletest//:gtest_main",
    ],
)
