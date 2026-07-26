# Tutorials

Tutorials are result-first learning paths backed by complete, runnable examples.

## Modern GPU Graphics in Vulkan

Learn the execution of a modern Vulkan-backed GPU frame without beginning with raw Vulkan platform setup. Datoviz supplies the window, Canvas, frame acquisition, submission, presentation, and resize machinery; the tutorial uses the advanced `vklite` API to keep shaders, pipelines, buffers, commands, and ownership explicit.

The RC3 pilot contains three chapters:

1. [First live triangle](vulkan/first-triangle.md) — run a supplied program, inspect one frame, and change shader-generated positions and colors.
2. [Shaders and the graphics pipeline](vulkan/shaders-and-pipeline.md) — compile external GLSL at startup, observe interpolation, and connect shader stages to pipeline state.
3. [Vertex data and GPU buffers](vulkan/vertex-buffers.md) — move positions and colors from the shader into C data and an owned GPU buffer.

The pilot targets the Datoviz v0.4 release-candidate line. `vklite` is an advanced, unstable API: these chapters define a tested tutorial profile, not a general stability promise.
