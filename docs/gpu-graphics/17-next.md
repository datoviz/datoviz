# 17. Where to go next

You now have the core loop of a small Vulkan renderer: the CPU prepares resources and records commands, and the GPU turns them into pixels along this path.

## The complete path

```text
CPU mesh arrays  -> vertex/index buffers --+
CPU matrices     -> push constants --------+-> vertex shader
                                                     |
                                      rasterization and interpolation
                                                     |
CPU material     -> uniform buffer --------+         v
CPU pixels       -> staging -> image/view -+-> fragment shader
sampler rules    -> sampler ---------------+         |
                                                     v
                                      depth test and color attachment
                                                     |
                                                     v
                                            present or capture
```

The arrows mean different things. Uploads copy CPU bytes. Descriptors store references. Bind commands select resources for later draws. Submission queues recorded work, and a completion signal says when referenced resources may be changed or destroyed.

## The Vulkan pieces you did not write

The GPU context creates the Vulkan instance, chooses a physical device, enables features, selects queue families, and creates the logical device that owns every resource you made.

Before any of that, Datoviz finds and loads the **Vulkan loader**, the system library that discovers installed drivers and routes each Vulkan call to the right one. Every call you made through vklite went through it. The loader can also insert **layers** between your program and the driver. `dvz_gpu_ctx_config_validation()` asked for the Khronos validation layer: when it is installed, the loader places it in front of the driver, it checks each call against the specification, and Datoviz's debug messenger prints its messages and counts the errors that `dvz_gpu_ctx_error_count()` reported at the end of every chapter. Validation slows every call, so release builds usually turn it off.

The window and canvas create the surface and swapchain, acquire images, recover from resizes, and present. `dvz_canvas_frame()` and `dvz_canvas_submit()` wrap that contract.

The canvas also owns each frame slot's command pool, command buffer, semaphores, and fence. Semaphores order GPU operations such as acquisition, rendering, and presentation. Fences tell the CPU that submitted work has finished. Together they let the CPU record one frame while the GPU executes another, and say when a frame slot is safe to reuse.

The rendering wrapper records dynamic rendering with color and depth attachments. Raw Vulkan can also use classic render passes and framebuffers; either way, attachments and image layout transitions must agree.

`DvzSlots` and `DvzDescriptors` stand in for descriptor set layouts, pipeline layouts, descriptor pools, and `vkUpdateDescriptorSets`. You still choose every set and descriptor binding; the device manages the pool.

The VMA-backed allocator chooses memory types and binds memory. You still decide which resources are host-visible or device-local, how bytes reach them, and when they are safe to change or destroy.

## What changes when

The final program sorts its state by how often it changes, a useful starting point for larger renderers.

| Frequency | State in this course | Delivery |
| --- | --- | --- |
| Per frame | current frame target and extent; camera and arcball result | borrowed canvas frame values and newly recorded commands |
| Per draw | projection and model-view matrices | push constants copied into the command buffer |
| When material or scene data changes | tint and lighting coefficients | bytes in the uniform buffer referenced by set 0 binding 0; safe updates require synchronization |
| When texture content or sampling changes | image pixels, image view, filtering, and addressing | staged image upload and set 0 binding 1 |
| When geometry changes | vertex records, indices, and draw count | vertex and index buffer uploads |
| Rarely | shader stages, vertex layout, topology, depth, culling, attachment formats, descriptor layouts | graphics pipeline and pipeline layout creation |

## Good next experiments

Write a compute shader that fills a storage image, then barrier and sample it in a graphics pass. Add a second pipeline for a wireframe or outline pass. Enable MSAA, then study blending and transparency. Add an ImGui overlay for camera and material controls.

When the renderer itself is no longer the lesson, move up to the Scene API. It runs on the same GPU foundations but manages resources, visuals, and cameras for you. The low-level path remains there whenever a custom pipeline earns the extra control.

## Checkpoint

1. Which objects coordinate image acquisition and presentation?
2. Why do fences matter even when command recording has already finished?
3. For each final shader input, can you identify where its bytes originate, how Vulkan interprets them, whether they are copied or referenced, and how long the source or resource must live?
4. Which part of the course would you replace with the Scene API in a larger application?
