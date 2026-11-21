# 7. Index buffers

*About 505 lines of C; raw Vulkan would need about 1350.*

![The same colored square drawn with four vertices and six indices.](../assets/gpu-graphics/07-index-buffers.webp)

Chapter 6 moved the geometry into a vertex buffer: the square is six vertex records, read in threes as two triangles. But a square has only four corners. The two corners on the diagonal are stored twice, and the vertex shader runs on each copy.

For a square, that is two extra records. For a real mesh it is most of the data, because each corner is shared by several triangles. Chapter 9's cube has 8 corners but 12 triangles, which would be 36 records without sharing. We want to store each corner once and describe the triangles separately.

This chapter adds an **index buffer** for that. The image is identical to chapter 6, but the geometry is now stored the way every later mesh in the course is stored.

In this chapter you will:

1. shrink the vertex array to the four unique corners;
2. describe the two triangles as six indices into that array;
3. upload the indices into a second buffer;
4. bind it and replace the draw with an indexed draw;
5. release it with the vertex buffer.

## Vertices and the triangles that use them

An index buffer is an array of integers. Without one, a draw takes vertex records in order, three at a time. With one, it reads the indices in order, three at a time, and each index says which vertex record to fetch. The vertex records stay unique; the indices describe the triangles.

This saves both memory and work. An index here is 2 bytes, while a `Vertex` is 20. When the same index comes back soon, the GPU can often reuse the vertex shader's result instead of running it again.

## Four vertices, six references

Start with the data. Replace the six-record `VERTICES` array with these four corners, which go around the square in order:

```c
// The four unique corners, in order around the square.
static const Vertex VERTICES[4] = {
    {{-0.65f, -0.65f}, {1.0f, 0.2f, 0.2f}},
    {{ 0.65f, -0.65f}, {0.2f, 1.0f, 0.2f}},
    {{ 0.65f,  0.65f}, {0.2f, 0.3f, 1.0f}},
    {{-0.65f,  0.65f}, {1.0f, 0.8f, 0.2f}},
};
```

Then add the index array below it:

```c
// Two triangles as indices into VERTICES; corners 0 and 2 are shared.
static const uint16_t INDICES[6] = {0, 1, 2, 0, 2, 3};
```

The first three indices form one triangle, the last three the other; corners 0 and 2 are reused. An index selects the complete record, not just its position, so a reused vertex also reuses its color and, later, its texture coordinates and normal. When two faces meet at one position but need different attributes, as at texture seams and sharp lighting creases, duplicate the vertex on purpose.

The index type is a size trade-off. `uint16_t` covers indices up to 65535 and halves index bandwidth compared with `uint32_t`; larger meshes need 32-bit indices. The GPU cannot see the C type, so three things must agree:

- the C element type, here `uint16_t`;
- the `VkIndexType` you bind with, here `VK_INDEX_TYPE_UINT16`;
- the uploaded byte count, here `sizeof(INDICES)`, 12 bytes.

## Create the second buffer

The indices need GPU memory too, in a second buffer created exactly like chapter 6's. Add `DvzBuffer* index_buffer;` to `Renderer`, after `vertex_buffer`. In `main()`, allocate its wrapper beside the vertex-buffer wrapper, and replace the single check with one that covers both:

```c
    // Allocate both buffer wrappers. The renderer owns them, even if a later step fails.
    renderer.vertex_buffer = dvz_buffer_create_wrapper();
    renderer.index_buffer = dvz_buffer_create_wrapper();
    COURSE_CHECK(
        renderer.vertex_buffer != NULL && renderer.index_buffer != NULL,
        "buffer allocation failed");
```

The vertex buffer keeps chapter 6's configure, create and copy steps. With two buffer results in scope, rename its `buffer_result` to `vertex_buffer_result`.

Then, after uploading the vertices, create and upload the index buffer. Only the usage flag differs from the vertex buffer: `VK_BUFFER_USAGE_INDEX_BUFFER_BIT` declares that draws will read it as indices.

```c
    // Index buffer: the same steps with the index usage flag, then copy INDICES in.
    dvz_buffer(renderer.device, dvz_gpu_ctx_alloc(gpu), renderer.index_buffer);
    dvz_buffer_size(renderer.index_buffer, sizeof(INDICES));
    dvz_buffer_usage(renderer.index_buffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    dvz_buffer_flags(
        renderer.index_buffer, DVZ_ALLOC_MAPPED | DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE);
    int index_buffer_result = dvz_buffer_create(renderer.index_buffer);
    COURSE_CHECK(index_buffer_result == 0, "index buffer creation failed");
    dvz_buffer_upload(renderer.index_buffer, 0, sizeof(INDICES), INDICES);
```

Chapter 6's copy rule applies again: the upload copies the 12 bytes of `INDICES` before it returns, while the `DvzBuffer` must live until the last draw that reads it has finished. Both buffers exist before the draw callback is registered, so no frame ever records with a missing buffer.

## Bind the indices and draw

The vertex buffer stays bound as in chapter 6, and the index buffer joins it. In `draw()`, after binding the vertex buffer, bind the index buffer and replace `dvz_cmd_draw()` with its indexed form:

```c
    // Read unsigned 16-bit indices from the index buffer, from byte 0.
    dvz_cmd_bind_index_buffer(
        renderer->commands, renderer->index_buffer, 0, VK_INDEX_TYPE_UINT16);

    // First index 0, vertex offset 0, 6 indices, first instance 0, 1 instance.
    dvz_cmd_draw_indexed(renderer->commands, 0, 0, 6, 0, 1);
```

Read the bind call as: read this buffer from byte offset 0, as unsigned 16-bit indices. Only one index buffer is bound at a time, so there is no binding number. `VK_INDEX_TYPE_UINT16` is the second of the three things that must agree; it tells the GPU to step through the buffer 2 bytes at a time.

The arguments to `dvz_cmd_draw_indexed()` are first index, vertex offset, index count, first instance, and instance count. So this draw reads 6 indices starting at index 0, one instance. Each value selects a record from the bound vertex buffer, after adding the vertex offset (zero here).

## Release both buffers

The index buffer follows the same lifetime rule as the vertex buffer. In cleanup, after the device wait, release it before the vertex buffer:

```c
    // Destroy both buffers, then free their wrappers. A NULL wrapper was never allocated.
    if (renderer.index_buffer != NULL)
        dvz_buffer_destroy(renderer.index_buffer);
    dvz_buffer_free(renderer.index_buffer);
```

Chapter 6's vertex-buffer lines follow unchanged, then `destroy_pipeline(&renderer)`. The single device wait at the top of `cleanup:` covers both buffers.

## Build and run

Build and run as before; the capture goes to `chapter07.png`.

=== "Linux"

    ```sh
    cmake --build build
    ./build/vkcourse
    ./build/vkcourse --png chapter07.png
    ```

=== "macOS"

    ```sh
    cmake --build build
    ./build/vkcourse
    ./build/vkcourse --png chapter07.png
    ```

=== "Windows"

    In a Developer PowerShell for Visual Studio, with the `PATH` setup from chapter 1:

    ```powershell
    cmake --build build --config Release
    .\build\Release\vkcourse.exe
    .\build\Release\vkcourse.exe --png chapter07.png
    ```

`chapter07.png` matches chapter 6, and the terminal ends with `validation errors: 0`.

??? tip "Try it"

    1. Reverse each triangle to `{2, 1, 0, 3, 2, 0}`. The image does not change, because face culling is still off; chapter 10 turns it on.
    2. Change the index count from 6 to 3. One triangle remains.
    3. Change the `INDICES` type to `uint32_t` but keep `VK_INDEX_TYPE_UINT16`. The square vanishes: Vulkan reads each 32-bit index as two 16-bit values, so the first six values, `0, 0, 1, 0, 2, 0`, make two degenerate triangles. Change the binding to `VK_INDEX_TYPE_UINT32` and the square returns. Restore both lines afterward.

??? info "Under the hood: index buffers"

    Raw Vulkan needs a second buffer and allocation like chapter 6's, then `vkCmdBindIndexBuffer` and `vkCmdDrawIndexed`. Indexing never searches for duplicate positions; you supply the reuse explicitly. The GPU follows the index stream and may reuse recent vertex-shader results through a hardware cache.

??? warning "When it goes wrong"

    Spikes or missing triangles often mean an index exceeds the vertex count. A scrambled mesh suggests the bound index type disagrees with the array's element type.

## What carries over

- An indexed mesh, unique vertices plus triangles that refer to them, is the standard form in every GPU API and in mesh formats such as glTF. OBJ indexes each attribute separately, so loaders convert it (chapter 16).
- An index selects a whole vertex, not a position. Share a vertex only when every attribute agrees; duplicate it where they differ.
- Index width trades range against size: 16 bits suit meshes with at most 65536 vertices, 32 bits the rest.

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
