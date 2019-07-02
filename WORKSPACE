load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")


# proto_library, cc_proto_library, and java_proto_library rules implicitly
# depend on @com_google_protobuf for protoc and proto runtimes.
# This statement defines the @com_google_protobuf repo.
http_archive(
    name = "com_google_protobuf",
    #sha256 = "f976a4cd3f1699b6d20c1e944ca1de6754777918320c719742e1674fcf247b7e",
    strip_prefix = "protobuf-3.8.0",
    urls = ["https://github.com/google/protobuf/archive/v3.8.0.zip"],
)

http_archive(
    name = "bazel_skylib",
    sha256 = "bbccf674aa441c266df9894182d80de104cabd19be98be002f6d478aaa31574d",
    strip_prefix = "bazel-skylib-2169ae1c374aab4a09aa90e65efe1a3aad4e279b",
    urls = ["https://github.com/bazelbuild/bazel-skylib/archive/2169ae1c374aab4a09aa90e65efe1a3aad4e279b.tar.gz"],
)

# Abseil
http_archive(
    name = "com_google_absl",
    strip_prefix = "abseil-cpp-master",
    urls = ["https://github.com/abseil/abseil-cpp/archive/master.zip"],
)

# GoogleTest/GoogleMock framework. Used by most unit-tests.
http_archive(
    name = "com_google_googletest",
    urls = ["https://github.com/google/googletest/archive/master.zip"],
    strip_prefix = "googletest-master",
)

# glog
# TODO(b/134946989) Switch to using Abseil logging
git_repository(
    name = "com_google_glog",
    commit = "e364e754a60af6f0eadd9902c4e76ecc060fee9c",
    remote = "https://github.com/google/glog.git",
)

# gflags
# Used by glog.
# TODO(b/134946989) Can go away when glog goes away.
http_archive(
    name = "com_github_gflags_gflags",
    sha256 = "6e16c8bc91b1310a44f3965e616383dbda48f83e8c1eaa2370a215057b00cabe",
    strip_prefix = "gflags-77592648e3f3be87d6c7123eb81cbad75f9aef5a",
    urls = [
        "https://mirror.bazel.build/github.com/gflags/gflags/archive/77592648e3f3be87d6c7123eb81cbad75f9aef5a.tar.gz",
        "https://github.com/gflags/gflags/archive/77592648e3f3be87d6c7123eb81cbad75f9aef5a.tar.gz",
    ],
)

# farmhash
new_git_repository(
    name = "com_google_farmhash",
    build_file_content = """
package(default_visibility = ["//visibility:public"])

cc_library(
    name = "farmhash",
    hdrs = ["src/farmhash.h"],
    srcs = ["src/farmhash.cc"],
    deps = [
       # "@com_google_absl//base:core_headers",
       # "@com_google_absl//base:endian",
       # "@com_google_absl//numeric:int128",
    ],
)""",
    commit = "2f0e005b81e296fa6963e395626137cf729b710c",
    remote = "https://github.com/google/farmhash.git",
)

# Google Benchmark
git_repository(
    name = "com_google_benchmark",
    remote = "https://github.com/google/benchmark",
    tag = "v1.5.0",
)

# Google Cloud CPP
http_archive(
    name = "com_google_cloud_cpp",
    sha256 = "fd0c3e3b50f32af332b53857f8cd1bfa009e33d1eeecabc5c79a4825d906a90c",
    strip_prefix = "google-cloud-cpp-0.10.0",
    urls = [
        "https://github.com/googleapis/google-cloud-cpp/archive/v0.10.0.tar.gz",
    ],
)

# ===== re2 =====
http_archive(
    name = "com_googlesource_code_re2",
    sha256 = "5306526bcdf35ff34c67913bef8f7b15a3960f4f0ab3a2b6a260af4f766902d4",
    strip_prefix = "re2-c4f65071cc07eb34d264b25f7b9bbb679c4d5a5a",
    urls = [
        "https://mirror.bazel.build/github.com/google/re2/archive/c4f65071cc07eb34d264b25f7b9bbb679c4d5a5a.tar.gz",
        "https://github.com/google/re2/archive/c4f65071cc07eb34d264b25f7b9bbb679c4d5a5a.tar.gz",
    ],
)

load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")
protobuf_deps()

http_archive(
    name = "io_bazel_rules_go",
    urls = ["https://github.com/bazelbuild/rules_go/releases/download/0.18.6/rules_go-0.18.6.tar.gz"],
    sha256 = "f04d2373bcaf8aa09bccb08a98a57e721306c8f6043a2a0ee610fd6853dcde3d",
)
load("@io_bazel_rules_go//go:deps.bzl", "go_rules_dependencies", "go_register_toolchains")
go_rules_dependencies()
go_register_toolchains()

http_archive(
    name = "bazel_gazelle",
    urls = ["https://github.com/bazelbuild/bazel-gazelle/releases/download/0.17.0/bazel-gazelle-0.17.0.tar.gz"],
    sha256 = "3c681998538231a2d24d0c07ed5a7658cb72bfb5fd4bf9911157c0e9ac6a2687",
)

load("@bazel_gazelle//:deps.bzl", "gazelle_dependencies", "go_repository")
gazelle_dependencies()

go_repository(
    name = "com_github_golang_protobuf",
    importpath = "github.com/golang/protobuf",
    tag = "v1.3.1",
)

go_repository(
    name = "org_golang_google_grpc",
    importpath = "google.golang.org/grpc",
    tag = "v1.20.1",
)

go_repository(
    name = "com_github_golang_glog",
    importpath = "github.com/golang/glog",
    tag = "23def4e6c14b4da8ac2ed8007337bc5eb5007998",
)
