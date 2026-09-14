# 16. Where to go next

You now have the core loop of a small Vulkan renderer: a canvas supplies a frame, vklite records a pass, a pipeline fixes the shader and raster state, buffers hold geometry, and an image stores sampled pixels. The final viewer is short because Datoviz keeps the platform machinery behind those boundaries.

## The Vulkan pieces you did not write

The GPU context creates the Vulkan instance, chooses a physical device, enables features, creates the logical device, and selects queue families. `dvz_gpu_ctx` is where those decisions live.

The window and canvas create the surface and swapchain, acquire the next image, recover from a resize, and present the rendered image. `dvz_canvas_frame` and `dvz_canvas_submit` wrap that frame contract.

The canvas also owns per-frame command pools, command buffers, semaphores, and fences. Those objects let the CPU prepare another frame while the GPU executes the previous one, then make reuse safe when a frame slot comes around again.

The rendering wrapper records dynamic rendering with color and depth attachments. Raw Vulkan can use classic render passes and framebuffers or modern dynamic rendering; the attachment descriptions and layout transitions still have to agree.

Slots and descriptors stand in for descriptor set layouts, descriptor pools, allocation, and `vkUpdateDescriptorSets`. The vklite objects expose the binding decisions while the device owns the pool.

The VMA-backed allocator chooses memory types and binds buffers and images. You still decide which resources are host visible, which are device local, and when a submitted resource is safe to destroy.

## Good next experiments

Try a compute shader that writes a storage image, then sample that image in a graphics pass. Add a second graphics pipeline for a wireframe or outline pass. Enable MSAA for smoother edges, and study blending and transparency once depth ordering is clear. An ImGui overlay is a useful way to expose camera and material controls.

When the renderer itself is no longer the lesson, move up to the Scene API. It keeps the same GPU foundations but handles retained resources, visuals, camera state, and planning for you. The low-level path remains available whenever a custom pipeline or unusual data flow earns the extra control.

## Checkpoint

1. Which objects coordinate image acquisition and presentation?
2. Why do fences matter even when command recording has already finished?
3. Which part of the course would you replace with the Scene API in a larger application?
