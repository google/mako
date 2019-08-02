# GCE-compatible alternatives to the top-level packages in //net/grpc.
#
# TODO(b/138388186): Deprecate these on move onto //net/grpc and //third_party/grpc.
package(default_visibility = ["//:internal"])

licenses(["notice"])

cc_library(
    name = "grpc_gce",
    deps = ["@com_github_grpc_grpc//:grpc"],
    alwayslink = 1,
)

cc_library(
    name = "grpc++_gce",
    deps = ["@com_github_grpc_grpc//:grpc++"],
    alwayslink = 1,
)
