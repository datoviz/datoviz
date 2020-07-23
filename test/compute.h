#include <visky/visky.h>

BEGIN_INCL_NO_WARN
#include <stb_image.h>
END_INCL_NO_WARN



static void compute_image(VkyPanel* panel)
{
    const uint32_t IMAGE_SIZE = 512;
    VkyScene* scene = panel->scene;
    VkyCanvas* canvas = panel->scene->canvas;

    vky_set_panel_aspect_ratio(panel, 1);

    // Create the visual.
    VkyTextureParams tex_params = {
        IMAGE_SIZE,
        IMAGE_SIZE,
        1,
        4,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FILTER_NEAREST,
        VK_SAMPLER_ADDRESS_MODE_REPEAT,
        VK_IMAGE_LAYOUT_GENERAL,
        true};
    VkyTexture* texture = vky_add_texture(canvas->gpu, tex_params);
    VkyVisual* visual =
        vky_visual_mesh(scene, VKY_MESH_COLOR_UV, VKY_MESH_SHADING_NONE, 0, texture);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Square.
    float x = 1.0f;
    // TODO: function to convert vec2 uv => u16vec4
    VkyMeshVertex vertices[] = {
        {{-x, -x, 0}, {0, 0, 0}, {0, 0, 255, 255}},
        {{+x, -x, 0}, {0, 0, 0}, {255, 255, 255, 255}},
        {{-x, +x, 0}, {0, 0, 0}, {0, 0, 0, 0}},
        {{-x, +x, 0}, {0, 0, 0}, {0, 0, 0, 0}},
        {{+x, -x, 0}, {0, 0, 0}, {255, 255, 255, 255}},
        {{+x, +x, 0}, {0, 0, 0}, {255, 255, 0, 0}},
    };
    vky_visual_upload(visual, (VkyData){0, NULL, 6, vertices, 0, NULL});

    // Color texture.
    int width, height, depth;
    char path[1024];
    snprintf(path, sizeof(path), "%s/textures/%s", DATA_DIR, "image.png");
    stbi_uc* pixels = stbi_load(path, &width, &height, &depth, STBI_rgb_alpha);
    if (pixels == NULL)
    {
        log_error("unable to load %s", path);
    }
    vky_upload_texture(texture, pixels);
    stbi_image_free(pixels);

    // Compute pipeline.
    VkyComputePipeline* cpipeline = calloc(1, sizeof(VkyComputePipeline));
    canvas->compute_pipeline = cpipeline;
    VkyResourceLayout resource_layout = vky_create_resource_layout(canvas->gpu, 1);
    vky_add_resource_binding(&resource_layout, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    *cpipeline = vky_create_compute_pipeline(canvas->gpu, "compute_tex.comp.spv", resource_layout);
    VkyTexture* resources[] = {texture};
    vky_bind_resources(cpipeline->resource_layout, cpipeline->descriptor_sets, (void**)resources);
    vky_begin_compute(canvas->gpu);
    vky_compute_acquire(
        cpipeline, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, texture,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    vky_compute(cpipeline, IMAGE_SIZE, IMAGE_SIZE, 1);
    vky_compute_release(
        cpipeline, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, texture,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    VkDescriptorType descriptor_types[] = {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE};
    vky_end_compute(canvas->gpu, 1, descriptor_types, (void**)resources);
}
