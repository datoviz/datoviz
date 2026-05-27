/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual bounds                                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"
#include "_visual_internal.h"
#include "datoviz/math/_cglm.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define BOUNDS_OVERLAY_COLOR_R 255
#define BOUNDS_OVERLAY_COLOR_G 214
#define BOUNDS_OVERLAY_COLOR_B 72
#define BOUNDS_OVERLAY_ALPHA_VISIBLE 245
#define BOUNDS_OVERLAY_ALPHA_OCCLUDED 120
#define BOUNDS_OVERLAY_WIDTH_VISIBLE 2.0f
#define BOUNDS_OVERLAY_WIDTH_OCCLUDED 1.5f
#define BOUNDS_OVERLAY_Z_LAYER_VISIBLE 9500
#define BOUNDS_OVERLAY_Z_LAYER_OCCLUDED 9499



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Reset a bounds object to an invalid empty state.
 *
 * @param out output bounds
 */
static void _bounds_reset(DvzBounds* out)
{
    ANN(out);
    out->valid = false;
    out->dims = 0;
    for (uint32_t i = 0; i < 3; i++)
    {
        out->min[i] = DBL_MAX;
        out->max[i] = -DBL_MAX;
    }
}



/**
 * Finalize dimensions after points have been included.
 *
 * @param out output bounds
 * @param force_3d whether the bounds should be reported as 3D even with flat Z
 */
static void _bounds_finalize(DvzBounds* out, bool force_3d)
{
    ANN(out);
    if (!out->valid)
    {
        out->dims = 0;
        return;
    }
    out->dims = force_3d || out->min[2] != out->max[2] ? 3 : 2;
}



/**
 * Include one finite 3D point in a bounds object.
 *
 * @param out output bounds
 * @param x x coordinate
 * @param y y coordinate
 * @param z z coordinate
 */
static void _bounds_include_point(DvzBounds* out, double x, double y, double z)
{
    ANN(out);
    if (!isfinite(x) || !isfinite(y) || !isfinite(z))
        return;

    if (x < out->min[0])
        out->min[0] = x;
    if (y < out->min[1])
        out->min[1] = y;
    if (z < out->min[2])
        out->min[2] = z;
    if (x > out->max[0])
        out->max[0] = x;
    if (y > out->max[1])
        out->max[1] = y;
    if (z > out->max[2])
        out->max[2] = z;
    out->valid = true;
}



/**
 * Include another bounds object in a bounds object.
 *
 * @param out output bounds
 * @param bounds bounds to include
 */
static void _bounds_include_bounds(DvzBounds* out, const DvzBounds* bounds)
{
    ANN(out);
    ANN(bounds);
    if (!bounds->valid)
        return;
    _bounds_include_point(out, bounds->min[0], bounds->min[1], bounds->min[2]);
    _bounds_include_point(out, bounds->max[0], bounds->max[1], bounds->max[2]);
}



/**
 * Return a dense visual attribute with the expected item size.
 *
 * @param visual the visual
 * @param attr_name attribute name
 * @param item_size expected item byte size
 * @return the attribute, or NULL when unavailable
 */
static const DvzVisualAttr*
_bounds_attr(const DvzVisual* visual, const char* attr_name, uint32_t item_size)
{
    ANN(visual);
    ANN(attr_name);
    int idx = _attr_index(visual, attr_name);
    if (idx < 0)
        return NULL;
    const DvzVisualAttr* attr = &visual->attrs[idx];
    if (attr->data == NULL || attr->item_count == 0 || attr->item_size != item_size)
        return NULL;
    return attr;
}



/**
 * Include packed vec3f positions in a bounds object.
 *
 * @param out output bounds
 * @param data packed vec3f data
 * @param item_count number of positions
 */
static void _bounds_include_vec3f(DvzBounds* out, const float* data, uint64_t item_count)
{
    ANN(out);
    ANN(data);
    for (uint64_t i = 0; i < item_count; i++)
    {
        _bounds_include_point(
            out, (double)data[3 * i + 0], (double)data[3 * i + 1],
            (double)data[3 * i + 2]);
    }
}



/**
 * Compute bounds from one dense vec3f position attribute.
 *
 * @param visual the visual
 * @param attr_name position attribute name
 * @param out output bounds
 * @return whether bounds were produced
 */
static bool _bounds_from_position_attr(
    const DvzVisual* visual, const char* attr_name, DvzBounds* out)
{
    ANN(visual);
    ANN(attr_name);
    ANN(out);
    const DvzVisualAttr* attr = _bounds_attr(visual, attr_name, 3 * sizeof(float));
    if (attr == NULL)
        return false;
    _bounds_include_vec3f(out, (const float*)attr->data, attr->item_count);
    return out->valid;
}



/**
 * Compute bounds for segment endpoint attributes.
 *
 * @param visual the segment visual
 * @param out output bounds
 * @return whether bounds were produced
 */
static bool _bounds_from_segment(const DvzVisual* visual, DvzBounds* out)
{
    ANN(visual);
    ANN(out);
    const DvzVisualAttr* start = _bounds_attr(visual, "position_start", 3 * sizeof(float));
    const DvzVisualAttr* end = _bounds_attr(visual, "position_end", 3 * sizeof(float));
    if (start == NULL || end == NULL || start->item_count != end->item_count)
        return false;
    _bounds_include_vec3f(out, (const float*)start->data, start->item_count);
    _bounds_include_vec3f(out, (const float*)end->data, end->item_count);
    return out->valid;
}


/**
 * Compute bounds for straight vector endpoint attributes.
 *
 * @param visual the vector visual
 * @param out output bounds
 * @return whether bounds were produced
 */
static bool _bounds_from_vector(const DvzVisual* visual, DvzBounds* out)
{
    ANN(visual);
    ANN(out);
    const DvzVisualAttr* position = _bounds_attr(visual, "position", 3 * sizeof(float));
    const DvzVisualAttr* vector = _bounds_attr(visual, "vector", 3 * sizeof(float));
    if (position == NULL || vector == NULL || position->item_count != vector->item_count)
        return false;

    const float* pos = (const float*)position->data;
    const float* vec = (const float*)vector->data;
    float scale = visual->vector.scale;
    float tail_factor = 0.0f;
    float head_factor = 1.0f;
    if (visual->vector.anchor == DVZ_VECTOR_ANCHOR_CENTER)
    {
        tail_factor = -0.5f;
        head_factor = 0.5f;
    }
    else if (visual->vector.anchor == DVZ_VECTOR_ANCHOR_HEAD)
    {
        tail_factor = -1.0f;
        head_factor = 0.0f;
    }

    for (uint64_t i = 0; i < position->item_count; i++)
    {
        double start[3] = {0};
        double end[3] = {0};
        for (uint32_t k = 0; k < 3; k++)
        {
            double delta = (double)vec[3 * i + k] * (double)scale;
            start[k] = (double)pos[3 * i + k] + (double)tail_factor * delta;
            end[k] = (double)pos[3 * i + k] + (double)head_factor * delta;
        }
        _bounds_include_point(out, start[0], start[1], start[2]);
        _bounds_include_point(out, end[0], end[1], end[2]);
    }
    return out->valid;
}



/**
 * Compute bounds for sphere centers expanded by radii.
 *
 * @param visual the sphere visual
 * @param out output bounds
 * @return whether bounds were produced
 */
static bool _bounds_from_sphere(const DvzVisual* visual, DvzBounds* out)
{
    ANN(visual);
    ANN(out);
    const DvzVisualAttr* position = _bounds_attr(visual, "position", 3 * sizeof(float));
    const DvzVisualAttr* radius_attr = _bounds_attr(visual, "radius", sizeof(float));
    if (position == NULL || radius_attr == NULL ||
        position->item_count != radius_attr->item_count)
        return false;

    const float* pos = (const float*)position->data;
    const float* radius = (const float*)radius_attr->data;
    for (uint64_t i = 0; i < position->item_count; i++)
    {
        double r = (double)radius[i];
        if (!isfinite(r) || r < 0.0)
            continue;
        double x = (double)pos[3 * i + 0];
        double y = (double)pos[3 * i + 1];
        double z = (double)pos[3 * i + 2];
        _bounds_include_point(out, x - r, y - r, z - r);
        _bounds_include_point(out, x + r, y + r, z + r);
    }
    return out->valid;
}


/**
 * Expand already computed sphere bounds for the generated wire overlay.
 *
 * @param visual the sphere visual
 * @param bounds bounds to expand in place
 */
static void _bounds_expand_sphere_wire_overlay(const DvzVisual* visual, DvzBounds* bounds)
{
    ANN(visual);
    ANN(bounds);
    if (visual->type != DVZ_VISUAL_TYPE_SPHERE || !bounds->valid)
        return;

    const DvzVisualAttr* radius_attr = _bounds_attr(visual, "radius", sizeof(float));
    if (radius_attr == NULL)
        return;
    const float* radius = (const float*)radius_attr->data;
    double max_radius = 0.0;
    for (uint64_t i = 0; i < radius_attr->item_count; i++)
    {
        double r = (double)radius[i];
        if (isfinite(r) && r > max_radius)
            max_radius = r;
    }
    if (!(max_radius > 0.0))
        return;

    const double pad = (sqrt(3.0) - 1.0) * max_radius;
    for (uint32_t dim = 0; dim < 3; dim++)
    {
        bounds->min[dim] -= pad;
        bounds->max[dim] += pad;
    }
}


/**
 * Return bounds suitable for a perspective wire overlay.
 *
 * @param visual the visual
 * @param out output bounds
 * @return 0 when bounds are available, -1 otherwise
 */
static int _bounds_overlay_source_bounds(const DvzVisual* visual, DvzBounds* out)
{
    ANN(visual);
    ANN(out);
    if (dvz_visual_bounds(visual, out) != 0)
        return -1;
    _bounds_expand_sphere_wire_overlay(visual, out);
    return 0;
}



/**
 * Include one transformed AABB corner in bounds.
 *
 * @param out output bounds
 * @param transform column-major mat4 transform
 * @param x input x coordinate
 * @param y input y coordinate
 * @param z input z coordinate
 */
static void
_bounds_include_transformed_point(DvzBounds* out, const float* transform, double x, double y, double z)
{
    ANN(out);
    ANN(transform);
    double tx = (double)transform[0] * x + (double)transform[4] * y +
                (double)transform[8] * z + (double)transform[12];
    double ty = (double)transform[1] * x + (double)transform[5] * y +
                (double)transform[9] * z + (double)transform[13];
    double tz = (double)transform[2] * x + (double)transform[6] * y +
                (double)transform[10] * z + (double)transform[14];
    double tw = (double)transform[3] * x + (double)transform[7] * y +
                (double)transform[11] * z + (double)transform[15];
    if (tw != 0.0)
    {
        tx /= tw;
        ty /= tw;
        tz /= tw;
    }
    _bounds_include_point(out, tx, ty, tz);
}



/**
 * Expand mesh bounds by per-instance transforms when available.
 *
 * @param visual the mesh visual
 * @param base base position bounds
 * @param out output bounds
 * @return whether bounds were produced
 */
static bool _bounds_from_mesh(const DvzVisual* visual, const DvzBounds* base, DvzBounds* out)
{
    ANN(visual);
    ANN(base);
    ANN(out);
    const DvzVisualAttr* transforms =
        _bounds_attr(visual, "instance_transform", 16 * sizeof(float));
    if (transforms == NULL)
    {
        _bounds_include_bounds(out, base);
        return out->valid;
    }

    const float* transform = (const float*)transforms->data;
    for (uint64_t i = 0; i < transforms->item_count; i++)
    {
        const float* mat = &transform[16 * i];
        for (uint32_t x = 0; x < 2; x++)
        {
            for (uint32_t y = 0; y < 2; y++)
            {
                for (uint32_t z = 0; z < 2; z++)
                {
                    double px = x == 0 ? base->min[0] : base->max[0];
                    double py = y == 0 ? base->min[1] : base->max[1];
                    double pz = z == 0 ? base->min[2] : base->max[2];
                    _bounds_include_transformed_point(out, mat, px, py, pz);
                }
            }
        }
    }
    return out->valid;
}



/**
 * Compute bounds for image visuals.
 *
 * @param visual the image visual
 * @param out output bounds
 * @return whether bounds were produced
 */
static bool _bounds_from_image(const DvzVisual* visual, DvzBounds* out)
{
    ANN(visual);
    ANN(out);
    const DvzVisualAttr* position = _bounds_attr(visual, "position", 3 * sizeof(float));
    if (position == NULL)
        return false;

    const DvzVisualAttr* extent = _bounds_attr(visual, "extent", 2 * sizeof(float));
    if (extent == NULL)
    {
        _bounds_include_vec3f(out, (const float*)position->data, position->item_count);
        return out->valid;
    }
    if (position->item_count != extent->item_count)
        return false;

    const float* pos = (const float*)position->data;
    const float* ext = (const float*)extent->data;
    const DvzVisualAttr* anchor = _bounds_attr(visual, "anchor", 2 * sizeof(float));
    const float* anc =
        anchor != NULL && anchor->item_count == position->item_count ? (const float*)anchor->data
                                                                     : NULL;

    for (uint64_t i = 0; i < position->item_count; i++)
    {
        double x = (double)pos[3 * i + 0];
        double y = (double)pos[3 * i + 1];
        double z = (double)pos[3 * i + 2];
        double w = (double)ext[2 * i + 0];
        double h = (double)ext[2 * i + 1];
        double ax = anc != NULL ? (double)anc[2 * i + 0] : 0.0;
        double ay = anc != NULL ? (double)anc[2 * i + 1] : 0.0;
        double x0 = x - 0.5 * (ax + 1.0) * w;
        double y0 = y - 0.5 * (ay + 1.0) * h;
        _bounds_include_point(out, x0, y0, z);
        _bounds_include_point(out, x0 + w, y0 + h, z);
    }
    return out->valid;
}



/**
 * Compute bounds for glyph visuals from anchor positions and local pixel bounds.
 *
 * @param visual the glyph visual
 * @param out output bounds
 * @return whether bounds were produced
 */
static bool _bounds_from_glyph(const DvzVisual* visual, DvzBounds* out)
{
    ANN(visual);
    ANN(out);
    const DvzVisualAttr* position = _bounds_attr(visual, "position", 3 * sizeof(float));
    const DvzVisualAttr* bounds = _bounds_attr(visual, "bounds", 4 * sizeof(float));
    if (position == NULL || bounds == NULL || position->item_count != bounds->item_count)
        return false;

    const float* pos = (const float*)position->data;
    const float* rect = (const float*)bounds->data;
    for (uint64_t i = 0; i < position->item_count; i++)
    {
        double x = (double)pos[3 * i + 0];
        double y = (double)pos[3 * i + 1];
        double z = (double)pos[3 * i + 2];
        _bounds_include_point(out, x + (double)rect[4 * i + 0], y + (double)rect[4 * i + 1], z);
        _bounds_include_point(out, x + (double)rect[4 * i + 2], y + (double)rect[4 * i + 3], z);
    }
    return out->valid;
}



/**
 * Find one visual attachment on a panel.
 *
 * @param panel the panel
 * @param visual the visual
 * @return the attachment, or NULL when absent
 */
static const DvzPanelAttach*
_panel_find_visual_attach(const DvzPanel* panel, const DvzVisual* visual)
{
    ANN(panel);
    ANN(visual);
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        if (panel->visuals[i].visual == visual)
            return &panel->visuals[i];
    }
    return NULL;
}



/**
 * Project one visual-space point to panel screen pixels.
 *
 * @param mvp MVP transform
 * @param panel panel owning the viewport
 * @param x input x coordinate
 * @param y input y coordinate
 * @param z input z coordinate
 * @param out output screen bounds
 */
static void _bounds_include_screen_point(
    DvzMVP* mvp, const DvzPanel* panel, double x, double y, double z, DvzBounds* out)
{
    ANN(mvp);
    ANN(panel);
    ANN(out);

    vec4 p = {(float)x, (float)y, (float)z, 1.0f};
    vec4 tmp0 = {0};
    vec4 tmp1 = {0};
    vec4 clip = {0};
    glm_mat4_mulv(mvp->model, p, tmp0);
    glm_mat4_mulv(mvp->view, tmp0, tmp1);
    glm_mat4_mulv(mvp->proj, tmp1, clip);
    if (clip[3] == 0.0f)
        return;

    float px = 0.0f;
    float py = 0.0f;
    float pw = 0.0f;
    float ph = 0.0f;
    _scene_panel_pixel_rect(panel, &px, &py, &pw, &ph);
    double ndc_x = (double)clip[0] / (double)clip[3];
    double ndc_y = (double)clip[1] / (double)clip[3];
    double sx = (double)px + 0.5 * (ndc_x + 1.0) * (double)pw;
    double sy = (double)py + 0.5 * (1.0 - ndc_y) * (double)ph;
    _bounds_include_point(out, sx, sy, 0.0);
}



/**
 * Project a visual-space AABB to panel screen pixels.
 *
 * @param panel the panel
 * @param attach the panel attachment
 * @param visual_bounds source visual-space bounds
 * @param out output screen bounds
 * @return whether bounds were produced
 */
static bool _bounds_project_screen(
    const DvzPanel* panel, const DvzPanelAttach* attach, const DvzBounds* visual_bounds,
    DvzBounds* out)
{
    ANN(panel);
    ANN(attach);
    ANN(visual_bounds);
    ANN(out);

    DvzMVP mvp = {0};
    glm_mat4_identity(mvp.model);
    glm_mat4_identity(mvp.view);
    glm_mat4_identity(mvp.proj);
    if (attach->controller_mode != DVZ_CONTROLLER_FIXED)
        _scene_panel_apply_mvp(panel, &mvp);

    for (uint32_t x = 0; x < 2; x++)
    {
        for (uint32_t y = 0; y < 2; y++)
        {
            for (uint32_t z = 0; z < 2; z++)
            {
                double px = x == 0 ? visual_bounds->min[0] : visual_bounds->max[0];
                double py = y == 0 ? visual_bounds->min[1] : visual_bounds->max[1];
                double pz = z == 0 ? visual_bounds->min[2] : visual_bounds->max[2];
                _bounds_include_screen_point(&mvp, panel, px, py, pz, out);
            }
        }
    }
    if (!out->valid)
        return false;
    out->dims = 2;
    out->min[2] = 0.0;
    out->max[2] = 0.0;
    return true;
}


/**
 * Return the number of wireframe line segments required for one bounds box.
 *
 * @param bounds the bounds box
 * @return line segment count
 */
static uint32_t _bounds_wire_line_count(const DvzBounds* bounds)
{
    ANN(bounds);
    if (!bounds->valid)
        return 0;
    return bounds->dims == 3 ? 12 : 4;
}



/**
 * Append one edge to packed segment arrays.
 *
 * @param start output start positions
 * @param end output end positions
 * @param colors output colors
 * @param widths output line widths
 * @param line_count current output line count
 * @param a edge start position
 * @param b edge end position
 */
static void _bounds_wire_append_edge(
    float (*start)[3], float (*end)[3], DvzColor* colors, float* widths, uint32_t* line_count,
    const double a[3], const double b[3])
{
    ANN(start);
    ANN(end);
    ANN(colors);
    ANN(widths);
    ANN(line_count);
    uint32_t i = *line_count;
    for (uint32_t dim = 0; dim < 3; dim++)
    {
        start[i][dim] = (float)a[dim];
        end[i][dim] = (float)b[dim];
    }
    colors[i] = dvz_color_rgba(
        BOUNDS_OVERLAY_COLOR_R, BOUNDS_OVERLAY_COLOR_G, BOUNDS_OVERLAY_COLOR_B,
        BOUNDS_OVERLAY_ALPHA_VISIBLE);
    widths[i] = BOUNDS_OVERLAY_WIDTH_VISIBLE;
    *line_count = i + 1;
}



/**
 * Append one bounds box as wireframe line segments.
 *
 * @param bounds source bounds box
 * @param start output start positions
 * @param end output end positions
 * @param colors output colors
 * @param widths output line widths
 * @param line_count current output line count
 */
static void _bounds_wire_append_box(
    const DvzBounds* bounds, float (*start)[3], float (*end)[3], DvzColor* colors, float* widths,
    uint32_t* line_count)
{
    ANN(bounds);
    ANN(start);
    ANN(end);
    ANN(colors);
    ANN(widths);
    ANN(line_count);
    if (!bounds->valid)
        return;

    double p[8][3] = {
        {bounds->min[0], bounds->min[1], bounds->min[2]},
        {bounds->max[0], bounds->min[1], bounds->min[2]},
        {bounds->max[0], bounds->max[1], bounds->min[2]},
        {bounds->min[0], bounds->max[1], bounds->min[2]},
        {bounds->min[0], bounds->min[1], bounds->max[2]},
        {bounds->max[0], bounds->min[1], bounds->max[2]},
        {bounds->max[0], bounds->max[1], bounds->max[2]},
        {bounds->min[0], bounds->max[1], bounds->max[2]},
    };
    const uint32_t edges_2d[4][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}};
    const uint32_t edges_3d[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
        {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7},
    };
    uint32_t edge_count = _bounds_wire_line_count(bounds);
    for (uint32_t i = 0; i < edge_count; i++)
    {
        const uint32_t* edge = bounds->dims == 3 ? edges_3d[i] : edges_2d[i];
        _bounds_wire_append_edge(start, end, colors, widths, line_count, p[edge[0]], p[edge[1]]);
    }
}



/**
 * Ensure a panel has one generated bounds overlay visual.
 *
 * @param panel the panel
 * @param occluded whether to create the occluded x-ray pass visual
 * @return the generated visual, or NULL on error
 */
static DvzVisual* _bounds_overlay_visual(DvzPanel* panel, bool occluded)
{
    ANN(panel);
    if (!occluded && panel->bounds_visual != NULL)
        return panel->bounds_visual;
    if (occluded && panel->bounds_occluded_visual != NULL)
        return panel->bounds_occluded_visual;
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return NULL;

    DvzVisual* visual = dvz_segment(panel->figure->scene, 0);
    if (visual == NULL)
        return NULL;
    if (dvz_visual_set_depth_test(visual, true) != 0)
        return NULL;
    visual->depth_compare_op = occluded ? VK_COMPARE_OP_GREATER : VK_COMPARE_OP_LESS_OR_EQUAL;
    DvzVisualAttachDesc attach = {
        .z_layer =
            occluded ? BOUNDS_OVERLAY_Z_LAYER_OCCLUDED : BOUNDS_OVERLAY_Z_LAYER_VISIBLE,
        .controller_mode = DVZ_CONTROLLER_APPLY,
    };
    if (dvz_panel_add_visual(panel, visual, &attach) != 0)
        return NULL;
    if (occluded)
        panel->bounds_occluded_visual = visual;
    else
        panel->bounds_visual = visual;
    dvz_visual_set_visible(visual, false);
    return visual;
}



/**
 * Rebuild one panel's generated bounds overlay visual.
 *
 * @param panel the panel
 * @return whether the overlay is synchronized
 */
static bool _bounds_overlay_sync_panel(DvzPanel* panel)
{
    ANN(panel);
    if (!panel->bounds_visible)
    {
        if (panel->bounds_visual != NULL)
            dvz_visual_set_visible(panel->bounds_visual, false);
        if (panel->bounds_occluded_visual != NULL)
            dvz_visual_set_visible(panel->bounds_occluded_visual, false);
        return true;
    }

    DvzVisual* overlay = _bounds_overlay_visual(panel, false);
    DvzVisual* occluded_overlay = _bounds_overlay_visual(panel, true);
    if (overlay == NULL || occluded_overlay == NULL)
        return false;

    uint64_t max_lines = 0;
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        const DvzPanelAttach* attach = &panel->visuals[i];
        DvzVisual* visual = attach->visual;
        if (visual == NULL || visual == overlay || visual == occluded_overlay ||
            !visual->visible || attach->controller_mode == DVZ_CONTROLLER_FIXED)
        {
            continue;
        }

        DvzBounds bounds = {0};
        if (_bounds_overlay_source_bounds(visual, &bounds) != 0)
            continue;
        uint64_t next = 0;
        if (_dvz_add_u64_overflows(max_lines, _bounds_wire_line_count(&bounds), &next))
        {
            log_error("bounds overlay line count overflow");
            return false;
        }
        max_lines = next;
    }

    if (max_lines == 0 || max_lines > UINT32_MAX)
    {
        dvz_visual_set_visible(overlay, false);
        dvz_visual_set_visible(occluded_overlay, false);
        return max_lines == 0;
    }

    uint64_t start_bytes = 0;
    uint64_t end_bytes = 0;
    uint64_t color_bytes = 0;
    uint64_t width_bytes = 0;
    if (_dvz_mul_u64_overflows(max_lines, 3u * sizeof(float), &start_bytes) ||
        _dvz_mul_u64_overflows(max_lines, 3u * sizeof(float), &end_bytes) ||
        _dvz_mul_u64_overflows(max_lines, sizeof(DvzColor), &color_bytes) ||
        _dvz_mul_u64_overflows(max_lines, sizeof(float), &width_bytes) ||
        start_bytes > SIZE_MAX || end_bytes > SIZE_MAX || color_bytes > SIZE_MAX ||
        width_bytes > SIZE_MAX)
    {
        log_error("bounds overlay allocation size overflow");
        return false;
    }

    float(*start)[3] = (float(*)[3])dvz_calloc((DvzSize)start_bytes, 1);
    float(*end)[3] = (float(*)[3])dvz_calloc((DvzSize)end_bytes, 1);
    DvzColor* colors = (DvzColor*)dvz_calloc((DvzSize)color_bytes, 1);
    float* widths = (float*)dvz_calloc((DvzSize)width_bytes, 1);
    if (start == NULL || end == NULL || colors == NULL || widths == NULL)
    {
        dvz_free(start);
        dvz_free(end);
        dvz_free(colors);
        dvz_free(widths);
        log_error("bounds overlay allocation failed");
        return false;
    }

    uint32_t line_count = 0;
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        const DvzPanelAttach* attach = &panel->visuals[i];
        DvzVisual* visual = attach->visual;
        if (visual == NULL || visual == overlay || visual == occluded_overlay ||
            !visual->visible || attach->controller_mode == DVZ_CONTROLLER_FIXED)
        {
            continue;
        }

        DvzBounds bounds = {0};
        if (_bounds_overlay_source_bounds(visual, &bounds) != 0)
            continue;
        _bounds_wire_append_box(&bounds, start, end, colors, widths, &line_count);
    }

    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position_start", .data = start, .item_count = line_count},
        {.attr_name = "position_end", .data = end, .item_count = line_count},
        {.attr_name = "color", .data = colors, .item_count = line_count},
        {.attr_name = "stroke_width", .data = widths, .item_count = line_count},
    };
    int rc = line_count > 0 ? dvz_visual_set_data_many(overlay, updates, 4) : -1;
    bool visible = rc == 0 && line_count > 0;

    for (uint32_t i = 0; i < line_count; i++)
    {
        colors[i].a = BOUNDS_OVERLAY_ALPHA_OCCLUDED;
        widths[i] = BOUNDS_OVERLAY_WIDTH_OCCLUDED;
    }
    if (visible)
        rc = dvz_visual_set_data_many(occluded_overlay, updates, 4);
    bool occluded_visible = rc == 0 && line_count > 0;

    dvz_visual_set_visible(overlay, visible);
    dvz_visual_set_visible(occluded_overlay, occluded_visible);

    dvz_free(start);
    dvz_free(end);
    dvz_free(colors);
    dvz_free(widths);
    return visible && occluded_visible;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the retained visual-space bounding box of one visual.
 *
 * @param visual the visual
 * @param out output bounding box
 * @return 0 when bounds are available, -1 otherwise
 */
int dvz_visual_bounds(const DvzVisual* visual, DvzBounds* out)
{
    ANN(visual);
    ANN(out);
    _bounds_reset(out);

    bool force_3d = false;
    if (visual->type == DVZ_VISUAL_TYPE_SEGMENT)
    {
        (void)_bounds_from_segment(visual, out);
    }
    else if (visual->type == DVZ_VISUAL_TYPE_VECTOR)
    {
        if (!_bounds_from_vector(visual, out))
            (void)_bounds_from_position_attr(visual, "position", out);
    }
    else if (visual->type == DVZ_VISUAL_TYPE_SPHERE)
    {
        force_3d = true;
        (void)_bounds_from_sphere(visual, out);
    }
    else if (visual->type == DVZ_VISUAL_TYPE_IMAGE)
    {
        (void)_bounds_from_image(visual, out);
    }
    else if (visual->type == DVZ_VISUAL_TYPE_GLYPH)
    {
        (void)_bounds_from_glyph(visual, out);
    }
    else if (visual->type == DVZ_VISUAL_TYPE_VOLUME)
    {
        force_3d = true;
        for (uint32_t i = 0; i < 3; i++)
        {
            if (!isfinite(visual->volume.bounds_min[i]) || !isfinite(visual->volume.bounds_max[i]))
                return -1;
        }
        _bounds_include_point(
            out, visual->volume.bounds_min[0], visual->volume.bounds_min[1],
            visual->volume.bounds_min[2]);
        _bounds_include_point(
            out, visual->volume.bounds_max[0], visual->volume.bounds_max[1],
            visual->volume.bounds_max[2]);
    }
    else
    {
        DvzBounds position_bounds = {0};
        _bounds_reset(&position_bounds);
        (void)_bounds_from_position_attr(visual, "position", &position_bounds);
        if (position_bounds.valid && visual->type == DVZ_VISUAL_TYPE_MESH)
            (void)_bounds_from_mesh(visual, &position_bounds, out);
        else
            _bounds_include_bounds(out, &position_bounds);
    }

    _bounds_finalize(out, force_3d);
    return out->valid ? 0 : -1;
}



/**
 * Return one visual's bounds in the coordinate space of one panel attachment.
 *
 * @param panel the panel
 * @param visual visual attached to the panel
 * @param space target bounds space
 * @param out output bounding box
 * @return 0 when bounds are available, -1 otherwise
 */
int dvz_panel_visual_bounds(
    const DvzPanel* panel, const DvzVisual* visual, DvzBoundsSpace space, DvzBounds* out)
{
    ANN(panel);
    ANN(visual);
    ANN(out);
    _bounds_reset(out);

    const DvzPanelAttach* attach = _panel_find_visual_attach(panel, visual);
    if (attach == NULL)
        return -1;

    DvzBounds visual_bounds = {0};
    if (dvz_visual_bounds(visual, &visual_bounds) != 0)
        return -1;
    if (space == DVZ_BOUNDS_SPACE_VISUAL)
    {
        *out = visual_bounds;
        return 0;
    }
    if (space != DVZ_BOUNDS_SPACE_SCREEN)
        return -1;
    return _bounds_project_screen(panel, attach, &visual_bounds, out) ? 0 : -1;
}



/**
 * Return the union of all visible visual bounds attached to one panel.
 *
 * @param panel the panel
 * @param space target bounds space
 * @param out output bounding box
 * @return 0 when at least one visible visual has bounds, -1 otherwise
 */
int dvz_panel_bounds(const DvzPanel* panel, DvzBoundsSpace space, DvzBounds* out)
{
    ANN(panel);
    ANN(out);
    _bounds_reset(out);

    bool force_3d = false;
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        const DvzVisual* visual = panel->visuals[i].visual;
        if (visual == NULL || visual == panel->bounds_visual ||
            visual == panel->bounds_occluded_visual || !visual->visible)
            continue;

        DvzBounds bounds = {0};
        if (dvz_panel_visual_bounds(panel, visual, space, &bounds) != 0)
            continue;
        _bounds_include_bounds(out, &bounds);
        force_3d = force_3d || bounds.dims == 3;
    }

    _bounds_finalize(out, force_3d && space == DVZ_BOUNDS_SPACE_VISUAL);
    if (space == DVZ_BOUNDS_SPACE_SCREEN && out->valid)
        out->dims = 2;
    return out->valid ? 0 : -1;
}



/**
 * Show or hide the panel-owned visual bounds overlay.
 *
 * @param panel the panel
 * @param visible whether bounds boxes should be shown
 * @return 0 on success, -1 on error
 */
int dvz_panel_set_bounds_visible(DvzPanel* panel, bool visible)
{
    ANN(panel);
    if (panel->figure == NULL)
        return -1;
    panel->bounds_visible = visible;
    if (panel->bounds_visual != NULL)
        dvz_visual_set_visible(panel->bounds_visual, visible);
    if (panel->bounds_occluded_visual != NULL)
        dvz_visual_set_visible(panel->bounds_occluded_visual, visible);
    _scene_notify_request_frame(panel->figure);
    return 0;
}



/**
 * Return whether the panel-owned visual bounds overlay is enabled.
 *
 * @param panel the panel
 * @return whether bounds boxes should be shown
 */
bool dvz_panel_bounds_visible(const DvzPanel* panel)
{
    ANN(panel);
    return panel->bounds_visible;
}



/**
 * Rebuild generated bounds overlay visuals before frame emission.
 *
 * @param figure the figure
 */
void _scene_prepare_bounds_visuals(DvzFigure* figure)
{
    ANN(figure);
    for (uint32_t i = 0; i < figure->panel_count; i++)
    {
        if (!_bounds_overlay_sync_panel(&figure->panels[i]))
            log_error("failed to update panel bounds overlay");
    }
}
