# VKLite Presentation Layer Plan (Surface + Swapchain)

## Goal
Move Vulkan surface/swapchain/present mechanics out of `src/canvas` into a minimal vklite layer so higher
modules (canvas, future DRP renderer, scene) use vklite instead of raw Vulkan for presentation.

## Scope
- Add a vklite surface wrapper (`DvzSurface`) and a vklite swapchain wrapper (`DvzSwapchain`).
- Keep window creation in `src/window` (GLFW/headless), but expose native `VkSurfaceKHR` to vklite.
- Canvas remains the orchestration layer (frame lifecycle, blit/copy to swapchain images, streams).

## Non-goals
- Do not refactor window creation or GLFW usage into vklite.
- Do not redesign canvas rendering or stream/video logic beyond swapping to vklite presentation calls.
- Do not introduce a new public API unrelated to presentation.

## New Public Headers
Create:
- `include/datoviz/vklite/surface.h`
- `include/datoviz/vklite/swapchain.h`

Update:
- `include/datoviz/vklite.h` to include both new headers.

## New Source Files
Create:
- `src/vklite/surface.c`
- `src/vklite/swapchain.c`

`src/vklite/CMakeLists.txt` already globs `*.c*`, so new files are picked up automatically.

## Core Types

### DvzSurface
Wraps the native surface and cached capabilities.

Suggested fields:
- `DvzObject obj`
- `DvzInstance* instance`
- `VkSurfaceKHR surface`
- `VkSurfaceCapabilitiesKHR caps`
- `VkSurfaceFormatKHR* formats` + count
- `VkPresentModeKHR* present_modes` + count

### DvzSwapchain
Wraps swapchain state and per-image resources.

Suggested fields:
- `DvzObject obj`
- `DvzDevice* device`
- `DvzSurface* surface`
- `VkSwapchainKHR handle`
- `VkFormat format`
- `VkColorSpaceKHR color_space`
- `VkPresentModeKHR present_mode`
- `uint32_t width, height`
- `uint32_t image_count`
- `VkImage* images` + count
- `VkImageView* views` + count
- `DvzSemaphore* acquire_semaphores` (optional)
- `DvzSemaphore* present_semaphores` (optional)

### DvzSwapchainConfig
Compact configuration for creation/recreation:
- `uint32_t width, height`
- `VkFormat format`
- `VkColorSpaceKHR color_space`
- `VkPresentModeKHR present_mode`
- `uint32_t image_count`
- `bool vsync`

## Public API (Minimal)

### surface.h
- `DVZ_EXPORT void dvz_surface_init(DvzSurface* surface, DvzInstance* instance);`
- `DVZ_EXPORT void dvz_surface_wrap_native(DvzSurface* surface, VkSurfaceKHR vk_surface);`
- `DVZ_EXPORT void dvz_surface_refresh(DvzSurface* surface, DvzGpu* gpu);`
- `DVZ_EXPORT void dvz_surface_destroy(DvzSurface* surface);`

### swapchain.h
- `DVZ_EXPORT void dvz_swapchain_init(DvzSwapchain* swapchain, DvzDevice* device, DvzSurface* surface);`
- `DVZ_EXPORT void dvz_swapchain_config(DvzSwapchain* swapchain, DvzSwapchainConfig* cfg);`
- `DVZ_EXPORT void dvz_swapchain_recreate(DvzSwapchain* swapchain);`
- `DVZ_EXPORT bool dvz_swapchain_acquire(DvzSwapchain* swapchain, uint32_t* image_index, DvzSemaphore* signal);`
- `DVZ_EXPORT bool dvz_swapchain_present(DvzSwapchain* swapchain, uint32_t image_index, DvzSemaphore* wait);`
- `DVZ_EXPORT void dvz_swapchain_destroy(DvzSwapchain* swapchain);`

Notes:
- `acquire`/`present` return `bool` for success; treat `VK_ERROR_OUT_OF_DATE_KHR`/`VK_SUBOPTIMAL_KHR` as
  non-fatal and signal that a recreation is needed.
- `DvzSemaphore` is from vklite/sync; if optional, allow NULL to use internal semaphores.

## Migration Map (from canvas)

### Move out of `src/canvas/swapchain_sink.c`
Relocate into vklite:
- `vkGetPhysicalDeviceSurfaceCapabilitiesKHR`
- `vkGetPhysicalDeviceSurfaceFormatsKHR`
- `vkGetPhysicalDeviceSurfacePresentModesKHR`
- `vkCreateSwapchainKHR` / `vkDestroySwapchainKHR`
- `vkGetSwapchainImagesKHR`
- Swapchain image view creation/destruction
- `vkAcquireNextImageKHR` / `vkQueuePresentKHR`

### Keep in canvas
- High-level swapchain sink orchestration
- Color conversion + blit/copy into swapchain images
- Stream sink management
- Canvas lifecycle and presentation timing

### Window integration
Keep `src/window/backend_glfw.c` creating `VkSurfaceKHR`. Add a small accessor so canvas can pass it to
vklite:
- `VkSurfaceKHR dvz_window_surface_handle(DvzWindowSurface* surface);`

## Implementation Steps (Ordered)

1) **Add headers**
   - Create `include/datoviz/vklite/surface.h` and `include/datoviz/vklite/swapchain.h`.
   - Add Doxygen docstrings to every public function.
   - Update `include/datoviz/vklite.h` to include both.

2) **Add source files**
   - Implement `src/vklite/surface.c` with full Vulkan queries and cached caps/formats/modes.
   - Implement `src/vklite/swapchain.c` creation + recreation + image view setup.

3) **Integrate into canvas**
   - Replace raw Vulkan surface/swapchain calls in `src/canvas/swapchain_sink.c` with vklite calls.
   - Keep internal logic but redirect creation/acquire/present to vklite.

4) **Window surface accessor**
   - Add `dvz_window_surface_handle()` (or similar) to `src/window/window_internal.h` + implementation.
   - Canvas calls that and passes the `VkSurfaceKHR` to `dvz_surface_wrap_native()`.

5) **Tests**
   - Add unit tests under `src/vklite/tests/` for surface + swapchain basic creation and
     acquire/present (if possible with headless/GLFW).
   - Update `src/vklite/tests/test_vklite.c` to include new tests.

6) **Cleanup**
   - Remove unused raw Vulkan helpers from canvas once vklite covers them.
   - Keep any necessary internal canvas helpers if they’re not strictly presentation-related.

## Suggested Test Coverage
- Create a surface from GLFW, wrap it in vklite, query formats/present modes.
- Create swapchain with chosen format/mode; verify images + views count.
- Acquire + present cycle for a frame (no rendering required).
- Recreate swapchain when size changes or when `VK_ERROR_OUT_OF_DATE_KHR` returned.

## Risks / Edge Cases
- Surface format/present mode selection varies by platform; keep selection logic flexible.
- Swapchain recreation must handle in-flight images and command buffers cleanly.
- Ensure dynamic rendering / layout transitions remain correct when blitting into swapchain images.

## Completion Checklist
- [ ] New vklite headers and source files added
- [ ] Canvas swapchain sink no longer calls Vulkan surface/swapchain directly
- [ ] Window exposes native surface handle to canvas
- [ ] vklite tests for surface/swapchain are present and wired into `dvztest`
- [ ] `just build` and `./dvztest vklite` succeed

