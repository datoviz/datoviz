# 1. First Live Triangle

The first result is a colored triangle in a live, resizable window. Its three positions and colors initially live in the vertex shader, so no vertex buffer is needed yet.

## Run it

From the standalone tutorial build:

```console
./build/gpu-tutorial/first_triangle --live
```

For a bounded, deterministic check:

```console
./build/gpu-tutorial/first_triangle --offscreen --frames 3 --validate --png first-triangle.png
```

The program should report three submitted and three drawn frames, zero format mismatches, zero invalid frame contracts, and no creation failure.

## One frame

Canvas calls the renderer while a frame command buffer is already recording. The essential draw sequence is:

```c
dvz_commands_wrap_borrowed_recording(renderer->device, frame->command_buffer, commands);
dvz_cmd_rendering_default(commands, frame->image_view, frame->extent.width, frame->extent.height, clear, rendering);
dvz_cmd_rendering_begin(commands, rendering);
dvz_cmd_bind_graphics(commands, renderer->pipeline);
dvz_cmd_set_viewport_scissor(commands, frame->extent);
dvz_cmd_draw(commands, 0, 3, 0, 1);
dvz_cmd_rendering_end(commands);
dvz_commands_unwrap(commands);
```

The CPU records these commands now; the GPU executes them later after Canvas submits the frame.

| Value | Ownership | Validity |
| --- | --- | --- |
| `frame->command_buffer` | borrowed from Canvas | draw callback only |
| `frame->image_view` | borrowed from Canvas | current frame resource set |
| command and rendering wrappers | owned by the renderer | renderer lifetime |
| graphics pipeline and layout | owned by the renderer | until explicit destroy |

Wrapping does not transfer ownership. Unwrapping only detaches the borrowed handle; it does not end, reset, submit, or destroy the Vulkan command buffer.

## Where the triangle comes from

Open [`vklite_triangle.vert`](https://github.com/datoviz/datoviz/blob/v0.4-dev/examples/c/tutorial/shaders/vklite_triangle.vert). `gl_VertexIndex` is `0`, `1`, then `2` for the three vertices requested by `dvz_cmd_draw()`. The shader uses that index to select one position and one color.

Clip-space `x` and `y` coordinates normally span `-1` to `+1`. Change the first position from `(0.0, -0.65)` to `(0.0, -0.25)`, save the shader, and restart the executable. The triangle changes without recompiling C because the program reads and compiles the GLSL file at startup.

The fragment shader receives the smoothly interpolated color and writes one RGBA value for each covered pixel.

## Deliberate failure

Copy the shader directory, introduce an invalid token in the copied fragment shader, and run:

```console
./build/gpu-tutorial/first_triangle --offscreen --frames 1 --shader-dir /path/to/copied/shaders
```

Compilation should fail before drawing and name the copied file and source line. A missing provider, incompatible provider, missing file, invalid request, and GLSL compilation error have distinct statuses in the [runtime shader API](../../reference/c-api/runtime-shader.md).

## Checkpoint

You should be able to identify the pipeline bind, dynamic viewport and scissor, three-vertex draw, rendering scope, and borrowed command-buffer lifetime.

## Exercise

Turn the triangle into a narrow arrowhead by changing only the three shader positions. Predict the image before restarting, then explain why no CPU-side geometry changed.

Next: [Shaders and the graphics pipeline](shaders-and-pipeline.md).
