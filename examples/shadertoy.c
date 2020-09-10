#include <visky/visky.h>

BEGIN_INCL_NO_WARN
#include <stb_image.h>
END_INCL_NO_WARN


static VkyUniformBuffer ubo;


typedef struct Params Params;
struct Params
{
    vec2 mouse;
    float time;
    int32_t frame;
};
static Params params = {0};

static void callback(VkyCanvas* canvas)
{
    if (canvas->event_controller->mouse->cur_state == VKY_MOUSE_STATE_DRAG)
    {
        glm_vec2_copy(canvas->event_controller->mouse->cur_pos, params.mouse);
    }
    params.time = canvas->local_time;
    params.frame = (int32_t)canvas->frame_count;
    for (uint32_t image_index = 0; image_index < canvas->image_count; image_index++)
    {
        vky_upload_uniform_buffer(&ubo, image_index, &params);
    }
}


int main()
{
    log_set_level_env();

    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    VkyGpu* gpu = app->gpu;
    VkyCanvas* canvas = vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_BLACK, 1, 1);

    // Set the panel controllers.
    VkyPanel* panel = vky_get_panel(scene, 0, 0);
    vky_set_controller(panel, VKY_CONTROLLER_NONE, NULL);
    vky_set_panel_aspect_ratio(panel, 1);
    vky_set_constant(VKY_PANZOOM_MIN_ZOOM_ID, 1);

    // Create the visual.
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_UNDEFINED);

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "shadertoy.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "shadertoy.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout = vky_create_vertex_layout(canvas->gpu, 0, sizeof(vec3));
    vky_add_vertex_attribute(&vertex_layout, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);

    // Resource layout.
    VkyResourceLayout resource_layout =
        vky_create_resource_layout(canvas->gpu, canvas->image_count);
    vky_add_resource_binding(&resource_layout, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
    vky_add_resource_binding(&resource_layout, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    vky_add_resource_binding(&resource_layout, 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    vky_add_resource_binding(&resource_layout, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){true});

    // Extra resources.
    ubo = vky_create_uniform_buffer(gpu, canvas->image_count, sizeof(vec3));

    // Noise texture.
    char path[1024];
    snprintf(path, sizeof(path), "%s/textures/noise.bin", DATA_DIR);
    uint8_t* pixels = (uint8_t*)read_file(path, NULL);
    VkyTextureParams tex_params = {
        256,
        256,
        1,
        1,
        VK_FORMAT_R8_UNORM,
        VK_FILTER_NEAREST,
        VK_SAMPLER_ADDRESS_MODE_REPEAT,
        VK_IMAGE_LAYOUT_GENERAL,
        true};
    VkyTexture* noise_texture = vky_add_texture(canvas->gpu, &tex_params);
    vky_upload_texture(noise_texture, pixels);
    free(pixels);

    // Resources.
    vky_add_uniform_buffer_resource(visual, &scene->grid->dynamic_buffer);
    vky_add_texture_resource(visual, &canvas->gpu->textures[0]);
    vky_add_uniform_buffer_resource(visual, &ubo);
    vky_add_texture_resource(visual, noise_texture);

    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Upload the data.
    vec3 data[6] = {
        {-1, -1, 0}, {+1, -1, 0}, {-1, +1, 0}, {-1, +1, 0}, {+1, -1, 0}, {+1, +1, 0},
    };
    vky_visual_upload(visual, (VkyData){6, data});

    vky_add_frame_callback(canvas, callback);
    vky_run_app(app);

    vky_destroy_uniform_buffer(&ubo);
    vky_destroy_scene(scene);
    vky_destroy_app(app);
    return 0;
}
