# Go wrap of storage clients
# The test and libraries importing these wrappers:
#  mako/clients/go/storage:all
package(default_visibility = ["//go/clients/storage:__subpackages__"])

licenses(["notice"])

load("//swig:swig.bzl", "mako_go_wrap_cc")

mako_go_wrap_cc(
    name = "g3storage_wrap",
    srcs = ["google3_storage.swig"],
    cdeps = [
        "//cxx/clients/storage:google3_storage",
        "//cxx/spec:storage",
        "//spec/proto:mako_cc_proto",
        "@com_google_absl//absl/strings",
    ],
    godeps = [
        "@com_github_golang_protobuf//proto:go_default_library",  # Required by protofunc.swig
        "//spec/proto:mako_go_proto",
    ],
    importpath = "github.com/google/mako/cxx/clients/storage/go/g3storage_wrap",
    swig_includes = [
        "//swig:swig_includes",
        "//swig/go:swig_includes",
    ],
)

mako_go_wrap_cc(
    name = "mako_client_wrap",
    srcs = ["mako_client.swig"],
    cdeps = [
        "//cxx/clients/storage:google3_storage",
        "//cxx/spec:storage",
        "//cxx/clients/storage:mako_client_internal",
    ],
    godeps = [
        ":g3storage_wrap",
        "@com_github_golang_protobuf//proto:go_default_library",  # Required by protofunc.swig
    ],
    importpath = "github.com/google/mako/cxx/clients/storage/go/mako_client_wrap",
    swig_includes = [
        ":google3_storage.swig",
        "//swig:swig_includes",
        "//swig/go:swig_includes",
    ],
)

mako_go_wrap_cc(
    name = "fakestorage_wrap",
    srcs = ["fake_storage.swig"],
    cdeps = [
        "//cxx/clients/storage:fake_google3_storage",
        "//spec/proto:mako_cc_proto",
    ],
    godeps = [
        "@com_github_golang_protobuf//proto:go_default_library",  # Required by protofunc.swig
        "//spec/proto:mako_go_proto",
    ],
    importpath = "github.com/google/mako/cxx/clients/storage/go/fakestorage_wrap",
    swig_includes = [
        "//swig:swig_includes",
        "//swig/go:swig_includes",
    ],
)
