#include "../include/visky/visky.h"



/*************************************************************************************************/
/*  Fake sphere visual                                                                           */
/*************************************************************************************************/

VkyVisual* vky_visual_fake_sphere(VkyScene* scene, const VkyFakeSphereParams* params)
{
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_FAKE_SPHERE);
    VkyCanvas* canvas = scene->canvas;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "fake_sphere.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "fake_sphere.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout =
        vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyFakeSphereVertex));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VKY_DEFAULT_VERTEX_FORMAT_POS, offsetof(VkyFakeSphereVertex, pos));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VKY_DEFAULT_VERTEX_FORMAT_COLOR, offsetof(VkyFakeSphereVertex, color));
    vky_add_vertex_attribute(
        &vertex_layout, 2, VK_FORMAT_R32_SFLOAT, offsetof(VkyFakeSphereVertex, radius));

    // Params.
    vky_visual_params(visual, sizeof(VkyFakeSphereParams), params);

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_POINT_LIST, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){true});

    // Resources.
    vky_add_common_resources(visual);

    return visual;
}
