# 17. Where to go next

You now have the core loop of a small Vulkan renderer. The CPU owns application state, prepares resources, and records commands. Submission places those commands on a device queue. The GPU fetches vertex records through an index buffer, runs the vertex shader, assembles and rasterizes triangles, interpolates their outputs, runs the fragment shader, tests depth, and writes the color attachment. The canvas presents that attachment or copies it back for a capture.

The shader inputs now have three clear routes. Vertex attributes come from bound buffers according to the pipeline's stride, formats, offsets, and locations. Push constants copy the current matrices into the command stream for a draw. Descriptor set 0 refers to persistent resources: the material uniform buffer at binding 0 and the texture view plus sampler at binding 1. Those resources stay alive while submitted work can use them.

## The complete path

```text
CPU mesh arrays  -> vertex/index buffers --+
CPU matrices     -> push constants --------+-> vertex shader -> triangles
                                              -> rasterization and interpolation
CPU material     -> uniform buffer --------+-> fragment shader -> depth/color attachments
CPU pixels       -> staging -> image/view --+
sampler rules    -> sampler ----------------+
                                                        |
                                                        v
                                             present or capture
```

The arrows do not all mean the same thing. Upload calls copy CPU bytes. A descriptor stores a resource reference. A bind command selects resources for later draws. Queue submission schedules recorded work, and a completion signal establishes when referenced resources may be changed or destroyed.

## The Vulkan pieces you did not write

The GPU context creates the Vulkan instance, chooses a physical device, enables features, creates the logical device, and selects queue families. `dvz_gpu_ctx` is where those decisions live. The logical device is the parent of the buffers, images, shader modules, pipelines, and other device resources you created.

The window and canvas create the surface and swapchain, acquire the next image, recover from a resize, and present the rendered image. `dvz_canvas_frame` and `dvz_canvas_submit` wrap that frame contract.

The canvas also owns per-frame command pools, command buffers, semaphores, and fences. Semaphores order GPU operations such as acquisition, rendering, and presentation. Fences let the CPU learn that submitted work has completed. Together they let the CPU prepare later work while the GPU executes an earlier frame, then make command buffers and frame resources safe to reuse.

The rendering wrapper records dynamic rendering with color and depth attachments. Raw Vulkan can use classic render passes and framebuffers or modern dynamic rendering; the attachment descriptions and layout transitions still have to agree.

`DvzSlots` and `DvzDescriptors` stand in for descriptor set layouts, pipeline-layout integration, descriptor pools, allocation, and `vkUpdateDescriptorSets`. The vklite objects expose the set and binding decisions while the device manages the pool. The descriptor set still refers to the uniform buffer, image view, and sampler that your renderer owns.

The VMA-backed allocator chooses memory types and binds buffers and images. You still decide which resources are host visible, which are device local, how bytes reach them, and when a submitted resource is safe to change or destroy.

## What changes when

The final program separates state by how often it changes. This is a useful starting point for larger renderers, though real applications may make different choices.

| Frequency | State in this course | Delivery |
| --- | --- | --- |
| Per frame | current frame target and extent; camera and arcball result | borrowed canvas frame values and newly recorded commands |
| Per draw | projection and model-view matrices | push constants copied into the command buffer |
| When material or scene data changes | tint and lighting coefficients | bytes in the uniform buffer referenced by set 0 binding 0; safe updates require synchronization |
| When texture content or sampling changes | image pixels, image view, filtering, and addressing | staged image upload and set 0 binding 1 |
| When geometry changes | vertex records, indices, and draw count | vertex and index buffer uploads |
| Rarely | shader stages, vertex layout, topology, depth, culling, attachment formats, descriptor layouts | graphics pipeline and pipeline layout creation |

## Good next experiments

Try a compute shader that writes a storage image, then use a barrier and sample that image in a graphics pass. Add a second graphics pipeline for a wireframe or outline pass. Enable MSAA to evaluate more than one coverage sample per pixel, and study blending and transparency once opaque depth ordering is clear. An ImGui overlay is a useful way to expose camera and material controls.

When the renderer itself is no longer the lesson, move up to the Scene API. It keeps the same GPU foundations but handles retained resources, visuals, camera state, and planning for you. The low-level path remains available whenever a custom pipeline or unusual data flow earns the extra control.

## Checkpoint

1. Which objects coordinate image acquisition and presentation?
2. Why do fences matter even when command recording has already finished?
3. For each final shader input, can you identify where its bytes originate, how Vulkan interprets them, whether they are copied or referenced, and how long the source or resource must live?
4. Which part of the course would you replace with the Scene API in a larger application?
