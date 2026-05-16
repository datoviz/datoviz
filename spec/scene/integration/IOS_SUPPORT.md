# iOS Support Plan

Status on 2026-05-16: iOS is not an active Datoviz target yet. A future iOS build should reuse the
native Vulkan-facing architecture through MoltenVK rather than creating a separate Metal renderer.


## Goal

Run the active scene -> DRP2 -> vklite/canvas path on iOS through MoltenVK and a hosted UIKit or
SwiftUI surface.

The first supported slice should include:

1. offscreen rendering through MoltenVK;
2. hosted rendering into a `CAMetalLayer`-backed view;
3. resize, scale, pause, and foreground/background lifecycle handling;
4. touch events translated into the shared Datoviz input router;
5. asset/shader packaging suitable for an iOS app bundle.


## Recommended Architecture

Use hosted integration as the boundary.

The iOS host should provide:

1. the Vulkan instance extensions required by MoltenVK for a Metal-backed surface;
2. a `VkSurfaceKHR` created from the host view's `CAMetalLayer`;
3. framebuffer size and content-scale updates from UIKit;
4. touch and gesture events translated to Datoviz pointer or future touch events.

Datoviz should continue to own:

1. scene state and frame planning;
2. DRP2 command streams;
3. vklite runtime resources;
4. canvas swapchain and offscreen targets.

Do not add iOS-specific scene semantics, and do not bypass DRP2 for presentation.


## Build Work

The first build slice should:

1. support an iOS CMake toolchain and Xcode project generation;
2. build Datoviz as a static library or framework suitable for app embedding;
3. link MoltenVK and required Apple frameworks through a narrow platform layer;
4. allow `DVZ_WITH_GLFW=OFF`;
5. gate CUDA, desktop video paths, and desktop GLFW examples;
6. package shader and data assets in the application bundle.

The preferred first deliverable is a minimal UIKit host with one Datoviz view and one retained
scene example.


## Runtime Work

iOS lifecycle handling must cover:

1. app background and foreground transitions;
2. layer size and content-scale changes;
3. device rotation;
4. surface replacement or temporary unavailability;
5. memory-pressure cleanup where possible;
6. clear diagnostics when a required MoltenVK feature is unavailable.

MoltenVK-specific limitations should be treated as backend capabilities. The scene layer should
adapt or report diagnostics rather than special-casing iOS visuals.


## Validation

Initial validation should include:

1. one offscreen pixel-readback smoke;
2. one hosted view clear/present smoke;
3. one retained scene smoke with mesh or points;
4. one arcball or panzoom touch interaction smoke;
5. repeated resize/rotation while rendering.

Device validation is required before claiming support. Simulator validation is useful for build and
UI integration but does not replace testing on a physical iOS device.
