/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene text / annotation                                                                       */
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
#include "_visual_internal.h"
#include "core/scene_notify_internal.h"
#include "datoviz/scene.h"
#include "text_internal.h"



/*************************************************************************************************/
/*  Text                                                                                         */
/*************************************************************************************************/

#define DVZ_TEXT_STYLE_KNOWN_FLAGS     0u
#define DVZ_TEXT_PLACEMENT_KNOWN_FLAGS 0u
#define DVZ_TEXT_LAYOUT_KNOWN_FLAGS    0u


bool _text_style_is_zero(const DvzTextStyle* style)
{
    if (style == NULL)
        return true;
    return style->struct_size == 0 && style->flags == 0 && style->font == NULL &&
           style->size_px == 0.0f && style->renderer == 0 && style->color[0] == 0 &&
           style->color[1] == 0 && style->color[2] == 0 && style->color[3] == 0 &&
           style->style_flags == 0 && !style->bold && !style->italic && !style->underline;
}


bool _text_placement_is_zero(const DvzTextPlacement* placement)
{
    if (placement == NULL)
        return true;
    return placement->struct_size == 0 && placement->flags == 0 && placement->mode == 0 &&
           placement->anchor == 0 && placement->position[0] == 0.0 &&
           placement->position[1] == 0.0 && placement->position[2] == 0.0 &&
           placement->offset[0] == 0.0f && placement->offset[1] == 0.0f &&
           placement->text_anchor[0] == 0.0f && placement->text_anchor[1] == 0.0f &&
           !placement->has_text_anchor && placement->angle == 0.0f && !placement->depth_test;
}


bool _text_style_validate(const DvzTextStyle* style)
{
    if (style == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(style, DvzTextStyle, DVZ_TEXT_STYLE_KNOWN_FLAGS))
    {
        log_error("invalid text style ABI");
        return false;
    }
    return true;
}


bool _text_placement_validate(const DvzTextPlacement* placement)
{
    if (placement == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(placement, DvzTextPlacement, DVZ_TEXT_PLACEMENT_KNOWN_FLAGS))
    {
        log_error("invalid text placement ABI");
        return false;
    }
    return true;
}


static bool _text_layout_validate(const DvzTextLayout* layout)
{
    if (layout == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(layout, DvzTextLayout, DVZ_TEXT_LAYOUT_KNOWN_FLAGS))
    {
        log_error("invalid text layout ABI");
        return false;
    }
    if (layout->line_height < 0.0f || layout->line_gap_px < 0.0f || layout->wrap_width_px < 0.0f)
    {
        log_error("invalid negative text layout metric");
        return false;
    }
    return true;
}


/**
 * Return the default retained text style.
 *
 * @return default text style
 */
DvzTextStyle dvz_text_style(void)
{
    return (DvzTextStyle){
        DVZ_STRUCT_INIT_FIELDS(DvzTextStyle),
        .size_px = 0.0f,
        .renderer = DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS,
        .color = {255, 255, 255, 255},
    };
}


/**
 * Return the default retained text placement.
 *
 * @return default text placement
 */
DvzTextPlacement dvz_text_placement(void)
{
    return (DvzTextPlacement){
        DVZ_STRUCT_INIT_FIELDS(DvzTextPlacement),
        .mode = DVZ_TEXT_PLACEMENT_SCREEN,
        .anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT,
    };
}


DvzTextLayout dvz_text_layout(void)
{
    return (DvzTextLayout){
        DVZ_STRUCT_INIT_FIELDS(DvzTextLayout),
        .line_height = 1.0f,
        .line_gap_px = 0.0f,
        .wrap_width_px = 0.0f,
        .align = DVZ_TEXT_ALIGN_LEFT,
    };
}


/**
 * Return the default retained text style.
 *
 * @param scene the scene
 * @return default text style
 */
static DvzTextStyle _text_resolve_style(const DvzScene* scene, const DvzTextStyle* style)
{
    DvzTextStyle resolved = style != NULL ? *style : dvz_text_style();
    if (resolved.size_px <= 0.0f)
    {
        DvzFontDefaults defaults = scene != NULL ? scene->font_defaults : dvz_font_defaults();
        if (defaults.text_size_px <= 0.0f)
            defaults = dvz_font_defaults();
        resolved.size_px = defaults.text_size_px;
    }
    return resolved;
}



/**
 * Return the default retained text placement.
 *
 * @return default text placement
 */
static DvzTextPlacement _text_default_placement(void)
{
    return dvz_text_placement();
}


static DvzTextLayout _text_default_layout(void)
{
    return dvz_text_layout();
}


/**
 * Copy UTF-8 text content without applying short-label limits.
 *
 * @param src source string, or NULL for empty text
 * @return owned copy, or NULL on allocation failure
 */
char* _scene_text_strdup(const char* src)
{
    const char* resolved = src != NULL ? src : "";
    size_t len = strlen(resolved);
    if (len == SIZE_MAX)
    {
        log_error("text string length overflow");
        return NULL;
    }
    char* copy = (char*)dvz_calloc((DvzSize)len + 1u, 1);
    if (copy == NULL)
    {
        log_error("text string allocation failed");
        return NULL;
    }
    if (len > 0)
        dvz_memcpy(copy, len, resolved, len);
    copy[len] = '\0';
    return copy;
}


static void _text_free_collection(DvzText* text)
{
    ANN(text);
    if (text->strings != NULL)
    {
        for (uint32_t i = 0; i < text->item_count; i++)
            dvz_free(text->strings[i]);
    }
    dvz_free(text->strings);
    dvz_free(text->positions);
    dvz_free(text->offsets);
    dvz_free(text->anchors);
    dvz_free(text->sizes_px);
    dvz_free(text->colors);
    dvz_free(text->angles);
    text->strings = NULL;
    text->positions = NULL;
    text->offsets = NULL;
    text->anchors = NULL;
    text->sizes_px = NULL;
    text->colors = NULL;
    text->angles = NULL;
    text->item_count = 0;
}


static bool _text_color_zero(DvzColor color)
{
    return color.r == 0 && color.g == 0 && color.b == 0 && color.a == 0;
}


static DvzColor _text_style_dvz_color(const DvzTextStyle* style)
{
    uint8_t rgba[4] = {0};
    _text_style_color(style, rgba);
    return (DvzColor){rgba[0], rgba[1], rgba[2], rgba[3]};
}


static void _text_item_default_anchor(const DvzText* text, float out[2])
{
    ANN(text);
    ANN(out);
    out[0] = 0.0f;
    out[1] = 0.0f;
    if (text->placement.has_text_anchor)
    {
        out[0] = text->placement.text_anchor[0];
        out[1] = text->placement.text_anchor[1];
    }
}


static int _text_alloc_collection(
    const DvzText* text, const DvzTextItem* items, uint32_t item_count, char*** out_strings,
    double (**out_positions)[3], float (**out_offsets)[2], float (**out_anchors)[2],
    float** out_sizes, DvzColor** out_colors, float** out_angles)
{
    ANN(text);
    ANN(out_strings);
    ANN(out_positions);
    ANN(out_offsets);
    ANN(out_anchors);
    ANN(out_sizes);
    ANN(out_colors);
    ANN(out_angles);
    *out_strings = NULL;
    *out_positions = NULL;
    *out_offsets = NULL;
    *out_anchors = NULL;
    *out_sizes = NULL;
    *out_colors = NULL;
    *out_angles = NULL;
    if (item_count == 0)
        return 0;
    ANN(items);

    char** strings = (char**)dvz_calloc(item_count, sizeof(char*));
    double(*positions)[3] = (double(*)[3])dvz_calloc(item_count, sizeof(double[3]));
    float(*offsets)[2] = (float(*)[2])dvz_calloc(item_count, sizeof(float[2]));
    float(*anchors)[2] = (float(*)[2])dvz_calloc(item_count, sizeof(float[2]));
    float* sizes = (float*)dvz_calloc(item_count, sizeof(float));
    DvzColor* colors = (DvzColor*)dvz_calloc(item_count, sizeof(DvzColor));
    float* angles = (float*)dvz_calloc(item_count, sizeof(float));
    if (
        strings == NULL || positions == NULL || offsets == NULL || anchors == NULL ||
        sizes == NULL || colors == NULL || angles == NULL)
    {
        dvz_free(strings);
        dvz_free(positions);
        dvz_free(offsets);
        dvz_free(anchors);
        dvz_free(sizes);
        dvz_free(colors);
        dvz_free(angles);
        log_error("text item allocation failed");
        return -1;
    }

    float default_anchor[2] = {0};
    _text_item_default_anchor(text, default_anchor);
    DvzColor default_color = _text_style_dvz_color(&text->style);
    for (uint32_t i = 0; i < item_count; i++)
    {
        const char* src = items[i].string != NULL ? items[i].string : "";
        strings[i] = _scene_text_strdup(src);
        if (strings[i] == NULL)
        {
            for (uint32_t j = 0; j < i; j++)
                dvz_free(strings[j]);
            dvz_free(strings);
            dvz_free(positions);
            dvz_free(offsets);
            dvz_free(anchors);
            dvz_free(sizes);
            dvz_free(colors);
            dvz_free(angles);
            return -1;
        }
        positions[i][0] = items[i].position[0];
        positions[i][1] = items[i].position[1];
        positions[i][2] = items[i].position[2];
        offsets[i][0] = items[i].offset[0];
        offsets[i][1] = items[i].offset[1];
        anchors[i][0] = items[i].anchor[0];
        anchors[i][1] = items[i].anchor[1];
        if (anchors[i][0] == 0.0f && anchors[i][1] == 0.0f)
        {
            anchors[i][0] = default_anchor[0];
            anchors[i][1] = default_anchor[1];
        }
        sizes[i] = items[i].size_px > 0.0f ? items[i].size_px : text->style.size_px;
        colors[i] = _text_color_zero(items[i].color) ? default_color : items[i].color;
        angles[i] = items[i].angle;
    }

    *out_strings = strings;
    *out_positions = positions;
    *out_offsets = offsets;
    *out_anchors = anchors;
    *out_sizes = sizes;
    *out_colors = colors;
    *out_angles = angles;
    return 0;
}



/**
 * Return whether a renderer enum is implemented by the retained text path.
 *
 * @param renderer the renderer
 * @return whether the renderer is supported
 */
static bool _text_renderer_supported(DvzTextRenderer renderer)
{
    return renderer == DVZ_TEXT_RENDERER_AUTO ||
           renderer == DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS ||
           renderer == DVZ_TEXT_RENDERER_BITMAP_ATLAS ||
           renderer == DVZ_TEXT_RENDERER_MSDF_ATLAS;
}



/**
 * Mark a retained text object dirty and request a frame.
 *
 * @param text the text object
 * @param flags dirty flags
 */
static void _text_mark_dirty(DvzText* text, uint32_t flags)
{
    ANN(text);
    text->dirty_flags |= flags;
    text->version++;
    _scene_notify_request_frame(text->panel != NULL ? text->panel->figure : NULL);
}



/**
 * Create a retained text object attached to a panel.
 *
 * @param panel the panel
 * @param flags creation flags
 * @return the text object, or NULL on allocation failure
 */
DvzText* dvz_text(DvzPanel* panel, uint32_t flags)
{
    ANN(panel);
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return NULL;
    DvzScene* scene = panel->figure->scene;
    if (scene->text_count >= DVZ_SCENE_MAX_TEXTS)
    {
        log_error("maximum text count reached");
        return NULL;
    }
    DvzText* text = &scene->texts[scene->text_count++];
    dvz_memset(text, sizeof(DvzText), 0, sizeof(DvzText));
    text->scene = scene;
    text->id = _scene_next_id(scene);
    text->panel = panel;
    text->style = _text_resolve_style(scene, NULL);
    text->placement = _text_default_placement();
    text->layout = _text_default_layout();
    text->flags = flags;
    text->dirty_flags = DVZ_TEXT_DIRTY_ALL;
    text->version = 1;
    _scene_notify_request_frame(panel->figure);
    return text;
}


DvzId dvz_text_id(const DvzText* text)
{
    return text != NULL && text->scene != NULL ? text->id : DVZ_ID_NONE;
}



/**
 * Destroy a retained text object.
 *
 * @param text the text object
 */
void dvz_text_destroy(DvzText* text)
{
    if (text == NULL)
        return;
    if (text->visual != NULL)
    {
        if (text->visual->type == DVZ_VISUAL_TYPE_TEXT &&
            _visual_family_state(text->visual)->text.glyph_visual != NULL)
        {
            dvz_visual_set_visible(
                _visual_family_state(text->visual)->text.glyph_visual, false);
        }
        dvz_visual_set_visible(text->visual, false);
    }
    _text_free_collection(text);
    _scene_notify_request_frame(text->panel != NULL ? text->panel->figure : NULL);
    text->scene = NULL;
    text->panel = NULL;
    text->legacy_string = NULL;
    text->dirty_flags = DVZ_TEXT_DIRTY_NONE;
}



/**
 * Set the UTF-8 content of a retained text object.
 *
 * @param text the text object
 * @param string the string, or NULL to clear
 */
int dvz_text_set_items(DvzText* text, const DvzTextItem* items, uint32_t item_count)
{
    ANN(text);
    if (item_count > 0 && items == NULL)
        return -1;
    char** strings = NULL;
    double(*positions)[3] = NULL;
    float(*offsets)[2] = NULL;
    float(*anchors)[2] = NULL;
    float* sizes = NULL;
    DvzColor* colors = NULL;
    float* angles = NULL;
    if (_text_alloc_collection(
            text, items, item_count, &strings, &positions, &offsets, &anchors, &sizes, &colors,
            &angles) != 0)
    {
        return -1;
    }
    _text_free_collection(text);
    text->strings = strings;
    text->positions = positions;
    text->offsets = offsets;
    text->anchors = anchors;
    text->sizes_px = sizes;
    text->colors = colors;
    text->angles = angles;
    text->item_count = item_count;
    text->legacy_string = NULL;
    _text_mark_dirty(text, DVZ_TEXT_DIRTY_STRING | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER);
    return 0;
}


int dvz_text_set_string(DvzText* text, const char* string)
{
    ANN(text);
    DvzTextItem item = {DVZ_STRUCT_INIT_FIELDS(DvzTextItem), .string = string};
    item.position[0] = text->placement.position[0];
    item.position[1] = text->placement.position[1];
    item.position[2] = text->placement.position[2];
    item.offset[0] = text->placement.offset[0];
    item.offset[1] = text->placement.offset[1];
    item.size_px = text->style.size_px;
    item.color = _text_style_dvz_color(&text->style);
    item.angle = text->placement.angle;
    if (text->placement.has_text_anchor)
    {
        item.anchor[0] = text->placement.text_anchor[0];
        item.anchor[1] = text->placement.text_anchor[1];
    }
    return dvz_text_set_items(text, &item, 1);
}


int dvz_text_set_position(DvzText* text, const double position[3])
{
    ANN(text);
    ANN(position);
    if (text->item_count != 1 || text->positions == NULL)
        return -1;
    text->positions[0][0] = position[0];
    text->positions[0][1] = position[1];
    text->positions[0][2] = position[2];
    _text_mark_dirty(text, DVZ_TEXT_DIRTY_PLACEMENT | DVZ_TEXT_DIRTY_LAYOUT);
    return 0;
}


int dvz_text_set_layout(DvzText* text, const DvzTextLayout* layout)
{
    ANN(text);
    if (layout != NULL && !_text_layout_validate(layout))
        return -1;
    text->layout = layout != NULL ? *layout : _text_default_layout();
    _text_mark_dirty(text, DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER);
    return 0;
}


static int _text_check_item_count(const DvzText* text, uint32_t item_count)
{
    ANN(text);
    if (item_count == 0 || text->item_count != item_count)
    {
        log_error("text array setter item_count mismatch");
        return -1;
    }
    return 0;
}


int dvz_text_set_strings(DvzText* text, const char* const* strings, uint32_t item_count)
{
    ANN(text);
    ANN(strings);
    DvzTextItem* items = (DvzTextItem*)dvz_calloc(item_count, sizeof(DvzTextItem));
    if (items == NULL)
        return -1;
    for (uint32_t i = 0; i < item_count; i++)
    {
        items[i] = (DvzTextItem){DVZ_STRUCT_INIT_FIELDS(DvzTextItem), .string = strings[i]};
        if (text->item_count == item_count)
        {
            items[i].position[0] = text->positions[i][0];
            items[i].position[1] = text->positions[i][1];
            items[i].position[2] = text->positions[i][2];
            items[i].offset[0] = text->offsets[i][0];
            items[i].offset[1] = text->offsets[i][1];
            items[i].anchor[0] = text->anchors[i][0];
            items[i].anchor[1] = text->anchors[i][1];
            items[i].size_px = text->sizes_px[i];
            items[i].color = text->colors[i];
            items[i].angle = text->angles[i];
        }
    }
    int rc = dvz_text_set_items(text, items, item_count);
    dvz_free(items);
    return rc;
}


int dvz_text_set_positions(DvzText* text, const double (*positions)[3], uint32_t item_count)
{
    ANN(text);
    ANN(positions);
    if (_text_check_item_count(text, item_count) != 0)
        return -1;
    dvz_memcpy(text->positions, (DvzSize)item_count * sizeof(double[3]), positions, (DvzSize)item_count * sizeof(double[3]));
    _text_mark_dirty(text, DVZ_TEXT_DIRTY_PLACEMENT | DVZ_TEXT_DIRTY_LAYOUT);
    return 0;
}


int dvz_text_set_offsets(DvzText* text, const float (*offsets)[2], uint32_t item_count)
{
    ANN(text);
    ANN(offsets);
    if (_text_check_item_count(text, item_count) != 0)
        return -1;
    dvz_memcpy(text->offsets, (DvzSize)item_count * sizeof(float[2]), offsets, (DvzSize)item_count * sizeof(float[2]));
    _text_mark_dirty(text, DVZ_TEXT_DIRTY_LAYOUT);
    return 0;
}


int dvz_text_set_anchors(DvzText* text, const float (*anchors)[2], uint32_t item_count)
{
    ANN(text);
    ANN(anchors);
    if (_text_check_item_count(text, item_count) != 0)
        return -1;
    dvz_memcpy(text->anchors, (DvzSize)item_count * sizeof(float[2]), anchors, (DvzSize)item_count * sizeof(float[2]));
    _text_mark_dirty(text, DVZ_TEXT_DIRTY_LAYOUT);
    return 0;
}


int dvz_text_set_sizes(DvzText* text, const float* sizes_px, uint32_t item_count)
{
    ANN(text);
    ANN(sizes_px);
    if (_text_check_item_count(text, item_count) != 0)
        return -1;
    dvz_memcpy(text->sizes_px, (DvzSize)item_count * sizeof(float), sizes_px, (DvzSize)item_count * sizeof(float));
    _text_mark_dirty(text, DVZ_TEXT_DIRTY_STYLE | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER);
    return 0;
}


int dvz_text_set_colors(DvzText* text, const DvzColor* colors, uint32_t item_count)
{
    ANN(text);
    ANN(colors);
    if (_text_check_item_count(text, item_count) != 0)
        return -1;
    dvz_memcpy(text->colors, (DvzSize)item_count * sizeof(DvzColor), colors, (DvzSize)item_count * sizeof(DvzColor));
    _text_mark_dirty(text, DVZ_TEXT_DIRTY_STYLE | DVZ_TEXT_DIRTY_RENDER);
    return 0;
}


int dvz_text_set_angles(DvzText* text, const float* angles, uint32_t item_count)
{
    ANN(text);
    ANN(angles);
    if (_text_check_item_count(text, item_count) != 0)
        return -1;
    dvz_memcpy(text->angles, (DvzSize)item_count * sizeof(float), angles, (DvzSize)item_count * sizeof(float));
    _text_mark_dirty(text, DVZ_TEXT_DIRTY_PLACEMENT | DVZ_TEXT_DIRTY_LAYOUT);
    return 0;
}



/**
 * Set the style of a retained text object.
 *
 * @param text the text object
 * @param style the style descriptor, or NULL for defaults
 * @return 0 on success, -1 on error
 */
int dvz_text_set_style(DvzText* text, const DvzTextStyle* style)
{
    ANN(text);
    if (style != NULL && !_text_style_validate(style))
        return -1;
    DvzTextStyle resolved = _text_resolve_style(text->scene, style);
    if (resolved.font != NULL && resolved.font->scene != text->scene)
    {
        log_error("cannot bind a font from a different scene");
        return -1;
    }
    if (!_text_renderer_supported(resolved.renderer))
    {
        log_error("text renderer %d is not implemented for retained text yet", resolved.renderer);
        return -1;
    }
    text->style = resolved;
    _text_mark_dirty(text, DVZ_TEXT_DIRTY_STYLE | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER);
    return 0;
}



/**
 * Set the placement of a retained text object.
 *
 * @param text the text object
 * @param placement the placement descriptor, or NULL for defaults
 */
int dvz_text_set_placement(DvzText* text, const DvzTextPlacement* placement)
{
    ANN(text);
    if (placement != NULL && !_text_placement_validate(placement))
        return -1;
    text->placement = placement != NULL ? *placement : _text_default_placement();
    if (text->item_count == 1 && text->positions != NULL)
    {
        text->positions[0][0] = text->placement.position[0];
        text->positions[0][1] = text->placement.position[1];
        text->positions[0][2] = text->placement.position[2];
        text->offsets[0][0] = text->placement.offset[0];
        text->offsets[0][1] = text->placement.offset[1];
        if (text->placement.has_text_anchor)
        {
            text->anchors[0][0] = text->placement.text_anchor[0];
            text->anchors[0][1] = text->placement.text_anchor[1];
        }
        text->angles[0] = text->placement.angle;
    }
    _text_mark_dirty(text, DVZ_TEXT_DIRTY_PLACEMENT | DVZ_TEXT_DIRTY_LAYOUT);
    return 0;
}



/**
 * Select the renderer used by a retained text object.
 *
 * @param text the text object
 * @param renderer renderer selection
 * @return 0 on success, -1 on error
 */
int dvz_text_set_renderer(DvzText* text, DvzTextRenderer renderer)
{
    ANN(text);
    if (!_text_renderer_supported(renderer))
    {
        log_error("text renderer %d is not implemented for retained text yet", renderer);
        return -1;
    }
    if (text->style.renderer == renderer)
        return 0;
    text->style.renderer = renderer;
    _text_mark_dirty(text, DVZ_TEXT_DIRTY_STYLE | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER);
    return 0;
}
