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
#include <inttypes.h>
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

static void _scene_mark_legend_dirty(DvzLegend* legend);

static bool _scale_categories_have_duplicate_ids(
    const DvzScaleCategory* categories, uint32_t count);

static int32_t _scale_category_index(const DvzScale* scale, DvzCategoryId id);

static void _scale_category_copy(DvzScaleCategoryState* dst, const DvzScaleCategory* src);



static void _colorbar_report(DvzDiagnosticReport* report, const char* message);

static void _colorbar_hide(DvzColorbar* colorbar);

static void _colorbar_fail(DvzColorbar* colorbar, DvzDiagnosticReport* report, const char* message);



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
#define LEGEND_RESERVE_PX 140.0f
#define LEGEND_EDGE_OFFSET_PX 8.0f
#define LEGEND_PLOT_GAP_PX 12.0f
#define LEGEND_ENTRY_GAP_PX 6.0f
#define LEGEND_MARK_SIZE_PX 12.0f
#define LEGEND_MARK_LABEL_GAP_PX 6.0f



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
    for (uint32_t i = 0; i < scene->legend_count; i++)
    {
        DvzLegend* legend = &scene->legends[i];
        if (legend->scene == scene && legend->scale == scale)
            _scene_mark_legend_dirty(legend);
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
 * Mark one retained legend layout as dirty.
 *
 * @param legend the legend
 */
static void _scene_mark_legend_dirty(DvzLegend* legend)
{
    if (legend == NULL)
        return;
    legend->dirty = true;
    legend->version = legend->version == UINT64_MAX ? 1 : legend->version + 1;
    _scene_notify_request_frame(legend->panel != NULL ? legend->panel->figure : NULL);
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
 * Return whether a category table contains duplicate category ids.
 *
 * @param categories category descriptors
 * @param count number of descriptors
 * @return whether any category id appears more than once
 */
static bool _scale_categories_have_duplicate_ids(
    const DvzScaleCategory* categories, uint32_t count)
{
    ANN(categories);
    for (uint32_t i = 0; i < count; i++)
    {
        for (uint32_t j = i + 1; j < count; j++)
        {
            if (categories[i].category_id == categories[j].category_id)
                return true;
        }
    }
    return false;
}


/**
 * Return the index of a retained category id.
 *
 * @param scale the categorical scale
 * @param id the category id
 * @return the category index, or -1 when absent
 */
static int32_t _scale_category_index(const DvzScale* scale, DvzCategoryId id)
{
    ANN(scale);
    for (uint32_t i = 0; i < scale->category_count; i++)
    {
        if (scale->categories[i].category_id == id)
            return (int32_t)i;
    }
    return -1;
}


/**
 * Copy a public category descriptor into retained category state.
 *
 * @param dst retained category state
 * @param src public category descriptor
 */
static void _scale_category_copy(DvzScaleCategoryState* dst, const DvzScaleCategory* src)
{
    ANN(dst);
    ANN(src);
    dvz_memset(dst, sizeof(DvzScaleCategoryState), 0, sizeof(DvzScaleCategoryState));
    dst->category_id = src->category_id;
    dst->order = src->order;
    dst->flags = src->flags;
    dst->has_label = src->label != NULL && src->label[0] != '\0';
    if (dst->has_label)
        dvz_strlcpy(dst->label, src->label, sizeof(dst->label));
    dst->color = src->color;
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
 * Return whether a legend anchor is supported by the first slice.
 *
 * @param anchor the anchor
 * @return whether the anchor is a panel edge
 */
static bool _legend_anchor_supported(DvzSceneAnchor anchor)
{
    return anchor == DVZ_SCENE_ANCHOR_PANEL_LEFT || anchor == DVZ_SCENE_ANCHOR_PANEL_RIGHT ||
           anchor == DVZ_SCENE_ANCHOR_PANEL_TOP || anchor == DVZ_SCENE_ANCHOR_PANEL_BOTTOM;
}


/**
 * Return a positive legend descriptor value or its fallback.
 *
 * @param value descriptor value
 * @param fallback default value
 * @return value when positive, otherwise fallback
 */
static float _legend_positive_or_default(float value, float fallback)
{
    return value > 0.0f && isfinite(value) ? value : fallback;
}


/**
 * Refresh aggregate attached legend reserve for one panel.
 *
 * @param panel the panel
 */
void _scene_panel_refresh_legend_reserve(DvzPanel* panel)
{
    if (panel == NULL)
        return;
    DvzPanelReserve reserve = {0};
    for (uint32_t i = 0; i < panel->legend_count; i++)
    {
        DvzLegend* legend = panel->legends[i];
        if (legend == NULL || legend->panel != panel)
            continue;
        DvzPanelReserve applied = {0};
        if (legend->placement_mode == DVZ_LEGEND_PLACEMENT_ATTACHED &&
            _legend_anchor_supported(legend->anchor))
        {
            float reserve_px = _legend_positive_or_default(legend->reserve_px, LEGEND_RESERVE_PX);
            switch (legend->anchor)
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
        legend->auto_reserve = applied;
    }
    _scene_panel_set_legend_reserve(panel, &reserve);
}


/**
 * Apply the deterministic first-slice panel reserve for a legend edge.
 *
 * @param legend the legend
 */
static void _legend_apply_auto_reserve(DvzLegend* legend)
{
    ANN(legend);
    if (legend->panel == NULL)
        return;
    _scene_panel_refresh_legend_reserve(legend->panel);
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


/**
 * Hide all derived visuals owned by a legend.
 *
 * @param legend the legend
 */
static void _legend_hide(DvzLegend* legend)
{
    if (legend == NULL)
        return;
    if (legend->mark_visual != NULL)
        dvz_visual_set_visible(legend->mark_visual, false);
    if (legend->text_visual != NULL)
    {
        dvz_visual_set_visible(legend->text_visual, false);
        if (legend->text_visual->text.glyph_visual != NULL)
            dvz_visual_set_visible(legend->text_visual->text.glyph_visual, false);
    }
}


/**
 * Hide one invalid legend and report the validation failure once per dirty cycle.
 *
 * @param legend the legend
 * @param report optional diagnostic report
 * @param message the diagnostic message
 */
static void _legend_fail(DvzLegend* legend, DvzDiagnosticReport* report, const char* message)
{
    ANN(legend);
    ANN(message);
    if (legend->dirty || report != NULL)
        _colorbar_report(report, message);
    legend->dirty = false;
    _legend_hide(legend);
}


/**
 * Ensure one legend-derived visual is attached to the panel.
 *
 * @param legend the legend
 * @param visual the visual
 * @param z_layer z layer for panel sorting
 * @return whether the visual is attached
 */
static bool _legend_attach_visual(DvzLegend* legend, DvzVisual* visual, int32_t z_layer)
{
    ANN(legend);
    ANN(legend->panel);
    ANN(visual);
    DvzVisualAttachDesc attach = {.z_layer = z_layer, .controller_mode = DVZ_CONTROLLER_FIXED};
    for (uint32_t i = 0; i < legend->panel->visual_count; i++)
    {
        DvzPanelAttach* existing = &legend->panel->visuals[i];
        if (existing->visual != visual)
            continue;
        existing->z_layer = attach.z_layer;
        existing->controller_mode = attach.controller_mode;
        return true;
    }
    return dvz_panel_add_visual(legend->panel, visual, &attach) == 0;
}


/**
 * Ensure derived mark and text visuals exist for a legend.
 *
 * @param legend the legend
 * @return whether all derived visuals exist
 */
static bool _legend_ensure_visuals(DvzLegend* legend)
{
    ANN(legend);
    if (legend->scene == NULL || legend->panel == NULL)
        return false;
    if (legend->mark_visual == NULL)
    {
        legend->mark_visual = dvz_marker(legend->scene, 0);
        if (legend->mark_visual == NULL)
            return false;
        legend->mark_visual->visible = false;
    }
    if (!_legend_attach_visual(legend, legend->mark_visual, 1002))
        return false;

    if (legend->text_visual == NULL)
    {
        legend->text_visual = _scene_adornment_text_visual(legend->scene, legend->text_renderer);
        if (legend->text_visual == NULL)
            return false;
        legend->text_visual->visible = false;
    }
    return _legend_attach_visual(legend, legend->text_visual, 1003);
}


/**
 * Append one text item to legend text arrays.
 *
 * @param legend the legend
 * @param label text label
 * @param x text position x in panel-local pixels
 * @param y text position y in panel-local pixels
 * @param anchor_x text anchor x
 * @param anchor_y text anchor y
 * @param size text size in pixels
 */
static void _legend_append_text(
    DvzLegend* legend, const char* label, float x, float y, float anchor_x, float anchor_y,
    float size)
{
    ANN(legend);
    ANN(label);
    if (legend->text_count >= DVZ_SCENE_MAX_LEGEND_TEXTS)
        return;
    uint32_t i = legend->text_count++;
    dvz_strlcpy(legend->text_labels[i], label, sizeof(legend->text_labels[i]));
    legend->text_positions[i][0] = x;
    legend->text_positions[i][1] = y;
    legend->text_positions[i][2] = 0.0f;
    legend->text_anchors[i][0] = anchor_x;
    legend->text_anchors[i][1] = anchor_y;
    legend->text_sizes[i] = size;
    legend->text_colors[i][0] = 255;
    legend->text_colors[i][1] = 255;
    legend->text_colors[i][2] = 255;
    legend->text_colors[i][3] = 255;
    legend->text_angles[i] = 0.0f;
}


/**
 * Update the legend batched text visual.
 *
 * @param legend the legend
 */
static void _legend_update_text(DvzLegend* legend)
{
    ANN(legend);
    if (legend->text_visual == NULL || legend->text_count == 0)
    {
        if (legend->text_visual != NULL)
            dvz_visual_set_visible(legend->text_visual, false);
        return;
    }
    if (_scene_adornment_text_visual_set_renderer(legend->text_visual, legend->text_renderer) != 0)
    {
        dvz_visual_set_visible(legend->text_visual, false);
        return;
    }
    const char* strings[DVZ_SCENE_MAX_LEGEND_TEXTS] = {0};
    for (uint32_t i = 0; i < legend->text_count; i++)
        strings[i] = legend->text_labels[i];
    DvzVisualDataUpdate updates[5] = {
        {.attr_name = "position", .data = legend->text_positions, .item_count = legend->text_count},
        {.attr_name = "anchor", .data = legend->text_anchors, .item_count = legend->text_count},
        {.attr_name = "size", .data = legend->text_sizes, .item_count = legend->text_count},
        {.attr_name = "color", .data = legend->text_colors, .item_count = legend->text_count},
        {.attr_name = "angle", .data = legend->text_angles, .item_count = legend->text_count},
    };
    if (dvz_visual_set_strings(legend->text_visual, "text", strings, legend->text_count) == 0 &&
        dvz_visual_set_data_many(legend->text_visual, updates, 5) == 0)
    {
        dvz_visual_set_visible(legend->text_visual, true);
    }
    else
    {
        dvz_visual_set_visible(legend->text_visual, false);
    }
}


/**
 * Resolve an anchored detached legend rectangle to panel-local pixels.
 *
 * @param legend the legend
 * @param panel_x panel x origin in figure pixels
 * @param panel_y panel y origin in figure pixels
 * @param panel_width panel width in pixels
 * @param panel_height panel height in pixels
 * @param out output rectangle as x0, y0, x1, y1 in panel-local pixels
 */
static void _legend_detached_rect(
    const DvzLegend* legend, float panel_x, float panel_y, float panel_width, float panel_height,
    float out[4])
{
    ANN(legend);
    ANN(out);
    const DvzPlacement* placement = &legend->placement;
    float space_x = 0.0f;
    float space_y = 0.0f;
    float space_width = panel_width;
    float space_height = panel_height;
    if (placement->space == DVZ_PLACEMENT_SPACE_FIGURE && legend->panel != NULL &&
        legend->panel->figure != NULL)
    {
        space_x = -panel_x;
        space_y = -panel_y;
        space_width = legend->panel->figure->width > 0 ? (float)legend->panel->figure->width :
                                                         panel_width;
        space_height = legend->panel->figure->height > 0 ? (float)legend->panel->figure->height :
                                                           panel_height;
    }

    float default_height =
        (legend->scale != NULL ? (float)legend->scale->category_count : 1.0f) *
            (legend->mark_size_px + legend->entry_gap_px) +
        (legend->title[0] != '\0' ? 20.0f : 0.0f);
    float width = _legend_positive_or_default(placement->width_px, legend->reserve_px);
    float height = _legend_positive_or_default(placement->height_px, default_height);
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
 * Resolve the legend content rectangle to panel-local pixels.
 *
 * @param legend the legend
 * @param panel_x panel x origin in figure pixels
 * @param panel_y panel y origin in figure pixels
 * @param panel_width panel width in pixels
 * @param panel_height panel height in pixels
 * @param out output content rectangle as x0, y0, x1, y1 in panel-local pixels
 */
static void _legend_content_rect(
    const DvzLegend* legend, float panel_x, float panel_y, float panel_width, float panel_height,
    float out[4])
{
    ANN(legend);
    ANN(out);
    if (legend->placement_mode == DVZ_LEGEND_PLACEMENT_DETACHED)
    {
        _legend_detached_rect(legend, panel_x, panel_y, panel_width, panel_height, out);
        return;
    }

    float plot_x = 0.0f;
    float plot_y = 0.0f;
    float plot_width = 0.0f;
    float plot_height = 0.0f;
    _scene_panel_plot_pixel_rect(legend->panel, &plot_x, &plot_y, &plot_width, &plot_height);
    plot_x -= panel_x;
    plot_y -= panel_y;

    if (legend->anchor == DVZ_SCENE_ANCHOR_PANEL_LEFT)
    {
        out[0] = legend->edge_offset_px;
        out[2] = plot_x - legend->plot_gap_px;
        out[1] = plot_y + legend->edge_offset_px;
        out[3] = plot_y + plot_height - legend->edge_offset_px;
    }
    else if (legend->anchor == DVZ_SCENE_ANCHOR_PANEL_RIGHT)
    {
        out[0] = plot_x + plot_width + legend->plot_gap_px;
        out[2] = panel_width - legend->edge_offset_px;
        out[1] = plot_y + legend->edge_offset_px;
        out[3] = plot_y + plot_height - legend->edge_offset_px;
    }
    else if (legend->anchor == DVZ_SCENE_ANCHOR_PANEL_TOP)
    {
        out[0] = plot_x + legend->edge_offset_px;
        out[2] = plot_x + plot_width - legend->edge_offset_px;
        out[1] = legend->edge_offset_px;
        out[3] = plot_y - legend->plot_gap_px;
    }
    else
    {
        out[0] = plot_x + legend->edge_offset_px;
        out[2] = plot_x + plot_width - legend->edge_offset_px;
        out[1] = plot_y + plot_height + legend->plot_gap_px;
        out[3] = panel_height - legend->edge_offset_px;
    }
}


/**
 * Return sorted category indices for a legend scale.
 *
 * @param scale the categorical scale
 * @param out output index table
 * @return number of indices written
 */
static uint32_t _legend_sorted_category_indices(const DvzScale* scale, uint32_t* out)
{
    ANN(scale);
    ANN(out);
    uint32_t count = scale->category_count;
    for (uint32_t i = 0; i < count; i++)
        out[i] = i;
    for (uint32_t i = 0; i < count; i++)
    {
        for (uint32_t j = i + 1; j < count; j++)
        {
            const DvzScaleCategoryState* a = &scale->categories[out[i]];
            const DvzScaleCategoryState* b = &scale->categories[out[j]];
            if (b->order < a->order)
            {
                uint32_t tmp = out[i];
                out[i] = out[j];
                out[j] = tmp;
            }
        }
    }
    return count;
}


/**
 * Return whether a category id is highlighted in one retained legend.
 *
 * @param legend the legend
 * @param id the category id
 * @return whether the category is highlighted
 */
static bool _legend_category_highlighted(const DvzLegend* legend, DvzCategoryId id)
{
    ANN(legend);
    for (uint32_t i = 0; i < legend->highlight_count; i++)
    {
        if (legend->highlighted_ids[i] == id)
            return true;
    }
    return false;
}


/**
 * Rebuild the derived visuals for one retained legend.
 *
 * @param legend the legend
 * @param report optional diagnostic report
 */
static void _legend_update_visuals(DvzLegend* legend, DvzDiagnosticReport* report)
{
    ANN(legend);
    if (legend->scene == NULL || legend->panel == NULL || legend->scale == NULL)
        return;
    if (legend->placement_mode == DVZ_LEGEND_PLACEMENT_ATTACHED &&
        !_legend_anchor_supported(legend->anchor))
    {
        _legend_fail(legend, report, "attached legend anchor must be a panel edge");
        return;
    }
    if (legend->scale->kind != DVZ_SCALE_CATEGORICAL)
    {
        _legend_fail(legend, report, "legends require a categorical scale");
        return;
    }
    if (legend->scale->category_count == 0)
    {
        _legend_fail(legend, report, "legend scale has no retained categories");
        return;
    }

    float panel_x = 0.0f;
    float panel_y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    _scene_panel_pixel_rect(legend->panel, &panel_x, &panel_y, &width, &height);
    if (!(width > 0.0f) || !(height > 0.0f) || !isfinite(width) || !isfinite(height))
    {
        legend->realized_panel_width = 0.0f;
        legend->realized_panel_height = 0.0f;
        _legend_hide(legend);
        return;
    }
    legend->realized_panel_width = width;
    legend->realized_panel_height = height;
    if (!_legend_ensure_visuals(legend))
    {
        _legend_hide(legend);
        return;
    }

    float rect[4] = {0};
    _legend_content_rect(legend, panel_x, panel_y, width, height, rect);
    if (rect[0] < 0.0f || rect[1] < 0.0f || rect[2] > width || rect[3] > height ||
        rect[2] <= rect[0] || rect[3] <= rect[1])
    {
        _legend_fail(legend, report, "panel is too small for deterministic legend layout");
        return;
    }

    uint32_t sorted[DVZ_SCENE_MAX_SCALE_CATEGORIES] = {0};
    uint32_t count = _legend_sorted_category_indices(legend->scale, sorted);
    legend->entry_count = count;
    legend->text_count = 0;

    if (legend->title[0] != '\0')
    {
        _legend_append_text(
            legend, legend->title, rect[0], rect[1], 0.0f, 0.0f, COLORBAR_TITLE_TEXT_SIZE_PX);
    }

    float mark_positions[DVZ_SCENE_MAX_SCALE_CATEGORIES][3] = {{0}};
    DvzColor mark_colors[DVZ_SCENE_MAX_SCALE_CATEGORIES] = {{0}};
    float mark_sizes[DVZ_SCENE_MAX_SCALE_CATEGORIES] = {0};
    float mark_angles[DVZ_SCENE_MAX_SCALE_CATEGORIES] = {0};
    uint32_t mark_shapes[DVZ_SCENE_MAX_SCALE_CATEGORIES] = {0};
    float y = rect[1] + (legend->title[0] != '\0' ? 24.0f : 0.0f) + 0.5f * legend->mark_size_px;
    uint32_t mark_count = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzScaleCategoryState* category = &legend->scale->categories[sorted[i]];
        float mark_x = rect[0] + 0.5f * legend->mark_size_px;
        if (y + 0.5f * legend->mark_size_px > rect[3])
            break;
        _colorbar_pixel_to_visual(width, height, mark_x, y, 0.0f, mark_positions[mark_count]);
        mark_colors[mark_count] = category->color;
        mark_sizes[mark_count] = _legend_category_highlighted(legend, category->category_id) ?
                                     1.45f * legend->mark_size_px :
                                     legend->mark_size_px;
        mark_angles[mark_count] = 0.0f;
        mark_shapes[mark_count] = DVZ_MARKER_SHAPE_SQUARE;

        char fallback[32] = {0};
        const char* label = category->has_label ? category->label : fallback;
        if (!category->has_label)
            dvz_snprintf(fallback, sizeof(fallback), "%" PRId64, category->category_id);
        _legend_append_text(
            legend, label, rect[0] + legend->mark_size_px + legend->mark_label_gap_px, y, 0.0f,
            0.5f, COLORBAR_TICK_TEXT_SIZE_PX);
        y += legend->mark_size_px + legend->entry_gap_px;
        mark_count++;
    }

    DvzVisualDataUpdate updates[5] = {
        {.attr_name = "position", .data = mark_positions, .item_count = mark_count},
        {.attr_name = "color", .data = mark_colors, .item_count = mark_count},
        {.attr_name = "diameter", .data = mark_sizes, .item_count = mark_count},
        {.attr_name = "angle", .data = mark_angles, .item_count = mark_count},
        {.attr_name = "shape", .data = mark_shapes, .item_count = mark_count},
    };
    if (mark_count > 0 && dvz_visual_set_data_many(legend->mark_visual, updates, 5) == 0)
        dvz_visual_set_visible(legend->mark_visual, true);
    else
        dvz_visual_set_visible(legend->mark_visual, false);
    _legend_update_text(legend);
    legend->dirty = false;
}


/**
 * Return whether one retained legend needs its derived visuals rebuilt.
 *
 * @param legend the legend
 * @return whether the legend visual payloads need rebuilding
 */
static bool _legend_needs_visual_update(const DvzLegend* legend)
{
    ANN(legend);
    if (legend->dirty || legend->mark_visual == NULL || legend->text_visual == NULL)
        return true;
    if (legend->panel == NULL)
        return false;

    float panel_x = 0.0f;
    float panel_y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    _scene_panel_pixel_rect(legend->panel, &panel_x, &panel_y, &width, &height);
    (void)panel_x;
    (void)panel_y;
    if (!(width > 0.0f) || !(height > 0.0f) || !isfinite(width) || !isfinite(height))
        return true;
    return fabsf(width - legend->realized_panel_width) > COLORBAR_LAYOUT_EPS ||
           fabsf(height - legend->realized_panel_height) > COLORBAR_LAYOUT_EPS;
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
 * Sample a scene-owned colormap at a normalized coordinate.
 *
 * @param colormap the colormap, or NULL for grayscale fallback
 * @param t normalized scalar coordinate
 * @param out the output RGBA color
 * @return true when a color was written
 */
bool dvz_colormap_sample(const DvzColormap* colormap, double t, DvzColor* out)
{
    ANN(out);
    uint8_t rgba[4] = {0};
    const bool ok = _scene_color_from_colormap(colormap, t, rgba);
    *out = dvz_color_rgba(rgba[0], rgba[1], rgba[2], rgba[3]);
    return ok;
}



/**
 * Sample a built-in colormap at a normalized coordinate.
 *
 * @param builtin the built-in colormap selector
 * @param t normalized scalar coordinate
 * @param out the output RGBA color
 * @return true when a color was written
 */
bool dvz_colormap_builtin_sample(DvzBuiltinColormap builtin, double t, DvzColor* out)
{
    ANN(out);
    DvzColormap colormap = {
        .kind = DVZ_COLORMAP_CONTINUOUS,
        .builtin = builtin,
    };
    return dvz_colormap_sample(&colormap, t, out);
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
 * Replace retained categorical entries on a scale.
 *
 * @param scale the scale
 * @param categories category entry array, or NULL to clear
 * @param count the number of category entries
 * @return true when the category table was accepted
 */
bool dvz_scale_set_categories(
    DvzScale* scale, const DvzScaleCategory* categories, uint32_t count)
{
    ANN(scale);
    if (scale->kind != DVZ_SCALE_CATEGORICAL)
    {
        log_error("scale categories are valid only for categorical scales");
        return false;
    }
    if (categories == NULL || count == 0)
    {
        scale->category_count = 0;
        _scene_mark_scale_dirty(scale);
        return true;
    }
    if (count > DVZ_SCENE_MAX_SCALE_CATEGORIES)
    {
        log_error(
            "too many scale categories: %u > %u", count, DVZ_SCENE_MAX_SCALE_CATEGORIES);
        return false;
    }
    if (_scale_categories_have_duplicate_ids(categories, count))
    {
        log_error("duplicate scale category id");
        return false;
    }

    for (uint32_t i = 0; i < count; i++)
    {
        _scale_category_copy(&scale->categories[i], &categories[i]);
    }
    for (uint32_t i = count; i < scale->category_count; i++)
    {
        dvz_memset(
            &scale->categories[i], sizeof(DvzScaleCategoryState), 0,
            sizeof(DvzScaleCategoryState));
    }
    scale->category_count = count;
    _scene_mark_scale_dirty(scale);
    return true;
}


/**
 * Update or append retained categorical entries on a scale.
 *
 * @param scale the scale
 * @param categories category entry array
 * @param count the number of category entries
 * @return true when the category table was accepted
 */
bool dvz_scale_update_categories(
    DvzScale* scale, const DvzScaleCategory* categories, uint32_t count)
{
    ANN(scale);
    if (scale->kind != DVZ_SCALE_CATEGORICAL)
    {
        log_error("scale categories are valid only for categorical scales");
        return false;
    }
    if (categories == NULL || count == 0)
        return true;
    if (_scale_categories_have_duplicate_ids(categories, count))
    {
        log_error("duplicate scale category id");
        return false;
    }

    uint32_t append_count = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        if (_scale_category_index(scale, categories[i].category_id) < 0)
            append_count++;
    }
    if (append_count > DVZ_SCENE_MAX_SCALE_CATEGORIES - scale->category_count)
    {
        log_error(
            "too many scale categories: %u > %u", scale->category_count + append_count,
            DVZ_SCENE_MAX_SCALE_CATEGORIES);
        return false;
    }

    for (uint32_t i = 0; i < count; i++)
    {
        int32_t index = _scale_category_index(scale, categories[i].category_id);
        if (index < 0)
            index = (int32_t)scale->category_count++;
        _scale_category_copy(&scale->categories[(uint32_t)index], &categories[i]);
    }
    _scene_mark_scale_dirty(scale);
    return true;
}


/**
 * Remove retained categorical entries from a scale.
 *
 * @param scale the scale
 * @param ids category ids to remove
 * @param count the number of ids
 * @return true when the category table was updated
 */
bool dvz_scale_remove_categories(DvzScale* scale, const DvzCategoryId* ids, uint32_t count)
{
    ANN(scale);
    if (scale->kind != DVZ_SCALE_CATEGORICAL)
    {
        log_error("scale categories are valid only for categorical scales");
        return false;
    }
    if (ids == NULL || count == 0)
        return true;

    bool changed = false;
    for (uint32_t i = 0; i < count; i++)
    {
        int32_t index = _scale_category_index(scale, ids[i]);
        if (index < 0)
            continue;
        uint32_t ui = (uint32_t)index;
        for (uint32_t j = ui + 1; j < scale->category_count; j++)
            scale->categories[j - 1] = scale->categories[j];
        scale->category_count--;
        dvz_memset(
            &scale->categories[scale->category_count], sizeof(DvzScaleCategoryState), 0,
            sizeof(DvzScaleCategoryState));
        changed = true;
    }
    if (changed)
        _scene_mark_scale_dirty(scale);
    return true;
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
 * Create a panel-attached legend bound to a categorical scale.
 *
 * @param panel the panel
 * @param scale the categorical scale
 * @param desc the legend descriptor, or NULL for defaults
 * @return the legend, or NULL on allocation failure
 */
DvzLegend* dvz_legend(DvzPanel* panel, DvzScale* scale, const DvzLegendDesc* desc)
{
    ANN(panel);
    ANN(scale);
    if (panel->figure == NULL || panel->figure->scene == NULL)
    {
        log_error("cannot create a legend on a detached panel");
        return NULL;
    }
    DvzScene* scene = panel->figure->scene;
    if (scale->scene != scene)
    {
        log_error("cannot attach a scale from a different scene to a panel legend");
        return NULL;
    }
    if (scale->kind != DVZ_SCALE_CATEGORICAL)
    {
        log_error("legends require a categorical scale; use a colorbar for continuous scales");
        return NULL;
    }
    DvzLegendPlacementMode placement_mode =
        desc != NULL ? desc->placement_mode : DVZ_LEGEND_PLACEMENT_ATTACHED;
    DvzSceneAnchor anchor = desc != NULL && desc->anchor != DVZ_SCENE_ANCHOR_NONE ?
                                desc->anchor :
                                DVZ_SCENE_ANCHOR_PANEL_RIGHT;
    if (placement_mode == DVZ_LEGEND_PLACEMENT_ATTACHED && !_legend_anchor_supported(anchor))
    {
        log_error("attached legend anchor must be a panel edge");
        return NULL;
    }
    if (scene->legend_count >= DVZ_SCENE_MAX_LEGENDS)
    {
        log_error("maximum legend count reached");
        return NULL;
    }
    if (panel->legend_count >= DVZ_SCENE_MAX_PANEL_LEGENDS)
    {
        log_error("maximum panel legend count reached");
        return NULL;
    }

    DvzLegend* legend = &scene->legends[scene->legend_count++];
    dvz_memset(legend, sizeof(DvzLegend), 0, sizeof(DvzLegend));
    legend->scene = scene;
    legend->panel = panel;
    legend->scale = scale;
    legend->placement_mode = placement_mode;
    legend->anchor = anchor;
    legend->flags = desc != NULL ? desc->flags : 0;
    legend->reserve_px =
        _legend_positive_or_default(desc != NULL ? desc->reserve_px : 0.0f, LEGEND_RESERVE_PX);
    legend->edge_offset_px = _legend_positive_or_default(
        desc != NULL ? desc->edge_offset_px : 0.0f, LEGEND_EDGE_OFFSET_PX);
    legend->plot_gap_px = _legend_positive_or_default(
        desc != NULL ? desc->plot_gap_px : 0.0f, LEGEND_PLOT_GAP_PX);
    legend->entry_gap_px = _legend_positive_or_default(
        desc != NULL ? desc->entry_gap_px : 0.0f, LEGEND_ENTRY_GAP_PX);
    legend->mark_size_px = _legend_positive_or_default(
        desc != NULL ? desc->mark_size_px : 0.0f, LEGEND_MARK_SIZE_PX);
    legend->mark_label_gap_px = _legend_positive_or_default(
        desc != NULL ? desc->mark_label_gap_px : 0.0f, LEGEND_MARK_LABEL_GAP_PX);
    legend->text_renderer = _scene_adornment_text_renderer(
        desc != NULL && desc->text_renderer != 0 ? desc->text_renderer :
                                                   DVZ_TEXT_RENDERER_MSDF_ATLAS);
    legend->placement =
        desc != NULL ? desc->placement :
                       (DvzPlacement){
                           .space = DVZ_PLACEMENT_SPACE_PANEL,
                           .horizontal_anchor = DVZ_HORIZONTAL_ANCHOR_LEFT,
                           .vertical_anchor = DVZ_VERTICAL_ANCHOR_TOP,
                       };
    if (desc != NULL && desc->title != NULL)
        dvz_strlcpy(legend->title, desc->title, sizeof(legend->title));
    legend->dirty = true;
    legend->version = 1;
    panel->legends[panel->legend_count++] = legend;
    _legend_apply_auto_reserve(legend);
    return legend;
}


/**
 * Destroy a legend.
 *
 * @param legend the legend
 */
void dvz_legend_destroy(DvzLegend* legend)
{
    if (legend == NULL)
        return;
    _legend_hide(legend);
    DvzPanel* panel = legend->panel;
    if (panel != NULL)
    {
        for (uint32_t i = 0; i < panel->legend_count; i++)
        {
            if (panel->legends[i] != legend)
                continue;
            for (uint32_t j = i + 1; j < panel->legend_count; j++)
                panel->legends[j - 1] = panel->legends[j];
            panel->legends[panel->legend_count - 1] = NULL;
            panel->legend_count--;
            break;
        }
        _scene_panel_refresh_legend_reserve(panel);
    }
    legend->scene = NULL;
    legend->panel = NULL;
    legend->scale = NULL;
    legend->dirty = false;
    legend->mark_visual = NULL;
    legend->text_visual = NULL;
    legend->entry_count = 0;
}


/**
 * Update legend layout and placement parameters.
 *
 * @param legend the legend
 * @param desc layout descriptor
 * @return true when the layout was accepted
 */
bool dvz_legend_set_layout(DvzLegend* legend, const DvzLegendDesc* desc)
{
    ANN(legend);
    if (desc == NULL)
        return false;
    DvzLegendPlacementMode placement_mode = desc->placement_mode;
    DvzSceneAnchor anchor =
        desc->anchor != DVZ_SCENE_ANCHOR_NONE ? desc->anchor : DVZ_SCENE_ANCHOR_PANEL_RIGHT;
    if (placement_mode == DVZ_LEGEND_PLACEMENT_ATTACHED && !_legend_anchor_supported(anchor))
    {
        log_error("attached legend anchor must be a panel edge");
        return false;
    }
    legend->placement_mode = placement_mode;
    legend->anchor = anchor;
    legend->flags = desc->flags;
    legend->reserve_px = _legend_positive_or_default(desc->reserve_px, LEGEND_RESERVE_PX);
    legend->edge_offset_px =
        _legend_positive_or_default(desc->edge_offset_px, LEGEND_EDGE_OFFSET_PX);
    legend->plot_gap_px = _legend_positive_or_default(desc->plot_gap_px, LEGEND_PLOT_GAP_PX);
    legend->entry_gap_px = _legend_positive_or_default(desc->entry_gap_px, LEGEND_ENTRY_GAP_PX);
    legend->mark_size_px = _legend_positive_or_default(desc->mark_size_px, LEGEND_MARK_SIZE_PX);
    legend->mark_label_gap_px =
        _legend_positive_or_default(desc->mark_label_gap_px, LEGEND_MARK_LABEL_GAP_PX);
    legend->text_renderer = _scene_adornment_text_renderer(
        desc->text_renderer != 0 ? desc->text_renderer : DVZ_TEXT_RENDERER_MSDF_ATLAS);
    legend->placement = desc->placement;
    if (desc->title != NULL)
        dvz_strlcpy(legend->title, desc->title, sizeof(legend->title));
    _legend_apply_auto_reserve(legend);
    _scene_mark_legend_dirty(legend);
    return true;
}


/**
 * Set the legend title.
 *
 * @param legend the legend
 * @param title the title, or NULL to clear
 */
void dvz_legend_set_title(DvzLegend* legend, const char* title)
{
    ANN(legend);
    const char* src = title != NULL ? title : "";
    if (strcmp(legend->title, src) == 0)
        return;
    dvz_strlcpy(legend->title, src, sizeof(legend->title));
    _scene_mark_legend_dirty(legend);
}


/**
 * Highlight one categorical legend entry.
 *
 * @param legend the legend
 * @param id category id to highlight
 * @return true when the highlight state was accepted
 */
bool dvz_legend_set_highlight(DvzLegend* legend, DvzCategoryId id)
{
    return dvz_legend_set_highlights(legend, &id, 1);
}


/**
 * Clear all highlighted categorical legend entries.
 *
 * @param legend the legend
 * @return true when the highlight state was accepted
 */
bool dvz_legend_clear_highlight(DvzLegend* legend)
{
    ANN(legend);
    if (legend->highlight_count == 0)
        return true;
    dvz_memset(
        legend->highlighted_ids, sizeof(legend->highlighted_ids), 0,
        sizeof(legend->highlighted_ids));
    legend->highlight_count = 0;
    _scene_mark_legend_dirty(legend);
    return true;
}


/**
 * Highlight multiple categorical legend entries.
 *
 * @param legend the legend
 * @param ids category ids to highlight
 * @param count number of highlighted category ids
 * @return true when the highlight state was accepted
 */
bool dvz_legend_set_highlights(DvzLegend* legend, const DvzCategoryId* ids, uint32_t count)
{
    ANN(legend);
    if (count > DVZ_SCENE_MAX_SCALE_CATEGORIES)
    {
        log_error("too many legend highlights (%" PRIu32 " > %u)", count,
                  DVZ_SCENE_MAX_SCALE_CATEGORIES);
        return false;
    }
    if (count > 0 && ids == NULL)
    {
        log_error("legend highlight ids are required when count is nonzero");
        return false;
    }

    if (legend->highlight_count == count)
    {
        bool same = true;
        for (uint32_t i = 0; i < count; i++)
        {
            if (legend->highlighted_ids[i] != ids[i])
            {
                same = false;
                break;
            }
        }
        if (same)
            return true;
    }

    dvz_memset(
        legend->highlighted_ids, sizeof(legend->highlighted_ids), 0,
        sizeof(legend->highlighted_ids));
    if (count > 0)
        dvz_memcpy(
            legend->highlighted_ids, count * sizeof(DvzCategoryId), ids,
            count * sizeof(DvzCategoryId));
    legend->highlight_count = count;
    _scene_mark_legend_dirty(legend);
    return true;
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


/**
 * Rebuild all retained legend visuals before FramePlan emission.
 *
 * @param figure the figure
 * @param report optional diagnostic report
 */
void _scene_prepare_legend_visuals(DvzFigure* figure, DvzDiagnosticReport* report)
{
    if (figure == NULL || figure->scene == NULL)
        return;
    DvzScene* scene = figure->scene;
    for (uint32_t i = 0; i < scene->legend_count; i++)
    {
        DvzLegend* legend = &scene->legends[i];
        if (legend->scene != scene || legend->panel == NULL || legend->panel->figure != figure)
            continue;
        _legend_apply_auto_reserve(legend);
        if (_legend_needs_visual_update(legend))
            _legend_update_visuals(legend, report);
    }
}
