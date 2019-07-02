package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2.0

exports_files(["LICENSE"])

cc_library(
    name = "status",
    srcs = [
        "canonical_errors.cc",
        "status.cc",
    ],
    hdrs = [
        "canonical_errors.h",
        "status.h",
    ],
    deps = [
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/strings",
        "@com_google_cloud_cpp//google/cloud:google_cloud_cpp_common",
    ],
)

cc_library(
    name = "statusor",
    hdrs = ["statusor.h"],
    deps = [
        "@com_google_cloud_cpp//google/cloud:google_cloud_cpp_common",
    ],
)
