/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene scale, colormap, and colorbar helpers                                                   */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_scene.h"



/*************************************************************************************************/
/*  Function prototypes                                                                          */
/*************************************************************************************************/

static void _scene_mark_scale_dirty(DvzScale* scale);

static void _scene_mark_colormap_dirty(DvzColormap* colormap);

static void _scene_texture_bump_version(DvzVisual* visual);

static void _scene_mark_colorbar_dirty(DvzColorbar* colorbar);



static void _colorbar_report(DvzDiagnosticReport* report, const char* message);

static void _colorbar_hide(DvzColorbar* colorbar);

static void _colorbar_fail(DvzColorbar* colorbar, DvzDiagnosticReport* report, const char* message);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define COLORBAR_RAMP_SEGMENTS 64u
#define COLORBAR_VERTICAL_RESERVE_PX 96.0f
#define COLORBAR_HORIZONTAL_RESERVE_PX 72.0f
#define COLORBAR_RAMP_THICKNESS_PX 18.0f
#define COLORBAR_EDGE_PADDING_PX 8.0f
#define COLORBAR_TICK_LENGTH_PX 6.0f
#define COLORBAR_LABEL_GAP_PX 4.0f
#define COLORBAR_TITLE_GAP_PX 8.0f
#define COLORBAR_TICK_WIDTH_PX 1.0f
#define COLORBAR_TICK_TEXT_SIZE_PX 12.0f
#define COLORBAR_TITLE_TEXT_SIZE_PX 13.0f
#define COLORBAR_EPS 1e-12



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Sample an ordered colormap stop table.
 *
 * @param stops the stop table
 * @param count number of stops
 * @param t normalized scalar value
 * @param out_rgba the output RGBA color
 * @return true when a color was written
 */
static bool _colormap_sample_stops(
    const DvzColormapStop* stops, uint32_t count, double t, uint8_t out_rgba[4])
{
    ANN(stops);
    ANN(out_rgba);
    if (count < 2)
        return false;
    const DvzColormapStop* lo = &stops[0];
    const DvzColormapStop* hi = &stops[count - 1];
    for (uint32_t i = 1; i < count; i++)
    {
        if (t <= stops[i].position)
        {
            lo = &stops[i - 1];
            hi = &stops[i];
            break;
        }
    }
    double span = hi->position - lo->position;
    double u = span > 0.0 ? (t - lo->position) / span : 0.0;
    if (u < 0.0)
        u = 0.0;
    if (u > 1.0)
        u = 1.0;
    for (uint32_t c = 0; c < 4; c++)
    {
        double value = (1.0 - u) * lo->rgba[c] + u * hi->rgba[c];
        out_rgba[c] = (uint8_t)(value + 0.5);
    }
    return true;
}



/**
 * Return a compact built-in colormap stop table.
 *
 * @param builtin the built-in colormap
 * @param out_count output stop count
 * @return the static stop table, or NULL
 */
static const DvzColormapStop*
_colormap_builtin_stops(DvzBuiltinColormap builtin, uint32_t* out_count)
{
    ANN(out_count);
    static const DvzColormapStop viridis[] = {
        {.position = 0.00, .rgba = {68, 1, 84, 255}},
        {.position = 0.25, .rgba = {59, 82, 139, 255}},
        {.position = 0.50, .rgba = {33, 145, 140, 255}},
        {.position = 0.75, .rgba = {94, 201, 98, 255}},
        {.position = 1.00, .rgba = {253, 231, 37, 255}},
    };
    static const DvzColormapStop magma[] = {
        {.position = 0.00, .rgba = {0, 0, 4, 255}},
        {.position = 0.25, .rgba = {80, 18, 123, 255}},
        {.position = 0.50, .rgba = {182, 54, 121, 255}},
        {.position = 0.75, .rgba = {251, 136, 97, 255}},
        {.position = 1.00, .rgba = {252, 253, 191, 255}},
    };
    static const DvzColormapStop plasma[] = {
        {.position = 0.00, .rgba = {13, 8, 135, 255}},
        {.position = 0.25, .rgba = {126, 3, 168, 255}},
        {.position = 0.50, .rgba = {204, 71, 120, 255}},
        {.position = 0.75, .rgba = {248, 149, 64, 255}},
        {.position = 1.00, .rgba = {240, 249, 33, 255}},
    };
    static const DvzColormapStop inferno[] = {
        {.position = 0.00, .rgba = {0, 0, 4, 255}},
        {.position = 0.25, .rgba = {87, 16, 110, 255}},
        {.position = 0.50, .rgba = {188, 55, 84, 255}},
        {.position = 0.75, .rgba = {249, 142, 9, 255}},
        {.position = 1.00, .rgba = {252, 255, 164, 255}},
    };
    static const DvzColormapStop cividis[] = {
        {.position = 0.00, .rgba = {0, 32, 76, 255}},
        {.position = 0.25, .rgba = {59, 78, 109, 255}},
        {.position = 0.50, .rgba = {124, 123, 120, 255}},
        {.position = 0.75, .rgba = {188, 172, 103, 255}},
        {.position = 1.00, .rgba = {255, 233, 69, 255}},
    };
    static const DvzColormapStop turbo[] = {
        {.position = 0.00, .rgba = {48, 18, 59, 255}},
        {.position = 0.20, .rgba = {55, 91, 178, 255}},
        {.position = 0.40, .rgba = {49, 205, 207, 255}},
        {.position = 0.60, .rgba = {135, 255, 88, 255}},
        {.position = 0.80, .rgba = {255, 170, 36, 255}},
        {.position = 1.00, .rgba = {122, 4, 3, 255}},
    };
    static const DvzColormapStop gray[] = {
        {.position = 0.00, .rgba = {0, 0, 0, 255}},
        {.position = 1.00, .rgba = {255, 255, 255, 255}},
    };
    switch (builtin)
    {
    case DVZ_BUILTIN_COLORMAP_VIRIDIS:
        *out_count = DVZ_ARRAY_COUNT(viridis);
        return viridis;
    case DVZ_BUILTIN_COLORMAP_MAGMA:
        *out_count = DVZ_ARRAY_COUNT(magma);
        return magma;
    case DVZ_BUILTIN_COLORMAP_PLASMA:
        *out_count = DVZ_ARRAY_COUNT(plasma);
        return plasma;
    case DVZ_BUILTIN_COLORMAP_INFERNO:
        *out_count = DVZ_ARRAY_COUNT(inferno);
        return inferno;
    case DVZ_BUILTIN_COLORMAP_CIVIDIS:
        *out_count = DVZ_ARRAY_COUNT(cividis);
        return cividis;
    case DVZ_BUILTIN_COLORMAP_TURBO:
        *out_count = DVZ_ARRAY_COUNT(turbo);
        return turbo;
    case DVZ_BUILTIN_COLORMAP_GRAY:
        *out_count = DVZ_ARRAY_COUNT(gray);
        return gray;
    case DVZ_BUILTIN_COLORMAP_NONE:
    default:
        *out_count = 0;
        return NULL;
    }
}



/**
 * Mark visuals depending on one scale as needing refreshed texture data.
 *
 * @param scale the scale
 */
static void _scene_mark_scale_dirty(DvzScale* scale)
{
    if (scale == NULL || scale->scene == NULL)
        return;
    DvzScene* scene = scale->scene;
    for (uint32_t i = 0; i < scene->visual_count; i++)
    {
        DvzVisual* visual = &scene->visuals[i];
        if (visual->scene != scene || visual->scale != scale)
            continue;
        if ((visual->type == DVZ_VISUAL_TYPE_IMAGE || visual->type == DVZ_VISUAL_TYPE_VOLUME) &&
            visual->field != NULL &&
            _field_format_is_scalar(visual->field->desc.format))
        {
            _scene_visual_texture_mark_clean(visual);
            visual->texture.dirty = true;
            _scene_texture_bump_version(visual);
            _scene_notify_visual_changed(visual);
        }
    }
    for (uint32_t i = 0; i < scene->colorbar_count; i++)
    {
        DvzColorbar* colorbar = &scene->colorbars[i];
        if (colorbar->scene == scene && colorbar->scale == scale)
            _scene_mark_colorbar_dirty(colorbar);
    }
}



/**
 * Advance a retained visual texture version.
 *
 * @param visual the image visual
 */
static void _scene_texture_bump_version(DvzVisual* visual)
{
    ANN(visual);
    visual->texture.version =
        visual->texture.version == UINT64_MAX ? 1 : visual->texture.version + 1;
}



/**
 * Mark visuals depending on one colormap as needing refreshed texture data.
 *
 * @param colormap the colormap
 */
static void _scene_mark_colormap_dirty(DvzColormap* colormap)
{
    if (colormap == NULL || colormap->scene == NULL)
        return;
    DvzScene* scene = colormap->scene;
    for (uint32_t i = 0; i < scene->scale_count; i++)
    {
        DvzScale* scale = &scene->scales[i];
        if (scale->scene == scene && scale->colormap == colormap)
            _scene_mark_scale_dirty(scale);
    }
}


/**
 * Mark one retained colorbar layout as dirty.
 *
 * @param colorbar the colorbar
 */
static void _scene_mark_colorbar_dirty(DvzColorbar* colorbar)
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
 * Return the visual-space reserve needed for a pixel band.
 *
 * @param panel the panel
 * @param horizontal whether the reserve applies to the x dimension
 * @param pixels reserve in logical pixels
 * @return reserve in panel visual units
 */
static float _colorbar_pixels_to_reserve(
    const DvzPanel* panel, bool horizontal, float pixels)
{
    ANN(panel);
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    _scene_panel_pixel_rect(panel, &x, &y, &width, &height);
    (void)x;
    (void)y;
    float span = horizontal ? width : height;
    if (!(span > 0.0f) || !isfinite(span))
        return 0.0f;
    return 2.0f * pixels / span;
}



/**
 * Apply the deterministic first-slice panel reserve for a colorbar edge.
 *
 * @param colorbar the colorbar
 */
static void _colorbar_apply_auto_reserve(DvzColorbar* colorbar)
{
    ANN(colorbar);
    if (colorbar->panel == NULL || !_colorbar_anchor_supported(colorbar->anchor))
        return;
    DvzPanelLayoutReserve reserve = dvz_panel_layout_reserve();
    (void)dvz_panel_get_layout_reserve(colorbar->panel, &reserve);
    DvzPanelLayoutReserve next = reserve;
    switch (colorbar->anchor)
    {
    case DVZ_SCENE_ANCHOR_PANEL_LEFT:
        next.left = fmaxf(
            next.left,
            _colorbar_pixels_to_reserve(colorbar->panel, true, COLORBAR_VERTICAL_RESERVE_PX));
        break;
    case DVZ_SCENE_ANCHOR_PANEL_RIGHT:
        next.right = fmaxf(
            next.right,
            _colorbar_pixels_to_reserve(colorbar->panel, true, COLORBAR_VERTICAL_RESERVE_PX));
        break;
    case DVZ_SCENE_ANCHOR_PANEL_TOP:
        next.top = fmaxf(
            next.top,
            _colorbar_pixels_to_reserve(colorbar->panel, false, COLORBAR_HORIZONTAL_RESERVE_PX));
        break;
    case DVZ_SCENE_ANCHOR_PANEL_BOTTOM:
        next.bottom = fmaxf(
            next.bottom,
            _colorbar_pixels_to_reserve(colorbar->panel, false, COLORBAR_HORIZONTAL_RESERVE_PX));
        break;
    default:
        break;
    }
    if (fabsf(next.left - reserve.left) > 1e-6f ||
        fabsf(next.right - reserve.right) > 1e-6f ||
        fabsf(next.bottom - reserve.bottom) > 1e-6f ||
        fabsf(next.top - reserve.top) > 1e-6f)
    {
        (void)dvz_panel_set_layout_reserve(colorbar->panel, &next);
    }
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
        if (colorbar->text_visual->text.glyph_visual != NULL)
            dvz_visual_set_visible(colorbar->text_visual->text.glyph_visual, false);
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
        colorbar->text_visual = _scene_text_visual(colorbar->scene, 0);
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
        uint32_t k = 6u * i;
        _colorbar_pixel_to_visual(width, height, x0, y0, 0.0f, positions[k + 0]);
        _colorbar_pixel_to_visual(width, height, x1, y0, 0.0f, positions[k + 1]);
        _colorbar_pixel_to_visual(width, height, x1, y1, 0.0f, positions[k + 2]);
        _colorbar_pixel_to_visual(width, height, x0, y0, 0.0f, positions[k + 3]);
        _colorbar_pixel_to_visual(width, height, x0, y1, 0.0f, positions[k + 4]);
        _colorbar_pixel_to_visual(width, height, x1, y1, 0.0f, positions[k + 5]);
        if (vertical)
        {
            dvz_memcpy(colors[k + 0], sizeof(DvzColor), rgba0, sizeof(DvzColor));
            dvz_memcpy(colors[k + 1], sizeof(DvzColor), rgba0, sizeof(DvzColor));
            dvz_memcpy(colors[k + 2], sizeof(DvzColor), rgba1, sizeof(DvzColor));
            dvz_memcpy(colors[k + 3], sizeof(DvzColor), rgba0, sizeof(DvzColor));
            dvz_memcpy(colors[k + 4], sizeof(DvzColor), rgba1, sizeof(DvzColor));
            dvz_memcpy(colors[k + 5], sizeof(DvzColor), rgba1, sizeof(DvzColor));
        }
        else
        {
            dvz_memcpy(colors[k + 0], sizeof(DvzColor), rgba0, sizeof(DvzColor));
            dvz_memcpy(colors[k + 1], sizeof(DvzColor), rgba1, sizeof(DvzColor));
            dvz_memcpy(colors[k + 2], sizeof(DvzColor), rgba1, sizeof(DvzColor));
            dvz_memcpy(colors[k + 3], sizeof(DvzColor), rgba0, sizeof(DvzColor));
            dvz_memcpy(colors[k + 4], sizeof(DvzColor), rgba0, sizeof(DvzColor));
            dvz_memcpy(colors[k + 5], sizeof(DvzColor), rgba1, sizeof(DvzColor));
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
                x0 = ramp_x0 - COLORBAR_TICK_LENGTH_PX;
                x1 = ramp_x0;
                label_x = x0 - COLORBAR_LABEL_GAP_PX;
                anchor_x = 1.0f;
            }
            else
            {
                x0 = ramp_x1;
                x1 = ramp_x1 + COLORBAR_TICK_LENGTH_PX;
                label_x = x1 + COLORBAR_LABEL_GAP_PX;
                anchor_x = 0.0f;
            }
            y0 = y1 = label_y = y;
        }
        else
        {
            float x = ramp_x0 + (ramp_x1 - ramp_x0) * (float)t;
            if (colorbar->anchor == DVZ_SCENE_ANCHOR_PANEL_TOP)
            {
                y0 = ramp_y0 - COLORBAR_TICK_LENGTH_PX;
                y1 = ramp_y0;
                label_y = y0 - COLORBAR_LABEL_GAP_PX;
                anchor_y = 1.0f;
            }
            else
            {
                y0 = ramp_y1;
                y1 = ramp_y1 + COLORBAR_TICK_LENGTH_PX;
                label_y = y1 + COLORBAR_LABEL_GAP_PX;
                anchor_y = 0.0f;
            }
            x0 = x1 = label_x = x;
        }
        _colorbar_pixel_to_visual(width, height, x0, y0, 0.0f, starts[count]);
        _colorbar_pixel_to_visual(width, height, x1, y1, 0.0f, ends[count]);
        colors[count][0] = 255;
        colors[count][1] = 255;
        colors[count][2] = 255;
        colors[count][3] = 255;
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
                      fmaxf(COLORBAR_EDGE_PADDING_PX, ramp_x0 - COLORBAR_TITLE_GAP_PX - 4.0f) :
                      fminf(
                          width - COLORBAR_EDGE_PADDING_PX,
                          ramp_x1 + COLORBAR_TICK_LENGTH_PX + COLORBAR_LABEL_GAP_PX + 46.0f);
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
                      COLORBAR_EDGE_PADDING_PX :
                      height - COLORBAR_EDGE_PADDING_PX;
        float anchor_y = colorbar->anchor == DVZ_SCENE_ANCHOR_PANEL_TOP ? 0.0f : 1.0f;
        _colorbar_append_text(
            colorbar, colorbar->title, x, y, 0.5f, anchor_y, COLORBAR_TITLE_TEXT_SIZE_PX, 0.0f);
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
    if (!_colorbar_anchor_supported(colorbar->anchor))
    {
        _colorbar_fail(
            colorbar, report,
            "colorbar anchor is not supported in the first rendered colorbar slice");
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
        _colorbar_hide(colorbar);
        return;
    }
    if (!_colorbar_ensure_visuals(colorbar))
    {
        _colorbar_hide(colorbar);
        return;
    }

    float ramp_x0 = 0.0f;
    float ramp_y0 = 0.0f;
    float ramp_x1 = 0.0f;
    float ramp_y1 = 0.0f;
    if (_colorbar_vertical(colorbar))
    {
        if (colorbar->anchor == DVZ_SCENE_ANCHOR_PANEL_LEFT)
        {
            ramp_x1 = COLORBAR_VERTICAL_RESERVE_PX - COLORBAR_EDGE_PADDING_PX;
            ramp_x0 = ramp_x1 - COLORBAR_RAMP_THICKNESS_PX;
        }
        else
        {
            ramp_x0 = width - COLORBAR_VERTICAL_RESERVE_PX + COLORBAR_EDGE_PADDING_PX;
            ramp_x1 = ramp_x0 + COLORBAR_RAMP_THICKNESS_PX;
        }
        ramp_y0 = COLORBAR_EDGE_PADDING_PX;
        ramp_y1 = height - COLORBAR_EDGE_PADDING_PX;
    }
    else
    {
        ramp_x0 = COLORBAR_EDGE_PADDING_PX;
        ramp_x1 = width - COLORBAR_EDGE_PADDING_PX;
        if (colorbar->anchor == DVZ_SCENE_ANCHOR_PANEL_TOP)
        {
            ramp_y1 = COLORBAR_HORIZONTAL_RESERVE_PX - COLORBAR_EDGE_PADDING_PX;
            ramp_y0 = ramp_y1 - COLORBAR_RAMP_THICKNESS_PX;
        }
        else
        {
            ramp_y0 = height - COLORBAR_HORIZONTAL_RESERVE_PX + COLORBAR_EDGE_PADDING_PX;
            ramp_y1 = ramp_y0 + COLORBAR_RAMP_THICKNESS_PX;
        }
    }
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



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Resolve one RGBA color from a retained colormap.
 *
 * @param colormap the colormap, or NULL for grayscale fallback
 * @param t the normalized scalar value
 * @param out_rgba the output RGBA color
 * @return true when a color was written
 */
bool _scene_color_from_colormap(
    const DvzColormap* colormap, double t, uint8_t out_rgba[4])
{
    ANN(out_rgba);
    if (t < 0.0)
        t = 0.0;
    if (t > 1.0)
        t = 1.0;

    if (colormap != NULL && colormap->stop_count >= 2)
    {
        return _colormap_sample_stops(colormap->stops, colormap->stop_count, t, out_rgba);
    }

    if (colormap != NULL)
    {
        uint32_t builtin_count = 0;
        const DvzColormapStop* builtin =
            _colormap_builtin_stops(colormap->builtin, &builtin_count);
        if (builtin != NULL && builtin_count >= 2)
            return _colormap_sample_stops(builtin, builtin_count, t, out_rgba);
    }

    uint8_t gray = (uint8_t)(255.0 * t + 0.5);
    out_rgba[0] = gray;
    out_rgba[1] = gray;
    out_rgba[2] = gray;
    out_rgba[3] = 255;
    return true;
}



/**
 * Create a scene-owned scale object.
 *
 * @param scene the scene
 * @param desc the scale descriptor, or NULL for defaults
 * @return the scale, or NULL on allocation failure
 */
DvzScale* dvz_scale(DvzScene* scene, const DvzScaleDesc* desc)
{
    ANN(scene);
    if (scene->scale_count >= DVZ_SCENE_MAX_SCALES)
    {
        log_error("maximum scale count reached");
        return NULL;
    }
    DvzScale* scale = &scene->scales[scene->scale_count++];
    dvz_memset(scale, sizeof(DvzScale), 0, sizeof(DvzScale));
    scale->scene = scene;
    scale->kind = desc != NULL ? desc->kind : DVZ_SCALE_CONTINUOUS;
    if (desc != NULL)
    {
        if (desc->label != NULL)
            dvz_strlcpy(scale->label, desc->label, sizeof(scale->label));
        if (desc->unit != NULL)
            dvz_strlcpy(scale->unit, desc->unit, sizeof(scale->unit));
        _scene_format_state_copy(&scale->format, &desc->format);
    }
    return scale;
}



/**
 * Destroy a scale object.
 *
 * @param scale the scale
 */
void dvz_scale_destroy(DvzScale* scale)
{
    if (scale == NULL)
        return;
    scale->scene = NULL;
    scale->colormap = NULL;
    scale->has_domain = false;
    scale->has_view_range = false;
}



/**
 * Set the semantic domain on a scale.
 *
 * @param scale the scale
 * @param min the domain minimum
 * @param max the domain maximum
 */
void dvz_scale_set_domain(DvzScale* scale, double min, double max)
{
    ANN(scale);
    scale->domain_min = min;
    scale->domain_max = max;
    scale->has_domain = true;
    _scene_mark_scale_dirty(scale);
}



/**
 * Set the current visible range on a scale.
 *
 * @param scale the scale
 * @param min the view-range minimum
 * @param max the view-range maximum
 */
void dvz_scale_set_view_range(DvzScale* scale, double min, double max)
{
    ANN(scale);
    scale->view_min = min;
    scale->view_max = max;
    scale->has_view_range = true;
    _scene_mark_scale_dirty(scale);
}



/**
 * Bind a colormap to a scale.
 *
 * @param scale the scale
 * @param colormap the colormap
 */
void dvz_scale_set_colormap(DvzScale* scale, DvzColormap* colormap)
{
    ANN(scale);
    if (colormap != NULL && colormap->scene != scale->scene)
    {
        log_error("cannot bind a colormap from a different scene");
        return;
    }
    scale->colormap = colormap;
    _scene_mark_scale_dirty(scale);
}



/**
 * Override shared formatting policy on a scale.
 *
 * @param scale the scale
 * @param format the format descriptor, or NULL to clear the override
 */
void dvz_scale_set_format(DvzScale* scale, const DvzFormatDesc* format)
{
    ANN(scale);
    _scene_format_state_copy(&scale->format, format);
    _scene_mark_scale_dirty(scale);
}



/**
 * Create a scene-owned colormap object.
 *
 * @param scene the scene
 * @param desc the colormap descriptor, or NULL for defaults
 * @return the colormap, or NULL on allocation failure
 */
DvzColormap* dvz_colormap(DvzScene* scene, const DvzColormapDesc* desc)
{
    ANN(scene);
    if (scene->colormap_count >= DVZ_SCENE_MAX_COLORMAPS)
    {
        log_error("maximum colormap count reached");
        return NULL;
    }
    DvzColormap* colormap = &scene->colormaps[scene->colormap_count++];
    dvz_memset(colormap, sizeof(DvzColormap), 0, sizeof(DvzColormap));
    colormap->scene = scene;
    colormap->kind = desc != NULL ? desc->kind : DVZ_COLORMAP_CONTINUOUS;
    colormap->builtin = desc != NULL ? desc->builtin : DVZ_BUILTIN_COLORMAP_NONE;
    if (desc != NULL)
    {
        colormap->center = desc->center;
        colormap->has_center = desc->center != 0.0;
        if (desc->label != NULL)
            dvz_strlcpy(colormap->label, desc->label, sizeof(colormap->label));
    }
    return colormap;
}



/**
 * Create a scene-owned built-in colormap object.
 *
 * @param scene the scene
 * @param builtin the built-in colormap selector
 * @return the colormap, or NULL on allocation failure
 */
DvzColormap* dvz_colormap_builtin(DvzScene* scene, DvzBuiltinColormap builtin)
{
    DvzColormapDesc desc = {
        .kind = DVZ_COLORMAP_CONTINUOUS,
        .builtin = builtin,
    };
    return dvz_colormap(scene, &desc);
}



/**
 * Destroy a colormap object.
 *
 * @param colormap the colormap
 */
void dvz_colormap_destroy(DvzColormap* colormap)
{
    if (colormap == NULL)
        return;
    colormap->scene = NULL;
    colormap->stop_count = 0;
    colormap->has_center = false;
}



/**
 * Set custom color stops on a colormap.
 *
 * @param colormap the colormap
 * @param stops the color stops
 * @param count the number of stops
 */
void dvz_colormap_set_stops(DvzColormap* colormap, const DvzColormapStop* stops, uint32_t count)
{
    ANN(colormap);
    if (count > DVZ_SCENE_MAX_COLOR_STOPS)
    {
        log_error("too many color stops: %u > %u", count, DVZ_SCENE_MAX_COLOR_STOPS);
        return;
    }
    if (count > 0)
        ANN(stops);
    colormap->stop_count = count;
    if (count > 0)
        dvz_memcpy(
            colormap->stops, sizeof(colormap->stops), stops, count * sizeof(DvzColormapStop));
    _scene_mark_colormap_dirty(colormap);
}



/**
 * Set the diverging center on a colormap.
 *
 * @param colormap the colormap
 * @param center the semantic center value
 */
void dvz_colormap_set_center(DvzColormap* colormap, double center)
{
    ANN(colormap);
    colormap->center = center;
    colormap->has_center = true;
    _scene_mark_colormap_dirty(colormap);
}



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
    DvzSceneAnchor anchor = desc != NULL ? desc->anchor : DVZ_SCENE_ANCHOR_PANEL_RIGHT;
    if (!_colorbar_anchor_supported(anchor))
    {
        log_error("colorbar anchor is not supported in the first rendered colorbar slice");
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
    colorbar->orientation =
        desc != NULL ? desc->orientation : DVZ_COLORBAR_ORIENTATION_VERTICAL;
    colorbar->anchor = anchor;
    colorbar->flags = desc != NULL ? desc->flags : 0;
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
    if (colorbar->panel != NULL)
    {
        DvzPanel* panel = colorbar->panel;
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
        log_error("colorbar anchor is not supported in the first rendered colorbar slice");
        return false;
    }
    if (colorbar->anchor == anchor)
        return true;
    colorbar->anchor = anchor;
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
        _colorbar_update_visuals(colorbar, report);
    }
}
