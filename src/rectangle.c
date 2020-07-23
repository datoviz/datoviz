#include "../include/visky/visky.h"



/*************************************************************************************************/
/*  Rectangle visual                                                                             */
/*************************************************************************************************/

static VkyData vky_visual_rectangle_bake(VkyVisual* visual, VkyData data)
{
    uint32_t nv = 4 * data.item_count;
    uint32_t ni = 6 * data.item_count;

    ASSERT(nv > 0);
    ASSERT(ni > 0);

    data.vertex_count = nv;
    data.index_count = ni;

    if (data.items == NULL)
    {
        return data;
    }

    // Allocate the data buffer to be uploaded to the vertex buffer. Will be freed by visky.
    log_trace("allocating vertices and indices");
    VkyVertex* vertices = calloc(nv, sizeof(VkyVertex));
    VkyIndex* indices = calloc(ni, sizeof(VkyIndex));

    const VkyRectangleData* items = (const VkyRectangleData*)data.items;

    vec3 origin, u, v, w;
    VkyRectangleParams* params = (VkyRectangleParams*)visual->params;
    glm_vec3_copy(params->origin, origin);
    glm_vec3_copy(params->u, u);
    glm_vec3_copy(params->v, v);

    for (uint32_t i = 0; i < data.item_count; i++)
    {
        // p00
        glm_vec3_scale(u, items[i].p[0], w);
        glm_vec3_add(w, origin, vertices[4 * i + 0].pos);
        glm_vec3_scale(v, items[i].p[1], w);
        glm_vec3_add(w, vertices[4 * i + 0].pos, vertices[4 * i + 0].pos);

        // p01
        glm_vec3_scale(u, items[i].size[0], w);
        glm_vec3_add(vertices[4 * i + 0].pos, w, vertices[4 * i + 1].pos);

        // p10
        glm_vec3_scale(v, items[i].size[1], w);
        glm_vec3_add(vertices[4 * i + 0].pos, w, vertices[4 * i + 3].pos);

        // p11
        glm_vec3_add(vertices[4 * i + 1].pos, w, vertices[4 * i + 2].pos);

        // Copy the color.
        for (uint32_t j = 0; j < 4; j++)
        {
            memcpy(&vertices[4 * i + j].color, &items[i].color, sizeof(items[i].color));
        }

        indices[6 * i + 0] = 4 * i + 0;
        indices[6 * i + 1] = 4 * i + 1;
        indices[6 * i + 2] = 4 * i + 2;
        indices[6 * i + 3] = 4 * i + 2;
        indices[6 * i + 4] = 4 * i + 3;
        indices[6 * i + 5] = 4 * i + 0;

        ASSERT(6 * i + 5 < ni);
    }

    data.vertices = vertices;
    data.indices = indices;

    return data;
}



VkyVisual* vky_visual_rectangle(VkyScene* scene, VkyRectangleParams params)
{
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_RECTANGLE);
    VkyCanvas* canvas = scene->canvas;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "mesh_raw.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "mesh_raw.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout = vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyVertex));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VKY_DEFAULT_VERTEX_FORMAT_POS, offsetof(VkyVertex, pos));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VK_FORMAT_R8G8B8A8_UNORM, offsetof(VkyVertex, color));

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){true});

    vky_visual_params(visual, sizeof(VkyRectangleParams), &params);
    vky_add_common_resources(visual);
    visual->cb_bake_data = vky_visual_rectangle_bake;
    return visual;
}



/*************************************************************************************************/
/*  Area visual                                                                                  */
/*************************************************************************************************/

static VkyData vky_visual_area_bake(VkyVisual* visual, VkyData data)
{
    uint32_t nv = 2 * data.item_count;

    ASSERT(nv > 0);

    data.vertex_count = nv;
    data.index_count = 0;

    if (data.items == NULL)
        return data;

    // Allocate the data buffer to be uploaded to the vertex buffer. Will be freed by visky.
    log_trace("allocating vertices");
    VkyAreaVertex* vertices = calloc(nv, sizeof(VkyAreaVertex));

    const VkyAreaData* items = (const VkyAreaData*)data.items;

    vec3 origin, u, v, w;
    VkyAreaParams* params = (VkyAreaParams*)visual->params;
    glm_vec3_copy(params->origin, origin);
    glm_vec3_copy(params->u, u);
    glm_vec3_copy(params->v, v);

    for (uint32_t i = 0; i < data.item_count; i++)
    {
        // pos_bottom
        glm_vec3_scale(u, items[i].p[0], w);                               // w = u * x
        glm_vec3_add(w, origin, vertices[2 * i + 0].pos);                  // pos = origin + w
        glm_vec3_scale(v, items[i].p[1], w);                               // w = v * y
        glm_vec3_add(w, vertices[2 * i + 0].pos, vertices[2 * i + 0].pos); // pos += w;

        // pos_top
        glm_vec3_copy(vertices[2 * i + 0].pos, vertices[2 * i + 1].pos);   // pos = pos_prev
        glm_vec3_scale(v, items[i].h, w);                                  // w = v * h
        glm_vec3_add(vertices[2 * i + 1].pos, w, vertices[2 * i + 1].pos); // pos += w

        // Copy the color.
        for (uint32_t j = 0; j < 2; j++)
        {
            memcpy(&vertices[2 * i + j].color, &items[i].color, sizeof(items[i].color));
            vertices[2 * i + j].area_idx = items[i].area_idx;
        }
    }

    data.vertices = vertices;
    data.indices = NULL;

    return data;
}



VkyVisual* vky_visual_area(VkyScene* scene, VkyAreaParams params)
{
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_AREA);
    VkyCanvas* canvas = scene->canvas;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "area.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "area.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout =
        vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyAreaVertex));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VKY_DEFAULT_VERTEX_FORMAT_POS, offsetof(VkyAreaVertex, pos));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VK_FORMAT_R8G8B8A8_UNORM, offsetof(VkyAreaVertex, color));
    vky_add_vertex_attribute(
        &vertex_layout, 2, VK_FORMAT_R32_UINT, offsetof(VkyAreaVertex, area_idx));

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){true});

    vky_visual_params(visual, sizeof(VkyAreaParams), &params);
    vky_add_common_resources(visual);
    visual->cb_bake_data = vky_visual_area_bake;
    return visual;
}



/*************************************************************************************************/
/*  Axis rectangle visual                                                                        */
/*************************************************************************************************/

static VkyData vky_visual_rectangle_axis_bake(VkyVisual* visual, VkyData data)
{
    uint32_t nv = 4 * data.item_count;
    uint32_t ni = 6 * data.item_count;

    ASSERT(nv > 0);
    ASSERT(ni > 0);

    data.vertex_count = nv;
    data.index_count = ni;

    if (data.items == NULL)
    {
        return data;
    }

    // Allocate the data buffer to be uploaded to the vertex buffer. Will be freed by visky.
    log_trace("allocating vertices and indices");
    VkyVertex* vertices = calloc(nv, sizeof(VkyVertex));
    VkyIndex* indices = calloc(ni, sizeof(VkyIndex));

    const VkyRectangleAxisData* items = (const VkyRectangleAxisData*)data.items;

    float span_axis = 0;
    for (uint32_t i = 0; i < data.item_count; i++)
    {
        span_axis = (float)items[i].span_axis;
        if (items[i].span_axis == 0)
        {
            glm_vec3_copy((vec3){-1, items[i].ab[0], span_axis}, vertices[4 * i + 0].pos);
            glm_vec3_copy((vec3){+1, items[i].ab[0], span_axis}, vertices[4 * i + 1].pos);
            glm_vec3_copy((vec3){+1, items[i].ab[1], span_axis}, vertices[4 * i + 2].pos);
            glm_vec3_copy((vec3){-1, items[i].ab[1], span_axis}, vertices[4 * i + 3].pos);
        }
        else if (items[i].span_axis == 1)
        {
            glm_vec3_copy((vec3){items[i].ab[0], -1, span_axis}, vertices[4 * i + 0].pos);
            glm_vec3_copy((vec3){items[i].ab[1], -1, span_axis}, vertices[4 * i + 1].pos);
            glm_vec3_copy((vec3){items[i].ab[1], +1, span_axis}, vertices[4 * i + 2].pos);
            glm_vec3_copy((vec3){items[i].ab[0], +1, span_axis}, vertices[4 * i + 3].pos);
        }

        // Copy the color.
        for (uint32_t j = 0; j < 4; j++)
        {
            memcpy(&vertices[4 * i + j].color, &items[i].color, sizeof(items[i].color));
        }

        indices[6 * i + 0] = 4 * i + 0;
        indices[6 * i + 1] = 4 * i + 1;
        indices[6 * i + 2] = 4 * i + 2;
        indices[6 * i + 3] = 4 * i + 2;
        indices[6 * i + 4] = 4 * i + 3;
        indices[6 * i + 5] = 4 * i + 0;

        ASSERT(6 * i + 5 < ni);
    }

    data.vertices = vertices;
    data.indices = indices;

    return data;
}



VkyVisual* vky_visual_rectangle_axis(VkyScene* scene)
{
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_RECTANGLE_AXIS);
    VkyCanvas* canvas = scene->canvas;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "axrect.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "axrect.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout = vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyVertex));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VKY_DEFAULT_VERTEX_FORMAT_POS, offsetof(VkyVertex, pos));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VK_FORMAT_R8G8B8A8_UNORM, offsetof(VkyVertex, color));

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){true});

    vky_add_common_resources(visual);
    visual->cb_bake_data = vky_visual_rectangle_axis_bake;
    return visual;
}
