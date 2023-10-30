/*************************************************************************************************/
/*  Glyph                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/glyph.h"
#include "fileio.h"
#include "request.h"
#include "scene/atlas.h"
#include "scene/graphics.h"
#include "scene/scene.h"
#include "scene/viewset.h"
#include "scene/visual.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Internal functions                                                                           */
/*************************************************************************************************/

static void _visual_callback(
    DvzVisual* visual, DvzId canvas, //
    uint32_t first, uint32_t count,  //
    uint32_t first_instance, uint32_t instance_count)
{
    ANN(visual);
    ASSERT(count > 0);

    // NOTE: we draw 2 triangles, or 6 indices, for each glyph.
    dvz_visual_instance(visual, canvas, 6 * first, 0, 6 * count, first_instance, instance_count);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzVisual* dvz_glyph(DvzBatch* batch, int flags)
{
    ANN(batch);

    // NOTE: we force indexed rendering in this visual.
    flags |= DVZ_VISUALS_FLAGS_INDEXED;

    DvzVisual* visual = dvz_visual(batch, DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST, flags);
    ANN(visual);

    // Visual shaders.
    dvz_visual_shader(visual, "graphics_glyph");

    // Vertex attributes.
    int af = DVZ_ATTR_FLAGS_REPEAT_X4;
    dvz_visual_attr(visual, 0, FIELD(DvzGlyphVertex, pos), DVZ_FORMAT_R32G32B32_SFLOAT, af);
    dvz_visual_attr(visual, 1, FIELD(DvzGlyphVertex, normal), DVZ_FORMAT_R32G32B32_SFLOAT, af);
    dvz_visual_attr(visual, 2, FIELD(DvzGlyphVertex, anchor), DVZ_FORMAT_R32G32_SFLOAT, af);
    dvz_visual_attr(visual, 3, FIELD(DvzGlyphVertex, shift), DVZ_FORMAT_R32G32_SFLOAT, af);
    dvz_visual_attr(visual, 4, FIELD(DvzGlyphVertex, uv), DVZ_FORMAT_R32G32_SFLOAT, 0); // no rep
    dvz_visual_attr(visual, 5, FIELD(DvzGlyphVertex, angle), DVZ_FORMAT_R32_SFLOAT, af);
    dvz_visual_attr(visual, 6, FIELD(DvzGlyphVertex, color), DVZ_FORMAT_R8G8B8A8_UNORM, af);

    // Vertex stride.
    dvz_visual_stride(visual, 0, sizeof(DvzGlyphVertex));

    // Slots.
    dvz_visual_slot(visual, 0, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 1, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 2, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 3, DVZ_SLOT_TEX);

    // Params.
    DvzParams* params = dvz_visual_params(visual, 2, sizeof(DvzGlyphParams));
    dvz_params_attr(params, 0, FIELD(DvzGlyphParams, size));

    // Default texture to avoid Vulkan warning with unbound texture slot.
    dvz_visual_tex(
        visual, 3, DVZ_SCENE_DEFAULT_TEX_ID, DVZ_SCENE_DEFAULT_SAMPLER_ID, DVZ_ZERO_OFFSET);

    // Visual draw callback.
    dvz_visual_callback(visual, _visual_callback);

    return visual;
}



void dvz_glyph_alloc(DvzVisual* visual, uint32_t item_count)
{
    ANN(visual);
    log_debug("allocating the glyph visual");

    DvzBatch* batch = visual->batch;
    ANN(batch);

    // Create the visual: for each glyph, 4 vertices (corners) and 6 indices (quad).
    dvz_visual_alloc(visual, item_count, 4 * item_count, 6 * item_count);

    // Indices.
    DvzIndex* indices = (DvzIndex*)calloc(6 * item_count, sizeof(DvzIndex));
    for (uint32_t i = 0; i < item_count; i++)
    {
        indices[6 * i + 0] = 4 * i + 0;
        indices[6 * i + 1] = 4 * i + 1;
        indices[6 * i + 2] = 4 * i + 2;
        indices[6 * i + 3] = 4 * i + 0;
        indices[6 * i + 4] = 4 * i + 2;
        indices[6 * i + 5] = 4 * i + 3;
    }
    dvz_visual_index(visual, 0, item_count * 6, indices);
    FREE(indices);
}



void dvz_glyph_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 0, first, count, (void*)values);
}



void dvz_glyph_normal(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 1, first, count, (void*)values);
}



void dvz_glyph_anchor(DvzVisual* visual, uint32_t first, uint32_t count, vec2* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 2, first, count, (void*)values);
}



void dvz_glyph_shift(DvzVisual* visual, uint32_t first, uint32_t count, vec2* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 3, first, count, (void*)values);
}



void dvz_glyph_texcoords(
    DvzVisual* visual, uint32_t first, uint32_t count, vec4* coords, int flags)
{
    ANN(visual);
    // coords is u0,v0,u1,v1, need to upload 4 vec2 corresponding to each corner

    vec2* uv = (vec2*)calloc(4 * count, sizeof(vec2));
    float u0, v0, u1, v1; // upper-left, lower-right
    for (uint32_t i = 0; i < count; i++)
    {
        u0 = coords[i][0];
        v0 = coords[i][1];
        u1 = coords[i][2];
        v1 = coords[i][3];

        // lower-left
        uv[4 * i + 0][0] = u0;
        uv[4 * i + 0][1] = v1;
        // lower-right
        uv[4 * i + 1][0] = u1;
        uv[4 * i + 1][1] = v1;
        // upper-right
        uv[4 * i + 2][0] = u1;
        uv[4 * i + 2][1] = v0;
        // upper-left
        uv[4 * i + 3][0] = u0;
        uv[4 * i + 3][1] = v0;
    }
    dvz_visual_data(visual, 4, first, count, (void*)uv);
    FREE(uv);
}



void dvz_glyph_angle(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 5, first, count, (void*)values);
}



void dvz_glyph_color(DvzVisual* visual, uint32_t first, uint32_t count, cvec4* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 6, first, count, (void*)values);
}



void dvz_glyph_texture(DvzVisual* visual, DvzId tex)
{
    ANN(visual);

    DvzBatch* batch = visual->batch;
    ANN(batch);

    // TODO: check if LINEAR ok
    DvzId sampler =
        dvz_create_sampler(batch, DVZ_FILTER_LINEAR, DVZ_SAMPLER_ADDRESS_MODE_REPEAT).id;

    // Bind texture to the visual.
    dvz_visual_tex(visual, 3, tex, sampler, DVZ_ZERO_OFFSET);
}



void dvz_glyph_atlas(DvzVisual* visual, DvzAtlas* atlas)
{
    ANN(visual);

    DvzBatch* batch = visual->batch;
    ANN(batch);

    // Create the atlas texture.
    DvzId tex = dvz_atlas_texture(atlas, batch);
    ASSERT(tex != DVZ_ID_NONE);

    // Bind the texture to the glyph visual.
    dvz_glyph_texture(visual, tex);
}



void dvz_glyph_size(DvzVisual* visual, vec2 size)
{
    ANN(visual);
    dvz_visual_param(visual, 2, 0, size);
}
