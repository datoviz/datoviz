/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene colorbar annotations                                                                   */
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
#include "_log.h"
#include "_scene.h"
#include "core/scene_notify_internal.h"
#include "colormap_internal.h"
#include "core/format_state_internal.h"
#include "datoviz/scene.h"
#include "prepare_internal.h"
#include "scale_internal.h"
#include "annotation/text_visual_bridge.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define COLORBAR_RAMP_SEGMENTS 64u
#define COLORBAR_VERTICAL_RESERVE_PX 140.0f
#define COLORBAR_HORIZONTAL_RESERVE_PX 96.0f
#define COLORBAR_RAMP_THICKNESS_PX 36.0f
#define COLORBAR_EDGE_OFFSET_PX 0.0f
#define COLORBAR_PLOT_GAP_PX   12.0f
#define COLORBAR_TICK_LENGTH_PX 6.0f
#define COLORBAR_LABEL_GAP_PX 6.0f
#define COLORBAR_TITLE_GAP_PX 8.0f
#define COLORBAR_TICK_WIDTH_PX 1.0f
#define COLORBAR_TICK_TEXT_SIZE_PX 12.0f
#define COLORBAR_TITLE_TEXT_SIZE_PX 13.0f
#define COLORBAR_EPS 1e-12
#define COLORBAR_LAYOUT_EPS 1e-3f



/*************************************************************************************************/
/*  Function prototypes                                                                          */
/*************************************************************************************************/

static void _colorbar_hide(DvzColorbar* colorbar);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Mark one retained colorbar layout as dirty.
 *
 * @param colorbar the colorbar
 */
void _scene_mark_colorbar_dirty(DvzColorbar* colorbar)
{
    if (colorbar == NULL)
        return;
    colorbar->dirty = true;
    colorbar->version = colorbar->version == UINT64_MAX ? 1 : colorbar->version + 1;
    _scene_notify_request_frame(colorbar->panel != NULL ? colorbar->panel->figure : NULL);
}


/**
 * Return whether a colorbar anchor is supported by the first rendered slice.
 *
 * @param anchor the anchor
 * @return whether the anchor is a panel edge
 */
static bool _colorbar_anchor_supported(DvzSceneAnchor anchor)
{
    return anchor == DVZ_SCENE_ANCHOR_PANEL_LEFT || anchor == DVZ_SCENE_ANCHOR_PANEL_RIGHT ||
           anchor == DVZ_SCENE_ANCHOR_PANEL_TOP || anchor == DVZ_SCENE_ANCHOR_PANEL_BOTTOM;
}


/**
 * Return the default attached reserve for a colorbar orientation.
 *
 * @param orientation the colorbar orientation
 * @return default reserve in logical pixels
 */
static float _colorbar_default_reserve_px(DvzColorbarOrientation orientation)
{
    return orientation == DVZ_COLORBAR_ORIENTATION_HORIZONTAL ? COLORBAR_HORIZONTAL_RESERVE_PX :
                                                               COLORBAR_VERTICAL_RESERVE_PX;
}



/**
 * Return the default attached edge for a colorbar orientation.
 *
 * @param orientation the colorbar orientation
 * @return the default panel-edge anchor
 */
static DvzSceneAnchor _colorbar_default_anchor(DvzColorbarOrientation orientation)
{
    return orientation == DVZ_COLORBAR_ORIENTATION_HORIZONTAL ? DVZ_SCENE_ANCHOR_PANEL_BOTTOM :
                                                               DVZ_SCENE_ANCHOR_PANEL_RIGHT;
}



/**
 * Return whether an edge anchor matches a colorbar orientation.
 *
 * @param orientation the colorbar orientation
 * @param anchor the panel-edge anchor
 * @return whether the anchor can host the oriented colorbar
 */
static bool _colorbar_anchor_matches_orientation(
    DvzColorbarOrientation orientation, DvzSceneAnchor anchor)
{
    if (orientation == DVZ_COLORBAR_ORIENTATION_HORIZONTAL)
    {
        return anchor == DVZ_SCENE_ANCHOR_PANEL_TOP || anchor == DVZ_SCENE_ANCHOR_PANEL_BOTTOM;
    }
    return anchor == DVZ_SCENE_ANCHOR_PANEL_LEFT || anchor == DVZ_SCENE_ANCHOR_PANEL_RIGHT;
}




/**
 * Return a positive descriptor value or its fallback.
 *
 * @param value descriptor value
 * @param fallback default value
 * @return value when positive, otherwise fallback
 */
static float _colorbar_positive_or_default(float value, float fallback)
{
    return value > 0.0f && isfinite(value) ? value : fallback;
}



/**
 * Report a colorbar realization error through logs and optional diagnostics.
 *
 * @param report optional diagnostic report
 * @param message the diagnostic message
 */
static void _colorbar_report(DvzDiagnosticReport* report, const char* message)
{
    ANN(message);
    log_error("%s", message);
    if (report != NULL)
        (void)dvz_diagnostic_report_add(report, message);
}



/**
 * Hide one invalid colorbar and report the validation failure once per dirty cycle.
 *
 * @param colorbar the colorbar
 * @param report optional diagnostic report
 * @param message the diagnostic message
 */
static void _colorbar_fail(DvzColorbar* colorbar, DvzDiagnosticReport* report, const char* message)
{
    ANN(colorbar);
    ANN(message);
    if (colorbar->dirty || report != NULL)
        _colorbar_report(report, message);
    colorbar->dirty = false;
    _colorbar_hide(colorbar);
}



/**
 * Return whether a colorbar is laid out vertically.
 *
 * @param colorbar the colorbar
 * @return whether the orientation is vertical
 */
static bool _colorbar_vertical(const DvzColorbar* colorbar)
{
    ANN(colorbar);
    return colorbar->orientation == DVZ_COLORBAR_ORIENTATION_VERTICAL;
}



/**
 * Refresh aggregate attached colorbar reserve for one panel.
 *
 * @param panel the panel
 */
void _scene_panel_refresh_colorbar_reserve(DvzPanel* panel)
{
    if (panel == NULL)
        return;
    DvzPanelReserve reserve = {0};
    for (uint32_t i = 0; i < panel->colorbar_count; i++)
    {
        DvzColorbar* colorbar = panel->colorbars[i];
        if (colorbar == NULL || colorbar->panel != panel)
            continue;
        DvzPanelReserve applied = {0};
        if (colorbar->placement_mode == DVZ_COLORBAR_PLACEMENT_ATTACHED &&
            _colorbar_anchor_supported(colorbar->anchor))
        {
            float reserve_px = _colorbar_positive_or_default(
                colorbar->reserve_px, _colorbar_default_reserve_px(colorbar->orientation));
            switch (colorbar->anchor)
            {
            case DVZ_SCENE_ANCHOR_PANEL_LEFT:
                applied.left_px = reserve_px;
                reserve.left_px += reserve_px;
                break;
            case DVZ_SCENE_ANCHOR_PANEL_RIGHT:
                applied.right_px = reserve_px;
                reserve.right_px += reserve_px;
                break;
            case DVZ_SCENE_ANCHOR_PANEL_TOP:
                applied.top_px = reserve_px;
                reserve.top_px += reserve_px;
                break;
            case DVZ_SCENE_ANCHOR_PANEL_BOTTOM:
                applied.bottom_px = reserve_px;
                reserve.bottom_px += reserve_px;
                break;
            default:
                break;
            }
        }
        colorbar->auto_reserve = applied;
    }
    _scene_panel_set_colorbar_reserve(panel, &reserve);
}



/**
 * Apply the deterministic first-slice panel reserve for a colorbar edge.
 *
 * @param colorbar the colorbar
 */
static void _colorbar_apply_auto_reserve(DvzColorbar* colorbar)
{
    ANN(colorbar);
    if (colorbar->panel == NULL)
        return;
    _scene_panel_refresh_colorbar_reserve(colorbar->panel);
}


/**
 * Convert panel-local pixels to fixed panel visual coordinates.
 *
 * @param width panel width in pixels
 * @param height panel height in pixels
 * @param x x coordinate in pixels from the panel left
 * @param y y coordinate in pixels from the panel top
 * @param z z coordinate
 * @param out output visual position
 */
static void _colorbar_pixel_to_visual(
    float width, float height, float x, float y, float z, float out[3])
{
    ANN(out);
    out[0] = width > 0.0f ? 2.0f * x / width - 1.0f : -1.0f;
    out[1] = height > 0.0f ? 1.0f - 2.0f * y / height : 1.0f;
    out[2] = z;
}



/**
 * Return a nice step size with the standard 1/2/5 ladder.
 *
 * @param range raw step size
 * @param round_to_nearest whether to round to the nearest nice value
 * @return nice step size
 */
static double _colorbar_nice_number(double range, bool round_to_nearest)
{
    if (!(range > 0.0) || !isfinite(range))
        return 1.0;
    double exponent = floor(log10(range));
    double fraction = range / pow(10.0, exponent);
    double nice_fraction = 10.0;
    if (round_to_nearest)
    {
        if (fraction < 1.5)
            nice_fraction = 1.0;
        else if (fraction < 3.0)
            nice_fraction = 2.0;
        else if (fraction < 7.0)
            nice_fraction = 5.0;
    }
    else
    {
        if (fraction <= 1.0)
            nice_fraction = 1.0;
        else if (fraction <= 2.0)
            nice_fraction = 2.0;
        else if (fraction <= 5.0)
            nice_fraction = 5.0;
    }
    return nice_fraction * pow(10.0, exponent);
}



/**
 * Fill deterministic colorbar ticks for a domain.
 *
 * @param colorbar the colorbar
 * @param min domain minimum
 * @param max domain maximum
 */
static void _colorbar_compute_ticks(DvzColorbar* colorbar, double min, double max)
{
    ANN(colorbar);
    colorbar->tick_count = 0;
    if (!isfinite(min) || !isfinite(max) || !(max > min))
        return;
    double step = _colorbar_nice_number((max - min) / 4.0, true);
    if (!(step > 0.0) || !isfinite(step))
        return;
    colorbar->ticks[colorbar->tick_count++] = min;
    double first = ceil(min / step) * step;
    for (double value = first; value < max - 0.5 * COLORBAR_EPS; value += step)
    {
        if (value <= min + 0.5 * COLORBAR_EPS)
            continue;
        if (colorbar->tick_count + 1 >= DVZ_SCENE_MAX_COLORBAR_TICKS)
            break;
        colorbar->ticks[colorbar->tick_count++] = value;
    }
    if (colorbar->tick_count < DVZ_SCENE_MAX_COLORBAR_TICKS)
        colorbar->ticks[colorbar->tick_count++] = max;
}



/**
 * Format one colorbar tick label.
 *
 * @param colorbar the colorbar
 * @param value tick value
 * @param out output label buffer
 * @param out_size output label buffer size
 */
static void _colorbar_format_tick(
    const DvzColorbar* colorbar, double value, char* out, uint32_t out_size)
{
    ANN(colorbar);
    ANN(colorbar->scale);
    ANN(out);
    if (out_size == 0)
        return;
    const DvzSceneFormatState* format =
        colorbar->has_format ? &colorbar->format : &colorbar->scale->format;
    int32_t precision = format->precision;
    if (precision < 0)
        precision = 0;
    if (precision > 12)
        precision = 12;

    char value_str[64] = {0};
    if (format->scientific)
        dvz_snprintf(value_str, sizeof(value_str), "%.*e", precision, value);
    else
        dvz_snprintf(value_str, sizeof(value_str), "%.*f", precision, value);
    if (format->trim_trailing_zeros && !format->scientific)
    {
        char* dot = strchr(value_str, '.');
        if (dot != NULL)
        {
            char* end = value_str + strlen(value_str);
            while (end > dot + 1 && end[-1] == '0')
                *(--end) = '\0';
            if (end > dot && end[-1] == '.')
                *(--end) = '\0';
        }
    }

    char unit[32] = {0};
    if (format->unit[0] != '\0')
        dvz_strlcpy(unit, format->unit, sizeof(unit));
    else if (colorbar->scale->unit[0] != '\0')
        dvz_strlcpy(unit, colorbar->scale->unit, sizeof(unit));
    if (format->show_unit && unit[0] != '\0')
        dvz_snprintf(
            out, out_size, "%s%s %s%s", format->prefix, value_str, unit, format->suffix);
    else
        dvz_snprintf(out, out_size, "%s%s%s", format->prefix, value_str, format->suffix);
}


/**
 * Hide all derived visuals owned by a colorbar.
 *
 * @param colorbar the colorbar
 */
static void _colorbar_hide(DvzColorbar* colorbar)
{
    if (colorbar == NULL)
        return;
    if (colorbar->ramp_visual != NULL)
        dvz_visual_set_visible(colorbar->ramp_visual, false);
    if (colorbar->tick_visual != NULL)
        dvz_visual_set_visible(colorbar->tick_visual, false);
    if (colorbar->text_visual != NULL)
    {
        dvz_visual_set_visible(colorbar->text_visual, false);
        if (_visual_family_state(colorbar->text_visual)->text.glyph_visual != NULL)
            dvz_visual_set_visible(_visual_family_state(colorbar->text_visual)->text.glyph_visual, false);
    }
}



/**
 * Ensure one colorbar-derived visual is attached to the panel.
 *
 * @param colorbar the colorbar
 * @param visual the visual
 * @param z_layer z layer for panel sorting
 * @return whether the visual is attached
 */
static bool _colorbar_attach_visual(DvzColorbar* colorbar, DvzVisual* visual, int32_t z_layer)
{
    ANN(colorbar);
    ANN(colorbar->panel);
    ANN(visual);
    DvzVisualAttachDesc attach = {.z_layer = z_layer, .controller_mode = DVZ_CONTROLLER_FIXED};
    for (uint32_t i = 0; i < colorbar->panel->visual_count; i++)
    {
        DvzPanelAttach* existing = &colorbar->panel->visuals[i];
        if (existing->visual != visual)
            continue;
        existing->z_layer = attach.z_layer;
        existing->controller_mode = attach.controller_mode;
        return true;
    }
    return dvz_panel_add_visual(colorbar->panel, visual, &attach) == 0;
}



/**
 * Ensure derived ramp, tick, and text visuals exist for a colorbar.
 *
 * @param colorbar the colorbar
 * @return whether all derived visuals exist
 */
static bool _colorbar_ensure_visuals(DvzColorbar* colorbar)
{
    ANN(colorbar);
    if (colorbar->scene == NULL || colorbar->panel == NULL)
        return false;
    if (colorbar->ramp_visual == NULL)
    {
        colorbar->ramp_visual =
            dvz_primitive(colorbar->scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
        if (colorbar->ramp_visual == NULL)
            return false;
        colorbar->ramp_visual->visible = false;
    }
    if (!_colorbar_attach_visual(colorbar, colorbar->ramp_visual, 1000))
        return false;

    if (colorbar->tick_visual == NULL)
    {
        colorbar->tick_visual = dvz_segment(colorbar->scene, 0);
        if (colorbar->tick_visual == NULL)
            return false;
        colorbar->tick_visual->visible = false;
    }
    if (!_colorbar_attach_visual(colorbar, colorbar->tick_visual, 1001))
        return false;

    if (colorbar->text_visual == NULL)
    {
        colorbar->text_visual =
            _scene_adornment_text_visual(colorbar->scene, colorbar->text_renderer);
        if (colorbar->text_visual == NULL)
            return false;
        colorbar->text_visual->visible = false;
    }
    return _colorbar_attach_visual(colorbar, colorbar->text_visual, 1002);
}



/**
 * Append one text item to colorbar text arrays.
 *
 * @param colorbar the colorbar
 * @param label text label
 * @param x text position x in panel-local pixels
 * @param y text position y in panel-local pixels
 * @param anchor_x text anchor x
 * @param anchor_y text anchor y
 * @param size text size in pixels
 * @param angle text angle in radians
 */
static void _colorbar_append_text(
    DvzColorbar* colorbar, const char* label, float x, float y, float anchor_x, float anchor_y,
    float size, float angle)
{
    ANN(colorbar);
    ANN(label);
    if (colorbar->text_count >= DVZ_SCENE_MAX_COLORBAR_TEXTS)
        return;
    uint32_t i = colorbar->text_count++;
    dvz_strlcpy(colorbar->text_labels[i], label, sizeof(colorbar->text_labels[i]));
    colorbar->text_positions[i][0] = x;
    colorbar->text_positions[i][1] = y;
    colorbar->text_positions[i][2] = 0.0f;
    colorbar->text_anchors[i][0] = anchor_x;
    colorbar->text_anchors[i][1] = anchor_y;
    colorbar->text_sizes[i] = size;
    colorbar->text_colors[i][0] = 255;
    colorbar->text_colors[i][1] = 255;
    colorbar->text_colors[i][2] = 255;
    colorbar->text_colors[i][3] = 255;
    colorbar->text_angles[i] = angle;
}



/**
 * Update the colorbar batched text visual.
 *
 * @param colorbar the colorbar
 */
static void _colorbar_update_text(DvzColorbar* colorbar)
{
    ANN(colorbar);
    if (colorbar->text_visual == NULL || colorbar->text_count == 0)
    {
        if (colorbar->text_visual != NULL)
            dvz_visual_set_visible(colorbar->text_visual, false);
        return;
    }
    if (_scene_adornment_text_visual_set_renderer(
            colorbar->text_visual, colorbar->text_renderer) != 0)
    {
        dvz_visual_set_visible(colorbar->text_visual, false);
        return;
    }
    const char* strings[DVZ_SCENE_MAX_COLORBAR_TEXTS] = {0};
    for (uint32_t i = 0; i < colorbar->text_count; i++)
        strings[i] = colorbar->text_labels[i];
    DvzVisualDataUpdate updates[5] = {
        {.attr_name = "position", .data = colorbar->text_positions,
         .item_count = colorbar->text_count},
        {.attr_name = "anchor", .data = colorbar->text_anchors,
         .item_count = colorbar->text_count},
        {.attr_name = "size", .data = colorbar->text_sizes, .item_count = colorbar->text_count},
        {.attr_name = "color", .data = colorbar->text_colors, .item_count = colorbar->text_count},
        {.attr_name = "angle", .data = colorbar->text_angles, .item_count = colorbar->text_count},
    };
    if (dvz_visual_set_strings(colorbar->text_visual, "text", strings, colorbar->text_count) == 0 &&
        dvz_visual_set_data_many(colorbar->text_visual, updates, 5) == 0)
    {
        dvz_visual_set_visible(colorbar->text_visual, true);
    }
    else
    {
        dvz_visual_set_visible(colorbar->text_visual, false);
    }
}


/**
 * Return the normalized position of one value in the scale domain.
 *
 * @param value data value
 * @param min domain minimum
 * @param max domain maximum
 * @return normalized position
 */
static double _colorbar_value_t(double value, double min, double max)
{
    double denom = max - min;
    if (fabs(denom) < COLORBAR_EPS)
        return 0.0;
    double t = (value - min) / denom;
    if (t < 0.0)
        t = 0.0;
    if (t > 1.0)
        t = 1.0;
    return t;
}



/**
 * Update the fixed primitive ramp visual.
 *
 * @param colorbar the colorbar
 * @param width panel width in pixels
 * @param height panel height in pixels
 * @param ramp_x0 ramp left in pixels
 * @param ramp_y0 ramp top in pixels
 * @param ramp_x1 ramp right in pixels
 * @param ramp_y1 ramp bottom in pixels
 */
static void _colorbar_update_ramp(
    DvzColorbar* colorbar, float width, float height, float ramp_x0, float ramp_y0, float ramp_x1,
    float ramp_y1)
{
    ANN(colorbar);
    ANN(colorbar->ramp_visual);
    const uint32_t vertex_count = 6 * COLORBAR_RAMP_SEGMENTS;
    float positions[6 * COLORBAR_RAMP_SEGMENTS][3] = {{0}};
    DvzColor colors[6 * COLORBAR_RAMP_SEGMENTS] = {{0}};
    bool vertical = _colorbar_vertical(colorbar);
    for (uint32_t i = 0; i < COLORBAR_RAMP_SEGMENTS; i++)
    {
        double t0 = (double)i / (double)COLORBAR_RAMP_SEGMENTS;
        double t1 = (double)(i + 1u) / (double)COLORBAR_RAMP_SEGMENTS;
        float x0 = vertical ? ramp_x0 : ramp_x0 + (ramp_x1 - ramp_x0) * (float)t0;
        float x1 = vertical ? ramp_x1 : ramp_x0 + (ramp_x1 - ramp_x0) * (float)t1;
        float y0 = vertical ? ramp_y1 + (ramp_y0 - ramp_y1) * (float)t0 : ramp_y0;
        float y1 = vertical ? ramp_y1 + (ramp_y0 - ramp_y1) * (float)t1 : ramp_y1;
        uint8_t rgba0[4] = {0};
        uint8_t rgba1[4] = {0};
        (void)_scene_color_from_colormap(colorbar->scale->colormap, t0, rgba0);
        (void)_scene_color_from_colormap(colorbar->scale->colormap, t1, rgba1);
        const DvzColor color0 = dvz_color_rgba(rgba0[0], rgba0[1], rgba0[2], rgba0[3]);
        const DvzColor color1 = dvz_color_rgba(rgba1[0], rgba1[1], rgba1[2], rgba1[3]);
        uint32_t k = 6u * i;
        _colorbar_pixel_to_visual(width, height, x0, y0, 0.0f, positions[k + 0]);
        _colorbar_pixel_to_visual(width, height, x1, y0, 0.0f, positions[k + 1]);
        _colorbar_pixel_to_visual(width, height, x1, y1, 0.0f, positions[k + 2]);
        _colorbar_pixel_to_visual(width, height, x0, y0, 0.0f, positions[k + 3]);
        _colorbar_pixel_to_visual(width, height, x0, y1, 0.0f, positions[k + 4]);
        _colorbar_pixel_to_visual(width, height, x1, y1, 0.0f, positions[k + 5]);
        if (vertical)
        {
            colors[k + 0] = color0;
            colors[k + 1] = color0;
            colors[k + 2] = color1;
            colors[k + 3] = color0;
            colors[k + 4] = color1;
            colors[k + 5] = color1;
        }
        else
        {
            colors[k + 0] = color0;
            colors[k + 1] = color1;
            colors[k + 2] = color1;
            colors[k + 3] = color0;
            colors[k + 4] = color0;
            colors[k + 5] = color1;
        }
    }
    DvzVisualDataUpdate updates[2] = {
        {.attr_name = "position", .data = positions, .item_count = vertex_count},
        {.attr_name = "color", .data = colors, .item_count = vertex_count},
    };
    if (dvz_visual_set_data_many(colorbar->ramp_visual, updates, 2) == 0)
        dvz_visual_set_visible(colorbar->ramp_visual, true);
    else
        dvz_visual_set_visible(colorbar->ramp_visual, false);
}



/**
 * Update fixed tick segments and text labels.
 *
 * @param colorbar the colorbar
 * @param width panel width in pixels
 * @param height panel height in pixels
 * @param ramp_x0 ramp left in pixels
 * @param ramp_y0 ramp top in pixels
 * @param ramp_x1 ramp right in pixels
 * @param ramp_y1 ramp bottom in pixels
 * @param min scale minimum
 * @param max scale maximum
 */
static void _colorbar_update_ticks_and_text(
    DvzColorbar* colorbar, float width, float height, float ramp_x0, float ramp_y0, float ramp_x1,
    float ramp_y1, double min, double max)
{
    ANN(colorbar);
    ANN(colorbar->tick_visual);
    colorbar->text_count = 0;
    float starts[DVZ_SCENE_MAX_COLORBAR_TICKS][3] = {{0}};
    float ends[DVZ_SCENE_MAX_COLORBAR_TICKS][3] = {{0}};
    DvzColor colors[DVZ_SCENE_MAX_COLORBAR_TICKS] = {{0}};
    float widths[DVZ_SCENE_MAX_COLORBAR_TICKS] = {0};
    uint32_t count = 0;
    bool vertical = _colorbar_vertical(colorbar);
    for (uint32_t i = 0; i < colorbar->tick_count && i < DVZ_SCENE_MAX_COLORBAR_TICKS; i++)
    {
        double t = _colorbar_value_t(colorbar->ticks[i], min, max);
        float x0 = 0.0f;
        float y0 = 0.0f;
        float x1 = 0.0f;
        float y1 = 0.0f;
        float label_x = 0.0f;
        float label_y = 0.0f;
        float anchor_x = 0.5f;
        float anchor_y = 0.5f;
        if (vertical)
        {
            float y = ramp_y1 + (ramp_y0 - ramp_y1) * (float)t;
            if (colorbar->anchor == DVZ_SCENE_ANCHOR_PANEL_LEFT)
            {
                x0 = ramp_x0 - colorbar->tick_length_px;
                x1 = ramp_x0;
                label_x = x0 - colorbar->label_gap_px;
                anchor_x = 1.0f;
            }
            else
            {
                x0 = ramp_x1;
                x1 = ramp_x1 + colorbar->tick_length_px;
                label_x = x1 + colorbar->label_gap_px;
                anchor_x = 0.0f;
            }
            y0 = y1 = label_y = y;
        }
        else
        {
            float x = ramp_x0 + (ramp_x1 - ramp_x0) * (float)t;
            if (colorbar->anchor == DVZ_SCENE_ANCHOR_PANEL_TOP)
            {
                y0 = ramp_y0 - colorbar->tick_length_px;
                y1 = ramp_y0;
                label_y = y0 - colorbar->label_gap_px;
                anchor_y = 1.0f;
            }
            else
            {
                y0 = ramp_y1;
                y1 = ramp_y1 + colorbar->tick_length_px;
                label_y = y1 + colorbar->label_gap_px;
                anchor_y = 0.0f;
            }
            x0 = x1 = label_x = x;
        }
        _colorbar_pixel_to_visual(width, height, x0, y0, 0.0f, starts[count]);
        _colorbar_pixel_to_visual(width, height, x1, y1, 0.0f, ends[count]);
        colors[count] = dvz_color_rgb(255, 255, 255);
        widths[count] = COLORBAR_TICK_WIDTH_PX;

        char label[DVZ_SCENE_LABEL_SIZE] = {0};
        _colorbar_format_tick(colorbar, colorbar->ticks[i], label, sizeof(label));
        _colorbar_append_text(
            colorbar, label, label_x, label_y, anchor_x, anchor_y, COLORBAR_TICK_TEXT_SIZE_PX,
            0.0f);
        count++;
    }
    if (count > 0)
    {
        DvzVisualDataUpdate updates[4] = {
            {.attr_name = "position_start", .data = starts, .item_count = count},
            {.attr_name = "position_end", .data = ends, .item_count = count},
            {.attr_name = "color", .data = colors, .item_count = count},
            {.attr_name = "stroke_width", .data = widths, .item_count = count},
        };
        if (dvz_visual_set_data_many(colorbar->tick_visual, updates, 4) == 0)
            dvz_visual_set_visible(colorbar->tick_visual, true);
        else
            dvz_visual_set_visible(colorbar->tick_visual, false);
    }
    else
    {
        dvz_visual_set_visible(colorbar->tick_visual, false);
    }
}



/**
 * Append the colorbar title to the text batch.
 *
 * @param colorbar the colorbar
 * @param width panel width in pixels
 * @param height panel height in pixels
 * @param ramp_x0 ramp left in pixels
 * @param ramp_y0 ramp top in pixels
 * @param ramp_x1 ramp right in pixels
 * @param ramp_y1 ramp bottom in pixels
 */
static void _colorbar_update_title(
    DvzColorbar* colorbar, float width, float height, float ramp_x0, float ramp_y0, float ramp_x1,
    float ramp_y1)
{
    ANN(colorbar);
    if (colorbar->title[0] == '\0')
        return;
    bool vertical = _colorbar_vertical(colorbar);
    if (vertical)
    {
        float x = colorbar->anchor == DVZ_SCENE_ANCHOR_PANEL_LEFT ?
                      fmaxf(colorbar->edge_offset_px, ramp_x0 - COLORBAR_TITLE_GAP_PX - 4.0f) :
                      fminf(
                          width - colorbar->edge_offset_px,
                          ramp_x1 + colorbar->tick_length_px + colorbar->label_gap_px + 46.0f);
        float y = 0.5f * (ramp_y0 + ramp_y1);
        float angle = colorbar->anchor == DVZ_SCENE_ANCHOR_PANEL_LEFT ? -1.57079632679f :
                                                                      +1.57079632679f;
        _colorbar_append_text(
            colorbar, colorbar->title, x, y, 0.5f, 0.5f, COLORBAR_TITLE_TEXT_SIZE_PX, angle);
    }
    else
    {
        float x = 0.5f * (ramp_x0 + ramp_x1);
        float y = colorbar->anchor == DVZ_SCENE_ANCHOR_PANEL_TOP ?
                      colorbar->edge_offset_px :
                      height - colorbar->edge_offset_px;
        float anchor_y = colorbar->anchor == DVZ_SCENE_ANCHOR_PANEL_TOP ? 0.0f : 1.0f;
        _colorbar_append_text(
            colorbar, colorbar->title, x, y, 0.5f, anchor_y, COLORBAR_TITLE_TEXT_SIZE_PX, 0.0f);
    }
}


/**
 * Resolve an anchored detached placement rectangle to panel-local pixels.
 *
 * @param colorbar the colorbar
 * @param panel_x panel x origin in figure pixels
 * @param panel_y panel y origin in figure pixels
 * @param panel_width panel width in pixels
 * @param panel_height panel height in pixels
 * @param out output rectangle as x0, y0, x1, y1 in panel-local pixels
 */
static void _colorbar_detached_rect(
    const DvzColorbar* colorbar, float panel_x, float panel_y, float panel_width,
    float panel_height, float out[4])
{
    ANN(colorbar);
    ANN(out);
    const DvzPlacement* placement = &colorbar->placement;
    float space_x = 0.0f;
    float space_y = 0.0f;
    float space_width = panel_width;
    float space_height = panel_height;
    if (placement->space == DVZ_PLACEMENT_SPACE_FIGURE && colorbar->panel != NULL &&
        colorbar->panel->figure != NULL)
    {
        space_x = -panel_x;
        space_y = -panel_y;
        space_width = colorbar->panel->figure->width > 0 ? (float)colorbar->panel->figure->width :
                                                           panel_width;
        space_height = colorbar->panel->figure->height > 0 ?
                           (float)colorbar->panel->figure->height :
                           panel_height;
    }

    float width = _colorbar_positive_or_default(
        placement->width_px, _colorbar_vertical(colorbar) ? colorbar->ramp_width_px :
                                                            fmaxf(1.0f, panel_width));
    float height = _colorbar_positive_or_default(
        placement->height_px, _colorbar_vertical(colorbar) ? fmaxf(1.0f, panel_height) :
                                                             colorbar->ramp_width_px);
    float x = space_x + placement->offset_x_px;
    if (placement->horizontal_anchor == DVZ_HORIZONTAL_ANCHOR_CENTER)
        x = space_x + 0.5f * (space_width - width) + placement->offset_x_px;
    else if (placement->horizontal_anchor == DVZ_HORIZONTAL_ANCHOR_RIGHT)
        x = space_x + space_width - width + placement->offset_x_px;

    float y = space_y + placement->offset_y_px;
    if (placement->vertical_anchor == DVZ_VERTICAL_ANCHOR_CENTER)
        y = space_y + 0.5f * (space_height - height) + placement->offset_y_px;
    else if (placement->vertical_anchor == DVZ_VERTICAL_ANCHOR_BOTTOM)
        y = space_y + space_height - height + placement->offset_y_px;

    out[0] = x;
    out[1] = y;
    out[2] = x + width;
    out[3] = y + height;
}



/**
 * Resolve the color ramp rectangle to panel-local pixels.
 *
 * @param colorbar the colorbar
 * @param panel_x panel x origin in figure pixels
 * @param panel_y panel y origin in figure pixels
 * @param panel_width panel width in pixels
 * @param panel_height panel height in pixels
 * @param out output ramp rectangle as x0, y0, x1, y1 in panel-local pixels
 */
static void _colorbar_ramp_rect(
    const DvzColorbar* colorbar, float panel_x, float panel_y, float panel_width,
    float panel_height, float out[4])
{
    ANN(colorbar);
    ANN(out);
    if (colorbar->placement_mode == DVZ_COLORBAR_PLACEMENT_DETACHED)
    {
        float rect[4] = {0};
        _colorbar_detached_rect(colorbar, panel_x, panel_y, panel_width, panel_height, rect);
        if (_colorbar_vertical(colorbar))
        {
            out[0] = rect[0];
            out[1] = rect[1];
            out[2] = fminf(rect[2], rect[0] + colorbar->ramp_width_px);
            out[3] = rect[3];
        }
        else
        {
            out[0] = rect[0];
            out[1] = rect[1];
            out[2] = rect[2];
            out[3] = fminf(rect[3], rect[1] + colorbar->ramp_width_px);
        }
        return;
    }

    float plot_x = 0.0f;
    float plot_y = 0.0f;
    float plot_width = 0.0f;
    float plot_height = 0.0f;
    _scene_panel_plot_pixel_rect(
        colorbar->panel, &plot_x, &plot_y, &plot_width, &plot_height);
    plot_x -= panel_x;
    plot_y -= panel_y;

    if (_colorbar_vertical(colorbar))
    {
        if (colorbar->anchor == DVZ_SCENE_ANCHOR_PANEL_LEFT)
        {
            out[2] = plot_x - colorbar->plot_gap_px;
            out[0] = out[2] - colorbar->ramp_width_px;
        }
        else
        {
            out[0] = plot_x + plot_width + colorbar->plot_gap_px;
            out[2] = out[0] + colorbar->ramp_width_px;
        }
        out[1] = plot_y + colorbar->edge_offset_px;
        out[3] = plot_y + plot_height - colorbar->edge_offset_px;
    }
    else
    {
        out[0] = plot_x + colorbar->edge_offset_px;
        out[2] = plot_x + plot_width - colorbar->edge_offset_px;
        if (colorbar->anchor == DVZ_SCENE_ANCHOR_PANEL_TOP)
        {
            out[3] = plot_y - colorbar->plot_gap_px;
            out[1] = out[3] - colorbar->ramp_width_px;
        }
        else
        {
            out[1] = plot_y + plot_height + colorbar->plot_gap_px;
            out[3] = out[1] + colorbar->ramp_width_px;
        }
    }
}


/**
 * Rebuild the derived visuals for one retained colorbar.
 *
 * @param colorbar the colorbar
 * @param report optional diagnostic report
 */
static void _colorbar_update_visuals(DvzColorbar* colorbar, DvzDiagnosticReport* report)
{
    ANN(colorbar);
    if (colorbar->scene == NULL || colorbar->panel == NULL || colorbar->scale == NULL)
        return;
    if (colorbar->placement_mode == DVZ_COLORBAR_PLACEMENT_ATTACHED &&
        !_colorbar_anchor_supported(colorbar->anchor))
    {
        _colorbar_fail(
            colorbar, report,
            "attached colorbar anchor must be a panel edge");
        return;
    }
    if (colorbar->placement_mode == DVZ_COLORBAR_PLACEMENT_ATTACHED &&
        !_colorbar_anchor_matches_orientation(colorbar->orientation, colorbar->anchor))
    {
        _colorbar_fail(colorbar, report, "attached colorbar anchor must match its orientation");
        return;
    }
    if (colorbar->scale->kind != DVZ_SCALE_CONTINUOUS)
    {
        _colorbar_fail(
            colorbar, report, "categorical colorbar rendering is unsupported; use a legend");
        return;
    }
    double min = colorbar->scale->has_view_range ? colorbar->scale->view_min :
                                                    colorbar->scale->domain_min;
    double max = colorbar->scale->has_view_range ? colorbar->scale->view_max :
                                                    colorbar->scale->domain_max;
    if (!colorbar->scale->has_domain || !isfinite(min) || !isfinite(max) || !(max > min))
    {
        _colorbar_fail(
            colorbar, report, "colorbar scale domain must be finite and increasing");
        return;
    }
    float panel_x = 0.0f;
    float panel_y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    _scene_panel_pixel_rect(colorbar->panel, &panel_x, &panel_y, &width, &height);
    (void)panel_x;
    (void)panel_y;
    if (!(width > 0.0f) || !(height > 0.0f) || !isfinite(width) || !isfinite(height))
    {
        colorbar->realized_panel_width = 0.0f;
        colorbar->realized_panel_height = 0.0f;
        _colorbar_hide(colorbar);
        return;
    }
    colorbar->realized_panel_width = width;
    colorbar->realized_panel_height = height;
    if (!_colorbar_ensure_visuals(colorbar))
    {
        _colorbar_hide(colorbar);
        return;
    }

    float ramp[4] = {0};
    _colorbar_ramp_rect(colorbar, panel_x, panel_y, width, height, ramp);
    float ramp_x0 = ramp[0];
    float ramp_y0 = ramp[1];
    float ramp_x1 = ramp[2];
    float ramp_y1 = ramp[3];
    if (ramp_x0 < 0.0f || ramp_y0 < 0.0f || ramp_x1 > width || ramp_y1 > height ||
        ramp_x1 <= ramp_x0 || ramp_y1 <= ramp_y0)
    {
        _colorbar_fail(colorbar, report, "panel is too small for deterministic colorbar layout");
        return;
    }

    _colorbar_compute_ticks(colorbar, min, max);
    _colorbar_update_ramp(colorbar, width, height, ramp_x0, ramp_y0, ramp_x1, ramp_y1);
    _colorbar_update_ticks_and_text(
        colorbar, width, height, ramp_x0, ramp_y0, ramp_x1, ramp_y1, min, max);
    _colorbar_update_title(colorbar, width, height, ramp_x0, ramp_y0, ramp_x1, ramp_y1);
    _colorbar_update_text(colorbar);
    colorbar->dirty = false;
}



/**
 * Return whether one retained colorbar needs its derived visuals rebuilt.
 *
 * @param colorbar the colorbar
 * @return whether the colorbar visual payloads need rebuilding
 */
static bool _colorbar_needs_visual_update(const DvzColorbar* colorbar)
{
    ANN(colorbar);
    if (colorbar->dirty || colorbar->ramp_visual == NULL || colorbar->tick_visual == NULL ||
        colorbar->text_visual == NULL)
    {
        return true;
    }
    if (colorbar->panel == NULL)
        return false;

    float panel_x = 0.0f;
    float panel_y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    _scene_panel_pixel_rect(colorbar->panel, &panel_x, &panel_y, &width, &height);
    (void)panel_x;
    (void)panel_y;
    if (!(width > 0.0f) || !(height > 0.0f) || !isfinite(width) || !isfinite(height))
        return true;
    return fabsf(width - colorbar->realized_panel_width) > COLORBAR_LAYOUT_EPS ||
           fabsf(height - colorbar->realized_panel_height) > COLORBAR_LAYOUT_EPS;
}





/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a panel-attached colorbar bound to a scale.
 *
 * @param panel the panel
 * @param scale the scale
 * @param desc the colorbar descriptor, or NULL for defaults
 * @return the colorbar, or NULL on allocation failure
 */
DvzColorbar* dvz_colorbar(DvzPanel* panel, DvzScale* scale, const DvzColorbarDesc* desc)
{
    ANN(panel);
    ANN(scale);
    if (panel->figure == NULL || panel->figure->scene == NULL)
    {
        log_error("cannot create a colorbar on a detached panel");
        return NULL;
    }
    DvzScene* scene = panel->figure->scene;
    if (scale->scene != scene)
    {
        log_error("cannot attach a scale from a different scene to a panel colorbar");
        return NULL;
    }
    DvzColorbarPlacementMode placement_mode =
        desc != NULL ? desc->placement_mode : DVZ_COLORBAR_PLACEMENT_ATTACHED;
    DvzColorbarOrientation orientation =
        desc != NULL ? desc->orientation : DVZ_COLORBAR_ORIENTATION_VERTICAL;
    DvzSceneAnchor anchor = desc != NULL && desc->anchor != DVZ_SCENE_ANCHOR_NONE ?
                                desc->anchor :
                                _colorbar_default_anchor(orientation);
    if (placement_mode == DVZ_COLORBAR_PLACEMENT_ATTACHED && !_colorbar_anchor_supported(anchor))
    {
        log_error("attached colorbar anchor must be a panel edge");
        return NULL;
    }
    if (placement_mode == DVZ_COLORBAR_PLACEMENT_ATTACHED &&
        !_colorbar_anchor_matches_orientation(orientation, anchor))
    {
        log_error("attached colorbar anchor must match its orientation");
        return NULL;
    }
    if (scale->kind != DVZ_SCALE_CONTINUOUS)
    {
        log_error("categorical colorbar rendering is unsupported; use a legend");
        return NULL;
    }
    if (scene->colorbar_count >= DVZ_SCENE_MAX_COLORBARS)
    {
        log_error("maximum colorbar count reached");
        return NULL;
    }
    if (panel->colorbar_count >= DVZ_SCENE_MAX_PANEL_COLORBARS)
    {
        log_error("maximum panel colorbar count reached");
        return NULL;
    }
    DvzColorbar* colorbar = &scene->colorbars[scene->colorbar_count++];
    dvz_memset(colorbar, sizeof(DvzColorbar), 0, sizeof(DvzColorbar));
    colorbar->scene = scene;
    colorbar->panel = panel;
    colorbar->scale = scale;
    colorbar->placement_mode = placement_mode;
    colorbar->orientation = orientation;
    colorbar->anchor = anchor;
    colorbar->flags = desc != NULL ? desc->flags : 0;
    colorbar->reserve_px = _colorbar_positive_or_default(
        desc != NULL ? desc->reserve_px : 0.0f, _colorbar_default_reserve_px(orientation));
    colorbar->ramp_width_px = _colorbar_positive_or_default(
        desc != NULL ? desc->ramp_width_px : 0.0f, COLORBAR_RAMP_THICKNESS_PX);
    colorbar->edge_offset_px = _colorbar_positive_or_default(
        desc != NULL ? desc->edge_offset_px : 0.0f, COLORBAR_EDGE_OFFSET_PX);
    colorbar->plot_gap_px = _colorbar_positive_or_default(
        desc != NULL ? desc->plot_gap_px : 0.0f, COLORBAR_PLOT_GAP_PX);
    colorbar->tick_length_px = _colorbar_positive_or_default(
        desc != NULL ? desc->tick_length_px : 0.0f, COLORBAR_TICK_LENGTH_PX);
    colorbar->label_gap_px = _colorbar_positive_or_default(
        desc != NULL ? desc->label_gap_px : 0.0f, COLORBAR_LABEL_GAP_PX);
    colorbar->text_renderer = _scene_adornment_text_renderer(
        desc != NULL && desc->text_renderer != 0 ? desc->text_renderer :
                                                   DVZ_TEXT_RENDERER_MSDF_ATLAS);
    colorbar->placement =
        desc != NULL ? desc->placement :
                       (DvzPlacement){
                           .space = DVZ_PLACEMENT_SPACE_PANEL,
                           .horizontal_anchor = DVZ_HORIZONTAL_ANCHOR_LEFT,
                           .vertical_anchor = DVZ_VERTICAL_ANCHOR_TOP,
                       };
    if (desc != NULL && desc->title != NULL)
        dvz_strlcpy(colorbar->title, desc->title, sizeof(colorbar->title));
    colorbar->dirty = true;
    colorbar->version = 1;
    panel->colorbars[panel->colorbar_count++] = colorbar;
    _colorbar_apply_auto_reserve(colorbar);
    return colorbar;
}



/**
 * Destroy a colorbar.
 *
 * @param colorbar the colorbar
 */
void dvz_colorbar_destroy(DvzColorbar* colorbar)
{
    if (colorbar == NULL)
        return;
    _colorbar_hide(colorbar);
    DvzPanel* panel = colorbar->panel;
    if (colorbar->panel != NULL)
    {
        for (uint32_t i = 0; i < panel->colorbar_count; i++)
        {
            if (panel->colorbars[i] != colorbar)
                continue;
            for (uint32_t j = i + 1; j < panel->colorbar_count; j++)
                panel->colorbars[j - 1] = panel->colorbars[j];
            panel->colorbars[panel->colorbar_count - 1] = NULL;
            panel->colorbar_count--;
            break;
        }
        _scene_panel_refresh_colorbar_reserve(panel);
    }
    colorbar->scene = NULL;
    colorbar->panel = NULL;
    colorbar->scale = NULL;
    colorbar->has_format = false;
    colorbar->dirty = false;
    colorbar->ramp_visual = NULL;
    colorbar->tick_visual = NULL;
    colorbar->text_visual = NULL;
}



/**
 * Override formatting policy on a colorbar.
 *
 * @param colorbar the colorbar
 * @param format the format descriptor, or NULL to clear the override
 */
void dvz_colorbar_set_format(DvzColorbar* colorbar, const DvzFormatDesc* format)
{
    ANN(colorbar);
    colorbar->has_format = format != NULL;
    _scene_format_state_copy(&colorbar->format, format);
    _scene_mark_colorbar_dirty(colorbar);
}



/**
 * Set the colorbar orientation.
 *
 * @param colorbar the colorbar
 * @param orientation the orientation
 */
void dvz_colorbar_set_orientation(DvzColorbar* colorbar, DvzColorbarOrientation orientation)
{
    ANN(colorbar);
    if (colorbar->orientation == orientation)
        return;
    colorbar->orientation = orientation;
    colorbar->reserve_px = _colorbar_default_reserve_px(orientation);
    if (!_colorbar_anchor_matches_orientation(orientation, colorbar->anchor))
        colorbar->anchor = _colorbar_default_anchor(orientation);
    _colorbar_apply_auto_reserve(colorbar);
    _scene_mark_colorbar_dirty(colorbar);
}



/**
 * Set the panel-edge colorbar anchor.
 *
 * @param colorbar the colorbar
 * @param anchor the panel-edge anchor
 * @return true when the anchor was accepted
 */
bool dvz_colorbar_set_anchor(DvzColorbar* colorbar, DvzSceneAnchor anchor)
{
    ANN(colorbar);
    if (!_colorbar_anchor_supported(anchor))
    {
        log_error("attached colorbar anchor must be a panel edge");
        return false;
    }
    bool changed = colorbar->anchor != anchor;
    if (!_colorbar_anchor_matches_orientation(colorbar->orientation, anchor))
    {
        colorbar->orientation =
            anchor == DVZ_SCENE_ANCHOR_PANEL_TOP || anchor == DVZ_SCENE_ANCHOR_PANEL_BOTTOM ?
                DVZ_COLORBAR_ORIENTATION_HORIZONTAL :
                DVZ_COLORBAR_ORIENTATION_VERTICAL;
        colorbar->reserve_px = _colorbar_default_reserve_px(colorbar->orientation);
        changed = true;
    }
    if (!changed)
        return true;
    colorbar->anchor = anchor;
    _colorbar_apply_auto_reserve(colorbar);
    _scene_mark_colorbar_dirty(colorbar);
    return true;
}


/**
 * Update colorbar layout and placement parameters.
 *
 * @param colorbar the colorbar
 * @param desc layout descriptor
 * @return true when the layout was accepted
 */
bool dvz_colorbar_set_layout(DvzColorbar* colorbar, const DvzColorbarDesc* desc)
{
    ANN(colorbar);
    if (desc == NULL)
        return false;
    DvzColorbarPlacementMode placement_mode = desc->placement_mode;
    DvzColorbarOrientation orientation = desc->orientation;
    DvzSceneAnchor anchor =
        desc->anchor != DVZ_SCENE_ANCHOR_NONE ? desc->anchor : _colorbar_default_anchor(orientation);
    if (placement_mode == DVZ_COLORBAR_PLACEMENT_ATTACHED && !_colorbar_anchor_supported(anchor))
    {
        log_error("attached colorbar anchor must be a panel edge");
        return false;
    }
    if (placement_mode == DVZ_COLORBAR_PLACEMENT_ATTACHED &&
        !_colorbar_anchor_matches_orientation(orientation, anchor))
    {
        log_error("attached colorbar anchor must match its orientation");
        return false;
    }

    colorbar->placement_mode = placement_mode;
    colorbar->orientation = orientation;
    colorbar->anchor = anchor;
    colorbar->flags = desc->flags;
    colorbar->reserve_px =
        _colorbar_positive_or_default(desc->reserve_px, _colorbar_default_reserve_px(orientation));
    colorbar->ramp_width_px =
        _colorbar_positive_or_default(desc->ramp_width_px, COLORBAR_RAMP_THICKNESS_PX);
    colorbar->edge_offset_px =
        _colorbar_positive_or_default(desc->edge_offset_px, COLORBAR_EDGE_OFFSET_PX);
    colorbar->plot_gap_px =
        _colorbar_positive_or_default(desc->plot_gap_px, COLORBAR_PLOT_GAP_PX);
    colorbar->tick_length_px =
        _colorbar_positive_or_default(desc->tick_length_px, COLORBAR_TICK_LENGTH_PX);
    colorbar->label_gap_px =
        _colorbar_positive_or_default(desc->label_gap_px, COLORBAR_LABEL_GAP_PX);
    colorbar->text_renderer = _scene_adornment_text_renderer(
        desc->text_renderer != 0 ? desc->text_renderer : DVZ_TEXT_RENDERER_MSDF_ATLAS);
    colorbar->placement = desc->placement;
    if (desc->title != NULL)
        dvz_strlcpy(colorbar->title, desc->title, sizeof(colorbar->title));

    _colorbar_apply_auto_reserve(colorbar);
    _scene_mark_colorbar_dirty(colorbar);
    return true;
}



/**
 * Set the colorbar title.
 *
 * @param colorbar the colorbar
 * @param title the title, or NULL to clear
 */
void dvz_colorbar_set_title(DvzColorbar* colorbar, const char* title)
{
    ANN(colorbar);
    const char* src = title != NULL ? title : "";
    if (strcmp(colorbar->title, src) == 0)
        return;
    dvz_strlcpy(colorbar->title, src, sizeof(colorbar->title));
    _scene_mark_colorbar_dirty(colorbar);
}


/**
 * Rebuild all retained colorbar visuals before FramePlan emission.
 *
 * @param figure the figure
 * @param report optional diagnostic report
 */
void _scene_prepare_colorbar_visuals(DvzFigure* figure, DvzDiagnosticReport* report)
{
    if (figure == NULL || figure->scene == NULL)
        return;
    DvzScene* scene = figure->scene;
    for (uint32_t i = 0; i < scene->colorbar_count; i++)
    {
        DvzColorbar* colorbar = &scene->colorbars[i];
        if (colorbar->scene != scene || colorbar->panel == NULL ||
            colorbar->panel->figure != figure)
            continue;
        _colorbar_apply_auto_reserve(colorbar);
        if (_colorbar_needs_visual_update(colorbar))
            _colorbar_update_visuals(colorbar, report);
    }
}
