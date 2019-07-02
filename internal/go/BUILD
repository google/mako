# Go libraries for mako
package(default_visibility = ["//:internal"])

load("@io_bazel_rules_go//go:def.bzl", "go_library", "go_binary")

go_library(
    name = "common",
    srcs = [
        "common_deps.go",
    ],
    # Weird deps to make Glaze happy. It yells at you if deps consists of just a select statement.
    importpath = "github.com/google/mako/internal/go/common",
)
