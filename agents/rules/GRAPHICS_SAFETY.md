# Graphics Safety Rules

These rules apply to Vulkan-path and runtime changes in `vk`, `vklite`, `canvas`, `stream`,
`video`, `window`, `drp2`, `scene`, and `app`.


## Runtime Foundation

The active low-level graphics stack is the runtime foundation for v0.4. Use it rather than creating
parallel presentation, frame-stream, Vulkan-wrapper, or renderer paths.

Preserve immediate presentation paths for high-FPS benchmarking. Treat unexpected frame pacing or
overhead regressions as first-class issues to investigate.


## Owned And Borrowed Handles

Distinguish owned and borrowed Vulkan handles explicitly.

Borrowed swapchain/canvas image views, command buffers, semaphores, images, and frame targets must
not be destroyed, begun, ended, reset, or submitted by a subsystem that does not own that lifecycle.

A borrowed frame command buffer is usually already recording. Wrap it when recording into it, but do
not call `vkBeginCommandBuffer` or `vkEndCommandBuffer` unless the API contract gives ownership of
recording.

Track command-buffer recording state at the owner level when possible, especially across canvas,
stream, DRP2, and vklite boundaries.


## Images And Layouts

Image transitions require a live image wrapper or handle and a known previous layout.

Never transition:

1. A destroyed object.
2. A borrowed handle with unknown ownership.
3. An object whose image wrapper is `NULL`.


## Runtime Object Churn

Long-running live paths should not accumulate transient per-frame runtime objects indefinitely.
Destroy or recycle transient command/render-pass objects once their command stream has completed.

When investigating scene -> DRP2 -> app churn, descriptor pressure, or unexpected runtime object
creation, enable the DRP2 app stream trace before guessing:

```sh
DVZ_DRP2_TRACE=full
DVZ_DRP2_TRACE=1
DVZ_DRP2_TRACE=normal
```

Use `DVZ_DRP2_TRACE_COLOR=0` or `NO_COLOR=1` when capturing logs. The app trace covers app-frame
streams; request-only readback/probe streams may need focused logging or tests.


## Validation

Run Vulkan validation-layer smoke tests for changes touching:

1. `vk`
2. `vklite`
3. `canvas`
4. `scene`
5. `drp2`
6. Command buffers
7. Frame lifetimes
8. Render targets
9. Swapchains
10. Synchronization

On macOS, use:

```sh
direnv exec . just test [filter]
```

Graphics tests can require Cocoa, Metal, LaunchServices, Vulkan SDK paths, and GLFW access.
