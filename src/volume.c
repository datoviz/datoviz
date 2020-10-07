#include "../include/visky/visky.h"



/*************************************************************************************************/
/*  Volume visual                                                                                */
/*************************************************************************************************/

VkyVisual*
vky_visual_volume(VkyScene* scene, const VkyTextureParams* tex_params, const void* pixels)
{
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_VOLUME);
    VkyCanvas* canvas = scene->canvas;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "volume.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "volume.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout = vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyVertexUV));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VKY_DEFAULT_VERTEX_FORMAT_POS, offsetof(VkyVertexUV, pos));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(VkyVertexUV, uv));

    // Params.
    VkyVolumeParams params = {0};
    glm_mat4_copy(GLM_MAT4_IDENTITY, params.inv_proj_view);
    glm_mat4_copy(GLM_MAT4_IDENTITY, params.normal_mat);
    vky_visual_params(visual, sizeof(VkyVolumeParams), &params);

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);
    vky_add_resource_binding(&resource_layout, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){true});

    // Texture.
    VkyTexture* tex = vky_add_texture(canvas->gpu, tex_params);
    vky_upload_texture(tex, pixels);

    // Resources.
    vky_add_common_resources(visual);
    vky_add_texture_resource(visual, tex);

    // Square.
    float x = 1.0f;
    VkyVertexUV vertices[] = {
        {{-x, -x, 0}, {0, 0}}, {{+x, -x, 0}, {1, 0}}, {{-x, +x, 0}, {0, 1}},
        {{-x, +x, 0}, {0, 1}}, {{+x, -x, 0}, {1, 0}}, {{+x, +x, 0}, {1, 1}},
    };

    vky_visual_upload(visual, (VkyData){0, NULL, 6, vertices, 0, NULL});

    return visual;
}



/*************************************************************************************************/
/*  Volume slicer visual                                                                         */
/*************************************************************************************************/

VkyVisual*
vky_visual_volume_slicer(VkyScene* scene, const VkyTextureParams* tex_params, const void* pixels)
{
    // Create the visual.
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_UNDEFINED);
    VkyCanvas* canvas = scene->canvas;

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

    // Resource layout.
    VkyResourceLayout resource_layout =
        vky_create_resource_layout(canvas->gpu, canvas->image_count);
    vky_add_resource_binding(&resource_layout, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
    vky_add_resource_binding(&resource_layout, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){false});

    // 3D texture.
    VkyTexture* tex = vky_add_texture(canvas->gpu, tex_params);

    // Resources.
    vky_add_uniform_buffer_resource(visual, &scene->grid->dynamic_buffer);
    vky_add_texture_resource(visual, tex);

    double a = 1;
    VkyTexturedVertex3D vertices[] = {

        {{-a, -a, 0}, {1, .5, 1}}, //
        {{-a, +a, 0}, {0, .5, 1}}, //
        {{+a, -a, 0}, {1, .5, 0}}, //
        {{+a, -a, 0}, {1, .5, 0}}, //
        {{-a, +a, 0}, {0, .5, 1}}, //
        {{+a, +a, 0}, {0, .5, 0}}, //

    };
    vky_visual_upload(visual, (VkyData){0, NULL, 6, vertices, 0, NULL});

    vky_upload_texture(tex, pixels);

    return visual;
}
