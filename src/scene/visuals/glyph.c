/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Glyph                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/glyph.h"
#include "_string_utils.h"
#include "datoviz.h"
#include "datoviz_protocol.h"
#include "datoviz_types.h"
#include "fileio.h"
#include "scene/atlas.h"
#include "scene/graphics.h"
#include "scene/scene.h"
#include "scene/texture.h"
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
    flags |= DVZ_VISUAL_FLAGS_INDEXED;

    DvzVisual* visual = dvz_visual(batch, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, flags);
    ANN(visual);

    // Visual shaders.
    dvz_visual_shader(visual, "graphics_glyph");

    // Vertex attributes.
    int af = DVZ_ATTR_FLAGS_REPEAT_X4;
    dvz_visual_attr(visual, 0, FIELD(DvzGlyphVertex, pos), DVZ_FORMAT_R32G32B32_SFLOAT, af);
    dvz_visual_attr(visual, 1, FIELD(DvzGlyphVertex, axis), DVZ_FORMAT_R32G32B32_SFLOAT, af);
    dvz_visual_attr(visual, 2, FIELD(DvzGlyphVertex, size), DVZ_FORMAT_R32G32_SFLOAT, af);
    dvz_visual_attr(visual, 3, FIELD(DvzGlyphVertex, anchor), DVZ_FORMAT_R32G32_SFLOAT, af);
    dvz_visual_attr(visual, 4, FIELD(DvzGlyphVertex, shift), DVZ_FORMAT_R32G32_SFLOAT, af);
    dvz_visual_attr(visual, 5, FIELD(DvzGlyphVertex, uv), DVZ_FORMAT_R32G32_SFLOAT, 0); // no rep
    dvz_visual_attr(visual, 6, FIELD(DvzGlyphVertex, group_shape), DVZ_FORMAT_R32G32_SFLOAT, af);
    dvz_visual_attr(visual, 7, FIELD(DvzGlyphVertex, scale), DVZ_FORMAT_R32_SFLOAT, af);
    dvz_visual_attr(visual, 8, FIELD(DvzGlyphVertex, angle), DVZ_FORMAT_R32_SFLOAT, af);
    dvz_visual_attr(visual, 9, FIELD(DvzGlyphVertex, color), DVZ_FORMAT_COLOR, af);

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
    dvz_params_attr(params, 1, FIELD(DvzGlyphParams, bgcolor));

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
    log_debug("allocating the glyph visual: %d items", item_count);

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



void dvz_glyph_axis(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 1, first, count, (void*)values);
}



void dvz_glyph_size(DvzVisual* visual, uint32_t first, uint32_t count, vec2* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 2, first, count, (void*)values);
}



void dvz_glyph_anchor(DvzVisual* visual, uint32_t first, uint32_t count, vec2* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 3, first, count, (void*)values);
}



void dvz_glyph_shift(DvzVisual* visual, uint32_t first, uint32_t count, vec2* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 4, first, count, (void*)values);
}



void dvz_glyph_texcoords(
    DvzVisual* visual, uint32_t first, uint32_t count, vec4* coords, int flags)
{
    ANN(visual);
    // coords is u0,v0,w,h ; need to upload 4 vec2 corresponding to each corner

    vec2* uv = (vec2*)calloc(4 * count, sizeof(vec2));
    float u0, v0, u1, v1; // top left, bottom right
    for (uint32_t i = 0; i < count; i++)
    {
        u0 = coords[i][0];
        v0 = coords[i][1];
        u1 = u0 + coords[i][2];
        v1 = v0 + coords[i][3];
        // log_error("%.3f %.3f %.3f %.3f", u0, v0, u1, v1);
        // ASSERT(u0 <= u1);
        // ASSERT(v0 <= v1);

        // bottom left
        uv[4 * i + 0][0] = u0;
        uv[4 * i + 0][1] = v1;

        // bottom right
        uv[4 * i + 1][0] = u1;
        uv[4 * i + 1][1] = v1;

        // top right
        uv[4 * i + 2][0] = u1;
        uv[4 * i + 2][1] = v0;

        // top left
        uv[4 * i + 3][0] = u0;
        uv[4 * i + 3][1] = v0;
    }
    dvz_visual_data(visual, 5, 4 * first, 4 * count, (void*)uv);
    FREE(uv);
}



void dvz_glyph_group_size(
    DvzVisual* visual, uint32_t first, uint32_t count, vec2* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 6, first, count, (void*)values);
}



void dvz_glyph_scale(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 7, first, count, (void*)values);
}



void dvz_glyph_angle(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 8, first, count, (void*)values);
}



void dvz_glyph_color(
    DvzVisual* visual, uint32_t first, uint32_t count, DvzColor* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 9, first, count, (void*)values);
}



void dvz_glyph_bgcolor(DvzVisual* visual, vec4 bgcolor)
{
    ANN(visual);
    dvz_visual_param(visual, 2, 1, bgcolor);
}



void dvz_glyph_texture(DvzVisual* visual, DvzTexture* texture)
{
    ANN(visual);
    ANN(texture);

    // Bind texture to the visual.
    dvz_visual_tex(visual, 3, texture->tex, texture->sampler, DVZ_ZERO_OFFSET);
}



void dvz_glyph_atlas_font(DvzVisual* visual, DvzAtlasFont* af)
{
    ANN(visual);
    ANN(af);
    ANN(af->atlas);
    ANN(af->font);

    DvzBatch* batch = visual->batch;
    ANN(batch);

    // Store the pointer to the atlas.
    visual->user_data = (void*)af;

    // Create the atlas texture.
    DvzTexture* texture = dvz_atlas_texture(af->atlas, batch);
    if (texture == NULL)
    {
        log_error("failed creating atlas texture");
        return;
    }

    // Bind the texture to the glyph visual.
    dvz_glyph_texture(visual, texture);
}



void dvz_glyph_unicode(DvzVisual* visual, uint32_t count, uint32_t* codepoints)
{

    ANN(visual);
    ANN(codepoints);
    ASSERT(count > 0);

    DvzAtlasFont* af = (DvzAtlasFont*)visual->user_data;
    if (af == NULL)
    {
        log_error("please call dvz_glyph_atlas_font() first");
        return;
    }
    ANN(af);
    ANN(af->atlas);

    uvec3 shape = {0};
    dvz_atlas_shape(af->atlas, shape);
    float w = shape[0];
    float h = shape[1];

    vec4* texcoords = dvz_atlas_glyphs(af->atlas, count, codepoints); // to free

    // HACK: remove the padding around the glyphs in the atlas, because the freetype positioning
    // implementation assumes no padding, whereas the atlas requires them to prevent edge effects
    // in the fragment shader.
    float padw = 1.25;
    float padh = 1.5;

    for (uint32_t i = 0; i < count; i++)
    {
        // Now, we need to divide the texcoords (in pixels) by the atlas shape, to get uv
        // normalized coordinates.
        texcoords[i][0] = (texcoords[i][0] + padw) / w;
        texcoords[i][1] = (texcoords[i][1] + padh) / h;
        texcoords[i][2] = (texcoords[i][2] - 2 * padw) / w;
        texcoords[i][3] = (texcoords[i][3] - 2 * padh) / h;
    }

    dvz_glyph_texcoords(visual, 0, count, texcoords, 0);

    FREE(texcoords);
}



void dvz_glyph_ascii(DvzVisual* visual, const char* string)
{
    ANN(visual);
    ANN(string);

    // Convert ASCII to Unicode.
    uint32_t count = 0;
    uint32_t* codepoints = _ascii_to_utf32(string, &count);

    dvz_glyph_unicode(visual, count, codepoints);
}



void dvz_glyph_xywh(
    DvzVisual* visual, uint32_t first, uint32_t count, vec4* values, vec2 offset, int flags)
{
    ANN(visual);
    if (values == NULL)
        return;
    ANN(values);
    ASSERT(count > 0);

    vec2* size = (vec2*)calloc(count, sizeof(vec2));
    vec2* shift = (vec2*)calloc(count, sizeof(vec2));

    for (uint32_t i = 0; i < count; i++)
    {
        size[i][0] = values[i][2]; // w
        size[i][1] = values[i][3]; // h

        shift[i][0] = values[i][0] + offset[0]; // x
        shift[i][1] = values[i][1] + offset[1]; // y
    }

    dvz_glyph_size(visual, first, count, size, 0);
    dvz_glyph_shift(visual, first, count, shift, 0);

    // Compute the width of each group.
    uint32_t group_count = visual->group_count;
    uint32_t* group_size = visual->group_sizes;

    if (group_count > 0)
    {
        vec2* group_shapes = (vec2*)calloc(group_count, sizeof(vec2));
        uint32_t k = 0;
        float group_width = 0.0;
        float group_height = 0.0;

        // Loop over the groups.
        for (uint32_t i = 0; i < group_count; i++)
        {
            // Reset the width of the current group.
            group_width = 0;
            group_height = 0;

            // Loop over the glyphs in each group.
            for (uint32_t j = 0; j < group_size[i]; j++)
            {
                // NOTE: we add the width of all glyphs, but we take the max of the height of all
                // glyphs, to get the group width and height (group shape).
                group_width += size[k][0];
                group_height = MAX(group_height, size[k][1]);
                k++;
            }

            group_shapes[i][0] = group_width;
            group_shapes[i][1] = group_height;
        }

        // We need to pass the group size to each glyph, so that the vertex shader can compute
        // the displacement in pixels, relative to the group size (the coefficient is the
        // anchor).
        vec2* group_shapes_repeated = _repeat_group(
            sizeof(vec2), count, group_count, group_size, (void*)group_shapes, false);
        dvz_glyph_group_size(visual, first, count, group_shapes_repeated, 0);
        FREE(group_shapes_repeated);

        FREE(group_shapes);
    }

    FREE(size);
    FREE(shift);
}



// NOTE: size of positions array is group_count=tick_count
static inline void set_glyph_pos(
    DvzVisual* glyph, uint32_t glyph_count, uint32_t string_count, //
    uint32_t* string_sizes, vec3* string_positions)
{
    ASSERT(glyph_count > 0);
    ASSERT(string_count > 0);
    ANN(string_sizes);
    ANN(string_positions);

    vec3* pos = _repeat_group(
        sizeof(vec3), glyph_count, string_count, string_sizes, (void*)string_positions, false);
    dvz_glyph_position(glyph, 0, glyph_count, pos, 0);
    FREE(pos);
}



static inline void set_glyphs(
    DvzVisual* glyph, DvzAtlasFont* af, uint32_t glyph_count, uint32_t string_count, //
    uint32_t* string_sizes, const char* concatenated, uint32_t* string_offsets, vec2 offset,
    vec2 anchor)
{
    ANN(glyph);
    ANN(af);
    ANN(string_sizes);
    ANN(concatenated);
    ANN(string_offsets);

    ASSERT(glyph_count > 0);
    ASSERT(string_count > 0);

    // Set the size and shift properties of the glyph vsual by using the font to compute the
    // layout.
    vec4* xywh = dvz_font_ascii(af->font, concatenated);

    // Prepare a copy of the string with all glyphs concatenated, but without the spaces
    // between the groups.
    vec4* xywh_trimmed = (vec4*)calloc(glyph_count, sizeof(vec4));

    // WARNING, BUG FIX: +1 for trailing 0
    char* glyphs_trimmed = (char*)calloc(glyph_count + 1, sizeof(char));

    float x0 = 0.0;
    uint32_t idx = 0;
    uint32_t k = 0;
    for (uint32_t i = 0; i < string_count; i++)
    {
        idx = string_offsets[i];
        x0 = xywh[idx][0];
        for (uint32_t j = 0; j < string_sizes[i]; j++)
        {
            // NOTE: remove the x0 offset for each group.
            xywh_trimmed[k][0] = xywh[idx + j][0] - x0;
            xywh_trimmed[k][1] = xywh[idx + j][1];
            xywh_trimmed[k][2] = xywh[idx + j][2];
            xywh_trimmed[k][3] = xywh[idx + j][3];

            glyphs_trimmed[k] = concatenated[idx + j];
            k++;
        }
    }
    ASSERT(k == glyph_count);

    dvz_glyph_xywh(glyph, 0, glyph_count, xywh_trimmed, offset, 0);
    FREE(xywh);

    vec2* anchors = (vec2*)_repeat(glyph_count, sizeof(vec2), (void*)anchor);
    dvz_glyph_anchor(glyph, 0, glyph_count, anchors, 0);
    FREE(anchors);

    dvz_glyph_ascii(glyph, glyphs_trimmed);
    FREE(glyphs_trimmed);
}



// NOTE: the caller must free the result
static char* concatenate_with_spaces(
    uint32_t count, char** strings, uint32_t K, uint32_t* string_offsets, uint32_t* string_sizes,
    uint32_t* glyph_count)
{
    if (!strings || count == 0)
        return NULL;
    ASSERT(K > 0);

    uint32_t total_len = 0;            // total size of the concatenated strings with all spaces
    uint32_t computed_glyph_count = 0; // number of glyphs
    uint32_t space_len = K;
    uint32_t string_size = 0;

    // Compute the size of each string and the total size of the concatenated string.
    for (uint32_t i = 0; i < count; i++)
    {
        string_size = strlen(strings[i]);

        if (string_sizes != NULL)
        {
            string_sizes[i] = string_size;
        }

        if (string_offsets != NULL)
        {
            string_offsets[i] = total_len;
        }

        total_len += string_size;
        computed_glyph_count += string_size;
        if (i < count - 1)
            total_len += space_len;
    }

    if (glyph_count != NULL)
    {
        *glyph_count = computed_glyph_count;
    }

    // Allocate the concatenated string.
    char* result = (char*)calloc(total_len + 1, sizeof(char)); // WARNING: +1 for trailing 0
    ANN(result);

    // Concatenate the strings.
    char* ptr = result;
    for (uint32_t i = 0; i < count; i++)
    {
        uint32_t len = strlen(strings[i]);
        memcpy(ptr, strings[i], len);
        ptr += len;

        if (i < count - 1)
        {
            memset(ptr, ' ', space_len);
            ptr += space_len;
        }
    }

    *ptr = '\0';
    return result;
}



void dvz_glyph_strings(
    DvzVisual* glyph, uint32_t string_count, char** strings, vec3* string_positions, float* scales,
    DvzColor color, vec2 offset, vec2 anchor)
{
    ANN(glyph);
    ANN(strings);
    ANN(string_positions);
    ASSERT(string_count > 0);

    DvzAtlasFont* af = (DvzAtlasFont*)glyph->user_data;
    if (af == NULL)
    {
        log_error("please call dvz_glyph_atlas_font() first");
        return;
    }
    ANN(af);

    // NOTE: to avoid calling freetype() too many times, we concatenate all strings, call freetype
    // once, and the get back the offsets of each glyph.

    uint32_t space_count = 3; // spaces between strings.
    uint32_t glyph_count = 0;
    uint32_t* string_sizes = (uint32_t*)calloc(string_count, sizeof(uint32_t));
    uint32_t* string_offsets = (uint32_t*)calloc(string_count, sizeof(uint32_t));

    // Concatenate the strings with spaces.
    char* concatenated = concatenate_with_spaces(
        string_count, strings, space_count, string_offsets, string_sizes, &glyph_count);

    if (glyph_count == 0)
    {
        log_error("no glyphs to draw");
    }
    else
    {
        // Allocate the number of glyphs.
        dvz_glyph_alloc(glyph, glyph_count);

        // Set the groups.
        dvz_visual_groups(glyph, string_count, string_sizes);

        // Compute the glyph offsets and sizes with freetype called on the concatenated string,
        // then update the glyph visual with that information.
        set_glyphs(
            glyph, af, glyph_count, string_count, string_sizes, concatenated, string_offsets,
            offset, anchor);

        // Set the positions of the glyphs.
        set_glyph_pos(glyph, glyph_count, string_count, string_sizes, string_positions);

        DvzColor* glyph_color = dvz_mock_monochrome(glyph_count, color);
        dvz_glyph_color(glyph, 0, glyph_count, glyph_color, 0);
        FREE(glyph_color);

        // String scale are defined per string, we need to repeat them for each glyph.
        // NOTE: the vertex shader assumes all scales are identical across all glyphs of a given
        // string. Otherwise the vertex displacement computation will be wrong.
        if (scales != NULL)
        {
            float* glyph_scales = _repeat_group(
                sizeof(float), glyph_count, string_count, string_sizes, (void*)scales, false);
            dvz_glyph_scale(glyph, 0, glyph_count, glyph_scales, 0);
            FREE(glyph_scales);
        }
    }

    // Cleanup.
    FREE(concatenated);
    FREE(string_offsets);
    FREE(string_sizes);
}
