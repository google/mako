# Building Mako

Only building on Linux and MacOS is supported at this time.

## Prerequisites

#### Bazel

See Installing Bazel (https://docs.bazel.build/versions/master/install.html) for
instructions for installing Bazel on your system. Version v0.28.1 is known to
work.

#### Git

See Installing Git
(https://git-scm.com/book/en/v2/Getting-Started-Installing-Git).

## Cloning the repository
```bash
git clone github.com/google/mako
cd mako
```

## Building and running tests
```bash
bazel test ...
```

## Building with Mako as a dependency using Bazel (C++ or Go)

Import Mako as a dependency in your WORKSPACE file:

```
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
    name = "mako",
    remote = "https://github.com/google/mako.git",
    tag = "v0.0.1",
)
```

Then, to depend on a Mako Bazel target from your own Bazel targets:

for C++:
```
cc_library(
    name = "use_mako",
    srcs = ["use_mako.cc"],
    hdrs = ["use_mako.h"],
    deps = [
        "@mako//helpers/cxx/quickstore",
    ],
)
```

for Go:
```
go_library(
    name = "usemako",
    srcs = ["usemako.go"],
    importpath = "github.com/your/project/usemako"
    deps = [
        "@mako//helpers/go/quickstore",
    ],
)
```

<h2 id="go-build">Building with Mako as a dependency using `go build/test`</h2>

You should depend on http://github.com/google/mako in the same way you depend on
other third-party libraries. This might involve vendoring, using a tool like
[dep](https://github.com/golang/dep), or using
[Go Modules](https://github.com/golang/go/wiki/Modules). We recommend using
Go Modules.

Regardless of the tool you use, when you import Mako from your `.go` files it
will look like roughly like this:
```go
import (
	"github.com/google/mako/helpers/go/quickstore"
	qpb "github.com/google/mako/helpers/proto/quickstore/quickstore_proto"
)
```

Note when using `go build/test` that the Go client doesn't stand alone, it needs
to connect to a running Mako microservice. Learn more at
[CONCEPTS.md#microservice].

See the [GUIDE.md] for a step-by-step guide to writing and running a Mako
Quickstore test.

## Building the command-line tool
```bash
bazel build internal/tools/cli:mako
```

The binary will be found in `bazel-bin/internal/tools/cli/*/mako` (the wildcard
will differ based on your platform, check the Bazel output). You can copy this
binary into a more convenient location (e.g. somewhere on your $PATH).
Alternatively, run directly from Bazel:

```bash
bazel run internal/tools/cli:mako help
```

<h2 id="microservice">Building the Quickstore microservice</h2>
$ bazel build internal/quickstore_microservice:quickstore_microservice_mako

The binary will be found in
`bazel-bin/internal/quickstore_microservice/quickstore_microservice_mako`. You
can copy this binary into a more convenient location (e.g. somewhere on your
$PATH). Alternatively, run directly from Bazel:

```bash
bazel run internal/quickstore_microservice:quickstore_microservice_mako
```
