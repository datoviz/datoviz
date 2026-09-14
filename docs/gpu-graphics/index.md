# Modern GPU Graphics in Vulkan

This course teaches modern GPU graphics in C, with Vulkan running underneath.

You start with an empty file and end with an interactive 3D mesh viewer: a perspective camera you control with the mouse, depth-tested geometry, a texture you upload yourself, and per-fragment lighting. You write the program line by line in a small project of your own.

<picture>
  <source media="(prefers-reduced-motion: no-preference)" srcset="/assets/gpu-graphics/15-mesh-animated.webp">
  <img src="/assets/gpu-graphics/15-mesh.webp" alt="The textured, lit mesh built in chapter 15 rotating in the final viewer.">
</picture>

## Why this course exists

Vulkan gives you direct control over a GPU, but that control creates a steep starting point. Before a raw Vulkan program can draw its first triangle, it needs an instance, a physical device, a queue family, a logical device, a window surface, a swapchain and its image views, a command pool, and synchronization for every frame in flight. By then you may have written around 1000 lines, and none of them describes the triangle.

The course explains those concepts without asking you to implement the platform machinery. Datoviz's low-level layers (`vklite` and its canvas) own that code, so from chapter 2 onward you can work on shaders, pipelines, buffers, textures, matrices, and lighting. These low-level APIs are advanced and unstable. Use the Datoviz version specified in the setup chapter; the course does not promise compatibility with other releases.

The result feels like a classic OpenGL graphics course, even though Vulkan runs underneath it.

## What you write, and what is handled for you

| Handled for you | Yours to write |
| --- | --- |
| Instance, physical-device selection, logical device, queues | Both shader stages, in GLSL |
| Window, surface, swapchain, image acquisition, presentation | Pipeline state: topology, vertex layout, depth, and culling |
| Per-frame semaphores and fences, frame pacing, resize recovery | Vertex and index data, and the GPU buffers holding it |
| Command-pool and command-buffer allocation | Command recording: passes, binds, draws |
| Depth-image allocation, screenshot and video capture | Textures: staging, layout transitions, samplers, descriptors |
| GPU memory allocation | Matrices, camera, and lighting math |

Nothing in the left column is hidden from you. Each graphics chapter includes an **Under the hood** aside that explains what raw Vulkan would require at that point. The running raw-Vulkan line figures are approximate comparisons of the API surface, not measurements of one reference implementation. By the end, you will know what a swapchain and a fence are for without having written either one by hand.

## What you need

- A C11 compiler and CMake 3.21 or newer.
- A Vulkan-capable GPU with a working driver.
- Datoviz v0.4 installed: chapter 1 walks through it.
- No Vulkan experience. No graphics experience beyond knowing what a pixel is.

Some linear algebra helps from chapter 9 onward, but the course derives the matrices it uses.

## How the chapters work

You keep one file, `main.c`, open in your editor for the whole course. Each chapter adds to it. From chapter 5 onward, you also have `shader.vert` and `shader.frag`, which you can edit while the program runs.

Every chapter ends with a complete listing of the file as it should look at that point. If something breaks, you can resynchronize with one paste. Along the way:

- **Try it**: small experiments with a predicted result. Do them; they are where the understanding settles.
- **Under the hood**: the raw Vulkan you just avoided.
- **When it goes wrong**: the symptom you are most likely to hit in that chapter, and its cause.

## The chapters

**Part 1: A window and a frame**

1. [Setup](01-setup.md): an empty program that links Datoviz, and a build you trust.
2. [Your first window](02-window.md): a window, a GPU, a render loop, and a color of your choosing.
3. [How a frame works](03-frame.md): what the CPU records, what the GPU executes, and when.

**Part 2: Triangles, shaders, pipeline, and vertex data**

4. [Your first triangle](04-triangle.md): shaders, a pipeline, and a draw call.
5. [Shaders in their own files](05-shader-files.md): SPIR-V, compiler errors, and live reloading.
6. [Vertex buffers](06-vertex-buffers.md): geometry from your own memory.
7. [Index buffers](07-index-buffers.md): the same shape with fewer vertices.

**Part 3: Into 3D**

8. [Push constants](08-push-constants.md): getting changing values into a shader.
9. [Matrices and perspective](09-matrices.md): model, view, projection, and a cube that looks wrong.
10. [Depth and culling](10-depth-culling.md): why it looked wrong, and how to fix it.
11. [Mouse control](11-mouse-control.md): an arcball camera.

**Part 4: Surfaces, textures, and light**

12. [Uploading a texture](12-texture-upload.md): staging buffers, image layouts, and barriers.
13. [Sampling the texture](13-texture-sampling.md): samplers, descriptors, and UV coordinates.
14. [Lighting](14-lighting.md): normals, diffuse, and specular.
15. [A real mesh](15-mesh.md): from a hand-typed cube to generated geometry.

The course closes with [what Datoviz handled for you and where to go next](16-next.md).

Start with [Setup](01-setup.md).
