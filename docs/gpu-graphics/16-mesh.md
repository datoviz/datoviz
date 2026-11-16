# 16. A real mesh

*About 774 lines of C; raw Vulkan would need about 2200.*

![The final generated sphere viewer, textured and lit.](../assets/gpu-graphics/16-mesh.webp)

A hand-typed cube only goes so far. In this final chapter you replace it with a generated sphere, or with a Wavefront OBJ file of your own, and the result is a complete textured, lit, mouse-controlled mesh viewer. A **mesh** is CPU-side data: vertex attributes plus the indices that assemble them into triangles. Vulkan has no mesh object; you convert the arrays to your `Vertex` format and upload them, and the shaders, descriptors, and push-constant block stay exactly as they are.

## Replace the cube arrays

Add this include:

```c
#include <datoviz/geom.h>
```

Delete `VERTICES` and `INDICES`; keep `Vertex`, `Push`, and both size assertions. A generated or loaded mesh knows its size only at run time, so the renderer must remember its index count. Add this field beside the buffers in `Renderer`:

```c
    uint32_t index_count;
```

Add this helper between `create_texture()` and `draw()`. It loads an OBJ file when given a path and generates a sphere otherwise:

```c


/**
 * Convert generated CPU geometry to the pipeline's float vertices and uint32 indices.
 * @param renderer Renderer that owns the uploaded GPU buffers.
 * @param obj_path Wavefront OBJ file to load, or NULL for the generated sphere.
 * @return Zero on success.
 */
static int create_mesh(Renderer* renderer, const char* obj_path)
{
    int result = -1;
    DvzGeometry* geometry = obj_path != NULL
                                ? dvz_geometry_obj(obj_path, NULL)
                                : dvz_geometry_sphere(&(DvzGeometrySphereDesc){
                                      DVZ_STRUCT_INIT_FIELDS(DvzGeometrySphereDesc),
                                      .radius = 0.9,
                                      .rings = 24,
                                      .sectors = 48});
    Vertex* vertices = NULL;
    size_t vertex_count = geometry != NULL ? geometry->vertex_count : 0;
    size_t index_count = geometry != NULL ? geometry->index_count : 0;
    if (geometry == NULL || geometry->positions == NULL || geometry->normals == NULL ||
        geometry->texcoords == NULL || geometry->indices == NULL || geometry->vertex_count == 0 ||
        geometry->index_count == 0 || vertex_count > SIZE_MAX / sizeof(*vertices) ||
        index_count > SIZE_MAX / sizeof(DvzIndex))
        goto cleanup;

    vertices = (Vertex*)calloc(vertex_count, sizeof(*vertices));
    if (vertices == NULL)
        goto cleanup;
    for (uint32_t i = 0; i < geometry->vertex_count; i++)
    {
        for (uint32_t j = 0; j < 3; j++)
        {
            vertices[i].position[j] = (float)geometry->positions[i][j];
            vertices[i].normal[j] = (float)geometry->normals[i][j];
        }
        vertices[i].uv[0] = (float)geometry->texcoords[i][0];
        vertices[i].uv[1] = (float)geometry->texcoords[i][1];
    }
    DvzSize vertex_bytes = (DvzSize)geometry->vertex_count * sizeof(Vertex);
    DvzSize index_bytes = (DvzSize)geometry->index_count * sizeof(DvzIndex);
    renderer->index_count = geometry->index_count;
    renderer->vertex_buffer = dvz_buffer_create_wrapper();
    renderer->index_buffer = dvz_buffer_create_wrapper();
    if (renderer->vertex_buffer == NULL || renderer->index_buffer == NULL)
        goto cleanup;
    dvz_buffer(renderer->device, renderer->allocator, renderer->vertex_buffer);
    dvz_buffer_size(renderer->vertex_buffer, vertex_bytes);
    dvz_buffer_usage(renderer->vertex_buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    dvz_buffer_flags(
        renderer->vertex_buffer, DVZ_ALLOC_MAPPED | DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE);
    dvz_buffer(renderer->device, renderer->allocator, renderer->index_buffer);
    dvz_buffer_size(renderer->index_buffer, index_bytes);
    dvz_buffer_usage(renderer->index_buffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    dvz_buffer_flags(
        renderer->index_buffer, DVZ_ALLOC_MAPPED | DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE);
    if (dvz_buffer_create(renderer->vertex_buffer) != 0 ||
        dvz_buffer_create(renderer->index_buffer) != 0)
        goto cleanup;
    dvz_buffer_upload(renderer->vertex_buffer, 0, vertex_bytes, vertices);
    dvz_buffer_upload(renderer->index_buffer, 0, index_bytes, geometry->indices);
    result = 0;

cleanup:
    free(vertices);
    dvz_geometry_destroy(geometry);
    return result;
}
```

Both routes return an owned `DvzGeometry` whose attributes are doubles. The pipeline expects 32-bit floats, so the helper narrows each component into a `Vertex` array sized to this mesh, after checking that the byte counts cannot overflow. Indices stay `DvzIndex`, a 32-bit unsigned type, and upload as they are. Once both uploads have copied their bytes, the converted array and the geometry are freed; the GPU buffers stay with `Renderer`.

## Change initialization and the draw count

Add an `--obj PATH` option. Declare its variable beside `png_path`:

```c
    const char* obj_path = NULL;
```

Add a branch to the argument loop:

```c
        else if (strcmp(argv[argument_index], "--obj") == 0)
            obj_path = argv[++argument_index];
```

In `main()`, replace the whole vertex- and index-buffer block, from `renderer.vertex_buffer = ...` through the index upload, with:

```c
    int mesh_result = create_mesh(&renderer, obj_path);
    COURSE_CHECK(mesh_result == 0, "mesh creation failed");
```

Geometry indices are 32-bit `DvzIndex` values, and the count comes from the mesh. In `draw()`, replace the index binding and the draw:

```c
    dvz_cmd_bind_index_buffer(renderer->commands, renderer->index_buffer, 0, VK_INDEX_TYPE_UINT32);
    dvz_cmd_draw_indexed(renderer->commands, 0, 0, renderer->index_count, 0, 1);
```

The listing also accepts `--width`, `--height`, and `--time`, used to render the documentation preview.

## Load your own OBJ

`dvz_geometry_obj()` reads `v`, `vt`, `vn`, and polygonal `f` records, triangulates polygons, and ignores materials and groups. It computes missing normals; missing UVs become `(0, 0)`, so the whole mesh samples one checker texel.

It returns `NULL` for an unreadable or unsupported file, and the program then exits cleanly with `mesh creation failed`. The camera expects a model near the origin, about one unit across. Anything far off-center or much larger will be clipped or invisible; check its bounds with `dvz_geometry_bounds()`.

=== "Linux and macOS"

    ```sh
    ./build/vkcourse --obj path/to/model.obj
    ./build/vkcourse --obj path/to/model.obj --png model.png
    ```

=== "Windows (Visual Studio)"

    ```powershell
    .\build\Release\vkcourse.exe --obj path\to\model.obj
    .\build\Release\vkcourse.exe --obj path\to\model.obj --png model.png
    ```

## Keep the sphere's smooth normals

The sphere generator supplies radial normals, each pointing away from the center. Along the texture seam, duplicated vertices share a position and a normal but differ in UV (U = 0 on one side, U = 1 on the other). That is the rule from chapter 7 in action: split a vertex where attributes differ, and nowhere else.

`dvz_geometry_compute_normals()` recomputes normals by averaging the triangles that share each index. It cannot know that two seam vertices are the same point, so recomputed normals can show a faint seam. For the sphere, keep the generator's normals.

## Build and run

Build and run as before; the capture goes to `chapter16.png`.

=== "Linux and macOS"

    ```sh
    cmake --build build
    ./build/vkcourse
    ./build/vkcourse --png chapter16.png
    ```

=== "Windows (Visual Studio)"

    ```powershell
    cmake --build build --config Release
    .\build\Release\vkcourse.exe
    .\build\Release\vkcourse.exe --png chapter16.png
    ```

You see a smooth sphere with a curved checkerboard and a specular highlight. The tiles shrink toward the poles, where a rectangular image wraps around a point. The terminal ends with `validation errors: 0`.

!!! tip "Try it"

    1. Lower `rings` to `8` and `sectors` to `16`, and rebuild. The silhouette turns visibly polygonal.
    2. Recompute the normals: insert this after the geometry checks in `create_mesh()` and rebuild. The capture looks unchanged, because averaged triangle normals closely match the radial ones. Drag the texture seam into view: each side of it averaged only its own triangles, so a faint lighting line can appear there.

        ```c
            DvzResult normal_result = dvz_geometry_compute_normals(geometry);
            if (normal_result != DVZ_OK)
                goto cleanup;
        ```

    3. Draw a torus: replace the sphere arm of the ternary in `create_mesh()` with the code below. The torus draws with the same pipeline, texture, and lighting; only the vertex and index counts change.

        ```c
                                        : dvz_geometry_torus(&(DvzGeometryTorusDesc){
                                              DVZ_STRUCT_INIT_FIELDS(DvzGeometryTorusDesc),
                                              .major_radius = 0.6,
                                              .minor_radius = 0.25,
                                              .rings = 24,
                                              .sectors = 24});
        ```

??? info "Under the hood: geometry is CPU data"

    Vulkan accepts buffers and draw commands; it has no sphere or mesh. A generator or loader fills arrays, you choose a GPU format, and the draw selects an index range. Changing the geometry leaves the descriptor layout, shaders, attachments, and synchronization untouched.

!!! warning "When it goes wrong"

    A garbled mesh means the index binding still says `UINT16` while the buffer holds 32-bit values. Missing triangles mean the draw count is still 36. A blank window with an OBJ usually means the model is off-center or far too large; check its bounds.

## Checkpoint

1. What had to change to draw generated geometry through the existing pipeline?
2. Why does this chapter switch to 32-bit indices?
3. What does normal recomputation average, and why can that matter at a UV seam?

??? example "Your `main.c` after chapter 16"

    ```c
    --8<-- "examples/c/vulkan/step16.c"
    ```

??? example "Your `shader.vert` after chapter 16"

    ```glsl
    --8<-- "examples/c/vulkan/step16/shader.vert"
    ```

??? example "Your `shader.frag` after chapter 16"

    ```glsl
    --8<-- "examples/c/vulkan/step16/shader.frag"
    ```

Next: [Where to go next](17-next.md).
