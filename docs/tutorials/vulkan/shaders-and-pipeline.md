# 2. Shaders and the Graphics Pipeline

This chapter keeps positions in the vertex shader but changes the shader interface and fragment calculation. The result has a luminous cyan edge produced entirely by editing external GLSL.

## Run the pipeline experiment

```console
./build/gpu-tutorial/shaders_and_pipeline --live
```

Or validate it offscreen:

```console
./build/gpu-tutorial/shaders_and_pipeline --offscreen --frames 3 --validate --png shader-pipeline.png
```

This executable uses the shaders in [`shaders/pipeline/`](https://github.com/datoviz/datoviz/tree/v0.4-dev/examples/c/tutorial/shaders/pipeline). It is compiled from the same C renderer as chapter one; only the external shader directory differs.

## From GLSL to pixels

The program reads each shader with `dvz_read_text()`, then sends a typed request to `dvz_shader_compile()`. Vertex and fragment requests use `DVZ_SHADER_PROFILE_GRAPHICS`, whose contract targets Vulkan 1.0 and SPIR-V 1.0. The returned diagnostics and SPIR-V belong to `DvzShaderCompileResult` until `dvz_shader_compile_result_destroy()` releases them.

The vertex shader emits a three-component barycentric coordinate: each corner starts with one component equal to one and the other two equal to zero. Rasterization interpolates these values across the triangle. The fragment shader finds the smallest component, which approaches zero near an edge, and uses `smoothstep()` to blend from cyan at the boundary to a pastel interior.

This is a useful GPU pattern: the vertex stage supplies values at vertices, fixed-function rasterization interpolates them, and the fragment stage turns the interpolated values into pixels.

## Pipeline state

The renderer creates one graphics pipeline after the Canvas color format is known. Its relevant state is:

- vertex and fragment shader modules;
- triangle-list primitive topology;
- one color attachment with the resolved Canvas format;
- an empty pipeline layout because these chapters use no descriptors or push constants;
- dynamic viewport and scissor state, set from the current frame extent.

A Vulkan graphics pipeline packages compatible shader interfaces and most fixed-function state. Changing shader source requires runtime compilation and pipeline recreation on the next process start, but does not require recompiling the C executable.

## Experiment

In the fragment shader, change `0.08` in the `smoothstep()` call:

- `0.02` produces a thin edge;
- `0.20` produces a broad glow;
- reversing the two threshold arguments deliberately produces a very different transition.

Restart after each edit and connect the visible edge width to the interpolated `nearest_edge` value.

## Deliberate interface failure

Change the vertex shader output location from `0` to `1` without changing the fragment input. Each shader may still compile because runtime compilation treats stages independently; with validation enabled, cross-stage compatibility is checked when the stages meet in the graphics pipeline. Restore matching locations before continuing.

## Checkpoint

You should be able to distinguish build-time `glslc`, runtime shaderc, GLSL source, SPIR-V, shader modules, the pipeline layout, the graphics pipeline, rasterization, and per-frame commands.

## Exercise

Use the three barycentric components to give each edge a different color. Keep the vertex shader unchanged and implement the complete effect in the fragment shader.

Next: [Vertex data and GPU buffers](vertex-buffers.md).
