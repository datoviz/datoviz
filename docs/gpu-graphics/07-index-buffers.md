# 7. Index buffers

*About 409 lines of C; raw Vulkan would need about 1350.*

![The same colored square drawn with four vertices and six indices.](../assets/gpu-graphics/07-index-buffers.webp)

The chapter 6 square stores six vertex records for only four distinct corners. An **index buffer** fixes that: it is an integer array that tells primitive assembly which vertex records to fetch, and in what order. The records stay unique; the indices describe the triangles. The image is identical to chapter 6.

## Four vertices, six references

Replace the six-record `VERTICES` array with these four corners:

```c
static const Vertex VERTICES[4] = {
    {{-0.65f, -0.65f}, {1.0f, 0.2f, 0.2f}},
    {{ 0.65f, -0.65f}, {0.2f, 1.0f, 0.2f}},
    {{ 0.65f,  0.65f}, {0.2f, 0.3f, 1.0f}},
    {{-0.65f,  0.65f}, {1.0f, 0.8f, 0.2f}},
};
```

Then add the index array:

```c
static const uint16_t INDICES[6] = {0, 1, 2, 0, 2, 3};
```

The first three indices form one triangle, the last three the other; corners 0 and 2 are reused. An index selects the complete record, not just its position, so a reused vertex also reuses its color and, later, its texture coordinates and normal. When two faces meet at one position but need different attributes, as at texture seams and sharp lighting creases, duplicate the vertex on purpose.

`uint16_t` covers indices up to 65535 and halves index bandwidth compared with `uint32_t`; larger meshes need 32-bit indices. The C element type, the bound `VkIndexType`, and the uploaded byte count must agree.

## Create the second buffer

Add `DvzBuffer* index_buffer;` to `Renderer`. Allocate its wrapper beside the vertex-buffer wrapper and check both:

```c
    renderer.vertex_buffer = dvz_buffer_create_wrapper();
    renderer.index_buffer = dvz_buffer_create_wrapper();
    COURSE_CHECK(
        renderer.vertex_buffer != NULL && renderer.index_buffer != NULL,
        "buffer allocation failed");
```

After uploading the vertices, create and upload the index buffer. Only the usage flag differs from the vertex buffer:

```c
    dvz_buffer(renderer.device, dvz_gpu_ctx_alloc(gpu), renderer.index_buffer);
    dvz_buffer_size(renderer.index_buffer, sizeof(INDICES));
    dvz_buffer_usage(renderer.index_buffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    dvz_buffer_flags(
        renderer.index_buffer, DVZ_ALLOC_MAPPED | DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE);
    int index_buffer_result = dvz_buffer_create(renderer.index_buffer);
    COURSE_CHECK(index_buffer_result == 0, "index buffer creation failed");
    dvz_buffer_upload(renderer.index_buffer, 0, sizeof(INDICES), INDICES);
```

## Indexed drawing

```c
    dvz_cmd_bind_index_buffer(
        renderer->commands, renderer->index_buffer, 0, VK_INDEX_TYPE_UINT16);
    dvz_cmd_draw_indexed(renderer->commands, 0, 0, 6, 0, 1);
```

The arguments to `dvz_cmd_draw_indexed()` are first index, vertex offset, index count, first instance, and instance count. `VK_INDEX_TYPE_UINT16` tells Vulkan to read each entry as an unsigned 16-bit value. Each value selects a record from the bound vertex buffer, after adding the vertex offset (zero here).

In cleanup, after the device wait, release the index buffer before the vertex buffer:

```c
    if (renderer.index_buffer != NULL)
        dvz_buffer_destroy(renderer.index_buffer);
    dvz_buffer_free(renderer.index_buffer);
```

## Build and run

Build and run as before; the capture goes to `chapter07.png`.

=== "Linux and macOS"

    ```sh
    cmake --build build
    ./build/vkcourse
    ./build/vkcourse --png chapter07.png
    ```

=== "Windows (Visual Studio)"

    ```powershell
    cmake --build build --config Release
    .\build\Release\vkcourse.exe
    .\build\Release\vkcourse.exe --png chapter07.png
    ```

`chapter07.png` matches chapter 6, and the terminal ends with `validation errors: 0`.

!!! tip "Try it"

    1. Reverse each triangle to `{2, 1, 0, 3, 2, 0}`. The image does not change, because face culling is still off; chapter 10 turns it on.
    2. Change the index count from 6 to 3. One triangle remains.
    3. Change the `INDICES` type to `uint32_t` but keep `VK_INDEX_TYPE_UINT16`. The square vanishes: Vulkan reads each 32-bit index as two 16-bit values, so the first six values, `0, 0, 1, 0, 2, 0`, make two degenerate triangles. Change the binding to `VK_INDEX_TYPE_UINT32` and the square returns. Restore both lines afterward.

??? info "Under the hood: index buffers"

    Raw Vulkan needs a second buffer and allocation like chapter 6's, then `vkCmdBindIndexBuffer` and `vkCmdDrawIndexed`. Indexing never searches for duplicate positions; you supply the reuse explicitly. The GPU follows the index stream and may reuse recent vertex-shader results through a hardware cache.

!!! warning "When it goes wrong"

    Spikes or missing triangles often mean an index exceeds the vertex count. A scrambled mesh suggests the bound index type disagrees with the array's element type.

## Checkpoint

1. How do six indices describe a square using four vertex records?
2. When does a mesh need 32-bit indices?
3. Why might two faces deliberately duplicate a vertex at the same position?

??? example "Your `main.c` after chapter 7"

    ```c
    --8<-- "examples/c/vulkan/step07.c"
    ```

??? example "Your `shader.vert` after chapter 7"

    ```glsl
    --8<-- "examples/c/vulkan/step07/shader.vert"
    ```

??? example "Your `shader.frag` after chapter 7"

    ```glsl
    --8<-- "examples/c/vulkan/step07/shader.frag"
    ```

Next: [Push constants](08-push-constants.md).
