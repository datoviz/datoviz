#ifndef VKY_DEMO_INC_HEADER
#define VKY_DEMO_INC_HEADER

#include <visky/visky.h>


typedef struct VkyPanel VkyPanel;


static void _raytracing_callback(VkyCanvas* canvas, void* data)
{
    double t = canvas->local_time;
    VkyCamera* camera = (VkyCamera*)canvas->scene->grid->panels[0].controller;
    vec3 axis = {0, -1, 0};
    glm_rotate_make(camera->mvp.model, 2 * t, axis);
}


static void _demo_raytracing(VkyPanel* panel)
{
    VkyCanvas* canvas = panel->scene->canvas;
    VkyScene* scene = panel->scene;

    // Create the visual.
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_CUSTOM);

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "raytracing.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "raytracing.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout = vky_create_vertex_layout(canvas->gpu, 0, sizeof(vec3));
    vky_add_vertex_attribute(&vertex_layout, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);

    // Resource layout.
    VkyResourceLayout resource_layout =
        vky_create_resource_layout(canvas->gpu, canvas->image_count);
    vky_add_resource_binding(&resource_layout, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){true});

    // Resources.
    vky_add_uniform_buffer_resource(visual, &scene->grid->dynamic_buffer);

    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Upload the data.
    vec3 data[6] = {
        {-1, -1, 0}, {+1, -1, 0}, {-1, +1, 0}, {-1, +1, 0}, {+1, -1, 0}, {+1, +1, 0},
    };
    visual->data.item_count = 6;
    visual->data.items = data;
    vky_visual_data_raw(visual);

    vky_add_frame_callback(canvas, _raytracing_callback, NULL);
}


#endif
