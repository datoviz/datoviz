/*************************************************************************************************/
/*  Data graphics creation helpers                                                               */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/graphics_data.h"
#include "scene/graphics.h"
#include "scene/array.h"



/*************************************************************************************************/
/*  Graphics data callbacks                                                                      */
/*************************************************************************************************/

static void
_graphics_segment_callback(DvzGraphicsData* data, uint32_t item_count, const void* item)
{
    ANN(data);
    ANN(data->vertices);
    ANN(data->indices);

    ASSERT(item_count > 0);
    dvz_array_resize(data->vertices, 4 * item_count);
    dvz_array_resize(data->indices, 6 * item_count);

    if (item == NULL)
        return;
    ANN(item);
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



// Called when adding a single point to the path.
// NOTE: item_count is the TOTAL number of points, including junction points.
static void _graphics_path_callback(DvzGraphicsData* data, uint32_t item_count, const void* item)
{
    ANN(data);
    ANN(data->vertices);

    ASSERT(item_count > 0);
    dvz_array_resize(data->vertices, 4 * item_count); // 4 vertices per point
    // no indices

    if (item == NULL)
        return;
    ANN(item);
    ASSERT(data->current_idx < item_count);

    // Simply repeat the vertex 4 times.
    dvz_array_data(data->vertices, 4 * data->current_idx, 4, 1, item);

    data->current_idx++;
}



static void _graphics_text_callback(DvzGraphicsData* data, uint32_t item_count, const void* item)
{
    // NOTE: item_count is the total number of glyphs

    ANN(data);
    ANN(data->vertices);

    ASSERT(item_count > 0);
    dvz_array_resize(data->vertices, 4 * item_count);
    // DvzFontAtlas* atlas = &data->graphics->gpu->context->font_atlas;
    // ANN(atlas);

    if (item == NULL)
        return;
    ANN(item);
    ASSERT(data->current_idx < item_count);

    // const char* str = item;
    const DvzGraphicsTextItem* str_item = item;
    uint32_t n = strlen(str_item->string);
    DvzGraphicsTextVertex vertex = {0};
    vertex = str_item->vertex;
    ASSERT(n > 0);
    ASSERT(data->current_idx + n <= item_count);
    size_t g = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        // TODO: improve glyph size
        // size_t g = _font_atlas_glyph(atlas, str_item->string, i);

        // // Glyph size.
        // _font_atlas_glyph_size(atlas, str_item->font_size, vertex.glyph_size);

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



static void _graphics_image_callback(DvzGraphicsData* data, uint32_t item_count, const void* item)
{
    ANN(data);
    ANN(data->vertices);

    ASSERT(item_count > 0);
    dvz_array_resize(data->vertices, 6 * item_count);

    if (item == NULL)
        return;
    ANN(item);
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



static void
_graphics_volume_slice_callback(DvzGraphicsData* data, uint32_t item_count, const void* item)
{
    ANN(data);
    ANN(data->vertices);

    ASSERT(item_count > 0);
    dvz_array_resize(data->vertices, 6 * item_count);

    if (item == NULL)
        return;
    ANN(item);
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



static void _graphics_volume_callback(DvzGraphicsData* data, uint32_t item_count, const void* item)
{
    ANN(data);
    ANN(data->vertices);

    ASSERT(item_count > 0);
    dvz_array_resize(data->vertices, 36 * item_count);

    if (item == NULL)
        return;
    ANN(item);
    ASSERT(data->current_idx < item_count);

    const DvzGraphicsVolumeItem* item_vert = (const DvzGraphicsVolumeItem*)item;

    float x0 = item_vert->pos0[0];
    float y0 = item_vert->pos0[1];
    float z0 = item_vert->pos0[2];

    float x1 = item_vert->pos1[0];
    float y1 = item_vert->pos1[1];
    float z1 = item_vert->pos1[2];

    // pos
    DvzGraphicsVolumeVertex vertices[36] = {
        {{x0, y0, z1}}, // front
        {{x1, y0, z1}}, //
        {{x1, y1, z1}}, //
        {{x1, y1, z1}}, //
        {{x0, y1, z1}}, //
        {{x0, y0, z1}}, //
                        //
        {{x1, y0, z1}}, // right
        {{x1, y0, z0}}, //
        {{x1, y1, z0}}, //
        {{x1, y1, z0}}, //
        {{x1, y1, z1}}, //
        {{x1, y0, z1}}, //
                        //
        {{x0, y1, z0}}, // back
        {{x1, y1, z0}}, //
        {{x1, y0, z0}}, //
        {{x1, y0, z0}}, //
        {{x0, y0, z0}}, //
        {{x0, y1, z0}}, //
                        //
        {{x0, y0, z0}}, // left
        {{x0, y0, z1}}, //
        {{x0, y1, z1}}, //
        {{x0, y1, z1}}, //
        {{x0, y1, z0}}, //
        {{x0, y0, z0}}, //
                        //
        {{x0, y0, z0}}, // bottom
        {{x1, y0, z0}}, //
        {{x1, y0, z1}}, //
        {{x1, y0, z1}}, //
        {{x0, y0, z1}}, //
        {{x0, y0, z0}}, //
                        //
        {{x0, y1, z1}}, // top
        {{x1, y1, z1}}, //
        {{x1, y1, z0}}, //
        {{x1, y1, z0}}, //
        {{x0, y1, z0}}, //
        {{x0, y1, z1}}, //
    };

    dvz_array_data(data->vertices, 36 * data->current_idx, 36, 36, vertices);
    data->current_idx++;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

static void _default_callback(DvzGraphicsData* data, uint32_t item_count, const void* item)
{
    ANN(data);
    ANN(data->vertices);

    dvz_array_resize(data->vertices, item_count);
    if (item == NULL)
        return;
    ANN(item);
    ASSERT(data->current_idx < item_count);

    // Fill the vertices array by simply copying the current item (assumed to be a vertex).
    dvz_array_data(data->vertices, data->current_idx++, 1, 1, item);
}



// Used in visual bake:
DvzGraphicsData dvz_graphics_data(
    DvzGraphicsCallback callback, DvzArray* vertices, DvzArray* indices, void* user_data)
{
    ANN(vertices);

    DvzGraphicsData data = {0};
    data.callback = callback == NULL ? _default_callback : callback;
    data.vertices = vertices;
    data.indices = indices;
    data.user_data = user_data;
    return data;
}



void dvz_graphics_alloc(DvzGraphicsData* data, uint32_t item_count)
{
    ANN(data);
    if (item_count == 0)
    {
        log_error("empty graphics allocation");
        return;
    }
    ASSERT(item_count > 0);
    data->item_count = item_count;
    DvzGraphicsCallback callback = data->callback;

    // The graphics callback should allocate the vertices and indices arrays.
    ANN(callback);
    callback(data, item_count, NULL);
}



void dvz_graphics_append(DvzGraphicsData* data, const void* item)
{
    ANN(data);
    ANN(item);

    // call the callback with item_count and item
    DvzGraphicsCallback callback = data->callback;
    ANN(callback);

    callback(data, data->item_count, item);
}
