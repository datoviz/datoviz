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



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzVisual* dvz_glyph(DvzBatch* batch, int flags)
{
    ANN(batch);

    // NOTE: we force indexed rendering in this visual.
    flags |= DVZ_VISUALS_FLAGS_INDEXED;

    DvzVisual* visual = dvz_visual(batch, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, flags);
    ANN(visual);

    // Visual shaders.
    dvz_visual_shader(visual, "graphics_glyph");

    // Vertex attributes.
    dvz_visual_attr(visual, 0, FIELD(DvzGlyphVertex, pos), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
    dvz_visual_attr(visual, 1, FIELD(DvzGlyphVertex, normal), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
    dvz_visual_attr(visual, 2, FIELD(DvzGlyphVertex, anchor), DVZ_FORMAT_R32G32_SFLOAT, 0);
    dvz_visual_attr(visual, 3, FIELD(DvzGlyphVertex, shift), DVZ_FORMAT_R32G32_SFLOAT, 0);
    dvz_visual_attr(visual, 4, FIELD(DvzGlyphVertex, uv), DVZ_FORMAT_R32G32_SFLOAT, 0);
    dvz_visual_attr(visual, 5, FIELD(DvzGlyphVertex, angle), DVZ_FORMAT_R32_SFLOAT, 0);
    dvz_visual_attr(visual, 6, FIELD(DvzGlyphVertex, color), DVZ_FORMAT_R8G8B8A8_UNORM, 0);

    // Vertex stride.
    dvz_visual_stride(visual, 0, sizeof(DvzGlyphVertex));

    // Slots.
    dvz_visual_slot(visual, 0, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 1, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 2, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 3, DVZ_SLOT_TEX);

    // Params.
    DvzParams* params = dvz_visual_params(visual, 3, sizeof(DvzGlyphParams));
    dvz_params_attr(params, 0, FIELD(DvzGlyphParams, size));

    // Default texture to avoid Vulkan warning with unbound texture slot.
    dvz_visual_tex(
        visual, 3, DVZ_SCENE_DEFAULT_TEX_ID, DVZ_SCENE_DEFAULT_SAMPLER_ID, DVZ_ZERO_OFFSET);

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
    // TODO
}



void dvz_glyph_normal(DvzVisual* visual, uint32_t first, uint32_t count, vec3* normals, int flags)
{
    ANN(visual);
    // TODO
}



void dvz_glyph_color(DvzVisual* visual, uint32_t first, uint32_t count, cvec4* values, int flags)
{
    ANN(visual);
    // TODO
}



void dvz_glyph_anchor(DvzVisual* visual, uint32_t first, uint32_t count, vec2* values, int flags)
{
    ANN(visual);
    // TODO
}



void dvz_glyph_shift(DvzVisual* visual, uint32_t first, uint32_t count, vec2* values, int flags)
{
    ANN(visual);
    // TODO
}



void dvz_glyph_angle(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags)
{
    ANN(visual);
    // TODO
}



void dvz_glyph_texcoords(
    DvzVisual* visual, uint32_t first, uint32_t count, vec4* coords, int flags)
{
    ANN(visual);
    // TODO
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
    // TODO
}
