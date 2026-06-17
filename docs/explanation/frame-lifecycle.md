# Frame Lifecycle

A frame starts with retained scene state and ends with presentation, capture, or readback. The
runtime should execute the planned work; it should not rediscover scene semantics.

## Lifecycle Overview

The lifecycle is:

```text
scene mutation -> invalidation -> frame plan -> frame artifact ->
DRP2 setup/update/frame packets -> runtime execution -> presentation or capture
```

## Mutation and Invalidation

Scene mutation happens when user code creates objects, sets visual data, changes visibility,
updates sampled fields, changes controller state, resizes a figure, or requests capture/query work.
Those mutations mark the affected scene and resource state dirty.

## Planning and Artifacts

Frame planning gathers the dirty state and decides what the runtime needs. Setup work creates or
recreates resources and pipelines. Update work refreshes retained resources whose shape or content
changed. Frame work records the render, compute, copy, query, and presentation steps for the
current frame.

The frame artifact is the ownership boundary for emitted work. It owns stream snapshots and packet
spans long enough for the runtime or WASM host to consume them. JSON emission is a debug and
fixture-export view, not the browser render path.

## Runtime Execution

Runtime execution consumes the artifact through the supported backend path. Native execution uses
the vklite/canvas/stream/app stack. Browser execution consumes split DRP2 packets through the
experimental WebGPU path. The runtime may cache backend resources, but it should treat scene
semantics as already decided.

## Presentation, Capture, and Cleanup

Presentation displays an interactive frame. Capture renders a bounded frame and writes a raster
artifact. Query and readback work may complete asynchronously on browser paths and should expose
explicit status rather than blocking on hidden assumptions.

Cleanup follows ownership. Destroy app/runtime objects before destroying the scene. Release emitted
packet or readback views according to their documented lifetime; do not retain borrowed spans past
the next emit or explicit release.

See also:

- [Invalidation and caching](invalidation-and-caching.md)
- [GPU resource ownership](gpu-resource-ownership.md)
- [Objects and lifetimes](../reference/objects-and-lifetimes.md)
