#include "axes_3D.h"
#include "../include/visky/scene.h"

BEGIN_INCL_NO_WARN
#include <stb_image.h>
END_INCL_NO_WARN



/*************************************************************************************************/
/*  Axes 3D visual                                                                               */
/*************************************************************************************************/

static VkyData vky_axes_3D_bake(VkyVisual* visual, VkyData data)
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
    VkyAxes3DVertex* vertices = calloc(nv, sizeof(VkyAxes3DVertex));
    VkyIndex* indices = calloc(ni, sizeof(VkyIndex));
    const VkyAxes3DVertex* items = (const VkyAxes3DVertex*)data.items;
    double dpi = visual->scene->canvas->dpi_factor;

    for (uint32_t i = 0; i < data.item_count; i++)
    {
        for (uint32_t j = 0; j < 4; j++)
        {
            vertices[4 * i + j].tick = items[i].tick;
            vertices[4 * i + j].color = items[i].color;
            vertices[4 * i + j].linewidth = items[i].linewidth * dpi;
            vertices[4 * i + j].cap0 = items[i].cap0;
            vertices[4 * i + j].cap1 = items[i].cap1;
            vertices[4 * i + j].coord_side = items[i].coord_side;

            ASSERT(4 * i + j < nv);
        }

        indices[6 * i + 0] = 4 * i + 0;
        indices[6 * i + 1] = 4 * i + 1;
        indices[6 * i + 2] = 4 * i + 2;
        indices[6 * i + 3] = 4 * i + 0;
        indices[6 * i + 4] = 4 * i + 2;
        indices[6 * i + 5] = 4 * i + 3;

        ASSERT(6 * i + 5 < ni);
    }

    data.vertices = vertices;
    data.indices = indices;

    return data;
}

VkyVisual* vky_visual_axes_3D(VkyScene* scene)
{
    log_trace("vky_visual_axes_3D");
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_AXES_3D);
    VkyCanvas* canvas = scene->canvas;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "axes_3D.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "axes_3D.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout =
        vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyAxes3DVertex));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VK_FORMAT_R32_SFLOAT, offsetof(VkyAxes3DVertex, tick));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VKY_DEFAULT_VERTEX_FORMAT_COLOR, offsetof(VkyAxes3DVertex, color));
    vky_add_vertex_attribute(
        &vertex_layout, 2, VK_FORMAT_R32_SFLOAT, offsetof(VkyAxes3DVertex, linewidth));
    vky_add_vertex_attribute(
        &vertex_layout, 3, VK_FORMAT_R32_SINT, offsetof(VkyAxes3DVertex, cap0));
    vky_add_vertex_attribute(
        &vertex_layout, 4, VK_FORMAT_R32_SINT, offsetof(VkyAxes3DVertex, cap1));
    vky_add_vertex_attribute(
        &vertex_layout, 5, VK_FORMAT_R8_UINT, offsetof(VkyAxes3DVertex, coord_side));

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){false});

    // Resources.
    vky_add_common_resources(visual);

    visual->cb_bake_data = vky_axes_3D_bake;

    return visual;
}



/*************************************************************************************************/
/*  Axes 3D text visual                                                                          */
/*************************************************************************************************/

static VkyData vky_axes_3D_text_bake(VkyVisual* visual, VkyData data)
{

    ASSERT(data.items != NULL); // TODO: support allocation with no upload by specifying a max
                                // number of glyphs per string
    ASSERT(data.item_count > 0);

    // Input text as array of VkyAxes3DTextData.
    const VkyAxes3DTextData* text = (const VkyAxes3DTextData*)data.items;

    // Count the number of glyphs.
    uint32_t glyph_count = 0;
    for (uint32_t i = 0; i < data.item_count; i++)
    { // vertex_count is the number of strings here
        glyph_count += text[i].string_len;
    }

    uint32_t nv = glyph_count;
    ASSERT(nv > 0);

    data.vertex_count = nv * 4;
    data.index_count = 0; // we don't use the index buffer

    // Allocate the data buffer to be uploaded to the vertex buffer. Will be freed by visky.
    VkyAxes3DTextVertex* vertices = calloc(data.vertex_count, sizeof(VkyAxes3DTextVertex));

    uint32_t k = 0;
    VkyAxes3DTextVertex vertex = {0};
    // Go through all strings.
    for (uint32_t i = 0; i < data.item_count; i++)
    {
        uint32_t str_len = text[i].string_len;
        // For each string, go through the chars.
        for (uint32_t j = 0; j < str_len; j++)
        {
            char c[] = {text[i].string[j]};
            uint32_t ci = strcspn(VKY_TEXT_CHARS, c);
            vertex = (VkyAxes3DTextVertex){
                text[i].tick,
                text[i].coord_side,
                {ci, j, str_len, i}, // char, charIdx, strLen, strIdx
            };
            for (uint32_t u = 0; u < 4; u++)
            {
                vertices[4 * k + u] = vertex;
            }
            k++;
        }
    }

    data.vertices = vertices;
    data.indices = NULL;

    return data;
}

VkyVisual* vky_visual_axes_3D_text(VkyScene* scene)
{
    log_trace("vky_visual_axes_3D_text");
    VkyCanvas* canvas = scene->canvas;
    VkyVisual* visual = vky_create_visual(scene, VKY_VISUAL_AXES_3D_TEXT);

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "axes_3D_text.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "axes_3D_text.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout =
        vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyAxes3DTextVertex));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VK_FORMAT_R32_SFLOAT, offsetof(VkyAxes3DTextVertex, tick));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VK_FORMAT_R8_UINT, offsetof(VkyAxes3DTextVertex, coord_side));
    vky_add_vertex_attribute(
        &vertex_layout, 2, VK_FORMAT_R16G16B16A16_UINT, offsetof(VkyAxes3DTextVertex, glyph));

    // Font texture.
    VkyTexture* texture = vky_get_font_texture(canvas->gpu);

    // UBO params.
    float glyph_height = VKY_AXES_FONT_SIZE * canvas->dpi_factor;
    int width = (int)texture->params.width;
    int height = (int)texture->params.height;
    float glyph_width = glyph_height / height * width * 6 / 16.;
    VkyAxes3DTextParams params = {
        VKY_FONT_TEXTURE_SHAPE,
        {width, height},
        {glyph_width, glyph_height},
        {VKY_AXES_TEXT_COLOR_R, VKY_AXES_TEXT_COLOR_G, VKY_AXES_TEXT_COLOR_B,
         VKY_AXES_TEXT_COLOR_A},
    };
    vky_visual_params(visual, sizeof(VkyAxes3DTextParams), &params);

    // Resource layout.
    VkyResourceLayout resource_layout = vky_common_resource_layout(visual);
    vky_add_resource_binding(&resource_layout, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    // Pipeline.
    visual->pipeline = vky_create_graphics_pipeline(
        canvas, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){false});

    // Resources.
    vky_add_common_resources(visual);
    vky_add_texture_resource(visual, texture);

    visual->cb_bake_data = vky_axes_3D_text_bake;

    return visual;
}



/*************************************************************************************************/
/*  Axes 3D                                                                                      */
/*************************************************************************************************/

static VkyVisual* _add_3D_axes(VkyScene* scene)
{
    VkyVisual* segment_visual = vky_visual_axes_3D(scene);

    const uint32_t n_ticks = 9;
    const uint32_t n = 3 * 2 * n_ticks;
    VkyAxes3DVertex* vertices = calloc(n, sizeof(VkyAxes3DVertex));
    VkyAxes3DVertex* vertex = NULL;
    for (uint32_t side = 0; side < 3; side++)
    {
        for (uint32_t orientation = 0; orientation < 2; orientation++)
        {
            for (uint32_t i = 0; i < n_ticks; i++)
            {
                vertex = &vertices[2 * n_ticks * side + orientation * n_ticks + i];
                vertex->color = (VkyColorBytes){
                    // side == 0 ? 255 : 0, side == 1 ? 255 : 0, side == 2 ? 255 : 0, //
                    0, 0, 0, //
                    128};
                vertex->cap0 = 1;
                vertex->cap1 = 1;
                vertex->coord_side = orientation * 4 + side;
                vertex->linewidth = i == 0 || i == (n_ticks - 1) ? 2 : 1;
                vertex->tick = -1 + 2 * i / (float)(n_ticks - 1);
            }
        }
    }
    vky_visual_upload(segment_visual, (VkyData){n, vertices});

    free(vertices);
    return segment_visual;
}

static VkyVisual* _add_3D_axes_text(VkyScene* scene)
{
    const uint32_t n_ticks = 5;
    const uint32_t n = 3 * n_ticks;
    VkyVisual* text_visual = vky_visual_axes_3D_text(scene);
    VkyAxes3DTextData* text_vertices = calloc(n, sizeof(VkyAxes3DTextData));
    VkyAxes3DTextData* vertex = NULL;

    double tick = 0;
    char fmt[16] = {0};
    for (uint32_t side = 0; side < 3; side++)
    {
        for (uint32_t i = 0; i < n_ticks; i++)
        {
            vertex = &text_vertices[n_ticks * side + i];
            ASSERT(vertex != NULL);
            vertex->coord_side = side == 2 ? 5 : side;

            // Determine the tick.
            tick = -1 + 2 * i / (double)(n_ticks - 1);
            vertex->tick = tick;

            // Format tick text.
            strcpy(fmt, "%.XF"); // [2] = precision, [3] = f or e
            sprintf(&fmt[2], "%d", 1);
            fmt[3] = VKY_AXES_NORMAL_RANGE(fabs(tick)) ? 'f' : 'e';
            snprintf(vertex->string, VKY_AXES_MAX_GLYPHS_PER_TICK, fmt, tick);
            vertex->string_len = strlen(vertex->string);
        }
    }
    vky_visual_upload(text_visual, (VkyData){n, text_vertices});
    free(text_vertices);

    return text_visual;
}



VkyVisualBundle* vky_bundle_axes_3D(VkyScene* scene)
{
    log_trace("vky_bundle_axes_3D");
    VkyVisual* axes = _add_3D_axes(scene);
    VkyVisual* text = _add_3D_axes_text(scene);

    VkyVisualBundle* vb = vky_create_visual_bundle(scene);
    vky_add_visual_to_bundle(vb, axes);
    vky_add_visual_to_bundle(vb, text);

    return vb;
}



void vky_axes_3D_init(VkyPanel* panel)
{
    log_trace("vky_axes_3D_init");
    VkyVisualBundle* vb = vky_bundle_axes_3D(panel->scene);
    vky_add_visual_bundle_to_panel(vb, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);
}
