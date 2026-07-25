# 3. Vertex Data and GPU Buffers

The third result looks like the first triangle, but its positions and colors now begin in a C array, are uploaded into an owned GPU buffer, and enter the vertex shader through declared attributes.

## Run it

```console
./build/gpu-tutorial/vertex_buffers --live
```

Or validate it offscreen:

```console
./build/gpu-tutorial/vertex_buffers --offscreen --frames 3 --validate --png vertex-buffers.png
```

## CPU-side vertices

The complete renderer defines one record per vertex:

```c
typedef struct
{
    float position[2];
    float color[3];
} SpikeVertex;
```

`TRIANGLE_VERTICES` is ordinary read-only CPU memory. `sizeof(TRIANGLE_VERTICES)` is the byte count for all three records, while `offsetof(SpikeVertex, position)` and `offsetof(SpikeVertex, color)` describe where each field begins inside one record.

## Create and upload the GPU buffer

The renderer creates a host-visible vertex buffer and uploads the array:

```c
renderer->vertex_buffer = dvz_buffer_create_wrapper();
dvz_buffer(device, allocator, renderer->vertex_buffer);
dvz_buffer_size(renderer->vertex_buffer, sizeof(TRIANGLE_VERTICES));
dvz_buffer_usage(renderer->vertex_buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
dvz_buffer_flags(renderer->vertex_buffer, DVZ_ALLOC_MAPPED | DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE);
if (dvz_buffer_create(renderer->vertex_buffer) != 0)
    return -1;
dvz_buffer_upload(renderer->vertex_buffer, 0, sizeof(TRIANGLE_VERTICES), TRIANGLE_VERTICES);
```

The wrapper and Vulkan buffer are owned by the renderer. The device and allocator are borrowed from the GPU context and must outlive the buffer. The input array only needs to remain valid for the duration of `dvz_buffer_upload()`.

## Describe the records to the pipeline

A binding says how many bytes separate consecutive vertices. Attributes map fields in that record to shader input locations:

```c
dvz_graphics_vertex_binding(renderer->pipeline, 0, sizeof(SpikeVertex), VK_VERTEX_INPUT_RATE_VERTEX);
dvz_graphics_vertex_attr(renderer->pipeline, 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(SpikeVertex, position));
dvz_graphics_vertex_attr(renderer->pipeline, 0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SpikeVertex, color));
```

The matching vertex shader declares `location = 0` as a two-float position and `location = 1` as a three-float color. Format, location, offset, and stride must agree across C and GLSL.

## Bind before drawing

Pipeline state describes how to interpret a future buffer; it does not choose the buffer. The draw callback binds the owned buffer before issuing the same three-vertex draw:

```c
DvzSize offset = 0;
dvz_cmd_bind_vertex_buffers(commands, 0, 1, renderer->vertex_buffer, &offset);
dvz_cmd_draw(commands, 0, 3, 0, 1);
```

The GPU later fetches three records, runs the vertex shader three times, rasterizes one triangle, interpolates the colors, and runs the fragment shader for covered pixels.

## Deliberate layout failure

Change the color attribute format in C from `VK_FORMAT_R32G32B32_SFLOAT` to `VK_FORMAT_R32G32_SFLOAT`. The third color component is no longer supplied as intended, so the visible interpolation changes even though the buffer bytes and shader source did not. Restore the three-component format afterward.

## Checkpoint

You should be able to trace a position from the C array, through the upload, GPU buffer, binding, attribute description, vertex shader input, clip-space output, rasterizer, and final fragment.

## Exercise

Add a fourth vertex and draw two triangles forming a rectangle. Start without an index buffer: duplicate the shared corners so `dvz_cmd_draw()` consumes six vertices. Indexed geometry comes later in the full course.

## Complete source

The compiled source of truth is [`triangle.c`](https://github.com/datoviz/datoviz/blob/v0.4-dev/examples/c/tutorial/triangle.c), with chapter-specific shaders in [`shaders/vertex_buffer/`](https://github.com/datoviz/datoviz/tree/v0.4-dev/examples/c/tutorial/shaders/vertex_buffer) and the standalone build in [`CMakeLists.txt`](https://github.com/datoviz/datoviz/blob/v0.4-dev/examples/c/tutorial/CMakeLists.txt).

The pilot ends here. The next course checkpoint introduces changing per-frame state and transforms before moving to indexed 3D geometry and depth.
