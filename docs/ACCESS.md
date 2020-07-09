# Access to Mako

## Access to create projects and benchmarks

[Mako](https://mako.dev) is designed to be used by open source projects in which
Google is actively engaged. For this reason, the ability to create new projects
and benchmarks on Mako is limited to whitelisted identities.

If you’re interested in using Mako for an open source project, please add a
GitHub issue to [github.com/google/mako](https://github.com/google/mako) with
your request.

## Access to create benchmarks under a project

You must be a project owner in order to create a benchmark under a project. As
project owner, you can specify the benchmark owner(s).

## Access to run Mako tests

Mako performance tests (tests that report their performance data to Mako using a
Mako client) require write access to the benchmark associated with that test.
This access can only be granted by the owners of that particular Mako benchmark
(not by us, the maintainers of Mako).

If you would like access to run performance tests and upload results to a
benchmark in Mako, please direct your request to the owners of the repository
containing the Mako test.

## Managing access to your benchmark

If you are the owner of a Mako benchmark either directly or indirectly via
project ownership, you can manage its ownership, including adding and removing
owners. Use the Mako CLI to make changes to your benchmark. Read
[BUILDING.md](BUILDING.md#building-the-command-line-tool) to learn how to build
the Mako CLI.

To update a benchmark to modify its owners (or any other fields), use the
`update_benchmark` subcommand.

```bash
mako help update_benchmark
```

To open a local editor which will allow you to make changes to the benchmark,
which will be uploaded to Mako once the file is saved and quit, execute:

```bash
BENCHMARK_KEY=<your benchmark key> mako update_benchmark
--benchmark_key=${BENCHMARK_KEY}
```

When you modify a benchmark in this way, you’re modifying a protobuf
BenchmarkInfo object. To read more about BenchmarkInfo fields, check out
[mako.proto](../spec/proto/mako.proto).
