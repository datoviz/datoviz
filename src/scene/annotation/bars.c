/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene bars and interval series                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <math.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_scene.h"
#include "core/scene_notify_internal.h"
#include "datoviz/scene.h"
#include "prepare_internal.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_BARS_DESC_KNOWN_FLAGS 0u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _bars_orientation_valid(DvzBarsOrientation orientation)
{
    return orientation == DVZ_BARS_ORIENTATION_VERTICAL ||
           orientation == DVZ_BARS_ORIENTATION_HORIZONTAL;
}



static bool _bars_desc_validate(const DvzBarsDesc* desc)
{
    if (desc == NULL)
        return false;
    if (!DVZ_STRUCT_VALID(desc, DvzBarsDesc, DVZ_BARS_DESC_KNOWN_FLAGS))
    {
        log_error("invalid bars descriptor ABI");
        return false;
    }
    if (!_bars_orientation_valid(desc->orientation))
    {
        log_error("invalid bars orientation");
        return false;
    }
    if (!isfinite(desc->baseline))
    {
        log_error("invalid bars baseline");
        return false;
    }
    if (!isfinite(desc->gap_fraction) || desc->gap_fraction < 0.0f || desc->gap_fraction >= 1.0f)
    {
        log_error("invalid bars gap fraction");
        return false;
    }
    if (!isfinite(desc->outline_width_px) || desc->outline_width_px < 0.0f)
    {
        log_error("invalid bars outline width");
        return false;
    }
    return true;
}



static bool _bars_panel_valid(DvzPanel* panel, DvzScene** out_scene)
{
    if (out_scene != NULL)
        *out_scene = NULL;
    if (panel == NULL || panel->figure == NULL || panel->figure->scene == NULL)
        return false;
    if (out_scene != NULL)
        *out_scene = panel->figure->scene;
    return true;
}



static void _bars_apply_visual_defaults(DvzVisual* visual, bool blended)
{
    ANN(visual);
    (void)dvz_visual_set_depth_test(visual, false);
    if (blended)
        (void)dvz_visual_set_alpha_mode(visual, DVZ_ALPHA_BLENDED);
}



static void _bars_attach_visual(DvzPanel* panel, DvzVisual* visual, int32_t z_layer)
{
    ANN(panel);
    ANN(visual);
    DvzVisualAttachDesc attach = dvz_visual_attach_desc();
    attach.coord_space = DVZ_COORD_DATA;
    attach.z_layer = z_layer;
    if (dvz_panel_add_visual(panel, visual, &attach) != 0)
        log_error("failed to attach bars visual");
}



static bool _bars_interval_data_valid(
    uint32_t count, const double* starts, const double* ends, const double* values)
{
    if (count == 0)
        return true;
    if (starts == NULL || ends == NULL || values == NULL)
        return false;
    for (uint32_t i = 0; i < count; i++)
    {
        if (!isfinite(starts[i]) || !isfinite(ends[i]) || !isfinite(values[i]))
            return false;
        if (fabs(ends[i] - starts[i]) <= DBL_EPSILON)
            return false;
    }
    return true;
}



static bool _bars_set_copy(double** dst, const double* src, uint32_t count)
{
    ANN(dst);
    dvz_free(*dst);
    *dst = NULL;
    if (count == 0)
        return true;

    double* copy = (double*)dvz_calloc(count, sizeof(double));
    if (copy == NULL)
        return false;
    dvz_memcpy(copy, (DvzSize)count * sizeof(double), src, (DvzSize)count * sizeof(double));
    *dst = copy;
    return true;
}



static bool _bars_upload_fill(DvzBars* bars)
{
    ANN(bars);
    ANN(bars->fill_visual);

    if (bars->count == 0)
    {
        dvz_visual_set_visible(bars->fill_visual, false);
        if (bars->outline_visual != NULL)
            dvz_visual_set_visible(bars->outline_visual, false);
        return true;
    }
    if (bars->count > UINT32_MAX / 6u)
        return false;

    const uint32_t vertex_count = bars->count * 6u;
    vec3* positions = (vec3*)dvz_calloc(vertex_count, sizeof(vec3));
    DvzColor* colors = (DvzColor*)dvz_calloc(vertex_count, sizeof(DvzColor));
    if (positions == NULL || colors == NULL)
    {
        dvz_free(positions);
        dvz_free(colors);
        return false;
    }

    for (uint32_t i = 0; i < bars->count; i++)
    {
        double a = bars->starts[i];
        double b = bars->ends[i];
        if (b < a)
        {
            double tmp = a;
            a = b;
            b = tmp;
        }
        const double shrink = 0.5 * (double)bars->desc.gap_fraction * (b - a);
        a += shrink;
        b -= shrink;

        double x0 = a, x1 = b, y0 = bars->desc.baseline, y1 = bars->values[i];
        if (bars->desc.orientation == DVZ_BARS_ORIENTATION_HORIZONTAL)
        {
            x0 = bars->desc.baseline;
            x1 = bars->values[i];
            y0 = a;
            y1 = b;
        }

        const uint32_t k = 6u * i;
        positions[k + 0][0] = (float)x0;
        positions[k + 0][1] = (float)y0;
        positions[k + 1][0] = (float)x1;
        positions[k + 1][1] = (float)y0;
        positions[k + 2][0] = (float)x1;
        positions[k + 2][1] = (float)y1;
        positions[k + 3][0] = (float)x0;
        positions[k + 3][1] = (float)y0;
        positions[k + 4][0] = (float)x1;
        positions[k + 4][1] = (float)y1;
        positions[k + 5][0] = (float)x0;
        positions[k + 5][1] = (float)y1;
        for (uint32_t j = 0; j < 6; j++)
            colors[k + j] = bars->desc.fill_color;
    }

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = vertex_count},
        {.attr_name = "color", .data = colors, .item_count = vertex_count},
    };
    int rc = dvz_visual_set_data_many(bars->fill_visual, updates, 2);
    dvz_visual_set_visible(bars->fill_visual, true);
    dvz_free(colors);
    dvz_free(positions);
    return rc == 0;
}



static bool _bars_upload_outline(DvzBars* bars)
{
    ANN(bars);
    if (bars->outline_visual == NULL)
        return true;
    if (bars->count == 0)
    {
        dvz_visual_set_visible(bars->outline_visual, false);
        return true;
    }
    if (bars->count > UINT32_MAX / 4u)
        return false;

    const uint32_t segment_count = bars->count * 4u;
    vec3* starts = (vec3*)dvz_calloc(segment_count, sizeof(vec3));
    vec3* ends = (vec3*)dvz_calloc(segment_count, sizeof(vec3));
    DvzColor* colors = (DvzColor*)dvz_calloc(segment_count, sizeof(DvzColor));
    float* widths = (float*)dvz_calloc(segment_count, sizeof(float));
    if (starts == NULL || ends == NULL || colors == NULL || widths == NULL)
    {
        dvz_free(widths);
        dvz_free(colors);
        dvz_free(ends);
        dvz_free(starts);
        return false;
    }

    for (uint32_t i = 0; i < bars->count; i++)
    {
        double a = bars->starts[i];
        double b = bars->ends[i];
        if (b < a)
        {
            double tmp = a;
            a = b;
            b = tmp;
        }
        const double shrink = 0.5 * (double)bars->desc.gap_fraction * (b - a);
        a += shrink;
        b -= shrink;

        double x0 = a, x1 = b, y0 = bars->desc.baseline, y1 = bars->values[i];
        if (bars->desc.orientation == DVZ_BARS_ORIENTATION_HORIZONTAL)
        {
            x0 = bars->desc.baseline;
            x1 = bars->values[i];
            y0 = a;
            y1 = b;
        }

        const uint32_t k = 4u * i;
        starts[k + 0][0] = (float)x0;
        starts[k + 0][1] = (float)y0;
        ends[k + 0][0] = (float)x1;
        ends[k + 0][1] = (float)y0;

        starts[k + 1][0] = (float)x1;
        starts[k + 1][1] = (float)y0;
        ends[k + 1][0] = (float)x1;
        ends[k + 1][1] = (float)y1;

        starts[k + 2][0] = (float)x1;
        starts[k + 2][1] = (float)y1;
        ends[k + 2][0] = (float)x0;
        ends[k + 2][1] = (float)y1;

        starts[k + 3][0] = (float)x0;
        starts[k + 3][1] = (float)y1;
        ends[k + 3][0] = (float)x0;
        ends[k + 3][1] = (float)y0;

        for (uint32_t j = 0; j < 4; j++)
        {
            colors[k + j] = bars->desc.outline_color;
            widths[k + j] = bars->desc.outline_width_px;
        }
    }

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position_start", .data = starts, .item_count = segment_count},
        {.attr_name = "position_end", .data = ends, .item_count = segment_count},
        {.attr_name = "color", .data = colors, .item_count = segment_count},
        {.attr_name = "stroke_width", .data = widths, .item_count = segment_count},
    };
    int rc = dvz_visual_set_data_many(bars->outline_visual, updates, 4);
    dvz_visual_set_visible(bars->outline_visual, true);
    dvz_free(widths);
    dvz_free(colors);
    dvz_free(ends);
    dvz_free(starts);
    return rc == 0;
}



/*************************************************************************************************/
/*  Public API                                                                                   */
/*************************************************************************************************/

DvzBarsDesc dvz_bars_desc(void)
{
    return (DvzBarsDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzBarsDesc),
        .orientation = DVZ_BARS_ORIENTATION_VERTICAL,
        .baseline = 0.0,
        .gap_fraction = 0.0f,
        .fill_color = {255, 255, 255, 220},
        .outline_color = {255, 255, 255, 220},
        .outline_width_px = 0.0f,
    };
}



DvzBars* dvz_bars(DvzPanel* panel, const DvzBarsDesc* desc)
{
    DvzScene* scene = NULL;
    if (!_bars_panel_valid(panel, &scene))
        return NULL;

    DvzBarsDesc resolved = desc != NULL ? *desc : dvz_bars_desc();
    if (!_bars_desc_validate(&resolved))
        return NULL;
    if (scene->bars_count >= DVZ_SCENE_MAX_BARS)
    {
        log_error("maximum bars count reached");
        return NULL;
    }

    DvzVisual* fill = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    if (fill == NULL)
        return NULL;
    _bars_apply_visual_defaults(fill, resolved.fill_color.a < 255);
    dvz_visual_set_visible(fill, false);

    DvzVisual* outline = NULL;
    if (resolved.outline_width_px > 0.0f && resolved.outline_color.a > 0)
    {
        outline = dvz_segment(scene, 0);
        if (outline == NULL)
            return NULL;
        (void)dvz_segment_set_caps(outline, DVZ_SEGMENT_CAP_BUTT, DVZ_SEGMENT_CAP_BUTT);
        _bars_apply_visual_defaults(outline, resolved.outline_color.a < 255);
        dvz_visual_set_visible(outline, false);
    }

    DvzBars* bars = &scene->bars[scene->bars_count++];
    dvz_memset(bars, sizeof(DvzBars), 0, sizeof(DvzBars));
    bars->scene = scene;
    bars->panel = panel;
    bars->desc = resolved;
    bars->active = true;
    bars->dirty = true;
    bars->version = 1;
    bars->fill_visual = fill;
    bars->outline_visual = outline;

    _bars_attach_visual(panel, fill, resolved.z_layer);
    if (outline != NULL)
        _bars_attach_visual(panel, outline, resolved.z_layer + 1);
    _scene_notify_request_frame(panel->figure);
    return bars;
}



int dvz_bars_set_intervals(
    DvzBars* bars, uint32_t count, const double* starts, const double* ends,
    const double* values)
{
    if (bars == NULL || !bars->active)
        return -1;
    if (!_bars_interval_data_valid(count, starts, ends, values))
    {
        log_error("invalid bars interval data");
        return -1;
    }

    double* old_starts = bars->starts;
    double* old_ends = bars->ends;
    double* old_values = bars->values;
    bars->starts = NULL;
    bars->ends = NULL;
    bars->values = NULL;
    bool ok = _bars_set_copy(&bars->starts, starts, count) &&
              _bars_set_copy(&bars->ends, ends, count) &&
              _bars_set_copy(&bars->values, values, count);
    if (!ok)
    {
        dvz_free(bars->starts);
        dvz_free(bars->ends);
        dvz_free(bars->values);
        bars->starts = old_starts;
        bars->ends = old_ends;
        bars->values = old_values;
        return -1;
    }

    dvz_free(old_starts);
    dvz_free(old_ends);
    dvz_free(old_values);
    bars->count = count;
    bars->dirty = true;
    bars->version++;
    _scene_notify_request_frame(bars->panel != NULL ? bars->panel->figure : NULL);
    return 0;
}



/*************************************************************************************************/
/*  Internal API                                                                                 */
/*************************************************************************************************/

void _scene_bars_reset(DvzBars* bars)
{
    if (bars == NULL)
        return;
    dvz_free(bars->starts);
    dvz_free(bars->ends);
    dvz_free(bars->values);
    bars->starts = NULL;
    bars->ends = NULL;
    bars->values = NULL;
    bars->count = 0;
    bars->scene = NULL;
    bars->panel = NULL;
    bars->active = false;
}



void _scene_prepare_bars_visuals(DvzFigure* figure)
{
    if (figure == NULL || figure->scene == NULL)
        return;

    DvzScene* scene = figure->scene;
    for (uint32_t i = 0; i < scene->bars_count; i++)
    {
        DvzBars* bars = &scene->bars[i];
        if (!bars->active || bars->panel == NULL || bars->panel->figure != figure)
            continue;
        if (!_bars_upload_fill(bars) || !_bars_upload_outline(bars))
            log_error("failed to prepare bars visual %u", i);
        bars->dirty = false;
    }
}
