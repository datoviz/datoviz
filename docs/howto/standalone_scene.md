# Standalone examples

The repository contains in `examples/` a few code examples showing how to make standalone C applications that use Datoviz. They are proposed here too for your convenience.



## Standalone application using the scene API

This example shows how to make a scatter plot using the C scene API.

<!-- CODE_C examples/standalone_scene.c -->



## Standalone application using the canvas API

This example shows how to render a triangle using only the canvas API and vklite, without using existing graphics, visuals, panels, and so on. It shows the following processes:

* Creating a graphics with custom shaders.
* Creating a function callback for command buffer refill.
* Creating a vertex buffer manually.

<!-- CODE_C examples/standalone_canvas.c -->



## Standalone application using the vklite API

This example shows how to make an offscreen rendering of a triangle using *only* the vklite API, and no other Datoviz abstraction. It shows the following processes:

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

<!-- CODE_C examples/standalone_vklite.c -->
