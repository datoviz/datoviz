#include <visky/visky.h>

typedef struct VkyTexturedVertex3D VkyTexturedVertex3D;
struct VkyTexturedVertex3D
{
    vec3 pos;
    vec3 coords;
    int plane;
};

VkyUniformBuffer uniform_buffer;
vec3 vol_pos;

static void on_key_press(VkyCanvas* canvas)
{
    VkyKeyboard* keyboard = canvas->event_controller->keyboard;
    bool do_update = true;
    switch (keyboard->key)
    {
    case VKY_KEY_X:
        vol_pos[0] += .01;
        if (vol_pos[0] > .5)
            vol_pos[0] = -.5;
        break;
    case VKY_KEY_Y:
        vol_pos[1] += .01;
        if (vol_pos[1] > .5)
            vol_pos[1] = -.5;
        break;
    case VKY_KEY_Z:
        vol_pos[2] += .01;
        if (vol_pos[2] > .5)
            vol_pos[2] = -.5;
        break;
    default:
        do_update = false;
        break;
    }

    if (!do_update)
        return;

    for (uint32_t image_index = 0; image_index < canvas->image_count; image_index++)
    {
        vky_upload_uniform_buffer(&uniform_buffer, image_index, vol_pos);
    }
}

int main()
{
    log_set_level_env();

    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    VkyCanvas* canvas = vky_create_canvas(app, 1920, 1080);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_BLACK, 1, 1);

    // Set the panel controller.
    VkyPanel* panel = vky_get_panel(scene, 0, 0);
    vky_set_controller(panel, VKY_CONTROLLER_ARCBALL, NULL);

    // Create the visual.
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_UNDEFINED);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "volume_slice.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "volume_slice.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout =
        vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyTexturedVertex3D));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VkyTexturedVertex3D, pos));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VkyTexturedVertex3D, coords));
    vky_add_vertex_attribute(
        &vertex_layout, 2, VK_FORMAT_R32_SINT, offsetof(VkyTexturedVertex3D, plane));

    // Resource layout.
    VkyResourceLayout resource_layout =
        vky_create_resource_layout(canvas->gpu, canvas->image_count);
    vky_add_resource_binding(&resource_layout, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
    vky_add_resource_binding(&resource_layout, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    vky_add_resource_binding(&resource_layout, 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){true});

    // 3D texture.
    VkyTextureParams params = {256,
                               256,
                               256,
                               1,
                               VK_FORMAT_R8_UNORM,
                               VK_FILTER_LINEAR,
                               VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                               0,
                               false};
    VkyTexture* tex = vky_add_texture(canvas->gpu, params);

    // Uniform buffer with the volume position.
    uniform_buffer = vky_create_uniform_buffer(canvas->gpu, canvas->image_count, sizeof(vec3));
    glm_vec3_zero(vol_pos);

    for (uint32_t image_index = 0; image_index < canvas->image_count; image_index++)
    {
        vky_upload_uniform_buffer(&uniform_buffer, image_index, vol_pos);
    }

    // Resources.
    vky_add_uniform_buffer_resource(visual, &scene->grid->dynamic_buffer);
    vky_add_texture_resource(visual, tex);
    vky_add_uniform_buffer_resource(visual, &uniform_buffer);

    double a = .5;
    double x = 0, y = 0, z = 0;
    VkyTexturedVertex3D vertices[] = {

        {{-a, -a, z}, {.5 + z, 0, 0}, 0}, {{+a, -a, z}, {.5 + z, 1, 0}, 0},
        {{-a, +a, z}, {.5 + z, 0, 1}, 0}, {{-a, +a, z}, {.5 + z, 0, 1}, 0},
        {{+a, -a, z}, {.5 + z, 1, 0}, 0}, {{+a, +a, z}, {.5 + z, 1, 1}, 0},

        {{x, -a, -a}, {0, .5 + x, 0}, 1}, {{x, +a, -a}, {0, .5 + x, 1}, 1},
        {{x, -a, +a}, {1, .5 + x, 0}, 1}, {{x, -a, +a}, {1, .5 + x, 0}, 1},
        {{x, +a, -a}, {0, .5 + x, 1}, 1}, {{x, +a, +a}, {1, .5 + x, 1}, 1},

        {{-a, y, -a}, {0, 0, .5 + y}, 2}, {{+a, y, -a}, {0, 1, .5 + y}, 2},
        {{-a, y, +a}, {1, 0, .5 + y}, 2}, {{-a, y, +a}, {1, 0, .5 + y}, 2},
        {{+a, y, -a}, {0, 1, .5 + y}, 2}, {{+a, y, +a}, {1, 1, .5 + y}, 2},

    };
    vky_visual_upload(visual, (VkyData){0, NULL, 18, vertices, 0, NULL});

    char path[1024];
    snprintf(path, sizeof(path), "%s/volume/%s", DATA_DIR, "SKULL.img");
    char* pixels = read_file(path, NULL);
    vky_upload_texture(tex, pixels);
    free(pixels);

    vky_add_frame_callback(canvas, on_key_press);

    vky_run_app(app);

    vky_destroy_uniform_buffer(&uniform_buffer);
    vky_destroy_scene(scene);
    vky_destroy_app(app);
    return 0;
}
