# Scene Runtime Boundary

This document defines the allowed contract between the future scene layer and the DRP2 runtime.


## Allowed Dependencies

The scene layer may depend on:

1. capability query,
2. error reporting,
3. shader ingestion through DRP2-visible concepts,
4. resource creation and update through DRP2-visible concepts,
5. command-stream submission,
6. readback and offscreen target services expressed without backend handle leakage.


## Forbidden Dependencies

The scene layer must not depend on:

1. Vulkan handle types,
2. swapchain internals,
3. backend-specific synchronization objects,
4. backend allocator internals,
5. platform window-system handles,
6. backend command-buffer recording APIs.


## Design Rule

If the scene layer needs a reusable low-level behavior, prefer improving the DRP2 or runtime-facing
contract instead of adding a scene-private backend escape hatch.
