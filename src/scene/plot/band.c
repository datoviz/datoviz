/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene bands and ribbons                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_scene.h"
#include "core/scene_notify_internal.h"
#include "datoviz/scene.h"
#include "plot/internal.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_BAND_DESC_KNOWN_FLAGS 0u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _band_desc_validate(const DvzBandDesc* desc)
{
    if (desc == NULL)
        return false;
    if (!DVZ_STRUCT_VALID(desc, DvzBandDesc, DVZ_BAND_DESC_KNOWN_FLAGS))
    {
        log_error("invalid band descriptor ABI");
        return false;
    }
    if (!isfinite(desc->line_width_px) || desc->line_width_px < 0.0f)
    {
        log_error("invalid band line width");
        return false;
    }
    if (!isfinite(desc->bound_width_px) || desc->bound_width_px < 0.0f)
    {
        log_error("invalid band bound width");
        return false;
    }
    return true;
}



static bool _band_panel_valid(DvzPanel* panel, DvzScene** out_scene)
{
    if (out_scene != NULL)
        *out_scene = NULL;
    if (panel == NULL || panel->figure == NULL || panel->figure->scene == NULL)
        return false;
    if (out_scene != NULL)
        *out_scene = panel->figure->scene;
    return true;
}



static bool _band_scalar_valid_or_gap(double value)
{
    return isfinite(value) || isnan(value);
}



static bool _band_bounds_data_valid(
    uint32_t count, const double* x, const double* lower, const double* upper)
{
    if (count == 0)
        return true;
    if (x == NULL || lower == NULL || upper == NULL)
        return false;
    for (uint32_t i = 0; i < count; i++)
    {
        if (!_band_scalar_valid_or_gap(x[i]) || !_band_scalar_valid_or_gap(lower[i]) ||
            !_band_scalar_valid_or_gap(upper[i]))
            return false;
    }
    return true;
}



static bool _band_center_data_valid(uint32_t count, const double* x, const double* y)
{
    if (count == 0)
        return true;
    if (x == NULL || y == NULL)
        return false;
    for (uint32_t i = 0; i < count; i++)
    {
        if (!_band_scalar_valid_or_gap(x[i]) || !_band_scalar_valid_or_gap(y[i]))
            return false;
    }
    return true;
}



static bool _band_bounds_point_finite(const DvzBand* band, uint32_t i)
{
    ANN(band);
    return i < band->count && isfinite(band->x[i]) && isfinite(band->lower[i]) &&
           isfinite(band->upper[i]);
}



static bool _band_center_point_finite(
    const DvzBand* band, uint32_t i, const double* x, const double* y, bool derived)
{
    ANN(band);
    ANN(x);
    if (derived)
        return _band_bounds_point_finite(band, i);
    ANN(y);
    return i < band->center_count && isfinite(x[i]) && isfinite(y[i]);
}



static bool _band_copy_array(double** out, const double* src, uint32_t count)
{
    ANN(out);
    *out = NULL;
    if (count == 0)
        return true;

    double* copy = (double*)dvz_calloc(count, sizeof(double));
    if (copy == NULL)
        return false;
    dvz_memcpy(copy, (DvzSize)count * sizeof(double), src, (DvzSize)count * sizeof(double));
    *out = copy;
    return true;
}



static void _band_apply_visual_defaults(DvzVisual* visual, bool blended)
{
    ANN(visual);
    (void)dvz_visual_set_depth_test(visual, false);
    if (blended)
        (void)dvz_visual_set_alpha_mode(visual, DVZ_ALPHA_BLENDED);
}



static void _band_attach_visual(DvzPanel* panel, DvzVisual* visual, int32_t z_layer)
{
    ANN(panel);
    ANN(visual);
    DvzVisualAttachDesc attach = dvz_visual_attach_desc();
    attach.coord_space = DVZ_COORD_DATA;
    attach.z_layer = z_layer;
    if (dvz_panel_add_visual(panel, visual, &attach) != 0)
        log_error("failed to attach band visual");
}


static DvzVisual* _band_create_fill_visual(DvzScene* scene, const DvzBandDesc* desc)
{
    ANN(scene);
    ANN(desc);
    DvzVisual* fill = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    if (fill == NULL)
        return NULL;
    _band_apply_visual_defaults(fill, true);
    dvz_visual_set_visible(fill, false);
    return fill;
}



static DvzVisual* _band_create_line_visual(DvzScene* scene, const DvzBandDesc* desc)
{
    ANN(scene);
    ANN(desc);
    if (!desc->show_line || desc->line_width_px <= 0.0f || desc->line_color.a == 0)
        return NULL;
    DvzVisual* line = dvz_path(scene, 0);
    if (line == NULL)
        return NULL;
    _band_apply_visual_defaults(line, desc->line_color.a < 255);
    (void)dvz_path_set_caps(line, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND);
    (void)dvz_path_set_join(line, DVZ_PATH_JOIN_ROUND, 4.0f);
    dvz_visual_set_visible(line, false);
    return line;
}



static DvzVisual* _band_create_bounds_visual(DvzScene* scene, const DvzBandDesc* desc)
{
    ANN(scene);
    ANN(desc);
    if (!desc->show_bounds || desc->bound_width_px <= 0.0f || desc->bound_color.a == 0)
        return NULL;
    DvzVisual* bounds = dvz_path(scene, 0);
    if (bounds == NULL)
        return NULL;
    _band_apply_visual_defaults(bounds, desc->bound_color.a < 255);
    (void)dvz_path_set_caps(bounds, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND);
    (void)dvz_path_set_join(bounds, DVZ_PATH_JOIN_ROUND, 4.0f);
    dvz_visual_set_visible(bounds, false);
    return bounds;
}



static bool _band_upload_fill(DvzBand* band)
{
    ANN(band);
    ANN(band->fill_visual);

    if (band->count < 2)
    {
        dvz_visual_set_visible(band->fill_visual, false);
        return true;
    }
    if (band->count > UINT32_MAX / 6u + 1u)
        return false;

    const uint32_t max_vertex_count = 6u * (band->count - 1u);
    vec3* positions = (vec3*)dvz_calloc(max_vertex_count, sizeof(vec3));
    DvzColor* colors = (DvzColor*)dvz_calloc(max_vertex_count, sizeof(DvzColor));
    if (positions == NULL || colors == NULL)
    {
        dvz_free(colors);
        dvz_free(positions);
        return false;
    }

    uint32_t vertex_count = 0;
    for (uint32_t i = 0; i + 1u < band->count; i++)
    {
        if (!_band_bounds_point_finite(band, i) || !_band_bounds_point_finite(band, i + 1u))
            continue;

        const uint32_t k = vertex_count;
        positions[k + 0][0] = (float)band->x[i];
        positions[k + 0][1] = (float)band->lower[i];
        positions[k + 1][0] = (float)band->x[i + 1u];
        positions[k + 1][1] = (float)band->lower[i + 1u];
        positions[k + 2][0] = (float)band->x[i + 1u];
        positions[k + 2][1] = (float)band->upper[i + 1u];
        positions[k + 3][0] = (float)band->x[i];
        positions[k + 3][1] = (float)band->lower[i];
        positions[k + 4][0] = (float)band->x[i + 1u];
        positions[k + 4][1] = (float)band->upper[i + 1u];
        positions[k + 5][0] = (float)band->x[i];
        positions[k + 5][1] = (float)band->upper[i];
        for (uint32_t j = 0; j < 6u; j++)
            colors[k + j] = band->desc.fill_color;
        vertex_count += 6u;
    }

    if (vertex_count == 0)
    {
        dvz_visual_set_visible(band->fill_visual, false);
        dvz_free(colors);
        dvz_free(positions);
        return true;
    }

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = vertex_count},
        {.attr_name = "color", .data = colors, .item_count = vertex_count},
    };
    int rc = dvz_visual_set_data_many(band->fill_visual, updates, 2);
    dvz_visual_set_visible(band->fill_visual, true);
    dvz_free(colors);
    dvz_free(positions);
    return rc == 0;
}



static bool _band_upload_center_line(DvzBand* band)
{
    ANN(band);
    if (band->line_visual == NULL)
        return true;

    const bool derived = !band->has_center;
    const uint32_t source_count = derived ? band->count : band->center_count;
    const double* x = derived ? band->x : band->center_x;
    const double* y = derived ? NULL : band->center_y;
    if (source_count == 0)
    {
        dvz_visual_set_visible(band->line_visual, false);
        return true;
    }

    vec3* positions = (vec3*)dvz_calloc(source_count, sizeof(vec3));
    DvzColor* colors = (DvzColor*)dvz_calloc(source_count, sizeof(DvzColor));
    float* widths = (float*)dvz_calloc(source_count, sizeof(float));
    uint32_t* subpaths = (uint32_t*)dvz_calloc(source_count, sizeof(uint32_t));
    if (positions == NULL || colors == NULL || widths == NULL || subpaths == NULL)
    {
        dvz_free(subpaths);
        dvz_free(widths);
        dvz_free(colors);
        dvz_free(positions);
        return false;
    }

    uint32_t item_count = 0;
    uint32_t subpath_count = 0;
    uint32_t run = 0;
    for (uint32_t i = 0; i < source_count; i++)
    {
        if (!_band_center_point_finite(band, i, x, y, derived))
        {
            if (run > 0)
                subpaths[subpath_count++] = run;
            run = 0;
            continue;
        }
        positions[item_count][0] = (float)x[i];
        positions[item_count][1] =
            derived ? (float)(0.5 * (band->lower[i] + band->upper[i])) : (float)y[i];
        colors[item_count] = band->desc.line_color;
        widths[item_count] = band->desc.line_width_px;
        item_count++;
        run++;
    }
    if (run > 0)
        subpaths[subpath_count++] = run;

    if (item_count == 0 || subpath_count == 0)
    {
        dvz_visual_set_visible(band->line_visual, false);
        dvz_free(subpaths);
        dvz_free(widths);
        dvz_free(colors);
        dvz_free(positions);
        return true;
    }

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = item_count},
        {.attr_name = "color", .data = colors, .item_count = item_count},
        {.attr_name = "stroke_width", .data = widths, .item_count = item_count},
    };
    int rc = dvz_visual_set_data_many(band->line_visual, updates, 3);
    if (rc == 0)
        rc = dvz_path_set_subpaths(band->line_visual, subpath_count, subpaths);
    dvz_visual_set_visible(band->line_visual, true);
    dvz_free(subpaths);
    dvz_free(widths);
    dvz_free(colors);
    dvz_free(positions);
    return rc == 0;
}



static bool _band_upload_bound_lines(DvzBand* band)
{
    ANN(band);
    if (band->bounds_visual == NULL)
        return true;
    if (band->count == 0)
    {
        dvz_visual_set_visible(band->bounds_visual, false);
        return true;
    }
    if (band->count > UINT32_MAX / 2u)
        return false;

    const uint32_t max_count = 2u * band->count;
    vec3* positions = (vec3*)dvz_calloc(max_count, sizeof(vec3));
    DvzColor* colors = (DvzColor*)dvz_calloc(max_count, sizeof(DvzColor));
    float* widths = (float*)dvz_calloc(max_count, sizeof(float));
    uint32_t* subpaths = (uint32_t*)dvz_calloc(max_count, sizeof(uint32_t));
    if (positions == NULL || colors == NULL || widths == NULL || subpaths == NULL)
    {
        dvz_free(subpaths);
        dvz_free(widths);
        dvz_free(colors);
        dvz_free(positions);
        return false;
    }

    uint32_t item_count = 0;
    uint32_t subpath_count = 0;
    uint32_t i = 0;
    while (i < band->count)
    {
        while (i < band->count && !_band_bounds_point_finite(band, i))
            i++;
        const uint32_t start = i;
        while (i < band->count && _band_bounds_point_finite(band, i))
            i++;
        const uint32_t len = i - start;
        if (len == 0)
            continue;

        subpaths[subpath_count++] = len;
        for (uint32_t j = 0; j < len; j++)
        {
            const uint32_t idx = start + j;
            positions[item_count][0] = (float)band->x[idx];
            positions[item_count][1] = (float)band->lower[idx];
            colors[item_count] = band->desc.bound_color;
            widths[item_count] = band->desc.bound_width_px;
            item_count++;
        }

        subpaths[subpath_count++] = len;
        for (uint32_t j = 0; j < len; j++)
        {
            const uint32_t idx = start + j;
            positions[item_count][0] = (float)band->x[idx];
            positions[item_count][1] = (float)band->upper[idx];
            colors[item_count] = band->desc.bound_color;
            widths[item_count] = band->desc.bound_width_px;
            item_count++;
        }
    }

    if (item_count == 0 || subpath_count == 0)
    {
        dvz_visual_set_visible(band->bounds_visual, false);
        dvz_free(subpaths);
        dvz_free(widths);
        dvz_free(colors);
        dvz_free(positions);
        return true;
    }

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = item_count},
        {.attr_name = "color", .data = colors, .item_count = item_count},
        {.attr_name = "stroke_width", .data = widths, .item_count = item_count},
    };
    int rc = dvz_visual_set_data_many(band->bounds_visual, updates, 3);
    if (rc == 0)
        rc = dvz_path_set_subpaths(band->bounds_visual, subpath_count, subpaths);
    dvz_visual_set_visible(band->bounds_visual, true);
    dvz_free(subpaths);
    dvz_free(widths);
    dvz_free(colors);
    dvz_free(positions);
    return rc == 0;
}



/*************************************************************************************************/
/*  Public API                                                                                   */
/*************************************************************************************************/

DvzBandDesc dvz_band_desc(void)
{
    return (DvzBandDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzBandDesc),
        .fill_color = {128, 255, 219, 60},
        .line_color = {76, 201, 240, 255},
        .line_width_px = 4.0f,
        .show_line = true,
        .show_bounds = false,
        .bound_color = {128, 255, 219, 150},
        .bound_width_px = 1.5f,
    };
}



DvzBand* dvz_band(DvzPanel* panel, const DvzBandDesc* desc)
{
    DvzScene* scene = NULL;
    if (!_band_panel_valid(panel, &scene))
        return NULL;

    DvzBandDesc resolved = desc != NULL ? *desc : dvz_band_desc();
    if (!_band_desc_validate(&resolved))
        return NULL;
    if (scene->band_count >= DVZ_SCENE_MAX_BANDS)
    {
        log_error("maximum band count reached");
        return NULL;
    }

    DvzVisual* fill = _band_create_fill_visual(scene, &resolved);
    if (fill == NULL)
        return NULL;

    DvzVisual* line = _band_create_line_visual(scene, &resolved);
    if (resolved.show_line && resolved.line_width_px > 0.0f && resolved.line_color.a > 0 &&
        line == NULL)
        return NULL;

    DvzVisual* bounds = _band_create_bounds_visual(scene, &resolved);
    if (resolved.show_bounds && resolved.bound_width_px > 0.0f && resolved.bound_color.a > 0 &&
        bounds == NULL)
        return NULL;

    DvzBand* band = &scene->bands[scene->band_count++];
    dvz_memset(band, sizeof(DvzBand), 0, sizeof(DvzBand));
    band->scene = scene;
    band->panel = panel;
    band->desc = resolved;
    band->active = true;
    band->dirty = true;
    band->version = 1;
    band->fill_visual = fill;
    band->line_visual = line;
    band->bounds_visual = bounds;

    _band_attach_visual(panel, fill, resolved.z_layer);
    if (line != NULL)
        _band_attach_visual(panel, line, resolved.z_layer + 1);
    if (bounds != NULL)
        _band_attach_visual(panel, bounds, resolved.z_layer + 2);
    _scene_notify_request_frame(panel->figure);
    return band;
}



int dvz_band_set_bounds(
    DvzBand* band, uint32_t count, const double* x, const double* lower, const double* upper)
{
    if (band == NULL || !band->active)
        return -1;
    if (!_band_bounds_data_valid(count, x, lower, upper))
    {
        log_error("invalid band bounds data");
        return -1;
    }

    double* new_x = NULL;
    double* new_lower = NULL;
    double* new_upper = NULL;
    bool ok = _band_copy_array(&new_x, x, count) && _band_copy_array(&new_lower, lower, count) &&
              _band_copy_array(&new_upper, upper, count);
    if (!ok)
    {
        dvz_free(new_upper);
        dvz_free(new_lower);
        dvz_free(new_x);
        return -1;
    }

    dvz_free(band->x);
    dvz_free(band->lower);
    dvz_free(band->upper);
    band->x = new_x;
    band->lower = new_lower;
    band->upper = new_upper;
    band->count = count;
    band->dirty = true;
    band->version++;
    _scene_notify_request_frame(band->panel != NULL ? band->panel->figure : NULL);
    return 0;
}



int dvz_band_set_center(DvzBand* band, uint32_t count, const double* x, const double* y)
{
    if (band == NULL || !band->active)
        return -1;
    if (!_band_center_data_valid(count, x, y))
    {
        log_error("invalid band center data");
        return -1;
    }

    double* new_x = NULL;
    double* new_y = NULL;
    bool ok = _band_copy_array(&new_x, x, count) && _band_copy_array(&new_y, y, count);
    if (!ok)
    {
        dvz_free(new_y);
        dvz_free(new_x);
        return -1;
    }

    dvz_free(band->center_x);
    dvz_free(band->center_y);
    band->center_x = new_x;
    band->center_y = new_y;
    band->center_count = count;
    band->has_center = count > 0;
    band->dirty = true;
    band->version++;
    _scene_notify_request_frame(band->panel != NULL ? band->panel->figure : NULL);
    return 0;
}


int dvz_band_set_style(DvzBand* band, const DvzBandDesc* desc)
{
    if (band == NULL || !band->active || desc == NULL)
        return -1;
    DvzBandDesc resolved = *desc;
    if (!_band_desc_validate(&resolved))
        return -1;

    if (band->fill_visual != NULL)
        _band_apply_visual_defaults(band->fill_visual, true);

    const bool wants_line =
        resolved.show_line && resolved.line_width_px > 0.0f && resolved.line_color.a > 0;
    if (wants_line && band->line_visual == NULL)
    {
        band->line_visual = _band_create_line_visual(band->scene, &resolved);
        if (band->line_visual == NULL)
            return -1;
        _band_attach_visual(band->panel, band->line_visual, resolved.z_layer + 1);
    }
    if (band->line_visual != NULL)
    {
        if (wants_line)
            _band_apply_visual_defaults(band->line_visual, resolved.line_color.a < 255);
        else
            dvz_visual_set_visible(band->line_visual, false);
    }

    const bool wants_bounds =
        resolved.show_bounds && resolved.bound_width_px > 0.0f && resolved.bound_color.a > 0;
    if (wants_bounds && band->bounds_visual == NULL)
    {
        band->bounds_visual = _band_create_bounds_visual(band->scene, &resolved);
        if (band->bounds_visual == NULL)
            return -1;
        _band_attach_visual(band->panel, band->bounds_visual, resolved.z_layer + 2);
    }
    if (band->bounds_visual != NULL)
    {
        if (wants_bounds)
            _band_apply_visual_defaults(band->bounds_visual, resolved.bound_color.a < 255);
        else
            dvz_visual_set_visible(band->bounds_visual, false);
    }

    band->desc = resolved;
    band->dirty = true;
    band->version++;
    _scene_notify_request_frame(band->panel != NULL ? band->panel->figure : NULL);
    return 0;
}



/*************************************************************************************************/
/*  Internal API                                                                                 */
/*************************************************************************************************/

void _scene_band_reset(DvzBand* band)
{
    if (band == NULL)
        return;
    dvz_free(band->x);
    dvz_free(band->lower);
    dvz_free(band->upper);
    dvz_free(band->center_x);
    dvz_free(band->center_y);
    band->x = NULL;
    band->lower = NULL;
    band->upper = NULL;
    band->center_x = NULL;
    band->center_y = NULL;
    band->count = 0;
    band->center_count = 0;
    band->has_center = false;
    band->scene = NULL;
    band->panel = NULL;
    band->active = false;
}



void _scene_prepare_band_visuals(DvzFigure* figure)
{
    if (figure == NULL || figure->scene == NULL)
        return;

    DvzScene* scene = figure->scene;
    for (uint32_t i = 0; i < scene->band_count; i++)
    {
        DvzBand* band = &scene->bands[i];
        if (!band->active || band->panel == NULL || band->panel->figure != figure)
            continue;
        if (!_band_upload_fill(band) || !_band_upload_center_line(band) ||
            !_band_upload_bound_lines(band))
            log_error("failed to prepare band visual %u", i);
        band->dirty = false;
    }
}
