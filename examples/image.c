#include "../include/visky/visky.h"

#define USE_PROPS_API 1

int main()
{
    log_set_level_env();

    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    VkyCanvas* canvas = vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_BLACK, 1, 1);
    VkyPanel* panel = vky_get_panel(scene, 0, 0);
    vky_set_controller(panel, VKY_CONTROLLER_PANZOOM, NULL);

    const uint32_t size = 128;
    VkyImageCmapParams params = {0};
    params.cmap = VKY_CMAP_VIRIDIS;

    params.alpha = 1;
    params.scaling = 1;

    VkyTextureParams tex_params = vky_default_texture_params(size, size, 1);
    tex_params.format = VK_FORMAT_R8_UNORM;
    tex_params.format_bytes = 1;
    params.tex_params = &tex_params;

    VkyVisual* visual = vky_visual(scene, VKY_VISUAL_IMAGE_CMAP, &params, NULL);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Make an image.
    uint8_t* image = calloc(size * size, sizeof(uint8_t));
    for (uint32_t i = 0; i < size; i++)
    {
        for (uint32_t j = 0; j < size; j++)
        {
            image[size * i + j] = rand_byte();
        }
    }

    vky_visual_data_set_size(visual, 1, 0, NULL, NULL);
    vky_visual_data(visual, VKY_VISUAL_PROP_POS_GPU, 0, 1, (vec3){-1, -1, 0});
    vky_visual_data(visual, VKY_VISUAL_PROP_POS_GPU, 1, 1, (vec3){+1, +1, 0});
    vky_visual_data(visual, VKY_VISUAL_PROP_TEXTURE_COORDS, 0, 1, (vec2){0, 0});
    vky_visual_data(visual, VKY_VISUAL_PROP_TEXTURE_COORDS, 1, 1, (vec2){1, 1});
    vky_visual_image_upload(visual, (const uint8_t*)image);

    vky_run_app(app);
    vky_destroy_app(app);
    return 0;
}
