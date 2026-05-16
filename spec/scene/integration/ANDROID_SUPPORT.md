# Android Support Plan

Status on 2026-05-16: Android is not an active Datoviz target yet. The v0.4 architecture is close
enough that Android should be an integration lane, not a renderer fork.


## Goal

Run the active scene -> DRP2 -> vklite/canvas path on Android with Vulkan, using the same retained
scene semantics as desktop builds.

The Android port should support:

1. offscreen rendering tests without a Java UI;
2. a hosted Android surface supplied by an Activity or native host;
3. basic pointer/touch navigation through the shared input router;
4. asset/shader loading from packaged application assets;
5. swapchain teardown and recreation during surface loss, resize, pause, and resume.


## Recommended Architecture

Use the existing hosted-window boundary.

Android should provide:

1. the required Vulkan instance extensions, including `VK_KHR_surface` and
   `VK_KHR_android_surface`;
2. a native `VkSurfaceKHR` created from `ANativeWindow`;
3. framebuffer size, logical size, and content scale updates;
4. translated touch/pointer events through the app hosted-input functions.

Datoviz should continue to own:

1. GPU device selection;
2. DRP2 runtime creation;
3. canvas swapchain and frame lifecycle;
4. scene emission and request processing.

Do not add an Android-specific renderer or scene path.


## Build Work

The first CMake slice should:

1. support Android NDK toolchain configuration;
2. allow `DVZ_WITH_GLFW=OFF`;
3. build `common`, `math`, `thread`, `input`, `window`, `canvas`, `vk`, `vklite`, `drp2`,
   `scene`, and `app` without desktop-only dependencies;
4. gate video, GUI, and GLFW examples unless explicitly enabled;
5. package shaders, fixture assets, and example data through an Android asset strategy.

The preferred first deliverable is a small native Android example that creates a Datoviz app with
hosted surface callbacks rather than a broad Java/Kotlin UI.


## Runtime Work

Android lifecycle handling must cover:

1. Activity pause/resume;
2. `ANativeWindow` creation and destruction;
3. zero-sized or temporarily unavailable surfaces;
4. swapchain recreation after rotation or density changes;
5. device-lost or surface-lost diagnostics.

The existing wrap surface path already models surface loss and replacement. Android should reuse
that contract and add platform-specific adapters only at the boundary.


## Validation

Initial validation should include:

1. CPU-only unit tests under the Android toolchain where practical;
2. one offscreen render smoke that reads back pixels;
3. one hosted-surface smoke that clears and presents;
4. one scene smoke with pan/zoom or arcball input translated from touch;
5. repeated surface destroy/recreate while rendering.

Validation should run on at least one physical Vulkan-capable Android device before claiming
support. Emulator-only validation is not sufficient for the first supported target.
