# Canvas and Stream API

Canvas and stream sit between native runtime execution and output delivery. They turn completed
frame work into presentation, offscreen capture, video/live-image sinks, and frame timing metadata.

This is a lower-layer page. Most users should use the app, offscreen, capture, and video how-to
pages unless they are debugging or extending the runtime.

## Role in the Stack

The native output path is:

```text
DRP2 runtime execution -> canvas frame -> stream sinks -> presentation/capture/video
```

`DvzCanvas` owns the frame-facing runtime surface. It acquires a frame, runs the registered draw
callback, submits the current frame to the internal stream, and exposes diagnostics such as render
mode, present state, offscreen state, and frame timings.

`DvzStream` owns sink fan-out. A stream starts from a `DvzStreamFrame`, submits timeline wait
values to attached sinks, updates frame metadata when handles change, and stops sinks cleanly when
the output path is rebuilt or destroyed.

## Canvas Responsibilities

Canvas covers:

- interactive swapchain-backed frames;
- offscreen render targets;
- capture of screenshot/export pixels as tightly packed sRGB RGBA8;
- input-router access for window-backed canvases;
- optional video and live-image sink configuration;
- timing history for recent frame submissions.

Canvas does not decide scene semantics. A draw callback should execute already-planned work or call
the higher app/view path that emits and submits the current scene frame.

## Stream Responsibilities

Stream covers output delivery:

- registering and selecting sink backends;
- starting sinks from the current frame metadata;
- submitting frames after GPU work reaches a timeline value;
- updating sinks when image handles, extents, or synchronization handles change;
- stopping and detaching sinks during teardown.

The sink boundary is important for video, live-image interop, offscreen capture, and future hosted
output paths. Sinks receive frame metadata and synchronization information; they should not infer
new scene meaning from the frame.

## Lifetime and Capture Rules

Frame metadata may include borrowed Vulkan handles. A sink or callback must honor the documented
frame lifetime and copy out long-lived user data. Capture APIs return screenshot/export pixels, not
linear scientific data values.

Destroy app/view/runtime objects before destroying scene-owned state that their draw callbacks
depend on. Recreate or refresh sinks when swapchain or offscreen handles become dirty.

## Validation

Use focused tests while iterating:

```sh
just test canvas
just test stream
git diff --check
```

For GLFW, swapchain, offscreen, video, or external-handle work, add the narrow smoke that exercises
the affected render mode and recreate path.

See also:

- [Frame lifecycle](../explanation/frame-lifecycle.md)
- [GPU resource ownership](../explanation/gpu-resource-ownership.md)
- [Save screenshots](../how-to/screenshots.md)
- [Export videos](../how-to/video-export.md)
- [vklite](vklite.md)
