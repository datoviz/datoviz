# 3. Vertex Data and GPU Buffers

The third result looks identical to the first triangle, but the geometry now takes the normal path
instead of chapter 1's teaching shortcut: positions and colors begin in a C array, get uploaded
into a GPU buffer your renderer owns, and reach the vertex shader through declared attributes
instead of hardcoded shader constants.

![The triangle reproduced from interleaved positions and colors stored in a GPU vertex buffer](../../assets/tutorials/vulkan/vertex-buffers.webp)

## Run it

```console
./build/gpu-tutorial/vertex_buffers --live
```

Or validate it offscreen:

```console
./build/gpu-tutorial/vertex_buffers --offscreen --frames 3 --validate --png vertex-buffers.png
```

## The shader side: attributes instead of hardcoded arrays

Compare this chapter's vertex shader, [`shaders/vertex_buffer/vklite_triangle.vert`](https://github.com/datoviz/datoviz/blob/v0.4-dev/examples/c/tutorial/shaders/vertex_buffer/vklite_triangle.vert), to chapter 1's:

```glsl
#version 450

layout(location = 0) in vec2 position;
layout(location = 1) in vec3 color;
layout(location = 0) out vec3 vertex_color;

void main()
{
    gl_Position = vec4(position, 0.0, 1.0);
    vertex_color = color;
}
```

Chapter 1's shader *contained* the geometry: `const` arrays indexed by `gl_VertexIndex`. This one
*receives* it: `layout(location = 0) in vec2 position` and `layout(location = 1) in vec3 color` are
now shader **inputs** — values that arrive from outside, one pair per vertex, supplied by whatever
the C side binds before the draw call. The fragment shader is unchanged from chapter 1. The rest of
this chapter is about how those two `in` values get filled from CPU-side data.

## CPU-side vertices

The renderer defines one record per vertex:

```c
typedef struct
{
    float position[2];
    float color[3];
} SpikeVertex;

static const SpikeVertex TRIANGLE_VERTICES[3] = {
    {{0.00f, -0.65f}, {1.0f, 0.2f, 0.2f}},
    {{0.65f, +0.65f}, {0.2f, 1.0f, 0.2f}},
    {{-0.65f, +0.65f}, {0.2f, 0.4f, 1.0f}},
};
```

These are the exact same three positions and colors that lived inside chapter 1's vertex shader —
only their location changed, from shader constants to a plain C array. `TRIANGLE_VERTICES` is
ordinary read-only CPU memory; nothing here is GPU-visible yet. `sizeof(TRIANGLE_VERTICES)` is the
byte count for all three records, while `offsetof(SpikeVertex, position)` and
`offsetof(SpikeVertex, color)` describe where each field begins inside one record — both are
needed below to describe the layout to the GPU.

## Create and upload the GPU buffer

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

A GPU buffer needs to be told, upfront, both *what for* and *where it lives*:

- **`dvz_buffer_usage(..., VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)`** declares intended purpose — "this
  will be bound as vertex input," as opposed to, say, an index buffer or a staging buffer for a
  texture. Vulkan requires this upfront so the driver can place and optimize the buffer for that
  specific use rather than a generic one.
- **`dvz_buffer_flags(..., DVZ_ALLOC_MAPPED | DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE)`** picks
  *where* the buffer's memory lives, which is the real subject of this chapter. A GPU has (at
  least) two kinds of memory: **device-local** memory, fast for the GPU to read but not directly
  writable from the CPU, and **host-visible** memory, which the CPU *can* write into directly, at
  some performance cost. These flags request host-visible, CPU-mapped memory — the simplest option,
  and the right one for a small buffer written once at startup. Uploading a large or
  frequently-changing buffer would instead normally go through a device-local buffer plus a
  temporary host-visible **staging buffer** to copy from — a pattern used by this same repo's
  `texture_upload_spike` example (see `_renderer_create_texture()` in
  [`triangle.c`](https://github.com/datoviz/datoviz/blob/v0.4-dev/examples/c/tutorial/triangle.c)),
  but not one this triangle needs.
- **`dvz_buffer_upload`** then does the actual CPU→GPU copy, into the memory just configured.

The wrapper and Vulkan buffer are owned by the renderer. The device and allocator are borrowed from
the GPU context and must outlive the buffer. The input array only needs to remain valid for the
duration of `dvz_buffer_upload()` — after that call returns, the GPU has its own copy of the data.

## Describe the records to the pipeline

A binding says how many bytes separate consecutive vertices in the buffer; attributes map fields
inside that record to shader input locations:

```c
dvz_graphics_vertex_binding(renderer->pipeline, 0, sizeof(SpikeVertex), VK_VERTEX_INPUT_RATE_VERTEX);
dvz_graphics_vertex_attr(renderer->pipeline, 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(SpikeVertex, position));
dvz_graphics_vertex_attr(renderer->pipeline, 0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SpikeVertex, color));
```

- The `0` in all three calls is a **binding index** — a numbered vertex-buffer input slot on the
  pipeline. There is only one here, bound to `SpikeVertex` records, but a pipeline can declare
  several bindings at once (positions from one buffer, an unrelated per-instance attribute from
  another, for instance) — not needed in this course, but worth knowing the `0` isn't decorative.
- `VK_VERTEX_INPUT_RATE_VERTEX` says this binding advances once per vertex (the alternative,
  `..._INSTANCE`, advances once per instance instead — relevant once a draw uses more than one
  instance).
- The two attribute calls' second argument (`0`, then `1`) is the shader `location` each attribute
  feeds — matching `layout(location = 0) in vec2 position` and `layout(location = 1) in vec3 color`
  above.
- `VK_FORMAT_R32G32_SFLOAT` and `VK_FORMAT_R32G32B32_SFLOAT` look like image pixel formats because
  they are the same enum Vulkan uses for images — reused here to describe one vertex attribute's
  layout. Read the name left to right: `R32G32` is two 32-bit channels (matching `vec2 position`),
  `R32G32B32` is three (matching `vec3 color`), and `SFLOAT` means each channel is a signed
  floating-point number. Whatever a shader declares as `vec2`/`vec3`/`vec4`, the matching format is
  `R32G32`/`R32G32B32`/`R32G32B32A32_SFLOAT`.

Format, location, offset, and stride must agree across the C description above and the GLSL
declarations, or the GPU will read the wrong bytes for a given field without necessarily failing —
the "Deliberate layout failure" below produces exactly that.

## Bind before drawing

Pipeline state describes how to *interpret* a future buffer; it does not choose *which* buffer. The
draw callback binds the owned buffer before issuing the same three-vertex draw from chapter 1:

```c
DvzSize offset = 0;
dvz_cmd_bind_vertex_buffers(commands, 0, 1, renderer->vertex_buffer, &offset);
dvz_cmd_draw(commands, 0, 3, 0, 1);
```

The `0` here is the same binding index declared on the pipeline above — this is what connects "the
buffer bound at binding 0" to "the layout described for binding 0." `offset` lets you point at data
partway through a larger buffer; it's `0` because the three records start at the very beginning.

The GPU later fetches three records from the bound buffer, runs the vertex shader three times (one
per record), assembles and rasterizes one triangle, interpolates the colors, and runs the fragment
shader for every covered pixel — the same sequence as chapter 1, only the source of `position` and
`color` has moved from shader constants to buffer memory.

## Deliberate layout failure

Change the color attribute format in C from `VK_FORMAT_R32G32B32_SFLOAT` to
`VK_FORMAT_R32G32_SFLOAT`. The third color component is no longer supplied as intended, so the
visible interpolation changes even though the buffer bytes and shader source did not — the GPU
still reads real bytes, just the wrong number of them, so nothing about this necessarily produces
an error. Restore the three-component format afterward.

## Checkpoint

You should be able to trace a position from the C array, through the upload, into GPU buffer
memory, through the binding and attribute description (including what the binding index and
`VK_FORMAT_*` naming mean), into the vertex shader's `in` inputs, through `gl_Position`, the
rasterizer, and the final fragment. You should also be able to explain the difference between
host-visible/mapped memory and device-local memory, and why this chapter's small, one-time upload
picks the former.

## Exercise

Add a fourth vertex and draw two triangles forming a rectangle. Start without an index buffer:
duplicate the shared corners so `dvz_cmd_draw()` consumes six vertices. Indexed geometry comes
later in the full course.

## Complete source

The compiled source of truth is [`triangle.c`](https://github.com/datoviz/datoviz/blob/v0.4-dev/examples/c/tutorial/triangle.c), with chapter-specific shaders in [`shaders/vertex_buffer/`](https://github.com/datoviz/datoviz/tree/v0.4-dev/examples/c/tutorial/shaders/vertex_buffer) and the standalone build in [`CMakeLists.txt`](https://github.com/datoviz/datoviz/blob/v0.4-dev/examples/c/tutorial/CMakeLists.txt).

The pilot ends here. The next course checkpoint introduces changing per-frame state and transforms before moving to indexed 3D geometry and depth.
