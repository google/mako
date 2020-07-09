# C++ Internal storage helpers for mako.
package(default_visibility = ["//:internal"])

licenses(["notice"])

cc_library(
    name = "url",
    srcs = ["url.cc"],
    hdrs = ["url.h"],
    deps = [
        "//cxx/helpers/status",
        "//cxx/helpers/status:statusor",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/types:optional",
        "@com_google_glog//:glog",
        "@com_googlesource_code_re2//:re2",
    ],
)

cc_test(
    name = "url_test",
    srcs = ["url_test.cc"],
    deps = [
        ":url",
        "//cxx/helpers/status",
        "//cxx/helpers/status:status_matchers",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:optional",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "retry_strategy",
    hdrs = ["retry_strategy.h"],
    deps = [
        "//cxx/internal:backoff",
        "//cxx/internal:clock",
        "@com_google_absl//absl/time",
        "@com_google_glog//:glog",
    ],
)

cc_test(
    name = "retry_strategy_test",
    srcs = ["retry_strategy_test.cc"],
    deps = [
        ":retry_strategy",
        "//cxx/internal:clock_mock",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "transport",
    hdrs = ["transport.h"],
    deps = [
        "//cxx/helpers/status",
        "//proto/internal:mako_internal_cc_proto",
        "//proto/internal/storage_client:storage_cc_proto",
        "//spec/proto:mako_cc_proto",
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
        "//cxx/helpers/status",
        "//proto/internal:mako_internal_cc_proto",
        "//spec/proto:mako_cc_proto",
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
        "//cxx/helpers/status",
        "//proto/internal:mako_internal_cc_proto",
        "//proto/internal/storage_client:storage_cc_proto",
        "//spec/proto:mako_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "http_paths",
    srcs = ["http_paths.cc"],
    hdrs = ["http_paths.h"],
    deps = [
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "http_transport",
    srcs = ["http_transport.cc"],
    hdrs = ["http_transport.h"],
    deps = [
        ":http_client",
        ":http_paths",
        ":oauth_token_provider",
        ":transport",
        ":url",
        "//cxx/helpers/status",
        "//cxx/helpers/status:statusor",
        "//proto/internal:mako_internal_cc_proto",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/time",
        "@com_google_glog//:glog",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "google_oauth_fetcher",
    srcs = ["google_oauth_fetcher.cc"],
    hdrs = ["google_oauth_fetcher.h"],
    deps = [
        ":oauth_token_provider",
        "//cxx/helpers/status",
        "//cxx/helpers/status:statusor",
        "@com_github_googleapis_google_cloud_cpp//google/cloud/storage:storage_client",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/synchronization",
        "@com_google_absl//absl/types:optional",
        "@com_google_glog//:glog",
        "@com_googlesource_code_re2//:re2",
    ],
)

cc_test(
    name = "google_oauth_fetcher_test",
    size = "small",
    srcs = ["google_oauth_fetcher_test.cc"],
    deps = [
        ":google_oauth_fetcher",
        "//cxx/helpers/status:status_matchers",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "http_client",
    srcs = ["http_client.cc"],
    hdrs = ["http_client.h"],
    deps = [
        "//cxx/helpers/status",
        "//cxx/helpers/status:statusor",
        "//cxx/internal/utils:cleanup",
        "//cxx/internal/utils:googleinit",
        "//cxx/internal/utils:stringutil",
        "@com_github_curl_curl//:curl",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:optional",
        "@com_google_glog//:glog",
    ],
)

cc_library(
    name = "oauth_token_provider",
    hdrs = ["oauth_token_provider.h"],
    deps = ["//cxx/helpers/status:statusor"],
)
