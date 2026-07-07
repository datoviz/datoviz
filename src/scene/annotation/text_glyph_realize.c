/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene text glyph realization                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_scale_ticks.h"
#include "_scene.h"
#include "core/scene_notify_internal.h"
#include "core/panel_layout_internal.h"
#include "datoviz/scene.h"
#include "text_internal.h"
#include "_visual_internal.h"
#include "text_visual_bridge.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Resolve a text anchor in figure pixels.
 *
 * @param text the text object
 * @param out_x output anchor x coordinate
 * @param out_y output anchor y coordinate
 */
static void _text_anchor_pixels(const DvzText* text, float* out_x, float* out_y)
{
    ANN(text);
    ANN(text->panel);
    ANN(out_x);
    ANN(out_y);
    float px = 0;
    float py = 0;
    float pw = 0;
    float ph = 0;
    _scene_panel_pixel_rect(text->panel, &px, &py, &pw, &ph);

    switch (text->placement.anchor)
    {
    case DVZ_SCENE_ANCHOR_PANEL_TOP:
        *out_x = px + .5f * pw;
        *out_y = py;
        break;
    case DVZ_SCENE_ANCHOR_PANEL_TOP_RIGHT:
        *out_x = px + pw;
        *out_y = py;
        break;
    case DVZ_SCENE_ANCHOR_PANEL_LEFT:
        *out_x = px;
        *out_y = py + .5f * ph;
        break;
    case DVZ_SCENE_ANCHOR_PANEL_CENTER:
        *out_x = px + .5f * pw;
        *out_y = py + .5f * ph;
        break;
    case DVZ_SCENE_ANCHOR_PANEL_RIGHT:
        *out_x = px + pw;
        *out_y = py + .5f * ph;
        break;
    case DVZ_SCENE_ANCHOR_PANEL_BOTTOM_LEFT:
        *out_x = px;
        *out_y = py + ph;
        break;
    case DVZ_SCENE_ANCHOR_PANEL_BOTTOM:
        *out_x = px + .5f * pw;
        *out_y = py + ph;
        break;
    case DVZ_SCENE_ANCHOR_PANEL_BOTTOM_RIGHT:
        *out_x = px + pw;
        *out_y = py + ph;
        break;
    case DVZ_SCENE_ANCHOR_SCREEN:
        *out_x = (float)text->placement.position[0];
        *out_y = (float)text->placement.position[1];
        return;
    case DVZ_SCENE_ANCHOR_NONE:
    case DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT:
    default:
        *out_x = px;
        *out_y = py;
        break;
    }
    *out_x += (float)text->placement.position[0];
    *out_y += (float)text->placement.position[1];
}



/**
 * Attach or update the generated glyph visual with the text visual attachment metadata.
 *
 * @param panel the owning panel
 * @param glyph_visual the generated glyph visual
 * @param desc the desired attachment descriptor
 * @return whether the glyph visual is attached with the desired metadata
 */
static bool _text_sync_glyph_visual_attach(
    DvzPanel* panel, DvzVisual* glyph_visual, const DvzVisualAttachDesc* desc,
    const DvzPanelAttach* source_attach)
{
    ANN(panel);
    ANN(glyph_visual);
    ANN(desc);
    bool source_has_role = source_attach != NULL && source_attach->has_generated_role;
    DvzGeneratedVisualRole source_role =
        source_has_role ? source_attach->generated_role : DVZ_GENERATED_VISUAL_DATA_DEFAULT;
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        DvzPanelAttach* attach = &panel->visuals[i];
        if (attach->visual != glyph_visual)
            continue;
        bool changed =
            attach->z_layer != desc->z_layer || attach->controller_mode != desc->controller_mode ||
            attach->coord_space != desc->coord_space ||
            attach->clip_rect != desc->clip_rect || attach->viewport_rect != desc->viewport_rect ||
            attach->has_generated_role != source_has_role ||
            (source_has_role && attach->generated_role != source_role);
        attach->z_layer = desc->z_layer;
        attach->controller_mode = desc->controller_mode;
        attach->coord_space = desc->coord_space;
        attach->clip_rect = desc->clip_rect;
        attach->viewport_rect = desc->viewport_rect;
        attach->has_generated_role = source_has_role;
        attach->generated_role = source_role;
        if (changed)
            _scene_notify_request_frame(panel->figure);
        return true;
    }
    if (dvz_panel_add_visual(panel, glyph_visual, desc) != 0)
        return false;
    DvzPanelAttach* attach = &panel->visuals[panel->visual_count - 1];
    attach->has_generated_role = source_has_role;
    attach->generated_role = source_role;
    return true;
}


static bool _text_find_panel_attach(
    DvzPanel* panel, DvzVisual* visual, const DvzPanelAttach** out_attach)
{
    ANN(panel);
    ANN(visual);
    ANN(out_attach);
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        if (panel->visuals[i].visual == visual)
        {
            *out_attach = &panel->visuals[i];
            return true;
        }
    }
    return false;
}


static bool _text_prepare_batched_visual(DvzFigure* figure, DvzText* text)
{
    ANN(figure);
    ANN(text);
    if (text->scene == NULL || text->panel == NULL || text->panel->figure != figure)
        return true;
    if (text->dirty_flags == DVZ_TEXT_DIRTY_NONE)
        return true;
    if (text->item_count == 0 || text->strings == NULL || text->positions == NULL)
    {
        if (text->visual != NULL)
            dvz_visual_set_visible(text->visual, false);
        text->dirty_flags = DVZ_TEXT_DIRTY_NONE;
        return true;
    }

    bool screen_placement = text->placement.mode == DVZ_TEXT_PLACEMENT_SCREEN;
    DvzVisualAttachDesc attach = dvz_visual_attach_desc();
    attach.z_layer = INT32_MAX / 4;
    attach.controller_mode = screen_placement ? DVZ_CONTROLLER_FIXED : DVZ_CONTROLLER_APPLY;
    if (text->placement.mode == DVZ_TEXT_PLACEMENT_DATA)
        attach.coord_space = DVZ_VISUAL_COORD_DATA;
    else
        attach.coord_space = DVZ_VISUAL_COORD_VIEW;

    if (text->visual != NULL && text->visual->type != DVZ_VISUAL_TYPE_TEXT)
    {
        dvz_visual_set_visible(text->visual, false);
        text->visual = NULL;
    }
    if (text->visual == NULL)
    {
        text->visual = _scene_text_visual(text->scene, 0);
        if (text->visual == NULL)
            return false;
    }
    if (_scene_text_visual_set_renderer(text->visual, text->style.renderer) != 0)
        return false;
    if (!_text_sync_glyph_visual_attach(text->panel, text->visual, &attach, NULL))
        return false;

    uint32_t count = text->item_count;
    float(*positions)[3] = (float(*)[3])dvz_calloc(count, sizeof(float[3]));
    if (positions == NULL)
        return false;
    float anchor_x = 0.0f;
    float anchor_y = 0.0f;
    if (screen_placement)
    {
        float panel_x = 0.0f;
        float panel_y = 0.0f;
        float panel_w = 0.0f;
        float panel_h = 0.0f;
        _scene_panel_pixel_rect(text->panel, &panel_x, &panel_y, &panel_w, &panel_h);
        (void)panel_w;
        (void)panel_h;
        _text_anchor_pixels(text, &anchor_x, &anchor_y);
        anchor_x -= (float)text->placement.position[0];
        anchor_y -= (float)text->placement.position[1];
        anchor_x -= panel_x;
        anchor_y -= panel_y;
    }
    for (uint32_t i = 0; i < count; i++)
    {
        positions[i][0] = (float)text->positions[i][0] + (screen_placement ? anchor_x : 0.0f);
        positions[i][1] = (float)text->positions[i][1] + (screen_placement ? anchor_y : 0.0f);
        positions[i][2] = (float)text->positions[i][2];
    }

    _visual_family_state(text->visual)->text.layout = text->layout;
    _visual_family_state(text->visual)->text.layout_version++;
    DvzVisualDataUpdate updates[6] = {
        {.attr_name = "position", .data = positions, .item_count = count},
        {.attr_name = "offset", .data = text->offsets, .item_count = count},
        {.attr_name = "anchor", .data = text->anchors, .item_count = count},
        {.attr_name = "size", .data = text->sizes_px, .item_count = count},
        {.attr_name = "color", .data = text->colors, .item_count = count},
        {.attr_name = "angle", .data = text->angles, .item_count = count},
    };
    bool ok = dvz_visual_set_strings(text->visual, "text", (const char* const*)text->strings, count) == 0 &&
              dvz_visual_set_data_many(text->visual, updates, 6) == 0;
    dvz_free(positions);
    if (!ok)
        return false;

    const DvzPanelAttach* panel_attach = NULL;
    if (!_text_find_panel_attach(text->panel, text->visual, &panel_attach))
        return false;
    ok = _text_visual_prepare(figure, text->panel, panel_attach, text->visual);
    if (ok)
    {
        DvzVisual* glyph = _visual_family_state(text->visual)->text.glyph_visual;
        if (glyph != NULL)
            dvz_visual_set_depth_test(glyph, text->placement.depth_test);
        text->dirty_flags = DVZ_TEXT_DIRTY_NONE;
        text->visual_version = text->version;
        text->visual_figure_width = figure->width;
        text->visual_figure_height = figure->height;
    }
    return ok;
}



/**
 * Write one glyph vertex record consumed by the shader-side quad generator.
 *
 * @param anchor_position the glyph anchor in the generated visual coordinate space
 * @param bounds_rect the local glyph pixel bounds as x0, y0, x1, y1
 * @param uv_rect the atlas UV rectangle as u0, v0, u1, v1
 * @param color the glyph color
 * @param angle the glyph rotation angle in radians
 * @param vertex_index the destination vertex index
 * @param positions the destination anchor positions
 * @param bounds the destination local bounds
 * @param texcoords the destination atlas UV bounds
 * @param colors the destination colors
 * @param angles the destination angles
 */
static void _text_write_glyph_vertex(
    const float anchor_position[3], const float bounds_rect[4], const float uv_rect[4],
    const uint8_t color[4], float angle, uint32_t vertex_index, float* positions, float* bounds,
    float* texcoords, uint8_t* colors, float* angles)
{
    ANN(anchor_position);
    ANN(bounds_rect);
    ANN(uv_rect);
    ANN(color);
    ANN(positions);
    ANN(bounds);
    ANN(texcoords);
    ANN(colors);
    ANN(angles);

    positions[3 * vertex_index + 0] = anchor_position[0];
    positions[3 * vertex_index + 1] = anchor_position[1];
    positions[3 * vertex_index + 2] = anchor_position[2];
    bounds[4 * vertex_index + 0] = bounds_rect[0];
    bounds[4 * vertex_index + 1] = bounds_rect[1];
    bounds[4 * vertex_index + 2] = bounds_rect[2];
    bounds[4 * vertex_index + 3] = bounds_rect[3];
    texcoords[4 * vertex_index + 0] = uv_rect[0];
    texcoords[4 * vertex_index + 1] = uv_rect[1];
    texcoords[4 * vertex_index + 2] = uv_rect[2];
    texcoords[4 * vertex_index + 3] = uv_rect[3];
    colors[4 * vertex_index + 0] = color[0];
    colors[4 * vertex_index + 1] = color[1];
    colors[4 * vertex_index + 2] = color[2];
    colors[4 * vertex_index + 3] = color[3];
    angles[vertex_index] = angle;
}



/**
 * Update or create the internal glyph visual for one retained text object.
 *
 * @param figure the figure being emitted
 * @param text the text object
 * @return whether preparation succeeded
 */
bool _text_prepare_visual(DvzFigure* figure, DvzText* text)
{
    ANN(figure);
    ANN(text);
    if (text->id != DVZ_ID_NONE)
        return _text_prepare_batched_visual(figure, text);
    if (text->scene == NULL || text->panel == NULL || text->panel->figure != figure)
        return true;
    const char* string = text->legacy_string != NULL ? text->legacy_string : "";
    float default_size_px = text->scene->font_defaults.text_size_px;
    DvzTextAtlasBackend backend =
        _text_renderer_backend(text->style.renderer, &text->style, default_size_px);
    bool use_builtin = backend == DVZ_TEXT_ATLAS_BACKEND_BUILTIN_BITMAP;
    bool screen_placement = text->placement.mode == DVZ_TEXT_PLACEMENT_SCREEN;
    DvzVisualAttachDesc attach = dvz_visual_attach_desc();
    attach.z_layer = INT32_MAX / 4;
    attach.controller_mode = screen_placement ? DVZ_CONTROLLER_FIXED : DVZ_CONTROLLER_APPLY;
    if (text->placement.mode == DVZ_TEXT_PLACEMENT_DATA)
        attach.coord_space = DVZ_VISUAL_COORD_DATA;
    else
        attach.coord_space = DVZ_VISUAL_COORD_VIEW;
    uint32_t visible = 0;
    DvzSampledField* atlas = NULL;
    DvzTextAtlas* font_atlas = NULL;
    uint64_t atlas_generation = 0;
    float scale = 1.0f;
    float glyph_w = 0.0f;
    float glyph_h = 0.0f;
    float line_h = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    if (!use_builtin)
    {
        DvzFont* font = _text_sdf_font(text->scene, &text->style);
        DvzTextAtlasSpec spec =
            _scene_text_atlas_spec(backend, _text_style_size_px(&text->style, default_size_px));
        if (font == NULL || !_scene_text_atlas_ensure_string(font, &spec, string))
            return false;
        font_atlas = _text_font_atlas(font, &spec);
        ANN(font_atlas);
        atlas = font_atlas->field;
        atlas_generation = font_atlas->generation;
        scale = _text_sdf_layout_scale(&text->style, font_atlas);
        _text_sdf_measure(string, font_atlas, scale, &width, &height, &visible);
        line_h = font_atlas->line_height * scale;
    }
    else
    {
        uint32_t columns = 0;
        uint32_t lines = 0;
        _text_measure_cells(string, &columns, &lines, &visible);
        atlas = _text_bitmap_atlas_field(text->scene);
        scale = _text_bitmap_layout_scale(&text->style);
        glyph_w = (float)DVZ_TEXT_BITMAP_GLYPH_WIDTH * scale;
        glyph_h = (float)DVZ_TEXT_BITMAP_GLYPH_HEIGHT * scale;
        line_h = (float)DVZ_TEXT_BITMAP_LINE_HEIGHT * scale;
        width = (float)columns * glyph_w;
        height = (float)(lines - 1u) * line_h + glyph_h;
    }
    if (text->visual != NULL && _visual_family_state(text->visual)->field != NULL &&
        text->visual_version == text->version &&
        text->visual_atlas_generation == atlas_generation &&
        text->visual_figure_width == figure->width && text->visual_figure_height == figure->height)
    {
        return true;
    }
    if (visible == 0 || width <= 0.0f || height <= 0.0f)
    {
        if (text->visual != NULL)
            dvz_visual_set_visible(text->visual, false);
        return true;
    }
    if (atlas == NULL)
        return false;
    if (!isfinite(width) || !isfinite(height) || width <= 0.0f || height <= 0.0f ||
        width > (float)UINT32_MAX || height > (float)UINT32_MAX)
    {
        log_error("text glyph dimensions overflow");
        return false;
    }

    uint64_t max_vertices = 0;
    uint64_t position_bytes = 0;
    uint64_t bounds_bytes = 0;
    uint64_t texcoord_bytes = 0;
    uint64_t color_bytes = 0;
    uint64_t angle_bytes = 0;
    if (_dvz_mul_u64_overflows(visible, 6u, &max_vertices) ||
        _dvz_mul_u64_overflows(max_vertices, 3u * sizeof(float), &position_bytes) ||
        _dvz_mul_u64_overflows(max_vertices, 4u * sizeof(float), &bounds_bytes) ||
        _dvz_mul_u64_overflows(max_vertices, 4u * sizeof(float), &texcoord_bytes) ||
        _dvz_mul_u64_overflows(max_vertices, 4u * sizeof(uint8_t), &color_bytes) ||
        _dvz_mul_u64_overflows(max_vertices, sizeof(float), &angle_bytes) ||
        max_vertices > UINT32_MAX || position_bytes > SIZE_MAX || bounds_bytes > SIZE_MAX ||
        texcoord_bytes > SIZE_MAX || color_bytes > SIZE_MAX || angle_bytes > SIZE_MAX)
    {
        log_error("text glyph vertex buffer size overflow");
        return false;
    }

    float* positions = (float*)dvz_calloc((DvzSize)position_bytes, 1);
    float* bounds = (float*)dvz_calloc((DvzSize)bounds_bytes, 1);
    float* texcoords = (float*)dvz_calloc((DvzSize)texcoord_bytes, 1);
    uint8_t* colors = (uint8_t*)dvz_calloc((DvzSize)color_bytes, 1);
    float* angles = (float*)dvz_calloc((DvzSize)angle_bytes, 1);
    if (positions == NULL || bounds == NULL || texcoords == NULL || colors == NULL ||
        angles == NULL)
    {
        dvz_free(positions);
        dvz_free(bounds);
        dvz_free(texcoords);
        dvz_free(colors);
        dvz_free(angles);
        log_error("text glyph vertex allocation failed");
        return false;
    }

    uint8_t color[4] = {0};
    _text_style_color(&text->style, color);
    float anchor_clip[3] = {0};
    if (screen_placement)
    {
        float anchor_x = 0;
        float anchor_y = 0;
        _text_anchor_pixels(text, &anchor_x, &anchor_y);
        float z = (float)text->placement.position[2];
        _text_pixel_to_clip(figure, anchor_x, anchor_y, z, anchor_clip);
    }
    else
    {
        anchor_clip[0] = (float)text->placement.position[0];
        anchor_clip[1] = (float)text->placement.position[1];
        anchor_clip[2] = (float)text->placement.position[2];
    }
    float align_x = 0;
    float align_y = 0;
    _text_placement_alignment(&text->placement, (float)width, (float)height, &align_x, &align_y);
    align_x += text->placement.offset[0];
    align_y += text->placement.offset[1];

    uint32_t column = 0;
    uint32_t row = 0;
    uint32_t byte_index = 0;
    uint32_t cp = 0;
    uint32_t vertex_count = 0;
    float cursor_x = 0.0f;
    while (_text_utf8_next(string, &byte_index, &cp))
    {
        if (cp == '\n')
        {
            column = 0;
            cursor_x = 0.0f;
            row++;
            continue;
        }
        if (cp == '\t')
        {
            if (!use_builtin)
            {
                DvzTextAtlasGlyph* space = _scene_text_atlas_glyph(font_atlas, ' ');
                cursor_x += space != NULL ? 4.0f * space->advance * scale :
                                             2.0f * font_atlas->em_px * scale;
            }
            else
            {
                column += 4u;
            }
            continue;
        }

        float uv[4] = {0};
        float x0 = 0.0f;
        float y0 = 0.0f;
        float x1 = 0.0f;
        float y1 = 0.0f;
        if (!use_builtin)
        {
            DvzTextAtlasGlyph* glyph = _scene_text_atlas_glyph(font_atlas, cp);
            if (glyph == NULL)
                continue;
            float advance = glyph->advance * scale;
            if (glyph->width <= 0.0f || glyph->height <= 0.0f)
            {
                cursor_x += advance;
                continue;
            }
            x0 = align_x + cursor_x + glyph->xoff * scale;
            y0 = align_y + (float)row * line_h + font_atlas->ascent * scale +
                 glyph->yoff * scale;
            x1 = x0 + glyph->width * scale;
            y1 = y0 + glyph->height * scale;
            uv[0] = glyph->uv[0];
            uv[1] = glyph->uv[1];
            uv[2] = glyph->uv[2];
            uv[3] = glyph->uv[3];
            cursor_x += advance;
        }
        else
        {
            x0 = align_x + (float)column * glyph_w;
            y0 = align_y + (float)row * line_h;
            x1 = x0 + glyph_w;
            y1 = y0 + glyph_h;
            _text_bitmap_atlas_uv(cp, uv);
            column++;
        }
        float bounds_rect[4] = {x0, y0, x1, y1};
        for (uint32_t j = 0; j < 6; j++)
        {
            _text_write_glyph_vertex(
                anchor_clip, bounds_rect, uv, color, text->placement.angle, vertex_count,
                positions, bounds, texcoords, colors, angles);
            vertex_count++;
        }
    }
    if (vertex_count == 0)
    {
        dvz_free(positions);
        dvz_free(bounds);
        dvz_free(texcoords);
        dvz_free(colors);
        dvz_free(angles);
        if (text->visual != NULL)
            dvz_visual_set_visible(text->visual, false);
        return true;
    }

    bool ok = true;
    if (text->visual != NULL && text->visual->type != DVZ_VISUAL_TYPE_GLYPH)
    {
        dvz_visual_set_visible(text->visual, false);
        text->visual = NULL;
    }
    if (text->visual == NULL)
    {
        text->visual = dvz_glyph(text->scene, 0);
        if (text->visual == NULL)
            ok = false;
    }
    if (ok && !_text_sync_glyph_visual_attach(text->panel, text->visual, &attach, NULL))
        ok = false;
    if (ok && dvz_visual_set_alpha_mode(text->visual, DVZ_ALPHA_BLENDED) != 0)
        ok = false;
    if (ok && dvz_visual_set_depth_test(text->visual, text->placement.depth_test) != 0)
        ok = false;

    if (ok)
    {
        _visual_family_state(text->visual)->glyph_atlas_encoding =
            font_atlas != NULL ? font_atlas->encoding : DVZ_TEXT_ATLAS_ENCODING_BITMAP_ALPHA;
        _visual_family_state(text->visual)->glyph_distance_range_px =
            font_atlas != NULL ? font_atlas->distance_range_px : 1.0f;
        DvzVisualDataUpdate updates[5] = {
            {.attr_name = "position", .data = positions, .item_count = vertex_count},
            {.attr_name = "bounds", .data = bounds, .item_count = vertex_count},
            {.attr_name = "texcoords", .data = texcoords, .item_count = vertex_count},
            {.attr_name = "color", .data = colors, .item_count = vertex_count},
            {.attr_name = "angle", .data = angles, .item_count = vertex_count},
        };
        if (dvz_visual_set_data_many(text->visual, updates, 5) != 0 ||
            dvz_visual_set_field(text->visual, "field", atlas) != DVZ_OK)
        {
            ok = false;
        }
    }

    if (ok)
    {
        dvz_visual_set_visible(text->visual, true);
        text->metrics.advance[0] = (float)width;
        text->metrics.advance[1] = 0;
        text->metrics.ink_bounds[0] = 0;
        text->metrics.ink_bounds[1] = 0;
        text->metrics.ink_bounds[2] = (float)width;
        text->metrics.ink_bounds[3] = (float)height;
        text->metrics.layout_bounds[0] = 0;
        text->metrics.layout_bounds[1] = 0;
        text->metrics.layout_bounds[2] = (float)width;
        text->metrics.layout_bounds[3] = (float)height;
        text->metrics.baseline = font_atlas != NULL ? font_atlas->ascent * scale : 7.0f * scale;
        text->metrics.ascender =
            font_atlas != NULL ? font_atlas->ascent * scale : text->metrics.baseline;
        text->metrics.descender =
            font_atlas != NULL ? -font_atlas->descent * scale : 1.0f * scale;
        text->metrics.line_height = line_h;
        text->dirty_flags = DVZ_TEXT_DIRTY_NONE;
        text->visual_version = text->version;
        text->visual_atlas_generation = atlas_generation;
        text->visual_figure_width = figure->width;
        text->visual_figure_height = figure->height;
    }
    else if (text->visual != NULL)
    {
        dvz_visual_set_visible(text->visual, false);
    }

    dvz_free(positions);
    dvz_free(bounds);
    dvz_free(texcoords);
    dvz_free(colors);
    dvz_free(angles);
    return ok;
}



/**
 * Return a dense per-item attribute from a visual.
 *
 * @param visual the visual
 * @param name the attribute name
 * @return the attribute, or NULL when absent
 */
static const DvzVisualAttr* _text_visual_attr(const DvzVisual* visual, const char* name)
{
    ANN(visual);
    ANN(name);
    int idx = _attr_index(visual, name);
    if (idx < 0)
        return NULL;
    const DvzVisualAttr* attr = &visual->attrs[idx];
    return attr->data != NULL && attr->item_count > 0 ? attr : NULL;
}



/**
 * Resolve a text-visual realization version from strings and per-item attributes.
 *
 * @param visual the text visual
 * @return the realization version
 */
static uint64_t _text_visual_version(const DvzVisual* visual)
{
    ANN(visual);
    uint64_t version = _visual_family_state(visual)->text.strings_version +
                       _visual_family_state(visual)->text.renderer_version +
                       _visual_family_state(visual)->text.layout_version;
    for (uint32_t i = 0; i < visual->attr_count; i++)
        version += visual->attrs[i].version;
    return version;
}



/**
 * Resolve a text-visual realization version excluding anchor positions.
 *
 * @param visual the text visual
 * @return the layout realization version
 */
static uint64_t _text_visual_layout_version(const DvzVisual* visual)
{
    ANN(visual);
    uint64_t version = _visual_family_state(visual)->text.strings_version +
                       _visual_family_state(visual)->text.renderer_version +
                       _visual_family_state(visual)->text.layout_version;
    for (uint32_t i = 0; i < visual->attr_count; i++)
    {
        if (strcmp(visual->attrs[i].name, "position") == 0)
            continue;
        version += visual->attrs[i].version;
    }
    return version;
}


static float _text_layout_line_advance(const DvzTextLayout* layout, float natural_line_height)
{
    float line_height = layout != NULL ? layout->line_height : 0.0f;
    if (line_height <= 0.0f)
        line_height = 1.0f;
    float line_gap_px = layout != NULL ? layout->line_gap_px : 0.0f;
    if (line_gap_px < 0.0f)
        line_gap_px = 0.0f;
    return natural_line_height * line_height + line_gap_px;
}



/**
 * Replace only the derived glyph anchor-position attribute for one text visual.
 *
 * @param figure the figure being emitted
 * @param panel the panel carrying the text visual
 * @param attach the panel attachment for the text visual
 * @param visual the batched text visual
 * @param position_attr the source per-string position attribute
 * @param version the full realized text version after the update
 * @param layout_version the layout-only realized text version after the update
 * @return whether the derived glyph positions were updated
 */
static bool _text_visual_update_glyph_positions(
    DvzFigure* figure, DvzPanel* panel, const DvzPanelAttach* attach, DvzVisual* visual,
    const DvzVisualAttr* position_attr, uint64_t version, uint64_t layout_version)
{
    ANN(figure);
    ANN(panel);
    ANN(attach);
    ANN(visual);
    ANN(position_attr);
    DvzVisual* glyph_visual = _visual_family_state(visual)->text.glyph_visual;
    if (glyph_visual == NULL || _visual_family_state(visual)->text.spans == NULL)
        return false;

    int glyph_pos_idx = _attr_index(glyph_visual, "position");
    if (glyph_pos_idx < 0 || glyph_visual->attrs[glyph_pos_idx].item_count == 0)
        return false;
    uint32_t vertex_capacity = glyph_visual->attrs[glyph_pos_idx].item_count;

    uint64_t position_bytes = 0;
    if (_dvz_mul_u64_overflows(vertex_capacity, 3u * sizeof(float), &position_bytes) ||
        position_bytes > SIZE_MAX)
    {
        log_error("text visual glyph position buffer size overflow");
        return false;
    }
    float* positions = (float*)dvz_calloc((DvzSize)position_bytes, 1);
    if (positions == NULL)
    {
        log_error("text visual glyph position allocation failed");
        return false;
    }

    const float(*target)[3] = (const float(*)[3])position_attr->data;
    for (uint32_t i = 0; i < _visual_family_state(visual)->text.span_count; i++)
    {
        if (i >= position_attr->item_count)
            break;
        DvzTextGlyphSpan* span = &_visual_family_state(visual)->text.spans[i];
        uint64_t first_vertex64 = (uint64_t)span->first_glyph * 6u;
        uint64_t vertex_count64 = (uint64_t)span->glyph_count * 6u;
        if (
            first_vertex64 > UINT32_MAX || vertex_count64 > UINT32_MAX ||
            first_vertex64 + vertex_count64 > vertex_capacity)
        {
            dvz_free(positions);
            log_error("text visual glyph span exceeds reserved position capacity");
            return false;
        }

        float anchor_position[3] = {0};
        if (attach->controller_mode == DVZ_CONTROLLER_FIXED)
        {
            _text_panel_pixel_to_clip(
                panel, target[i][0], target[i][1], target[i][2], anchor_position);
        }
        else
        {
            anchor_position[0] = target[i][0];
            anchor_position[1] = target[i][1];
            anchor_position[2] = target[i][2];
        }
        uint32_t first_vertex = (uint32_t)first_vertex64;
        uint32_t vertex_count = (uint32_t)vertex_count64;
        for (uint32_t j = 0; j < vertex_count; j++)
        {
            uint32_t vertex_index = first_vertex + j;
            positions[3 * vertex_index + 0] = anchor_position[0];
            positions[3 * vertex_index + 1] = anchor_position[1];
            positions[3 * vertex_index + 2] = anchor_position[2];
        }
    }

    bool ok = dvz_visual_set_data(glyph_visual, "position", positions, vertex_capacity) == 0;
    dvz_free(positions);
    if (!ok)
        return false;
    dvz_visual_set_visible(glyph_visual, true);
    _visual_family_state(visual)->text.realized_version = version;
    _visual_family_state(visual)->text.realized_layout_version = layout_version;
    _visual_family_state(visual)->text.realized_controller_mode = attach->controller_mode;
    _visual_family_state(visual)->text.visual_figure_width = figure->width;
    _visual_family_state(visual)->text.visual_figure_height = figure->height;
    return true;
}



/**
 * Update or create the internal glyph visual for one batched text visual.
 *
 * @param figure the figure being emitted
 * @param panel the panel carrying the text visual
 * @param attach the panel attachment for the text visual
 * @param visual the batched text visual
 * @return whether preparation succeeded
 */
bool _text_visual_prepare(
    DvzFigure* figure, DvzPanel* panel, const DvzPanelAttach* attach, DvzVisual* visual)
{
    ANN(figure);
    ANN(panel);
    ANN(attach);
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_TEXT)
        return true;
    if (!visual->visible)
    {
        if (_visual_family_state(visual)->text.glyph_visual != NULL)
            dvz_visual_set_visible(_visual_family_state(visual)->text.glyph_visual, false);
        return true;
    }

    const uint32_t count = _visual_family_state(visual)->text.string_count;
    const DvzVisualAttr* position_attr = _text_visual_attr(visual, "position");
    if (count == 0 || _visual_family_state(visual)->text.strings == NULL || position_attr == NULL ||
        position_attr->item_count != count)
    {
        if (_visual_family_state(visual)->text.glyph_visual != NULL)
            dvz_visual_set_visible(_visual_family_state(visual)->text.glyph_visual, false);
        return true;
    }

    const DvzVisualAttr* anchor_attr = _text_visual_attr(visual, "anchor");
    const DvzVisualAttr* offset_attr = _text_visual_attr(visual, "offset");
    const DvzVisualAttr* size_attr = _text_visual_attr(visual, "size");
    const DvzVisualAttr* color_attr = _text_visual_attr(visual, "color");
    const DvzVisualAttr* angle_attr = _text_visual_attr(visual, "angle");
    if ((anchor_attr != NULL && anchor_attr->item_count != count) ||
        (offset_attr != NULL && offset_attr->item_count != count) ||
        (size_attr != NULL && size_attr->item_count != count) ||
        (color_attr != NULL && color_attr->item_count != count) ||
        (angle_attr != NULL && angle_attr->item_count != count))
    {
        log_error("text visual attributes must match string count");
        return false;
    }

    uint64_t version = _text_visual_version(visual);
    uint64_t layout_version = _text_visual_layout_version(visual);
    float screen_scale = _scene_screen_scale(figure);

    DvzTextRenderer renderer = _visual_family_state(visual)->text.renderer;
    float spec_size_px = 0.0f;
    if (size_attr != NULL && size_attr->data != NULL)
    {
        const float* item_sizes = (const float*)size_attr->data;
        for (uint32_t i = 0; i < count; i++)
        {
            if (item_sizes[i] > spec_size_px)
                spec_size_px = item_sizes[i];
        }
    }
    if (spec_size_px <= 0.0f)
        spec_size_px = visual->scene->font_defaults.text_size_px;
    spec_size_px *= screen_scale;
    DvzTextStyle backend_style = {
        DVZ_STRUCT_INIT_FIELDS(DvzTextStyle),
        .size_px = spec_size_px,
        .renderer = renderer,
    };
    DvzTextAtlasBackend backend =
        _text_renderer_backend(renderer, &backend_style, visual->scene->font_defaults.text_size_px);
    bool use_builtin = backend == DVZ_TEXT_ATLAS_BACKEND_BUILTIN_BITMAP;
    DvzTextAtlas* font_atlas = NULL;
    DvzSampledField* atlas = NULL;
    uint64_t atlas_generation = 0;
    if (!use_builtin)
    {
        DvzTextStyle atlas_style = {
            DVZ_STRUCT_INIT_FIELDS(DvzTextStyle),
            .font = NULL,
            .size_px = backend_style.size_px,
            .renderer = renderer,
        };
        DvzFont* font = _text_sdf_font(visual->scene, &atlas_style);
        const char* const* strings = (const char* const*)_visual_family_state(visual)->text.strings;
        DvzTextAtlasSpec spec = _scene_text_atlas_spec(backend, spec_size_px);
        if (font == NULL || !_scene_text_atlas_ensure_strings(
                                font, &spec, strings, _visual_family_state(visual)->text.string_count))
            return false;
        font_atlas = _text_font_atlas(font, &spec);
        ANN(font_atlas);
        atlas = font_atlas->field;
        atlas_generation = font_atlas->generation;
    }
    else
    {
        atlas = _text_bitmap_atlas_field(visual->scene);
    }
    if (atlas == NULL)
        return false;
    DvzVisualAttachDesc glyph_attach = dvz_visual_attach_desc();
    glyph_attach.z_layer = attach->z_layer;
    glyph_attach.controller_mode = attach->controller_mode;
    glyph_attach.coord_space = attach->coord_space;
    glyph_attach.clip_rect = attach->clip_rect;
    glyph_attach.viewport_rect = attach->viewport_rect;
    bool realized_cache_valid =
        _visual_family_state(visual)->text.glyph_visual != NULL &&
        _visual_family_state(_visual_family_state(visual)->text.glyph_visual)->field != NULL &&
        _visual_family_state(visual)->text.realized_version == version &&
        _visual_family_state(visual)->text.atlas_generation == atlas_generation &&
        _visual_family_state(visual)->text.realized_controller_mode == attach->controller_mode &&
        fabsf(_visual_family_state(visual)->text.screen_scale - screen_scale) <= 1e-6f &&
        _visual_family_state(visual)->text.visual_figure_width == figure->width &&
        _visual_family_state(visual)->text.visual_figure_height == figure->height;
    if (realized_cache_valid)
    {
        return _text_sync_glyph_visual_attach(
            panel, _visual_family_state(visual)->text.glyph_visual, &glyph_attach, attach);
    }
    bool position_only_dirty =
        _visual_family_state(visual)->text.glyph_visual != NULL &&
        _visual_family_state(_visual_family_state(visual)->text.glyph_visual)->field != NULL &&
        _visual_family_state(visual)->text.realized_layout_version == layout_version &&
        _visual_family_state(visual)->text.atlas_generation == atlas_generation &&
        fabsf(_visual_family_state(visual)->text.screen_scale - screen_scale) <= 1e-6f;
    if (position_only_dirty)
    {
        if (!_text_sync_glyph_visual_attach(
                panel, _visual_family_state(visual)->text.glyph_visual, &glyph_attach, attach))
            return false;
        return _text_visual_update_glyph_positions(
            figure, panel, attach, visual, position_attr, version, layout_version);
    }

    uint64_t vertex_count64 = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        uint32_t columns = 0;
        uint32_t lines = 0;
        uint32_t visible = 0;
        _text_measure_cells(_visual_family_state(visual)->text.strings[i], &columns, &lines, &visible);
        (void)columns;
        (void)lines;
        uint64_t vertices = 0;
        uint64_t next_vertex_count = 0;
        if (_dvz_mul_u64_overflows(visible, 6u, &vertices) ||
            _dvz_add_u64_overflows(vertex_count64, vertices, &next_vertex_count))
        {
            log_error("text visual glyph vertex count overflow");
            return false;
        }
        vertex_count64 = next_vertex_count;
    }
    if (vertex_count64 == 0)
    {
        if (_visual_family_state(visual)->text.glyph_visual != NULL)
            dvz_visual_set_visible(_visual_family_state(visual)->text.glyph_visual, false);
        return true;
    }
    if (vertex_count64 > UINT32_MAX)
    {
        log_error("text visual glyph vertex count exceeds uint32");
        return false;
    }
    uint32_t vertex_count_max = (uint32_t)vertex_count64;
    uint32_t allocation_vertex_count = vertex_count_max;
    if (_visual_family_state(visual)->text.reserved_glyph_vertices > allocation_vertex_count)
        allocation_vertex_count = _visual_family_state(visual)->text.reserved_glyph_vertices;

    uint64_t position_bytes = 0;
    uint64_t bounds_bytes = 0;
    uint64_t texcoord_bytes = 0;
    uint64_t color_bytes = 0;
    uint64_t angle_bytes = 0;
    if (_dvz_mul_u64_overflows(allocation_vertex_count, 3u * sizeof(float), &position_bytes) ||
        _dvz_mul_u64_overflows(allocation_vertex_count, 4u * sizeof(float), &bounds_bytes) ||
        _dvz_mul_u64_overflows(allocation_vertex_count, 4u * sizeof(float), &texcoord_bytes) ||
        _dvz_mul_u64_overflows(allocation_vertex_count, 4u * sizeof(uint8_t), &color_bytes) ||
        _dvz_mul_u64_overflows(allocation_vertex_count, sizeof(float), &angle_bytes) ||
        position_bytes > SIZE_MAX || bounds_bytes > SIZE_MAX || texcoord_bytes > SIZE_MAX ||
        color_bytes > SIZE_MAX || angle_bytes > SIZE_MAX)
    {
        log_error("text visual glyph buffer size overflow");
        return false;
    }

    float* positions = (float*)dvz_calloc((DvzSize)position_bytes, 1);
    float* bounds = (float*)dvz_calloc((DvzSize)bounds_bytes, 1);
    float* texcoords = (float*)dvz_calloc((DvzSize)texcoord_bytes, 1);
    uint8_t* colors = (uint8_t*)dvz_calloc((DvzSize)color_bytes, 1);
    float* glyph_angles = (float*)dvz_calloc((DvzSize)angle_bytes, 1);
    DvzTextGlyphSpan* spans = (DvzTextGlyphSpan*)dvz_calloc(count, sizeof(DvzTextGlyphSpan));
    if (positions == NULL || bounds == NULL || texcoords == NULL || colors == NULL ||
        glyph_angles == NULL || spans == NULL)
    {
        dvz_free(positions);
        dvz_free(bounds);
        dvz_free(texcoords);
        dvz_free(colors);
        dvz_free(glyph_angles);
        dvz_free(spans);
        log_error("text visual glyph allocation failed");
        return false;
    }

    const float(*target)[3] = (const float(*)[3])position_attr->data;
    const float(*offsets)[2] =
        offset_attr != NULL ? (const float(*)[2])offset_attr->data : NULL;
    const float(*text_anchors)[2] =
        anchor_attr != NULL ? (const float(*)[2])anchor_attr->data : NULL;
    const float* sizes = size_attr != NULL ? (const float*)size_attr->data : NULL;
    const uint8_t(*item_colors)[4] =
        color_attr != NULL ? (const uint8_t(*)[4])color_attr->data : NULL;
    const float* angles = angle_attr != NULL ? (const float*)angle_attr->data : NULL;
    uint32_t vertex_count = 0;

    for (uint32_t i = 0; i < count; i++)
    {
        DvzTextStyle style = {
            DVZ_STRUCT_INIT_FIELDS(DvzTextStyle),
            .size_px = (sizes != NULL ? sizes[i] : 12.0f) * screen_scale,
            .renderer = renderer,
            .color = {255, 255, 255, 255},
        };
        if (item_colors != NULL)
        {
            style.color[0] = item_colors[i][0];
            style.color[1] = item_colors[i][1];
            style.color[2] = item_colors[i][2];
            style.color[3] = item_colors[i][3];
        }
        uint8_t color[4] = {0};
        _text_style_color(&style, color);

        uint32_t visible = 0;
        float scale = 1.0f;
        float glyph_w = 0.0f;
        float glyph_h = 0.0f;
        float line_h = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        uint32_t columns = 0;
        uint32_t lines = 0;
        uint32_t cells_visible = 0;
        _text_measure_cells(
            _visual_family_state(visual)->text.strings[i], &columns, &lines, &cells_visible);
        if (!use_builtin)
        {
            scale = _text_sdf_layout_scale(&style, font_atlas);
            _text_sdf_measure(
                _visual_family_state(visual)->text.strings[i], font_atlas, scale, &width, &height, &visible);
            float natural_line_h = font_atlas->line_height * scale;
            line_h = _text_layout_line_advance(&_visual_family_state(visual)->text.layout, natural_line_h);
            if (lines > 1u)
                height += (float)(lines - 1u) * (line_h - natural_line_h);
        }
        else
        {
            visible = cells_visible;
            scale = _text_bitmap_layout_scale(&style);
            glyph_w = (float)DVZ_TEXT_BITMAP_GLYPH_WIDTH * scale;
            glyph_h = (float)DVZ_TEXT_BITMAP_GLYPH_HEIGHT * scale;
            line_h = _text_layout_line_advance(
                &_visual_family_state(visual)->text.layout,
                (float)DVZ_TEXT_BITMAP_LINE_HEIGHT * scale);
            width = (float)columns * glyph_w;
            height = (float)(lines - 1u) * line_h + glyph_h;
        }
        if (visible == 0 || width <= 0.0f || height <= 0.0f)
        {
            spans[i].first_glyph = vertex_count / 6u;
            spans[i].glyph_count = 0;
            continue;
        }

        float text_anchor[2] = {0.0f, 0.0f};
        if (text_anchors != NULL)
        {
            text_anchor[0] = text_anchors[i][0];
            text_anchor[1] = text_anchors[i][1];
        }
        float align_x = -text_anchor[0] * width;
        float align_y = -text_anchor[1] * height;
        if (offsets != NULL)
        {
            align_x += offsets[i][0];
            align_y += offsets[i][1];
        }
        float angle = angles != NULL ? angles[i] : 0.0f;
        float anchor_position[3] = {0};
        if (attach->controller_mode == DVZ_CONTROLLER_FIXED)
        {
            _text_panel_pixel_to_clip(
                panel, target[i][0], target[i][1], target[i][2], anchor_position);
        }
        else
        {
            anchor_position[0] = target[i][0];
            anchor_position[1] = target[i][1];
            anchor_position[2] = target[i][2];
        }
        spans[i].first_glyph = vertex_count / 6u;

        uint32_t column = 0;
        uint32_t row = 0;
        uint32_t byte_index = 0;
        uint32_t cp = 0;
        float cursor_x = 0.0f;
        while (_text_utf8_next(_visual_family_state(visual)->text.strings[i], &byte_index, &cp))
        {
            if (cp == '\n')
            {
                column = 0;
                cursor_x = 0.0f;
                row++;
                continue;
            }
            if (cp == '\t')
            {
                if (!use_builtin)
                {
                    DvzTextAtlasGlyph* space = _scene_text_atlas_glyph(font_atlas, ' ');
                    cursor_x += space != NULL ? 4.0f * space->advance * scale :
                                                 2.0f * font_atlas->em_px * scale;
                }
                else
                {
                    column += 4u;
                }
                continue;
            }

            float uv[4] = {0};
            float x0 = 0.0f;
            float y0 = 0.0f;
            float x1 = 0.0f;
            float y1 = 0.0f;
            if (!use_builtin)
            {
                DvzTextAtlasGlyph* glyph = _scene_text_atlas_glyph(font_atlas, cp);
                if (glyph == NULL)
                    continue;
                float advance = glyph->advance * scale;
                if (glyph->width <= 0.0f || glyph->height <= 0.0f)
                {
                    cursor_x += advance;
                    continue;
                }
                x0 = align_x + cursor_x + glyph->xoff * scale;
                y0 = align_y + (float)row * line_h + font_atlas->ascent * scale +
                     glyph->yoff * scale;
                x1 = x0 + glyph->width * scale;
                y1 = y0 + glyph->height * scale;
                uv[0] = glyph->uv[0];
                uv[1] = glyph->uv[1];
                uv[2] = glyph->uv[2];
                uv[3] = glyph->uv[3];
                cursor_x += advance;
            }
            else
            {
                x0 = align_x + (float)column * glyph_w;
                y0 = align_y + (float)row * line_h;
                x1 = x0 + glyph_w;
                y1 = y0 + glyph_h;
                _text_bitmap_atlas_uv(cp, uv);
                column++;
            }
            float bounds_rect[4] = {x0, y0, x1, y1};
            for (uint32_t j = 0; j < 6; j++)
            {
                _text_write_glyph_vertex(
                    anchor_position, bounds_rect, uv, color, angle, vertex_count, positions,
                    bounds, texcoords, colors, glyph_angles);
                vertex_count++;
            }
        }
        spans[i].glyph_count = vertex_count / 6u - spans[i].first_glyph;
    }

    bool ok = atlas != NULL;
    if (ok && _visual_family_state(visual)->text.glyph_visual == NULL)
    {
        _visual_family_state(visual)->text.glyph_visual = dvz_glyph(visual->scene, 0);
        if (_visual_family_state(visual)->text.glyph_visual == NULL)
            ok = false;
    }
    if (ok && !_text_sync_glyph_visual_attach(
                  panel, _visual_family_state(visual)->text.glyph_visual, &glyph_attach, attach))
        ok = false;
    if (ok && dvz_visual_set_alpha_mode(_visual_family_state(visual)->text.glyph_visual, DVZ_ALPHA_BLENDED) != 0)
        ok = false;
    if (ok && dvz_visual_set_depth_test(_visual_family_state(visual)->text.glyph_visual, false) != 0)
        ok = false;
    if (ok)
    {
        _visual_family_state(_visual_family_state(visual)->text.glyph_visual)->glyph_atlas_encoding =
            font_atlas != NULL ? font_atlas->encoding : DVZ_TEXT_ATLAS_ENCODING_BITMAP_ALPHA;
        _visual_family_state(_visual_family_state(visual)->text.glyph_visual)->glyph_distance_range_px =
            font_atlas != NULL ? font_atlas->distance_range_px : 1.0f;
        uint32_t upload_vertex_count = vertex_count;
        if (_visual_family_state(visual)->text.reserved_glyph_vertices > upload_vertex_count)
            upload_vertex_count = _visual_family_state(visual)->text.reserved_glyph_vertices;
        DvzVisualDataUpdate updates[5] = {
            {.attr_name = "position", .data = positions, .item_count = upload_vertex_count},
            {.attr_name = "bounds", .data = bounds, .item_count = upload_vertex_count},
            {.attr_name = "texcoords", .data = texcoords, .item_count = upload_vertex_count},
            {.attr_name = "color", .data = colors, .item_count = upload_vertex_count},
            {.attr_name = "angle", .data = glyph_angles, .item_count = upload_vertex_count},
        };
        ok = dvz_visual_set_data_many(_visual_family_state(visual)->text.glyph_visual, updates, 5) == 0 &&
             dvz_visual_set_field(_visual_family_state(visual)->text.glyph_visual, "field", atlas) ==
                 DVZ_OK;
    }
    if (ok)
    {
        dvz_visual_set_visible(_visual_family_state(visual)->text.glyph_visual, true);
        dvz_free(_visual_family_state(visual)->text.spans);
        _visual_family_state(visual)->text.spans = spans;
        _visual_family_state(visual)->text.span_count = count;
        spans = NULL;
        _visual_family_state(visual)->text.realized_version = version;
        _visual_family_state(visual)->text.realized_layout_version = layout_version;
        _visual_family_state(visual)->text.atlas_generation = atlas_generation;
        _visual_family_state(visual)->text.realized_controller_mode = attach->controller_mode;
        _visual_family_state(visual)->text.screen_scale = screen_scale;
        _visual_family_state(visual)->text.visual_figure_width = figure->width;
        _visual_family_state(visual)->text.visual_figure_height = figure->height;
    }
    else if (_visual_family_state(visual)->text.glyph_visual != NULL)
    {
        dvz_visual_set_visible(_visual_family_state(visual)->text.glyph_visual, false);
    }

    dvz_free(positions);
    dvz_free(bounds);
    dvz_free(texcoords);
    dvz_free(colors);
    dvz_free(glyph_angles);
    dvz_free(spans);
    return ok;
}
