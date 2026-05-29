/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene axis text realization                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_scene.h"
#include "axis_internal.h"
#include "datoviz/scene.h"
#include "text/internal.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Apply the axis text renderer to the derived text visual.
 *
 * @param axis the axis
 * @return whether the renderer was applied
 */
static bool _axis_apply_text_renderer(DvzAxis* axis)
{
    ANN(axis);
    if (axis->text_visual == NULL)
        return true;
    return _scene_adornment_text_visual_set_renderer(
               axis->text_visual, axis->style.text_renderer) == 0;
}


/**
 * Format one numeric tick value for the first rendered 2D axis label slice.
 *
 * @param value the tick value
 * @param step the major tick step
 * @param out output string buffer
 * @param out_size output string buffer size
 */
static void _axis_format_tick(double value, double step, char* out, uint32_t out_size)
{
    ANN(out);
    if (out_size == 0)
        return;
    if (isfinite(step) && step > 0.0 && fabs(value) < 0.5 * step * 1e-9)
        value = 0.0;
    dvz_snprintf(out, out_size, "%.6g", value);
}


/**
 * Return whether two axis text layouts are byte-identical.
 *
 * @param axis the axis
 * @param count text item count
 * @param labels text labels
 * @param positions text positions
 * @param anchors text anchors
 * @param sizes text sizes
 * @param colors text colors
 * @param angles text angles
 * @return whether the cached layout matches
 */
static bool _axis_text_cache_matches(
    const DvzAxis* axis, uint32_t count, char labels[][DVZ_SCENE_LABEL_SIZE],
    float positions[][3], float anchors[][2], float* sizes, uint8_t colors[][4],
    float* angles)
{
    ANN(axis);
    if (axis->text_count != count)
        return false;
    for (uint32_t i = 0; i < count; i++)
    {
        if (strcmp(axis->text_labels[i], labels[i]) != 0)
            return false;
        for (uint32_t j = 0; j < 3; j++)
        {
            if (fabsf(axis->text_positions[i][j] - positions[i][j]) > 1e-5f)
                return false;
        }
        for (uint32_t j = 0; j < 2; j++)
        {
            if (fabsf(axis->text_anchors[i][j] - anchors[i][j]) > 1e-5f)
                return false;
        }
        if (fabsf(axis->text_sizes[i] - sizes[i]) > 1e-5f)
            return false;
        if (memcmp(axis->text_colors[i], colors[i], 4) != 0)
            return false;
        if (fabsf(axis->text_angles[i] - angles[i]) > 1e-5f)
            return false;
    }
    return true;
}


/**
 * Store a successfully emitted axis text layout in the axis cache.
 *
 * @param axis the axis
 * @param count text item count
 * @param labels text labels
 * @param positions text positions
 * @param anchors text anchors
 * @param sizes text sizes
 * @param colors text colors
 * @param angles text angles
 */
static void _axis_text_cache_store(
    DvzAxis* axis, uint32_t count, char labels[][DVZ_SCENE_LABEL_SIZE],
    float positions[][3], float anchors[][2], float* sizes, uint8_t colors[][4],
    float* angles)
{
    ANN(axis);
    axis->text_count = count;
    for (uint32_t i = 0; i < count; i++)
    {
        dvz_strlcpy(axis->text_labels[i], labels[i], sizeof(axis->text_labels[i]));
        for (uint32_t j = 0; j < 3; j++)
            axis->text_positions[i][j] = positions[i][j];
        for (uint32_t j = 0; j < 2; j++)
            axis->text_anchors[i][j] = anchors[i][j];
        axis->text_sizes[i] = sizes[i];
        for (uint32_t j = 0; j < 4; j++)
            axis->text_colors[i][j] = colors[i][j];
        axis->text_angles[i] = angles[i];
    }
}


/**
 * Hide the derived text visual for an axis.
 *
 * @param axis the axis
 */
void _axis_hide_text(DvzAxis* axis)
{
    ANN(axis);
    axis->text_count = 0;
    if (axis->text_visual != NULL)
    {
        if (axis->text_visual->visible)
            dvz_visual_set_visible(axis->text_visual, false);
        if (_visual_family_state(axis->text_visual)->text.glyph_visual != NULL &&
            _visual_family_state(axis->text_visual)->text.glyph_visual->visible)
            dvz_visual_set_visible(_visual_family_state(axis->text_visual)->text.glyph_visual, false);
    }
}


/**
 * Ensure one derived screen-space text visual exists for an axis.
 *
 * @param axis the axis
 * @return whether the text visual exists
 */
static bool _axis_ensure_text_visual(DvzAxis* axis)
{
    ANN(axis);
    if (axis->text_visual != NULL)
        return true;
    if (axis->panel == NULL || axis->panel->figure == NULL || axis->panel->figure->scene == NULL)
        return false;
    axis->text_visual =
        _scene_adornment_text_visual(axis->panel->figure->scene, axis->style.text_renderer);
    if (axis->text_visual == NULL)
        return false;
    axis->text_visual->visible = false;
    DvzVisualAttachDesc attach = {.z_layer = 1001, .controller_mode = DVZ_CONTROLLER_FIXED};
    if (dvz_panel_add_visual(axis->panel, axis->text_visual, &attach) != 0)
    {
        axis->text_visual = NULL;
        return false;
    }
    return true;
}


/**
 * Append one text item to axis text layout arrays.
 *
 * @param count current text item count
 * @param labels text labels
 * @param strings string pointer table
 * @param positions text positions
 * @param anchors text anchors
 * @param sizes text sizes
 * @param colors text colors
 * @param angles text angles
 * @param label item label
 * @param x item x coordinate in pixels
 * @param y item y coordinate in pixels
 * @param anchor_x text anchor x
 * @param anchor_y text anchor y
 * @param size text size in pixels
 * @param color text color
 * @param angle text angle in radians
 */
static void _axis_append_text_item(
    uint32_t* count, char labels[][DVZ_SCENE_LABEL_SIZE], const char** strings,
    float positions[][3], float anchors[][2], float* sizes, uint8_t colors[][4], float* angles,
    const char* label, float x, float y, float anchor_x, float anchor_y, float size,
    const uint8_t color[4], float angle)
{
    ANN(count);
    ANN(labels);
    ANN(strings);
    ANN(positions);
    ANN(anchors);
    ANN(sizes);
    ANN(colors);
    ANN(angles);
    ANN(label);
    ANN(color);
    if (*count >= DVZ_SCENE_MAX_AXIS_TICKS + 1)
        return;
    uint32_t i = (*count)++;
    dvz_strlcpy(labels[i], label, DVZ_SCENE_LABEL_SIZE);
    strings[i] = labels[i];
    positions[i][0] = x;
    positions[i][1] = y;
    positions[i][2] = 0.0f;
    anchors[i][0] = anchor_x;
    anchors[i][1] = anchor_y;
    sizes[i] = size;
    for (uint32_t j = 0; j < 4; j++)
        colors[i][j] = color[j];
    angles[i] = angle;
}


/**
 * Return a positive axis text size, falling back to the built-in default.
 *
 * @param value configured size in logical pixels
 * @param fallback fallback size in logical pixels
 * @return resolved size in logical pixels
 */
static float _axis_text_size(float value, float fallback)
{
    return value > 0.0f && isfinite(value) ? value : fallback;
}


/**
 * Rebuild the derived text visual for tick labels and the axis label.
 *
 * @param axis the axis
 * @param x0 plot left in visual coordinates
 * @param x1 plot right in visual coordinates
 * @param y0 plot bottom in visual coordinates
 * @param y1 plot top in visual coordinates
 * @param visible_min visible data minimum
 * @param visible_max visible data maximum
 */
void _axis_update_text(
    DvzAxis* axis, float x0, float x1, float y0, float y1, double visible_min,
    double visible_max)
{
    ANN(axis);
    if (!axis->enabled || axis->tick_count == 0 || !(visible_max > visible_min))
    {
        _axis_hide_text(axis);
        return;
    }

    uint32_t count = 0;
    char labels[DVZ_SCENE_MAX_AXIS_TICKS + 1][DVZ_SCENE_LABEL_SIZE] = {{0}};
    const char* strings[DVZ_SCENE_MAX_AXIS_TICKS + 1] = {0};
    float positions[DVZ_SCENE_MAX_AXIS_TICKS + 1][3] = {{0}};
    float anchors[DVZ_SCENE_MAX_AXIS_TICKS + 1][2] = {{0}};
    float sizes[DVZ_SCENE_MAX_AXIS_TICKS + 1] = {0};
    uint8_t colors[DVZ_SCENE_MAX_AXIS_TICKS + 1][4] = {{0}};
    float angles[DVZ_SCENE_MAX_AXIS_TICKS + 1] = {0};

    for (uint32_t i = 0; i < axis->tick_count; i++)
    {
        float plot_min = axis->dim == DVZ_DIM_X ? x0 : y0;
        float plot_max = axis->dim == DVZ_DIM_X ? x1 : y1;
        float p =
            _axis_data_to_visual(axis->ticks[i], visible_min, visible_max, plot_min, plot_max);
        if (p < plot_min - 0.0001f || p > plot_max + 0.0001f)
            continue;

        char tick_label[DVZ_SCENE_LABEL_SIZE] = {0};
        _axis_format_tick(axis->ticks[i], axis->tick_lstep, tick_label, sizeof(tick_label));
        float px = 0.0f;
        float py = 0.0f;
        if (axis->dim == DVZ_DIM_X)
        {
            _axis_visual_to_pixels(axis, p, y0, &px, &py);
            py += axis->style.tick_gap_px > 0.0f && isfinite(axis->style.tick_gap_px) ?
                      axis->style.tick_gap_px :
                      AXIS_TEXT_TICK_GAP;
            _axis_append_text_item(
                &count, labels, strings, positions, anchors, sizes, colors, angles, tick_label, px,
                py, 0.5f, 0.0f,
                _axis_text_size(axis->style.tick_size_px, AXIS_TEXT_TICK_SIZE),
                axis->style.major_tick_color, 0.0f);
        }
        else
        {
            _axis_visual_to_pixels(axis, x0, p, &px, &py);
            px -= axis->style.tick_gap_px > 0.0f && isfinite(axis->style.tick_gap_px) ?
                      axis->style.tick_gap_px :
                      AXIS_TEXT_TICK_GAP;
            _axis_append_text_item(
                &count, labels, strings, positions, anchors, sizes, colors, angles, tick_label, px,
                py, 1.0f, 0.5f,
                _axis_text_size(axis->style.tick_size_px, AXIS_TEXT_TICK_SIZE),
                axis->style.major_tick_color, 0.0f);
        }
    }

    if (axis->label[0] != '\0')
    {
        float px = 0.0f;
        float py = 0.0f;
        if (axis->dim == DVZ_DIM_X)
        {
            _axis_visual_to_pixels(axis, 0.5f * (x0 + x1), y0, &px, &py);
            py += axis->style.label_gap_px > 0.0f && isfinite(axis->style.label_gap_px) ?
                      axis->style.label_gap_px :
                      AXIS_TEXT_LABEL_GAP;
            _axis_append_text_item(
                &count, labels, strings, positions, anchors, sizes, colors, angles, axis->label,
                px, py, 0.5f, 0.0f,
                _axis_text_size(axis->style.label_size_px, AXIS_TEXT_LABEL_SIZE),
                axis->style.spine_color, 0.0f);
        }
        else
        {
            _axis_visual_to_pixels(axis, x0, 0.5f * (y0 + y1), &px, &py);
            px -= axis->style.label_gap_px > 0.0f && isfinite(axis->style.label_gap_px) ?
                      axis->style.label_gap_px :
                      AXIS_TEXT_LABEL_GAP;
            _axis_append_text_item(
                &count, labels, strings, positions, anchors, sizes, colors, angles, axis->label,
                px, py, 0.5f, 0.5f,
                _axis_text_size(axis->style.label_size_px, AXIS_TEXT_LABEL_SIZE),
                axis->style.spine_color, AXIS_TEXT_Y_LABEL_ANGLE);
        }
    }

    if (count == 0)
    {
        _axis_hide_text(axis);
        return;
    }
    if (!_axis_ensure_text_visual(axis))
        return;
    if (!_axis_apply_text_renderer(axis))
        return;
    if (_axis_text_cache_matches(axis, count, labels, positions, anchors, sizes, colors, angles))
    {
        if (!axis->text_visual->visible)
            dvz_visual_set_visible(axis->text_visual, true);
        return;
    }

    DvzVisualDataUpdate updates[5] = {
        {.attr_name = "position", .data = positions, .item_count = count},
        {.attr_name = "anchor", .data = anchors, .item_count = count},
        {.attr_name = "size", .data = sizes, .item_count = count},
        {.attr_name = "color", .data = colors, .item_count = count},
        {.attr_name = "angle", .data = angles, .item_count = count},
    };
    if (dvz_visual_set_strings(axis->text_visual, "text", strings, count) == 0 &&
        dvz_visual_set_data_many(axis->text_visual, updates, 5) == 0)
    {
        if (!axis->text_visual->visible)
            dvz_visual_set_visible(axis->text_visual, true);
        _axis_text_cache_store(axis, count, labels, positions, anchors, sizes, colors, angles);
    }
    else
    {
        _axis_hide_text(axis);
    }
}

