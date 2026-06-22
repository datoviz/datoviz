/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene reference grid                                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_scene.h"
#include "core/scene_notify_internal.h"
#include "datoviz/ffi.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_REFERENCE_GRID_DESC_KNOWN_FLAGS 0u
#define DVZ_REFERENCE_GRID_MAX_INTERVALS    4096u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Validate one reference-grid descriptor.
 *
 * @param desc descriptor, or NULL
 * @return whether the descriptor is valid
 */
static bool _reference_grid_desc_validate(const DvzReferenceGridDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzReferenceGridDesc, DVZ_REFERENCE_GRID_DESC_KNOWN_FLAGS))
    {
        log_error("invalid reference grid descriptor ABI");
        return false;
    }
    return true;
}


/**
 * Normalize one vector in place.
 *
 * @param v vector to normalize
 * @return whether the vector had non-zero finite length
 */
static bool _reference_grid_normalize(vec3 v)
{
    const float length = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (!isfinite(length) || length <= 0.0f)
        return false;
    v[0] /= length;
    v[1] /= length;
    v[2] /= length;
    return true;
}


/**
 * Resolve the grid plane basis vectors.
 *
 * @param desc descriptor
 * @param out_u output U axis
 * @param out_v output V axis
 * @return whether the basis is valid
 */
static bool _reference_grid_basis(const DvzReferenceGridDesc* desc, vec3 out_u, vec3 out_v)
{
    ANN(desc);
    ANN(out_u);
    ANN(out_v);
    switch (desc->plane)
    {
    case DVZ_REFERENCE_GRID_XY:
        out_u[0] = 1.0f;
        out_v[1] = 1.0f;
        break;

    case DVZ_REFERENCE_GRID_XZ:
        out_u[0] = 1.0f;
        out_v[2] = 1.0f;
        break;

    case DVZ_REFERENCE_GRID_YZ:
        out_u[1] = 1.0f;
        out_v[2] = 1.0f;
        break;

    case DVZ_REFERENCE_GRID_CUSTOM:
        out_u[0] = desc->axis_u[0];
        out_u[1] = desc->axis_u[1];
        out_u[2] = desc->axis_u[2];
        out_v[0] = desc->axis_v[0];
        out_v[1] = desc->axis_v[1];
        out_v[2] = desc->axis_v[2];
        break;

    default:
        return false;
    }
    return _reference_grid_normalize(out_u) && _reference_grid_normalize(out_v);
}


/**
 * Return the integer lattice range for one grid axis.
 *
 * @param size axis size
 * @param spacing grid spacing
 * @param first output first lattice index
 * @param last output last lattice index
 * @return whether the lattice range is valid
 */
static bool _reference_grid_lattice_range(
    float size, float spacing, int32_t* first, int32_t* last)
{
    ANN(first);
    ANN(last);
    if (!isfinite(size) || !isfinite(spacing) || size <= 0.0f || spacing <= 0.0f)
        return false;
    const float half = 0.5f * size;
    const float first_f = ceilf(-half / spacing);
    const float last_f = floorf(+half / spacing);
    if (!isfinite(first_f) || !isfinite(last_f) || last_f < first_f)
        return false;
    const float count_f = last_f - first_f + 1.0f;
    if (!isfinite(count_f) || count_f < 1.0f ||
        count_f > (float)(DVZ_REFERENCE_GRID_MAX_INTERVALS + 1u))
        return false;
    *first = (int32_t)first_f;
    *last = (int32_t)last_f;
    return true;
}


/**
 * Append one line to grid arrays.
 *
 * @param start line start array
 * @param end line end array
 * @param color line color array
 * @param width line width array
 * @param count line count in/out
 * @param p0 start point
 * @param p1 end point
 * @param c line color
 * @param w line width
 */
static void _reference_grid_append_line(
    vec3* start, vec3* end, DvzColor* color, float* width, uint32_t* count, const vec3 p0,
    const vec3 p1, DvzColor c, float w)
{
    ANN(start);
    ANN(end);
    ANN(color);
    ANN(width);
    ANN(count);
    const uint32_t i = *count;
    start[i][0] = p0[0];
    start[i][1] = p0[1];
    start[i][2] = p0[2];
    end[i][0] = p1[0];
    end[i][1] = p1[1];
    end[i][2] = p1[2];
    color[i] = c;
    width[i] = w;
    *count = i + 1;
}


/**
 * Return whether one grid coordinate lies on a major interval from the grid origin.
 *
 * @param coord plane-local coordinate relative to the grid origin
 * @param step major-grid step
 * @return whether the coordinate is major
 */
static bool _reference_grid_coord_is_major(float coord, float step)
{
    if (!isfinite(coord) || !isfinite(step) || step <= 0.0f)
        return false;
    const float nearest = roundf(coord / step);
    const float distance = fabsf(coord - nearest * step);
    const float tolerance = 1e-5f * fmaxf(1.0f, fabsf(step));
    return distance <= tolerance;
}



/**
 * Return whether one grid coordinate lies on the grid origin.
 *
 * @param coord plane-local coordinate relative to the grid origin
 * @param spacing grid spacing
 * @return whether the coordinate is the origin line
 */
static bool _reference_grid_coord_is_origin(float coord, float spacing)
{
    if (!isfinite(coord) || !isfinite(spacing) || spacing <= 0.0f)
        return false;
    const float tolerance = 1e-5f * fmaxf(1.0f, fabsf(spacing));
    return fabsf(coord) <= tolerance;
}



/**
 * Return style for one grid line.
 *
 * @param desc descriptor
 * @param coord plane-local coordinate relative to the grid origin
 * @param out_color output color
 * @param out_width output width
 * @return whether this line should be emitted
 */
static bool _reference_grid_line_style(
    const DvzReferenceGridDesc* desc, float coord, DvzColor* out_color, float* out_width)
{
    ANN(desc);
    ANN(out_color);
    ANN(out_width);
    const float major_step = desc->spacing * (float)desc->major_every;
    const bool major =
        desc->show_major && desc->major_every > 0 &&
        _reference_grid_coord_is_major(coord, major_step);
    const bool axis =
        desc->show_axes && major && _reference_grid_coord_is_origin(coord, desc->spacing);

    if (axis)
    {
        *out_color = desc->axis_color;
        *out_width = desc->axis_width_px;
        return true;
    }
    if (major)
    {
        *out_color = desc->major_color;
        *out_width = desc->major_width_px;
        return true;
    }
    if (desc->show_minor)
    {
        *out_color = desc->minor_color;
        *out_width = desc->minor_width_px;
        return true;
    }
    return false;
}


/**
 * Rebuild grid segment data from one descriptor.
 *
 * @param grid reference grid
 * @return whether the visual was updated
 */
static bool _reference_grid_rebuild(DvzReferenceGrid* grid)
{
    ANN(grid);
    ANN(grid->visual);
    DvzReferenceGridDesc* desc = &grid->desc;

    int32_t first_u = 0;
    int32_t last_u = 0;
    int32_t first_v = 0;
    int32_t last_v = 0;
    if (!_reference_grid_lattice_range(desc->size[0], desc->spacing, &first_u, &last_u) ||
        !_reference_grid_lattice_range(desc->size[1], desc->spacing, &first_v, &last_v))
        return false;

    vec3 axis_u = {0};
    vec3 axis_v = {0};
    if (!_reference_grid_basis(desc, axis_u, axis_v))
        return false;

    const uint32_t count_u = (uint32_t)(last_u - first_u + 1);
    const uint32_t count_v = (uint32_t)(last_v - first_v + 1);
    const uint32_t capacity = count_u + count_v;
    vec3* starts = (vec3*)dvz_calloc(capacity, sizeof(vec3));
    vec3* ends = (vec3*)dvz_calloc(capacity, sizeof(vec3));
    DvzColor* colors = (DvzColor*)dvz_calloc(capacity, sizeof(DvzColor));
    float* widths = (float*)dvz_calloc(capacity, sizeof(float));
    if (starts == NULL || ends == NULL || colors == NULL || widths == NULL)
        goto fail;

    uint32_t count = 0;
    const float half_u = 0.5f * desc->size[0];
    const float half_v = 0.5f * desc->size[1];

    for (int32_t i = first_u; i <= last_u; i++)
    {
        const float u = desc->spacing * (float)i;
        DvzColor color = {0};
        float width = 0.0f;
        if (!_reference_grid_line_style(desc, u, &color, &width))
            continue;

        vec3 p0 = {0};
        vec3 p1 = {0};
        for (uint32_t d = 0; d < 3; d++)
        {
            p0[d] = desc->origin[d] + u * axis_u[d] - half_v * axis_v[d];
            p1[d] = desc->origin[d] + u * axis_u[d] + half_v * axis_v[d];
        }
        _reference_grid_append_line(starts, ends, colors, widths, &count, p0, p1, color, width);
    }

    for (int32_t i = first_v; i <= last_v; i++)
    {
        const float v = desc->spacing * (float)i;
        DvzColor color = {0};
        float width = 0.0f;
        if (!_reference_grid_line_style(desc, v, &color, &width))
            continue;

        vec3 p0 = {0};
        vec3 p1 = {0};
        for (uint32_t d = 0; d < 3; d++)
        {
            p0[d] = desc->origin[d] - half_u * axis_u[d] + v * axis_v[d];
            p1[d] = desc->origin[d] + half_u * axis_u[d] + v * axis_v[d];
        }
        _reference_grid_append_line(starts, ends, colors, widths, &count, p0, p1, color, width);
    }

    if (count == 0)
        goto fail;

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position_start", .data = starts, .item_count = count},
        {.attr_name = "position_end", .data = ends, .item_count = count},
        {.attr_name = "color", .data = colors, .item_count = count},
        {.attr_name = "stroke_width", .data = widths, .item_count = count},
    };
    const bool ok = dvz_visual_set_data_many(grid->visual, updates, 4) == 0;
    grid->line_count = ok ? count : 0;
    dvz_free(starts);
    dvz_free(ends);
    dvz_free(colors);
    dvz_free(widths);
    return ok;

fail:
    dvz_free(starts);
    dvz_free(ends);
    dvz_free(colors);
    dvz_free(widths);
    return false;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the default reference-grid descriptor.
 *
 * @return default reference-grid descriptor
 */
DvzReferenceGridDesc dvz_reference_grid_desc(void)
{
    return (DvzReferenceGridDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzReferenceGridDesc),
        .plane = DVZ_REFERENCE_GRID_XZ,
        .origin = {0.0f, -0.62f, 0.0f},
        .axis_u = {1.0f, 0.0f, 0.0f},
        .axis_v = {0.0f, 0.0f, 1.0f},
        .size = {4.0f, 4.0f},
        .spacing = 0.25f,
        .major_every = 4,
        .minor_color = {88, 100, 112, 95},
        .major_color = {126, 142, 158, 150},
        .axis_color = {210, 222, 232, 210},
        .minor_width_px = 1.0f,
        .major_width_px = 1.5f,
        .axis_width_px = 2.25f,
        .show_minor = true,
        .show_major = true,
        .show_axes = true,
        .depth_test = true,
    };
}


bool dvz_ffi_reference_grid_desc(DvzReferenceGridDesc* out)
{
    if (out == NULL)
        return false;
    *out = dvz_reference_grid_desc();
    return true;
}


/**
 * Create a retained plane-oriented reference grid attached to one panel.
 *
 * @param panel panel receiving the grid
 * @param desc descriptor, or NULL for defaults
 * @return the reference grid, or NULL on validation/allocation error
 */
DvzReferenceGrid* dvz_reference_grid(DvzPanel* panel, const DvzReferenceGridDesc* desc)
{
    if (panel == NULL || panel->figure == NULL || panel->figure->scene == NULL)
        return NULL;
    if (!_reference_grid_desc_validate(desc))
        return NULL;
    DvzReferenceGridDesc resolved = desc != NULL ? *desc : dvz_reference_grid_desc();
    if (!isfinite(resolved.minor_width_px) || resolved.minor_width_px < 0.0f ||
        !isfinite(resolved.major_width_px) || resolved.major_width_px < 0.0f ||
        !isfinite(resolved.axis_width_px) || resolved.axis_width_px < 0.0f)
        return NULL;

    DvzScene* scene = panel->figure->scene;
    DvzReferenceGrid* grid = NULL;
    for (uint32_t i = 0; i < scene->reference_grid_count; i++)
    {
        if (!scene->reference_grids[i].active)
        {
            grid = &scene->reference_grids[i];
            break;
        }
    }
    if (grid == NULL)
    {
        if (scene->reference_grid_count >= DVZ_SCENE_MAX_REFERENCE_GRIDS)
            return NULL;
        grid = &scene->reference_grids[scene->reference_grid_count++];
    }
    dvz_memset(grid, sizeof(DvzReferenceGrid), 0, sizeof(DvzReferenceGrid));
    grid->scene = scene;
    grid->panel = panel;
    grid->desc = resolved;
    grid->active = true;
    grid->visible = true;
    grid->version = 1;

    grid->visual = dvz_segment(scene, 0);
    if (grid->visual == NULL)
        goto fail;
    if (dvz_segment_set_caps(grid->visual, DVZ_SEGMENT_CAP_BUTT, DVZ_SEGMENT_CAP_BUTT) != 0)
        goto fail;
    if (dvz_visual_set_alpha_mode(grid->visual, DVZ_ALPHA_BLENDED) != 0)
        goto fail;
    if (dvz_visual_set_depth_test(grid->visual, resolved.depth_test) != 0)
        goto fail;
    if (!_reference_grid_rebuild(grid))
        goto fail;
    DvzVisualAttachDesc attach = dvz_visual_attach_desc();
    attach.controller_mode = DVZ_CONTROLLER_APPLY_VIEW_PROJ;
    if (dvz_panel_add_visual(panel, grid->visual, &attach) != 0)
        goto fail;

    _scene_notify_request_frame(panel->figure);
    return grid;

fail:
    dvz_reference_grid_destroy(grid);
    return NULL;
}


/**
 * Destroy a reference grid.
 *
 * @param grid the reference grid
 */
void dvz_reference_grid_destroy(DvzReferenceGrid* grid)
{
    if (grid == NULL || !grid->active)
        return;
    DvzFigure* figure = grid->panel != NULL ? grid->panel->figure : NULL;
    if (grid->visual != NULL)
        dvz_visual_set_visible(grid->visual, false);
    dvz_memset(grid, sizeof(DvzReferenceGrid), 0, sizeof(DvzReferenceGrid));
    _scene_notify_request_frame(figure);
}


/**
 * Set reference-grid visibility.
 *
 * @param grid the reference grid
 * @param visible whether the grid should be visible
 */
void dvz_reference_grid_set_visible(DvzReferenceGrid* grid, bool visible)
{
    if (grid == NULL || !grid->active)
        return;
    if (grid->visible == visible)
        return;
    grid->visible = visible;
    if (grid->visual != NULL)
        dvz_visual_set_visible(grid->visual, visible);
    grid->version = grid->version == UINT64_MAX ? 1 : grid->version + 1;
    _scene_notify_request_frame(grid->panel != NULL ? grid->panel->figure : NULL);
}
