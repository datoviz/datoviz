#define STB_IMAGE_IMPLEMENTATION
#include "../include/datoviz/graphics.h"
#include "../include/datoviz/atlas.h"
#include "../include/datoviz/canvas.h"
#include "../include/datoviz/context.h"


/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Utils                                                                                       */
/*************************************************************************************************/

static inline void _load_shader(
    DvzGraphics* graphics, VkShaderStageFlagBits stage, //
    VkDeviceSize size, const unsigned char* buffer)
{
    ASSERT(buffer != NULL);
    uint32_t* code = (uint32_t*)calloc(size, 1);
    memcpy(code, buffer, size);
    ASSERT(size % 4 == 0);
    dvz_graphics_shader_spirv(graphics, stage, size, code);
    FREE(code);
}

#define SHADER(stage, x)                                                                          \
    {                                                                                             \
        unsigned long size = 0;                                                                   \
        const unsigned char* buffer = dvz_resource_shader(x, &size);                              \
        ASSERT(size > 0);                                                                         \
        ASSERT(buffer != NULL);                                                                   \
        _load_shader(graphics, VK_SHADER_STAGE_##stage##_BIT, size, buffer);                      \
    }

#define PRIMITIVE(x)                                                                              \
    dvz_graphics_renderpass(graphics, &canvas->renderpass, 0);                                    \
    dvz_graphics_topology(graphics, VK_PRIMITIVE_TOPOLOGY_##x);                                   \
    dvz_graphics_polygon_mode(graphics, VK_POLYGON_MODE_FILL);


#define CREATE dvz_graphics_create(graphics);

#define ATTR_BEGIN(t)                                                                             \
    dvz_graphics_vertex_binding(graphics, 0, sizeof(t));                                          \
    uint32_t attr_idx = 0;

#define ATTR(t, fmt, f) dvz_graphics_vertex_attr(graphics, 0, attr_idx++, fmt, offsetof(t, f));

#define ATTR_POS(t, f) ATTR(t, VK_FORMAT_R32G32B32_SFLOAT, f)

#define ATTR_COL(t, f) ATTR(t, VK_FORMAT_R8G8B8A8_UNORM, f)



/*************************************************************************************************/
/*  Common                                                                                       */
/*************************************************************************************************/

static void _common_slots(DvzGraphics* graphics)
{
    dvz_graphics_slot(graphics, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // MVP
    dvz_graphics_slot(graphics, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // viewport
    // dvz_graphics_slot(graphics, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // color texture
}



/*************************************************************************************************/
/*  Basic graphics                                                                               */
/*************************************************************************************************/

static void _graphics_point(DvzCanvas* canvas, DvzGraphics* graphics)
{
    SHADER(VERTEX, "graphics_point_vert")
    SHADER(FRAGMENT, "graphics_point_frag")
    PRIMITIVE(POINT_LIST)

    // Depth test flag.
    if ((graphics->flags & DVZ_GRAPHICS_FLAGS_DEPTH_TEST_ENABLE) != 0)
        dvz_graphics_depth_test(graphics, DVZ_DEPTH_TEST_ENABLE);

    ATTR_BEGIN(DvzVertex)
    ATTR_POS(DvzVertex, pos)
    ATTR_COL(DvzVertex, color)

    _common_slots(graphics);
    dvz_graphics_slot(graphics, DVZ_USER_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    CREATE
}

static void _graphics_basic(DvzCanvas* canvas, DvzGraphics* graphics, VkPrimitiveTopology topology)
{
    SHADER(VERTEX, "graphics_basic_vert")
    SHADER(FRAGMENT, "graphics_basic_frag")

    dvz_graphics_renderpass(graphics, &canvas->renderpass, 0);
    dvz_graphics_topology(graphics, topology);
    dvz_graphics_polygon_mode(graphics, VK_POLYGON_MODE_FILL);

    // Depth test flag.
    if ((graphics->flags & DVZ_GRAPHICS_FLAGS_DEPTH_TEST_ENABLE) != 0)
        dvz_graphics_depth_test(graphics, DVZ_DEPTH_TEST_ENABLE);

    ATTR_BEGIN(DvzVertex)
    ATTR_POS(DvzVertex, pos)
    ATTR_COL(DvzVertex, color)

    _common_slots(graphics);

    CREATE
}



/*************************************************************************************************/
/*  Agg marker graphics                                                                          */
/*************************************************************************************************/

static void _graphics_marker(DvzCanvas* canvas, DvzGraphics* graphics)
{
    SHADER(VERTEX, "graphics_marker_vert")
    SHADER(FRAGMENT, "graphics_marker_frag")
    PRIMITIVE(POINT_LIST)

    // Depth test flag.
    if ((graphics->flags & DVZ_GRAPHICS_FLAGS_DEPTH_TEST_ENABLE) != 0)
        dvz_graphics_depth_test(graphics, DVZ_DEPTH_TEST_ENABLE);

    ATTR_BEGIN(DvzGraphicsMarkerVertex)
    ATTR_POS(DvzGraphicsMarkerVertex, pos)
    ATTR_COL(DvzGraphicsMarkerVertex, color)
    ATTR(DvzGraphicsMarkerVertex, VK_FORMAT_R32_SFLOAT, size)
    ATTR(DvzGraphicsMarkerVertex, VK_FORMAT_R8_UINT, marker)
    ATTR(DvzGraphicsMarkerVertex, VK_FORMAT_R8_UNORM, angle)
    ATTR(DvzGraphicsMarkerVertex, VK_FORMAT_R8_UINT, transform)

    _common_slots(graphics);
    dvz_graphics_slot(graphics, DVZ_USER_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    CREATE
}



/*************************************************************************************************/
/*  Segment graphics                                                                             */
/*************************************************************************************************/

static void
_graphics_segment_callback(DvzGraphicsData* data, uint32_t item_count, const void* item)
{
    ASSERT(data != NULL);
    ASSERT(data->vertices != NULL);
    ASSERT(data->indices != NULL);

    ASSERT(item_count > 0);
    dvz_array_resize(data->vertices, 4 * item_count);
    dvz_array_resize(data->indices, 6 * item_count);

    if (item == NULL)
        return;
    ASSERT(item != NULL);
    ASSERT(data->current_idx < item_count);

    // Fill the vertices array by simply repeating them 4 times.
    dvz_array_data(data->vertices, 4 * data->current_idx, 4, 1, item);

    // Fill the indices array.
    DvzIndex* indices = (DvzIndex*)data->indices->data;
    uint32_t i = data->current_idx;
    indices[6 * i + 0] = 4 * i + 0;
    indices[6 * i + 1] = 4 * i + 1;
    indices[6 * i + 2] = 4 * i + 2;
    indices[6 * i + 3] = 4 * i + 0;
    indices[6 * i + 4] = 4 * i + 2;
    indices[6 * i + 5] = 4 * i + 3;

    data->current_idx++;
}

static void _graphics_segment(DvzCanvas* canvas, DvzGraphics* graphics)
{
    SHADER(VERTEX, "graphics_segment_vert")
    SHADER(FRAGMENT, "graphics_segment_frag")
    PRIMITIVE(TRIANGLE_LIST)
    // PRIMITIVE(POINT_LIST) // DEBUG

    ATTR_BEGIN(DvzGraphicsSegmentVertex)
    ATTR_POS(DvzGraphicsSegmentVertex, P0)
    ATTR_POS(DvzGraphicsSegmentVertex, P1)
    ATTR(DvzGraphicsSegmentVertex, VK_FORMAT_R32G32B32A32_SFLOAT, shift)
    ATTR_COL(DvzGraphicsSegmentVertex, color)
    ATTR(DvzGraphicsSegmentVertex, VK_FORMAT_R32_SFLOAT, linewidth)
    ATTR(DvzGraphicsSegmentVertex, VK_FORMAT_R32_SINT, cap0)
    ATTR(DvzGraphicsSegmentVertex, VK_FORMAT_R32_SINT, cap1)
    ATTR(DvzGraphicsSegmentVertex, VK_FORMAT_R8_UINT, transform)

    _common_slots(graphics);
    dvz_graphics_callback(graphics, _graphics_segment_callback);

    CREATE
}



/*************************************************************************************************/
/*  Text graphics                                                                             */
/*************************************************************************************************/

static void _graphics_text_callback(DvzGraphicsData* data, uint32_t item_count, const void* item)
{
    // NOTE: item_count is the total number of glyphs

    ASSERT(data != NULL);
    ASSERT(data->vertices != NULL);

    ASSERT(item_count > 0);
    dvz_array_resize(data->vertices, 4 * item_count);
    DvzFontAtlas* atlas = &data->graphics->gpu->context->font_atlas;
    ASSERT(atlas != NULL);

    if (item == NULL)
        return;
    ASSERT(item != NULL);
    ASSERT(data->current_idx < item_count);

    // const char* str = item;
    const DvzGraphicsTextItem* str_item = item;
    uint32_t n = strlen(str_item->string);
    DvzGraphicsTextVertex vertex = {0};
    vertex = str_item->vertex;
    ASSERT(n > 0);
    ASSERT(data->current_idx + n <= item_count);
    for (uint32_t i = 0; i < n; i++)
    {
        size_t g = _font_atlas_glyph(atlas, str_item->string, i);

        // Glyph size.
        _font_atlas_glyph_size(atlas, str_item->font_size, vertex.glyph_size);

        // Glyph.
        vertex.glyph[0] = g;                   // char
        vertex.glyph[1] = i;                   // char idx
        vertex.glyph[2] = n;                   // str len
        vertex.glyph[3] = data->current_group; // str idx

        // Glyph colors.
        if (str_item->glyph_colors != NULL)
            memcpy(vertex.color, str_item->glyph_colors[i], sizeof(cvec4));

        // Fill the vertices array by simply repeating them 4 times.
        dvz_array_data(data->vertices, 4 * data->current_idx, 4, 1, &vertex);
        data->current_idx++; // glyph index
    }
    data->current_group++; // glyph index
}

static void _graphics_text(DvzCanvas* canvas, DvzGraphics* graphics)
{
    SHADER(VERTEX, "graphics_text_vert")
    SHADER(FRAGMENT, "graphics_text_frag")
    PRIMITIVE(TRIANGLE_STRIP)

    ATTR_BEGIN(DvzGraphicsTextVertex)
    ATTR_POS(DvzGraphicsTextVertex, pos)
    ATTR(DvzGraphicsTextVertex, VK_FORMAT_R32G32_SFLOAT, shift)
    ATTR_COL(DvzGraphicsTextVertex, color)
    ATTR(DvzGraphicsTextVertex, VK_FORMAT_R32G32_SFLOAT, glyph_size)
    ATTR(DvzGraphicsTextVertex, VK_FORMAT_R32G32_SFLOAT, anchor)
    ATTR(DvzGraphicsTextVertex, VK_FORMAT_R32_SFLOAT, angle)
    ATTR(DvzGraphicsTextVertex, VK_FORMAT_R16G16B16A16_UINT, glyph)
    ATTR(DvzGraphicsTextVertex, VK_FORMAT_R8_UINT, transform)

    _common_slots(graphics);
    dvz_graphics_slot(graphics, DVZ_USER_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    dvz_graphics_slot(graphics, DVZ_USER_BINDING + 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    dvz_graphics_callback(graphics, _graphics_text_callback);

    CREATE
}



/*************************************************************************************************/
/*  Image                                                                                        */
/*************************************************************************************************/

static void _graphics_image_callback(DvzGraphicsData* data, uint32_t item_count, const void* item)
{
    ASSERT(data != NULL);
    ASSERT(data->vertices != NULL);

    ASSERT(item_count > 0);
    dvz_array_resize(data->vertices, 6 * item_count);

    if (item == NULL)
        return;
    ASSERT(item != NULL);
    ASSERT(data->current_idx < item_count);

    const DvzGraphicsImageItem* item_vert = (const DvzGraphicsImageItem*)item;

    DvzGraphicsImageVertex vertices[6] = {0};

    _vec3_copy(item_vert->pos3, vertices[0].pos);
    _vec3_copy(item_vert->pos2, vertices[1].pos);
    _vec3_copy(item_vert->pos1, vertices[2].pos);
    _vec3_copy(item_vert->pos1, vertices[3].pos);
    _vec3_copy(item_vert->pos0, vertices[4].pos);
    _vec3_copy(item_vert->pos3, vertices[5].pos);

    _vec2_copy(item_vert->uv3, vertices[0].uv);
    _vec2_copy(item_vert->uv2, vertices[1].uv);
    _vec2_copy(item_vert->uv1, vertices[2].uv);
    _vec2_copy(item_vert->uv1, vertices[3].uv);
    _vec2_copy(item_vert->uv0, vertices[4].uv);
    _vec2_copy(item_vert->uv3, vertices[5].uv);

    dvz_array_data(data->vertices, 6 * data->current_idx, 6, 6, vertices);

    data->current_idx++;
}

static void _graphics_image(DvzCanvas* canvas, DvzGraphics* graphics)
{
    SHADER(VERTEX, "graphics_image_vert")
    SHADER(FRAGMENT, "graphics_image_frag")
    PRIMITIVE(TRIANGLE_LIST)

    ATTR_BEGIN(DvzGraphicsImageVertex)
    ATTR_POS(DvzGraphicsImageVertex, pos)
    ATTR(DvzGraphicsImageVertex, VK_FORMAT_R32G32_SFLOAT, uv)

    _common_slots(graphics);
    dvz_graphics_slot(graphics, DVZ_USER_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    for (uint32_t i = 1; i <= 4; i++)
        dvz_graphics_slot(
            graphics, DVZ_USER_BINDING + i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    CREATE

    dvz_graphics_callback(graphics, _graphics_image_callback);
}



static void _graphics_image_cmap(DvzCanvas* canvas, DvzGraphics* graphics)
{
    SHADER(VERTEX, "graphics_image_cmap_vert")
    SHADER(FRAGMENT, "graphics_image_cmap_frag")
    PRIMITIVE(TRIANGLE_LIST)

    ATTR_BEGIN(DvzGraphicsImageVertex)
    ATTR_POS(DvzGraphicsImageVertex, pos)
    ATTR(DvzGraphicsImageVertex, VK_FORMAT_R32G32_SFLOAT, uv)

    _common_slots(graphics);

    // Params buffer.
    dvz_graphics_slot(graphics, DVZ_USER_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    // Colormap texture.
    dvz_graphics_slot(graphics, DVZ_USER_BINDING + 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    // Scalar image.
    dvz_graphics_slot(graphics, DVZ_USER_BINDING + 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    CREATE

    dvz_graphics_callback(graphics, _graphics_image_callback);
}



/*************************************************************************************************/
/*  Volume slice                                                                                 */
/*************************************************************************************************/

static void
_graphics_volume_slice_callback(DvzGraphicsData* data, uint32_t item_count, const void* item)
{
    ASSERT(data != NULL);
    ASSERT(data->vertices != NULL);

    ASSERT(item_count > 0);
    dvz_array_resize(data->vertices, 6 * item_count);

    if (item == NULL)
        return;
    ASSERT(item != NULL);
    ASSERT(data->current_idx < item_count);

    const DvzGraphicsVolumeSliceItem* item_vert = (const DvzGraphicsVolumeSliceItem*)item;

    DvzGraphicsVolumeSliceVertex vertices[6] = {0};

    _vec3_copy(item_vert->pos3, vertices[0].pos);
    _vec3_copy(item_vert->pos2, vertices[1].pos);
    _vec3_copy(item_vert->pos1, vertices[2].pos);
    _vec3_copy(item_vert->pos1, vertices[3].pos);
    _vec3_copy(item_vert->pos0, vertices[4].pos);
    _vec3_copy(item_vert->pos3, vertices[5].pos);

    _vec3_copy(item_vert->uvw3, vertices[0].uvw);
    _vec3_copy(item_vert->uvw2, vertices[1].uvw);
    _vec3_copy(item_vert->uvw1, vertices[2].uvw);
    _vec3_copy(item_vert->uvw1, vertices[3].uvw);
    _vec3_copy(item_vert->uvw0, vertices[4].uvw);
    _vec3_copy(item_vert->uvw3, vertices[5].uvw);

    dvz_array_data(data->vertices, 6 * data->current_idx, 6, 6, vertices);

    data->current_idx++;
}

static void _graphics_volume_slice(DvzCanvas* canvas, DvzGraphics* graphics)
{
    SHADER(VERTEX, "graphics_volume_slice_vert")
    SHADER(FRAGMENT, "graphics_volume_slice_frag")
    PRIMITIVE(TRIANGLE_LIST)
    dvz_graphics_depth_test(graphics, DVZ_DEPTH_TEST_ENABLE);

    ATTR_BEGIN(DvzGraphicsVolumeSliceVertex)
    ATTR_POS(DvzGraphicsVolumeSliceVertex, pos)
    ATTR(DvzGraphicsVolumeSliceVertex, VK_FORMAT_R32G32B32_SFLOAT, uvw)

    _common_slots(graphics);
    dvz_graphics_slot(graphics, DVZ_USER_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    dvz_graphics_slot(graphics, DVZ_USER_BINDING + 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    dvz_graphics_slot(graphics, DVZ_USER_BINDING + 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    CREATE

    dvz_graphics_callback(graphics, _graphics_volume_slice_callback);
}



/*************************************************************************************************/
/*  Volume                                                                                       */
/*************************************************************************************************/

static void _graphics_volume_callback(DvzGraphicsData* data, uint32_t item_count, const void* item)
{
    ASSERT(data != NULL);
    ASSERT(data->vertices != NULL);

    ASSERT(item_count > 0);
    dvz_array_resize(data->vertices, 36 * item_count);

    if (item == NULL)
        return;
    ASSERT(item != NULL);
    ASSERT(data->current_idx < item_count);

    const DvzGraphicsVolumeItem* item_vert = (const DvzGraphicsVolumeItem*)item;

    float x0 = item_vert->pos0[0];
    float y0 = item_vert->pos0[1];
    float z0 = item_vert->pos0[2];

    float x1 = item_vert->pos1[0];
    float y1 = item_vert->pos1[1];
    float z1 = item_vert->pos1[2];

    // TODO: other volume orientations

    float u0 = item_vert->uvw0[0];
    float v0 = item_vert->uvw0[1];
    float w0 = item_vert->uvw0[2];

    float u1 = item_vert->uvw1[0];
    float v1 = item_vert->uvw1[1];
    float w1 = item_vert->uvw1[2];

    // pos, uvw
    DvzGraphicsVolumeVertex vertices[36] = {
        {{x0, y0, z1}, {u0, v0, w1}}, // front
        {{x1, y0, z1}, {u1, v0, w1}}, //
        {{x1, y1, z1}, {u1, v1, w1}}, //
        {{x1, y1, z1}, {u1, v1, w1}}, //
        {{x0, y1, z1}, {u0, v1, w1}}, //
        {{x0, y0, z1}, {u0, v0, w1}}, //
                                      //
        {{x1, y0, z1}, {u1, v0, w1}}, // right
        {{x1, y0, z0}, {u1, v0, w0}}, //
        {{x1, y1, z0}, {u1, v1, w0}}, //
        {{x1, y1, z0}, {u1, v1, w0}}, //
        {{x1, y1, z1}, {u1, v1, w1}}, //
        {{x1, y0, z1}, {u1, v0, w1}}, //
                                      //
        {{x0, y1, z0}, {u0, v1, w0}}, // back
        {{x1, y1, z0}, {u1, v1, w0}}, //
        {{x1, y0, z0}, {u1, v0, w0}}, //
        {{x1, y0, z0}, {u1, v0, w0}}, //
        {{x0, y0, z0}, {u0, v0, w0}}, //
        {{x0, y1, z0}, {u0, v1, w0}}, //
                                      //
        {{x0, y0, z0}, {u0, v0, w0}}, // left
        {{x0, y0, z1}, {u0, v0, w1}}, //
        {{x0, y1, z1}, {u0, v1, w1}}, //
        {{x0, y1, z1}, {u0, v1, w1}}, //
        {{x0, y1, z0}, {u0, v1, w0}}, //
        {{x0, y0, z0}, {u0, v0, w0}}, //
                                      //
        {{x0, y0, z0}, {u0, v0, w0}}, // bottom
        {{x1, y0, z0}, {u1, v0, w0}}, //
        {{x1, y0, z1}, {u1, v0, w1}}, //
        {{x1, y0, z1}, {u1, v0, w1}}, //
        {{x0, y0, z1}, {u0, v0, w1}}, //
        {{x0, y0, z0}, {u0, v0, w0}}, //
                                      //
        {{x0, y1, z1}, {u0, v1, w1}}, // top
        {{x1, y1, z1}, {u1, v1, w1}}, //
        {{x1, y1, z0}, {u1, v1, w0}}, //
        {{x1, y1, z0}, {u1, v1, w0}}, //
        {{x0, y1, z0}, {u0, v1, w0}}, //
        {{x0, y1, z1}, {u0, v1, w1}}, //
    };

    dvz_array_data(data->vertices, 36 * data->current_idx, 36, 36, vertices);
    data->current_idx++;
}

static void _graphics_volume(DvzCanvas* canvas, DvzGraphics* graphics)
{
    SHADER(VERTEX, "graphics_volume_vert")
    SHADER(FRAGMENT, "graphics_volume_frag")
    PRIMITIVE(TRIANGLE_LIST)
    dvz_graphics_depth_test(graphics, DVZ_DEPTH_TEST_ENABLE);

    ATTR_BEGIN(DvzGraphicsVolumeVertex)
    ATTR_POS(DvzGraphicsVolumeVertex, pos)
    ATTR(DvzGraphicsVolumeVertex, VK_FORMAT_R32G32B32_SFLOAT, uvw)

    _common_slots(graphics);
    dvz_graphics_slot(graphics, DVZ_USER_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    dvz_graphics_slot(graphics, DVZ_USER_BINDING + 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    dvz_graphics_slot(graphics, DVZ_USER_BINDING + 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    CREATE

    dvz_graphics_callback(graphics, _graphics_volume_callback);
}



/*************************************************************************************************/
/*  3D mesh                                                                                      */
/*************************************************************************************************/

static void _graphics_mesh(DvzCanvas* canvas, DvzGraphics* graphics)
{
    SHADER(VERTEX, "graphics_mesh_vert")
    SHADER(FRAGMENT, "graphics_mesh_frag")
    PRIMITIVE(TRIANGLE_LIST)
    dvz_graphics_depth_test(graphics, DVZ_DEPTH_TEST_ENABLE);
    // dvz_graphics_front_face(graphics, VK_FRONT_FACE_CLOCKWISE);
    // dvz_graphics_cull_mode(graphics, VK_CULL_MODE_FRONT_BIT);

    ATTR_BEGIN(DvzGraphicsMeshVertex)
    ATTR_POS(DvzGraphicsMeshVertex, pos)
    ATTR(DvzGraphicsMeshVertex, VK_FORMAT_R32G32B32_SFLOAT, normal)
    ATTR(DvzGraphicsMeshVertex, VK_FORMAT_R32G32_SFLOAT, uv)
    ATTR(DvzGraphicsMeshVertex, VK_FORMAT_R8_UNORM, alpha)

    _common_slots(graphics);
    dvz_graphics_slot(graphics, DVZ_USER_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    for (uint32_t i = 1; i <= 4; i++)
        dvz_graphics_slot(
            graphics, DVZ_USER_BINDING + i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    CREATE
}



/*************************************************************************************************/
/*  Graphics data                                                                                */
/*************************************************************************************************/

static void _default_callback(DvzGraphicsData* data, uint32_t item_count, const void* item)
{
    ASSERT(data != NULL);
    ASSERT(data->vertices != NULL);

    dvz_array_resize(data->vertices, item_count);
    if (item == NULL)
        return;
    ASSERT(item != NULL);
    ASSERT(data->current_idx < item_count);

    // Fill the vertices array by simply copying the current item (assumed to be a vertex).
    dvz_array_data(data->vertices, data->current_idx++, 1, 1, item);
}

// Used by graphics creator
void dvz_graphics_callback(DvzGraphics* graphics, DvzGraphicsCallback callback)
{
    // The callback must make sure the DvzArray* are not NULL and resize them
    ASSERT(graphics != NULL);
    graphics->callback = callback;
}



// Used in visual bake:
DvzGraphicsData
dvz_graphics_data(DvzGraphics* graphics, DvzArray* vertices, DvzArray* indices, void* user_data)
{
    ASSERT(graphics != NULL);
    ASSERT(vertices != NULL);

    DvzGraphicsData data = {0};
    data.graphics = graphics;
    data.vertices = vertices;
    data.indices = indices;
    data.user_data = user_data;

    if (graphics->callback == NULL)
        graphics->callback = _default_callback;
    return data;
}



void dvz_graphics_alloc(DvzGraphicsData* data, uint32_t item_count)
{
    ASSERT(data != NULL);
    if (item_count == 0)
    {
        log_error("empty graphics allocation");
        return;
    }
    ASSERT(item_count > 0);
    data->item_count = item_count;
    DvzGraphics* graphics = data->graphics;
    ASSERT(graphics != NULL);

    // The graphics callback should allocate the vertices and indices arrays.
    ASSERT(graphics->callback != NULL);
    graphics->callback(data, item_count, NULL);
}



void dvz_graphics_append(DvzGraphicsData* data, const void* item)
{
    ASSERT(data != NULL);
    ASSERT(item != NULL);
    DvzGraphics* graphics = data->graphics;
    ASSERT(graphics != NULL);

    // call the callback with item_count and item
    ASSERT(graphics->callback != NULL);
    graphics->callback(data, data->item_count, item);
}



/*************************************************************************************************/
/*  Graphics builtin                                                                             */
/*************************************************************************************************/

static DvzGraphics* _find_graphics(DvzCanvas* canvas, DvzGraphicsType type, int flags)
{
    ASSERT(canvas != NULL);
    ASSERT(type != DVZ_GRAPHICS_CUSTOM);

    DvzContainerIterator iter = dvz_container_iterator(&canvas->graphics);
    DvzGraphics* graphics = NULL;
    if (iter.item != NULL)
    {
        graphics = iter.item;
        if (graphics->type == type && graphics->flags == flags)
            return graphics;
        dvz_container_iter(&iter);
    }
    return NULL;
}



DvzGraphics* dvz_graphics_builtin(DvzCanvas* canvas, DvzGraphicsType type, int flags)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->gpu != NULL);
    ASSERT(type != DVZ_GRAPHICS_NONE);
    ASSERT(canvas->graphics.capacity > 0);

    // Try to find an existing graphics with the requested type and flags.
    DvzGraphics* graphics = _find_graphics(canvas, type, flags);
    if (graphics != NULL)
        return graphics;

    // If there is none, create a new one.
    graphics = dvz_container_alloc(&canvas->graphics);
    ASSERT(graphics != NULL);
    ASSERT(!dvz_obj_is_created(&graphics->obj));
    *graphics = dvz_graphics(canvas->gpu);
    graphics->type = type;
    graphics->flags = flags;

    switch (type)
    {

        // Basic graphics types.
    case DVZ_GRAPHICS_POINT:
        _graphics_point(canvas, graphics);
        break;

    case DVZ_GRAPHICS_LINE:
        _graphics_basic(canvas, graphics, VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
        break;

    case DVZ_GRAPHICS_LINE_STRIP:
        _graphics_basic(canvas, graphics, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
        break;

    case DVZ_GRAPHICS_TRIANGLE:
        _graphics_basic(canvas, graphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        break;

    case DVZ_GRAPHICS_TRIANGLE_STRIP:
        _graphics_basic(canvas, graphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
        break;

    case DVZ_GRAPHICS_TRIANGLE_FAN:
        _graphics_basic(canvas, graphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN);
        break;


        // Agg graphics types.
    case DVZ_GRAPHICS_MARKER:
        _graphics_marker(canvas, graphics);
        break;

    case DVZ_GRAPHICS_SEGMENT:
        _graphics_segment(canvas, graphics);
        break;

    case DVZ_GRAPHICS_TEXT:
        _graphics_text(canvas, graphics);
        break;


        // Image
    case DVZ_GRAPHICS_IMAGE:
        _graphics_image(canvas, graphics);
        break;

        // Image cmap
    case DVZ_GRAPHICS_IMAGE_CMAP:
        _graphics_image_cmap(canvas, graphics);
        break;

        // Volume slice
    case DVZ_GRAPHICS_VOLUME_SLICE:
        _graphics_volume_slice(canvas, graphics);
        break;

        // Volume
    case DVZ_GRAPHICS_VOLUME:
        _graphics_volume(canvas, graphics);
        break;


        // 3D meshes
    case DVZ_GRAPHICS_MESH:
        _graphics_mesh(canvas, graphics);
        break;

    case DVZ_GRAPHICS_CUSTOM:
        break;

    default:
        log_error("no graphics type specified");
        break;
    }

    return graphics;
}



void dvz_mvp_camera(DvzViewport viewport, vec3 eye, vec3 center, vec2 near_far, DvzMVP* mvp)
{
    vec3 up = {0, 1, 0};
    glm_lookat(eye, center, up, mvp->view);
    ASSERT(viewport.size_framebuffer[1] > 0);
    float ratio = viewport.size_framebuffer[0] / (float)viewport.size_framebuffer[1];
    glm_perspective(GLM_PI_4, ratio, near_far[0], near_far[1], mvp->proj);
}
