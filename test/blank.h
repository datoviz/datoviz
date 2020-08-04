#include <visky/visky.h>

static void blank(VkyPanel* panel)
{
    vky_clear_color(panel->scene, (VkClearColorValue){{255, 0, 0, 255}});
}

static void hello(VkyPanel* panel)
{
    VkyVisual* visual = vky_visual_text(panel->scene);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);
    VkyTextData data[1] = {
        {{0, 0, 0}, {0, 0}, {255, 0, 0, 255}, 30, {0, 0}, 0, 12, "Hello world!", false}};
    vky_visual_upload(visual, (VkyData){1, data});
}

static void triangle(VkyPanel* panel)
{
    VkyCanvas* canvas = panel->scene->canvas;

    // Create the visual.
    VkyVisual* visual = vky_create_visual(panel->scene, VKY_VISUAL_UNDEFINED);

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "default.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "default.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout =
        vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyDefaultVertex));

    vky_add_vertex_attribute(
        &vertex_layout, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VkyDefaultVertex, pos));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VkyDefaultVertex, color));

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

    // Make the data and upload it to the GPU.
    VkyDefaultVertex data[3] = {
        {{-1, +1, 0}, {1, 0, 0, 1}}, // vec3 pos, vec4 color
        {{+1, +1, 0}, {0, 1, 0, 1}},
        {{+0, -1, 0}, {0, 0, 1, 1}},
    };

    vky_visual_upload(visual, (VkyData){3, data});
}
