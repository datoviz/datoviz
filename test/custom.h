#include <visky/visky.h>



static void mandelbrot(VkyPanel* panel)
{
    VkyCanvas* canvas = panel->scene->canvas;
    vky_set_panel_aspect_ratio(panel, 1);

    // Create the visual.
    VkyVisual* visual = vky_create_visual(panel->scene, VKY_VISUAL_CUSTOM);

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "mandelbrot.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "mandelbrot.frag.spv");

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
    vky_add_uniform_buffer_resource(visual, &panel->scene->grid->dynamic_buffer);

    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Upload the data.
    vec3 data[6] = {
        {-1, -1, 0}, {+1, -1, 0}, {-1, +1, 0}, {-1, +1, 0}, {+1, -1, 0}, {+1, +1, 0},
    };
    vky_visual_data(visual, (VkyData){6, data});
}



static void raytracing(VkyPanel* panel) { raytracing_demo(panel); }
