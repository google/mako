# Rolling Window Reducer

## Overview

The Rolling Window Reducer is a library that allows you to smoothen
predownsampled data or extract aggregate data from it.

The Reducer works similarly to a rolling average:

1.  A window moves from the first point to the last point in your data set with
    a step size specified in the `RWRConfig` protobuf.

1.  For each step we perform the calculation specified (eg. sum, count or mean)
    on all the points within the window.

1.  A new `SamplePoint` is created. This `SamplePoint` has an `input_value`
    (x-val) as the middle of the window and a `KeyedValue` (y-value) as the
    window parameter the user is interested in.

Details of the configurable parameters of the Rolling Window Reducer are
provided in the
[rolling_window_reducer.proto](../proto/helpers/rolling_window_reducer/rolling_window_reducer.proto).

## Example

An example of the use of the Rolling Window Reducer to create a synthetic QPS
metric can be found in the
[Go Quickstore example](../examples/go_quickstore/example_test.go).
