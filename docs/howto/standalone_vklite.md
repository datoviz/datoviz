# How to use the vklite C API

This example shows how to **write a standalone C app using only the vklite API** (thin wrapper on top of the Vulkan C API), not the canvas or scene API.

Creating a Vulkan-aware window is a complex operation as it requires creating a swapchain and implementing a rendering loop using proper CPU-GPU and GPU-GPU synchronization. The canvas abstracts that complexity away and there are probably few reasons not to use it. Therefore, **the main reason to use the vklite API directly is probably when doing offscreen rendering and/or compute**, and when reusing existing visuals and graphics in Datoviz is not desirable.

On this page, we'll show **how to make an offscreen render of a triangle** using *only* the vklite API. We'll cover the following steps:

* Creating a GPU with custom queues.
* Creating a render pass.
* Creating GPU images for rendering.
* Creating framebuffers.
* Creating a graphics pipeline.
* Creating pipeline bindings.
* Creating a vertex buffer.
* Creating and recording a command buffer.
* Submitting a command buffer to the GPU and waiting until it has completed.
* Making a screenshot by creating a staging GPU image, and submitting a command buffer with transition barriers and a GPU image copy.

This "hello world" script is about 250 lines long (without comments), about 4x smaller than by using the raw Vulkan C API.

<!-- IMAGE ../images/screenshots/standalone_vklite.png -->

<!-- CODE_C examples/standalone_vklite.c -->
