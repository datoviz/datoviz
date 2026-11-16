# 14. Sampling the texture

*About 723 lines of C; raw Vulkan would need about 2050.*

![A checkerboard covers each face of the mouse-controlled cube.](../assets/gpu-graphics/14-texture-sampling.webp)

Chapter 13 put the checkerboard on the GPU. Now the cube shows it on every face. Drawing a texture takes four more pieces: UV (texture) coordinates on each vertex, an image view, a sampler, and a descriptor that hands the view and sampler to the fragment shader. The texture joins the material in descriptor set 0, at descriptor binding 1.

## Give each face its own UVs

An index selects a whole vertex, UV included. Eight shared corners cannot give six faces their own square UV maps, so split them: four vertices per face, 24 in total, still drawn as 12 triangles. Sharing positions is harmless; sharing a vertex with conflicting UVs is not.

The texture replaces the vertex colors, so replace `Vertex` with:

```c
typedef struct
{
    float position[3];
    float uv[2];
} Vertex;
```

Replace `VERTICES` and `INDICES`. Each face runs `(0, 0)`, `(1, 0)`, `(1, 1)`, `(0, 1)` and keeps chapter 10's outward winding:

```c
static const Vertex VERTICES[24] = {
    // Back face.
    {{0.65f, -0.65f, -0.65f}, {0.0f, 0.0f}},
    {{-0.65f, -0.65f, -0.65f}, {1.0f, 0.0f}},
    {{-0.65f, 0.65f, -0.65f}, {1.0f, 1.0f}},
    {{0.65f, 0.65f, -0.65f}, {0.0f, 1.0f}},
    // Front face.
    {{-0.65f, -0.65f, 0.65f}, {0.0f, 0.0f}},
    {{0.65f, -0.65f, 0.65f}, {1.0f, 0.0f}},
    {{0.65f, 0.65f, 0.65f}, {1.0f, 1.0f}},
    {{-0.65f, 0.65f, 0.65f}, {0.0f, 1.0f}},
    // Bottom face.
    {{-0.65f, -0.65f, -0.65f}, {0.0f, 0.0f}},
    {{0.65f, -0.65f, -0.65f}, {1.0f, 0.0f}},
    {{0.65f, -0.65f, 0.65f}, {1.0f, 1.0f}},
    {{-0.65f, -0.65f, 0.65f}, {0.0f, 1.0f}},
    // Top face.
    {{-0.65f, 0.65f, -0.65f}, {0.0f, 0.0f}},
    {{-0.65f, 0.65f, 0.65f}, {1.0f, 0.0f}},
    {{0.65f, 0.65f, 0.65f}, {1.0f, 1.0f}},
    {{0.65f, 0.65f, -0.65f}, {0.0f, 1.0f}},
    // Right face.
    {{0.65f, -0.65f, 0.65f}, {0.0f, 0.0f}},
    {{0.65f, -0.65f, -0.65f}, {1.0f, 0.0f}},
    {{0.65f, 0.65f, -0.65f}, {1.0f, 1.0f}},
    {{0.65f, 0.65f, 0.65f}, {0.0f, 1.0f}},
    // Left face.
    {{-0.65f, -0.65f, -0.65f}, {0.0f, 0.0f}},
    {{-0.65f, -0.65f, 0.65f}, {1.0f, 0.0f}},
    {{-0.65f, 0.65f, 0.65f}, {1.0f, 1.0f}},
    {{-0.65f, 0.65f, -0.65f}, {0.0f, 1.0f}},
};

// Each face winds counter-clockwise when seen from outside the cube.
static const uint16_t INDICES[36] = {
    0,  1,  2,  0,  2,  3,  4,  5,  6,  4,  6,  7,  8,  9,  10, 8,  10, 11,
    12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23,
};
```

The sizes and the 36-index draw adapt automatically. In `create_pipeline()`, replace the color attribute with the UV attribute:

```c
    dvz_graphics_vertex_attr(
        renderer->pipeline, 0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv));
```

Replace `shader.vert` with:

```glsl
#version 450

layout(push_constant) uniform Push { mat4 mvp; } push_data;
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 0) out vec2 uv;

void main()
{
    gl_Position = push_data.mvp * vec4(in_position, 1.0);
    uv = in_uv;
}
```

Replace `shader.frag` with:

```glsl
#version 450

layout(set = 0, binding = 0) uniform Material { vec4 tint; } material;
layout(set = 0, binding = 1) uniform sampler2D tex;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 out_color;

void main()
{
    out_color = texture(tex, uv) * material.tint;
}
```

The rasterizer interpolates UVs across each triangle, correcting for perspective so the texture stays fixed to a receding surface. `texture(tex, uv)` reads the image at that coordinate using the sampler's rules.

UV `(0, 0)` reads texel row 0, column 0: the white tile from chapter 13. Each face puts `(0, 0)` on its first vertex, so every face shows the white tile in that corner. Vulkan has no notion of which way up an image is; `v = 0` is simply the first row you wrote. Image files usually store their top row first, so if a loaded picture appears upside down, sample at `1.0 - v` or swap the UVs.

Set the tint to white in `create_material()`, so the checkerboard shows its own colors:

```c
    Material material = {.tint = {1.0f, 1.0f, 1.0f, 1.0f}};
```

## Create a view and sampler

The view and sampler describe this one texture, and the descriptor needs both, so create them in `create_texture()` with the image. Reload then reuses them instead of rebuilding them. Add them as owned fields after `texture`:

```c
    DvzImageViews* texture_view;
    DvzSampler* sampler;
```

The view needs the uploaded image, so in `create_texture()` add this after the checked submit and before `dvz_commands_destroy(upload)`:

```c
    renderer->texture_view = dvz_image_views_create_wrapper();
    renderer->sampler = dvz_sampler_create_wrapper();
    if (renderer->texture_view == NULL || renderer->sampler == NULL)
        goto error;
    dvz_image_views(renderer->texture, renderer->texture_view);
    if (dvz_image_views_create(renderer->texture_view) != 0)
        goto error;
    dvz_sampler(renderer->device, renderer->sampler);
    dvz_sampler_min_filter(renderer->sampler, VK_FILTER_NEAREST);
    dvz_sampler_mag_filter(renderer->sampler, VK_FILTER_NEAREST);
    dvz_sampler_address_mode(
        renderer->sampler, DVZ_SAMPLER_AXIS_U, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    dvz_sampler_address_mode(
        renderer->sampler, DVZ_SAMPLER_AXIS_V, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    if (dvz_sampler_create(renderer->sampler) != 0)
        goto error;
```

An **image view** selects which part of an image a shader sees, and how to interpret its format; descriptors refer to views, not to images. A **sampler** holds the lookup rules, separately from the pixels. Nearest filtering picks one texel, keeping the checker edges crisp; repeat addressing wraps coordinates outside `[0, 1]`.

## Declare and write the descriptor

In `create_pipeline()`, declare descriptor binding 1 as a combined image sampler for the fragment stage, beside binding 0:

```c
    dvz_slots_binding(
        renderer->slots, 0, 0, 1, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    dvz_slots_binding(
        renderer->slots, 0, 1, 1, VK_SHADER_STAGE_FRAGMENT_BIT,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
```

A combined image sampler descriptor refers to both the view and the sampler, which remain separate objects. Write it beside the material entry when the descriptor set is created:

```c
    if (dvz_graphics_create(renderer->pipeline) != 0)
        return -1;
    renderer->descriptors = dvz_descriptors_create_wrapper();
    if (renderer->descriptors == NULL)
        return -1;
    dvz_descriptors(renderer->slots, renderer->descriptors);
    dvz_descriptors_image(
        renderer->descriptors, 0, 1, 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        dvz_image_views_handle(renderer->texture_view, 0), dvz_sampler_handle(renderer->sampler));
    dvz_descriptors_buffer(
        renderer->descriptors, 0, 0, 0, dvz_buffer_handle(renderer->material_buffer), 0,
        sizeof(Material));
    return dvz_descriptors_handle(renderer->descriptors, 0) != VK_NULL_HANDLE ? 0 : -1;
```

The descriptor now needs the view and sampler, so the pipeline must be created after the texture. Move the checked pipeline creation to just after `create_texture()`:

```c
    int texture_result = create_texture(&renderer);
    COURSE_CHECK(texture_result == 0, "texture upload failed");
    int pipeline_result = create_pipeline(&renderer);
    COURSE_CHECK(pipeline_result == 0, "graphics pipeline creation failed");
```

`draw()` needs no change: it already binds set 0, and binding 1 is part of it.

## Keep reload and cleanup safe

A reload candidate builds a new descriptor set, so it needs the view and sampler. Add them to its initializer:

```c
            Renderer candidate = {
                .device = renderer.device,
                .color_format = renderer.color_format,
                .material_buffer = renderer.material_buffer,
                .texture_view = renderer.texture_view,
                .sampler = renderer.sampler,
            };
```

The candidate borrows them and owns its pipeline, slots, shaders, and descriptor set; the rest of the reload path is unchanged from chapter 12. Reload covers shader calculations within the existing interface; changing locations, descriptor bindings, or the push-constant block also needs C edits and a rebuild.

The descriptor set must go before the view and sampler it names, and the view before its image. At `cleanup:`, after the material buffer and before `renderer.texture`, add:

```c
    if (renderer.sampler != NULL)
        dvz_sampler_destroy(renderer.sampler);
    dvz_sampler_free(renderer.sampler);
    if (renderer.texture_view != NULL)
        dvz_image_views_destroy(renderer.texture_view);
    dvz_image_views_free(renderer.texture_view);
```

## Build and run

Build and run as before; the capture goes to `chapter14.png`.

=== "Linux and macOS"

    ```sh
    cmake --build build
    ./build/vkcourse
    ./build/vkcourse --png chapter14.png
    ```

=== "Windows (Visual Studio)"

    ```powershell
    cmake --build build --config Release
    .\build\Release\vkcourse.exe
    .\build\Release\vkcourse.exe --png chapter14.png
    ```

Every face shows a square checkerboard with one white corner tile. The terminal ends with `validation errors: 0`.

!!! tip "Try it"

    1. Replace both `VK_FILTER_NEAREST` values with `VK_FILTER_LINEAR`, rebuild, and zoom in. The checker edges blur, because linear filtering blends neighboring texels.
    2. On the front face, change the U of its second and third vertices (`(1.0f, 0.0f)` and `(1.0f, 1.0f)`) to `2.0f`. That face shows the checkerboard twice. With `VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE` on the U axis, the right half instead smears the last texel column.
    3. In `shader.frag`, output `vec4(uv, 0.0, 1.0)` and press **R**. Each face shows a gradient from black at its white-tile corner to yellow at the opposite corner: you are looking at the UVs themselves.

??? info "Under the hood: descriptor layouts and sets"

    Raw Vulkan creates a `VkImageView` and a `VkSampler`, adds a combined-image-sampler entry to the descriptor set layout, and writes a `VkDescriptorImageInfo` with `vkUpdateDescriptorSets`. `DvzSlots` owns the layout; `DvzDescriptors` owns the set and borrows the slots. The device owns the pool. None of them owns the view or sampler.

!!! warning "When it goes wrong"

    A set/binding validation error means the GLSL declaration, the slots, or the descriptor write disagree. A stretched side face means it still shares corners with its neighbors. If **R** fails after the first frame worked, check that the candidate receives the view and sampler.

## Checkpoint

1. Why do eight positions become 24 vertices when each face needs its own UVs?
2. What do the image view, sampler, and descriptor each contribute?
3. Why must reload replace the descriptor set along with its slots?

??? example "Your `main.c` after chapter 14"

    ```c
    --8<-- "examples/c/vulkan/step14.c"
    ```

??? example "Your `shader.vert` after chapter 14"

    ```glsl
    --8<-- "examples/c/vulkan/step14/shader.vert"
    ```

??? example "Your `shader.frag` after chapter 14"

    ```glsl
    --8<-- "examples/c/vulkan/step14/shader.frag"
    ```

Next: [Lighting](15-lighting.md).
