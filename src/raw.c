#include "../include/visky/visky.h"



/*************************************************************************************************/
/*  Image visual                                                                                 */
/*************************************************************************************************/

static VkyData vky_visual_image_bake(VkyVisual* visual, VkyData data)
{
    // Determine the actual number of vertices and indices.
    uint32_t image_count = data.item_count; // 1 data item per image
    uint32_t nv = 4 * image_count;          // total number of points
    uint32_t ni = 6 * image_count;

    ASSERT(nv > 0);
    ASSERT(ni > 0);

    data.vertex_count = nv;
    data.index_count = ni;

    if (data.items == NULL)
    {
        return data;
    }

    const VkyImageData* items = (const VkyImageData*)data.items;
    VkyTextureVertex* vertices = calloc(nv, sizeof(VkyTextureVertex));
    VkyIndex* indices = calloc(ni, sizeof(VkyIndex));

    for (uint32_t i = 0; i < image_count; i++)
    {
        // Vertices.
        vertices[4 * i + 0].pos[0] = items[i].p0[0];
        vertices[4 * i + 0].pos[1] = items[i].p0[1];
        vertices[4 * i + 0].pos[2] = items[i].p0[2];
        vertices[4 * i + 0].uv[0] = items[i].uv0[0];
        vertices[4 * i + 0].uv[1] = items[i].uv0[1];

        vertices[4 * i + 1].pos[0] = items[i].p1[0];
        vertices[4 * i + 1].pos[1] = items[i].p0[1];
        vertices[4 * i + 1].pos[2] = items[i].p0[2];
        vertices[4 * i + 1].uv[0] = items[i].uv1[0];
        vertices[4 * i + 1].uv[1] = items[i].uv0[1];

        vertices[4 * i + 2].pos[0] = items[i].p1[0];
        vertices[4 * i + 2].pos[1] = items[i].p1[1];
        vertices[4 * i + 2].pos[2] = items[i].p0[2];
        vertices[4 * i + 2].uv[0] = items[i].uv1[0];
        vertices[4 * i + 2].uv[1] = items[i].uv1[1];

        vertices[4 * i + 3].pos[0] = items[i].p0[0];
        vertices[4 * i + 3].pos[1] = items[i].p1[1];
        vertices[4 * i + 3].pos[2] = items[i].p0[2];
        vertices[4 * i + 3].uv[0] = items[i].uv0[0];
        vertices[4 * i + 3].uv[1] = items[i].uv1[1];

        // Indices.
        indices[6 * i + 0] = 4 * i + 0;
        indices[6 * i + 1] = 4 * i + 1;
        indices[6 * i + 2] = 4 * i + 2;
        indices[6 * i + 3] = 4 * i + 2;
        indices[6 * i + 4] = 4 * i + 3;
        indices[6 * i + 5] = 4 * i + 0;
    }

    // Pass-through for the vertices.
    data.vertices = vertices;
    data.indices = indices;

    return data;
}

VkyVisual* vky_visual_image(VkyScene* scene, const VkyTextureParams* params)
{
    ASSERT(params != NULL);
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_IMAGE);
    VkyCanvas* canvas = scene->canvas;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "image.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "image.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout =
        vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyTextureVertex));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VKY_DEFAULT_VERTEX_FORMAT_POS, offsetof(VkyTextureVertex, pos));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(VkyTextureVertex, uv));

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);
    vky_add_resource_binding(&resource_layout, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){true});

    // Add the texture.
    VkyTexture* tex = vky_add_texture(canvas->gpu, params);
    // HACK: to avoid empty texture, use an empty texture.
    void* pixels = calloc(params->width * params->height * params->depth, params->format_bytes);
    vky_upload_texture(tex, pixels);
    free(pixels);

    // Resources.
    vky_add_common_resources(visual);
    vky_add_texture_resource(visual, tex);

    visual->cb_bake_data = vky_visual_image_bake;

    return visual;
}



void vky_visual_image_upload(VkyVisual* visual, const void* pixels)
{
    ASSERT(visual->visual_type == VKY_VISUAL_IMAGE);
    ASSERT(visual->resource_count == 3); // MVP, color texture, visual's image
    VkyTexture* texture = (VkyTexture*)visual->resources[2];
    ASSERT(texture != NULL);
    vky_upload_texture(texture, pixels);
}



/*************************************************************************************************/
/*  Raw path visual                                                                              */
/*************************************************************************************************/

VkyVisual* vky_visual_path_raw(VkyScene* scene)
{
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_PATH_RAW);
    VkyCanvas* canvas = scene->canvas;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "raw_path.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "raw_path.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout = vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyVertex));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VKY_DEFAULT_VERTEX_FORMAT_POS, offsetof(VkyVertex, pos));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VKY_DEFAULT_VERTEX_FORMAT_COLOR, offsetof(VkyVertex, color));

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){true});

    // Resources.
    vky_add_common_resources(visual);

    return visual;
}



/*************************************************************************************************/
/*  Raw multi path visual                                                                        */
/*************************************************************************************************/

static VkyData vky_path_raw_multi_bake(VkyVisual* visual, VkyData data)
{
    VkyMultiRawPathParams* params = (VkyMultiRawPathParams*)visual->params;

    // Determine the actual number of vertices and indices.
    uint32_t nv = data.item_count; // total number of points
    uint32_t path_count = (uint32_t)params->info[0];
    ASSERT(nv % path_count == 0);
    uint32_t vertex_count_per_path = nv / path_count;
    uint32_t ni = nv;

    ASSERT(nv > 0);
    ASSERT(ni > 0);

    data.vertex_count = nv;
    data.index_count = ni;

    if (data.items == NULL)
    {
        return data;
    }

    // We don't allocate data for the vertices, we pass through the user data.
    data.no_vertices_alloc = true;
    // Allocate the data buffer to be uploaded to the vertex buffer. Will be freed by visky.
    VkyIndex* indices = calloc(ni, sizeof(VkyIndex));

    uint32_t offset = 0;
    for (uint32_t path_idx = 0; path_idx < path_count; path_idx++)
    {
        for (uint32_t i = 0; i < vertex_count_per_path; i++)
        {
            indices[offset + i] = path_idx + i * path_count;
        }
        offset += vertex_count_per_path;
    }
    ASSERT(offset == ni);

    // Pass-through for the vertices.
    data.vertices = data.items;
    data.indices = indices;

    return data;
}

VkyVisual* vky_visual_path_raw_multi(VkyScene* scene, const VkyMultiRawPathParams* params)
{
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_PATH_RAW_MULTI);
    VkyCanvas* canvas = scene->canvas;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "raw_multi_path.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "raw_multi_path.frag.spv");

    // Vertex layout.
    // TODO: customizable data format.
    VkyVertexLayout vertex_layout = vky_create_vertex_layout(canvas->gpu, 0, sizeof(int16_t));
    vky_add_vertex_attribute(&vertex_layout, 0, VK_FORMAT_R16_SSCALED, 0);
    // TODO: if Metal, need to use R32_SFLOAT and conversion from int16 to float in baking

    // Params.
    vky_visual_params(visual, sizeof(VkyMultiRawPathParams), params);

    // Resource layout.
    // VkyResourceLayout resource_layout = vky_create_resource_layout(canvas->gpu,
    // canvas->image_count);
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){true});

    // Resources.
    vky_add_common_resources(visual);

    visual->cb_bake_data = vky_path_raw_multi_bake;

    return visual;
}



/*************************************************************************************************/
/*  Raw markers visual                                                                           */
/*************************************************************************************************/

VkyVisual* vky_visual_marker_raw(VkyScene* scene, const VkyMarkersRawParams* params)
{
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_MARKER_RAW);
    VkyCanvas* canvas = scene->canvas;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "raw_markers.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "raw_markers.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout = vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyVertex));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VKY_DEFAULT_VERTEX_FORMAT_POS, offsetof(VkyVertex, pos));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VKY_DEFAULT_VERTEX_FORMAT_COLOR, offsetof(VkyVertex, color));



    // Default params.
    VkyMarkersRawParams vparams = {0};
    if (params != NULL)
    {
        memcpy(&vparams, params, sizeof(VkyMarkersRawParams));
    }
    else
    {
        // TODO: constants
        vparams.marker_size[0] = 10;
        vparams.marker_size[1] = 10;
    }
    // DPI scaling factor.
    vparams.marker_size[0] *= canvas->dpi_factor;
    vparams.marker_size[1] *= canvas->dpi_factor;
    vky_visual_params(visual, sizeof(VkyMarkersRawParams), &vparams);
    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_POINT_LIST, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){false});

    // Resources.
    vky_add_common_resources(visual);

    return visual;
}
