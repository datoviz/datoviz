/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene orientation gizmo                                                                      */
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
#include "core/orientation_gizmo_internal.h"
#include "core/scene_notify_internal.h"
#include "datoviz/scene.h"
#include "_visual_internal.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_ORIENTATION_GIZMO_DESC_KNOWN_FLAGS 0u

#define GIZMO_SEGMENTS          40u
#define GIZMO_AXIS_COUNT        3u
#define GIZMO_RING_SEGMENTS     96u
#define GIZMO_CYLINDER_VERTICES (GIZMO_SEGMENTS * 6u)
#define GIZMO_CONE_VERTICES     (GIZMO_SEGMENTS * 6u)
#define GIZMO_RING_POINTS       (GIZMO_RING_SEGMENTS + 1u)
#define GIZMO_HUB_VERTICES      36u
#define GIZMO_AXES_VERTICES                                                                  \
    (GIZMO_AXIS_COUNT * (GIZMO_CYLINDER_VERTICES + GIZMO_CONE_VERTICES) + GIZMO_HUB_VERTICES)
#define GIZMO_RINGS_POINTS (GIZMO_AXIS_COUNT * GIZMO_RING_POINTS)

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct GizmoMesh
{
    vec3* positions;
    vec3* normals;
    DvzColor* colors;
    uint32_t count;
    uint32_t capacity;
} GizmoMesh;


typedef struct GizmoPath
{
    vec3* positions;
    DvzColor* colors;
    float* stroke_widths;
    uint32_t count;
    uint32_t capacity;
} GizmoPath;


typedef struct GizmoGeometry
{
    float shaft_length;
    float tip_length;
    float shaft_radius;
    float tip_radius;
    float hub_half_size;
    float ring_radius;
    float ring_stroke_width;
    DvzColor colors[GIZMO_AXIS_COUNT];
} GizmoGeometry;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Validate one orientation-gizmo descriptor.
 *
 * @param desc descriptor, or NULL
 * @return whether the descriptor is valid
 */
static bool _orientation_gizmo_desc_validate(const DvzOrientationGizmoDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(
            desc, DvzOrientationGizmoDesc, DVZ_ORIENTATION_GIZMO_DESC_KNOWN_FLAGS))
    {
        log_error("invalid orientation gizmo descriptor ABI");
        return false;
    }
    return true;
}


/**
 * Resolve the inset panel descriptor from the source panel and placement.
 *
 * @param source_panel source panel
 * @param placement placement descriptor
 * @param out output normalized panel descriptor
 * @return whether the descriptor was resolved
 */
static bool _orientation_gizmo_panel_desc(
    const DvzPanel* source_panel, const DvzPlacement* placement, DvzPanelDesc* out)
{
    ANN(source_panel);
    ANN(placement);
    ANN(out);
    if (source_panel->figure == NULL || source_panel->figure->width == 0 ||
        source_panel->figure->height == 0)
        return false;

    float panel_x = 0.0f;
    float panel_y = 0.0f;
    float panel_width = 0.0f;
    float panel_height = 0.0f;
    _scene_panel_pixel_rect(source_panel, &panel_x, &panel_y, &panel_width, &panel_height);

    DvzRect panel_rect = {
        .x = panel_x,
        .y = panel_y,
        .width = panel_width,
        .height = panel_height,
    };
    DvzRect figure_rect = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)source_panel->figure->width,
        .height = (float)source_panel->figure->height,
    };
    DvzRect rect = {0};
    if (!dvz_placement_resolve(placement, &panel_rect, &figure_rect, &rect))
        return false;

    const float x = panel_x + rect.x;
    const float y = panel_y + rect.y;
    if (rect.width <= 0.0f || rect.height <= 0.0f)
        return false;
    *out = (DvzPanelDesc){
        .x = x / figure_rect.width,
        .y = y / figure_rect.height,
        .width = rect.width / figure_rect.width,
        .height = rect.height / figure_rect.height,
    };
    return true;
}


/**
 * Copy the linear rotation block from one matrix into an identity transform.
 *
 * @param src source matrix
 * @param out destination rotation transform
 */
static void _gizmo_rotation_matrix(mat4 src, mat4 out)
{
    ANN(src);
    ANN(out);
    glm_mat4_identity(out);
    for (uint32_t col = 0; col < 3; col++)
        for (uint32_t row = 0; row < 3; row++)
            out[col][row] = src[col][row];
}


/**
 * Return whether two retained transforms are close enough to avoid churn.
 *
 * @param a first matrix
 * @param b second matrix
 * @return whether matrices are nearly equal
 */
static bool _gizmo_mat4_close(mat4 a, mat4 b)
{
    ANN(a);
    ANN(b);
    for (uint32_t col = 0; col < 4; col++)
        for (uint32_t row = 0; row < 4; row++)
            if (fabsf(a[col][row] - b[col][row]) > 1e-6f)
                return false;
    return true;
}


/**
 * Set a visual-local transform from frame preparation without request-frame recursion.
 *
 * @param visual visual to update
 * @param transform retained local transform
 */
static void _gizmo_set_visual_transform(DvzVisual* visual, mat4 transform)
{
    ANN(visual);
    ANN(transform);

    if (visual->has_local_transform && _gizmo_mat4_close(visual->local_transform, transform))
        return;

    for (uint32_t col = 0; col < 4; col++)
        for (uint32_t row = 0; row < 4; row++)
            visual->local_transform[col][row] = transform[col][row];
    visual->has_local_transform = true;
    _visual_bump_version(&visual->local_transform_version);
}


/**
 * Synchronize one gizmo transform with its source panel's effective rendered orientation.
 *
 * @param gizmo orientation gizmo
 * @return whether synchronization succeeded
 */
static bool _orientation_gizmo_sync_transform(DvzOrientationGizmo* gizmo)
{
    ANN(gizmo);
    if (
        !gizmo->active || gizmo->source_panel == NULL || gizmo->panel == NULL ||
        gizmo->axes_visual == NULL || gizmo->rings_visual == NULL)
    {
        return false;
    }

    DvzMVP source_mvp = {0};
    DvzMVP gizmo_mvp = {0};
    _scene_panel_apply_mvp(gizmo->source_panel, &source_mvp);
    _scene_panel_apply_mvp(gizmo->panel, &gizmo_mvp);

    mat4 source_view = GLM_MAT4_IDENTITY_INIT;
    mat4 source_model = GLM_MAT4_IDENTITY_INIT;
    mat4 gizmo_view = GLM_MAT4_IDENTITY_INIT;
    mat4 inv_gizmo_view = GLM_MAT4_IDENTITY_INIT;
    mat4 source_effective = GLM_MAT4_IDENTITY_INIT;
    mat4 transform = GLM_MAT4_IDENTITY_INIT;
    _gizmo_rotation_matrix(source_mvp.view, source_view);
    _gizmo_rotation_matrix(source_mvp.model, source_model);
    _gizmo_rotation_matrix(gizmo_mvp.view, gizmo_view);
    glm_mat4_inv(gizmo_view, inv_gizmo_view);
    glm_mat4_mul(source_view, source_model, source_effective);
    glm_mat4_mul(inv_gizmo_view, source_effective, transform);

    _gizmo_set_visual_transform(gizmo->axes_visual, transform);
    _gizmo_set_visual_transform(gizmo->rings_visual, transform);
    return true;
}


/**
 * Fill the orthonormal basis for one gizmo axis.
 *
 * @param axis axis index, where 0 is X, 1 is Y, and 2 is Z
 * @param dir output axis direction
 * @param u output first radial basis vector
 * @param v output second radial basis vector
 */
static void _gizmo_axis_basis(uint32_t axis, vec3 dir, vec3 u, vec3 v)
{
    ANN(dir);
    ANN(u);
    ANN(v);

    dir[0] = dir[1] = dir[2] = 0.0f;
    u[0] = u[1] = u[2] = 0.0f;
    v[0] = v[1] = v[2] = 0.0f;

    if (axis == 0)
    {
        dir[0] = 1.0f;
        u[1] = 1.0f;
        v[2] = 1.0f;
    }
    else if (axis == 1)
    {
        dir[1] = 1.0f;
        u[2] = 1.0f;
        v[0] = 1.0f;
    }
    else
    {
        dir[2] = 1.0f;
        u[0] = 1.0f;
        v[1] = 1.0f;
    }
}


/**
 * Resolve geometry controls from the public descriptor.
 *
 * @param desc public orientation-gizmo descriptor
 * @return geometry controls
 */
static GizmoGeometry _gizmo_geometry(const DvzOrientationGizmoDesc* desc)
{
    ANN(desc);
    const float length = desc->axis_length;
    return (GizmoGeometry){
        .shaft_length = 0.78f * length,
        .tip_length = 0.22f * length,
        .shaft_radius = 0.024f * length,
        .tip_radius = 0.074f * length,
        .hub_half_size = 0.052f * length,
        .ring_radius = 0.66f * length,
        .ring_stroke_width = fmaxf(1.0f, 0.78f * desc->axis_width_px),
        .colors =
            {
                desc->x_color,
                desc->y_color,
                desc->z_color,
            },
    };
}


/**
 * Add two scaled vectors.
 *
 * @param a first input vector
 * @param a_scale scale applied to the first vector
 * @param b second input vector
 * @param b_scale scale applied to the second vector
 * @param out output vector
 */
static void _vec3_add_scaled(const vec3 a, float a_scale, const vec3 b, float b_scale, vec3 out)
{
    ANN(a);
    ANN(b);
    ANN(out);

    out[0] = a_scale * a[0] + b_scale * b[0];
    out[1] = a_scale * a[1] + b_scale * b[1];
    out[2] = a_scale * a[2] + b_scale * b[2];
}


/**
 * Normalize a vector, falling back to +Z for degenerate inputs.
 *
 * @param v vector to normalize in place
 */
static void _normalize3(vec3 v)
{
    ANN(v);

    const float len2 = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    if (len2 <= 0.0f)
    {
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 1.0f;
        return;
    }

    const float inv_len = 1.0f / sqrtf(len2);
    v[0] *= inv_len;
    v[1] *= inv_len;
    v[2] *= inv_len;
}


/**
 * Append one lit vertex to the gizmo mesh.
 *
 * @param mesh target mesh arrays
 * @param position vertex position
 * @param normal vertex normal
 * @param color vertex color
 * @return true when the vertex was appended
 */
static bool
_gizmo_vertex(GizmoMesh* mesh, const vec3 position, const vec3 normal, const DvzColor color)
{
    ANN(mesh);
    ANN(position);
    ANN(normal);
    if (mesh->count >= mesh->capacity)
        return false;

    dvz_memcpy(mesh->positions[mesh->count], sizeof(vec3), position, sizeof(vec3));
    dvz_memcpy(mesh->normals[mesh->count], sizeof(vec3), normal, sizeof(vec3));
    mesh->colors[mesh->count] = color;
    mesh->count++;
    return true;
}


/**
 * Append a triangle to the gizmo mesh.
 *
 * @param mesh target mesh arrays
 * @param p0 first vertex position
 * @param n0 first vertex normal
 * @param p1 second vertex position
 * @param n1 second vertex normal
 * @param p2 third vertex position
 * @param n2 third vertex normal
 * @param color triangle color
 * @return true when the triangle was appended
 */
static bool _gizmo_triangle(
    GizmoMesh* mesh, const vec3 p0, const vec3 n0, const vec3 p1, const vec3 n1, const vec3 p2,
    const vec3 n2, const DvzColor color)
{
    ANN(mesh);
    ANN(p0);
    ANN(n0);
    ANN(p1);
    ANN(n1);
    ANN(p2);
    ANN(n2);
    return _gizmo_vertex(mesh, p0, n0, color) && _gizmo_vertex(mesh, p1, n1, color) &&
           _gizmo_vertex(mesh, p2, n2, color);
}


/**
 * Compute a circular radial vector from an axis basis.
 *
 * @param u first radial basis vector
 * @param v second radial basis vector
 * @param angle angle in radians
 * @param radius radial scale
 * @param out output vector
 */
static void _gizmo_radial_at(const vec3 u, const vec3 v, float angle, float radius, vec3 out)
{
    ANN(u);
    ANN(v);
    ANN(out);

    const float c = cosf(angle);
    const float s = sinf(angle);
    out[0] = radius * (c * u[0] + s * v[0]);
    out[1] = radius * (c * u[1] + s * v[1]);
    out[2] = radius * (c * u[2] + s * v[2]);
}


/**
 * Compute a point on one oriented axis.
 *
 * @param dir axis direction
 * @param radial radial offset
 * @param distance distance along the axis
 * @param out output point
 */
static void _gizmo_axis_point(const vec3 dir, const vec3 radial, float distance, vec3 out)
{
    ANN(dir);
    ANN(radial);
    ANN(out);

    out[0] = distance * dir[0] + radial[0];
    out[1] = distance * dir[1] + radial[1];
    out[2] = distance * dir[2] + radial[2];
}


/**
 * Append one colored cylindrical shaft.
 *
 * @param mesh target mesh arrays
 * @param geometry gizmo geometry controls
 * @param axis axis index
 * @return true when all shaft triangles were appended
 */
static bool _gizmo_append_shaft(GizmoMesh* mesh, const GizmoGeometry* geometry, uint32_t axis)
{
    ANN(mesh);
    ANN(geometry);

    vec3 dir = {0};
    vec3 u = {0};
    vec3 v = {0};
    _gizmo_axis_basis(axis, dir, u, v);
    const DvzColor color = geometry->colors[axis % GIZMO_AXIS_COUNT];

    for (uint32_t i = 0; i < GIZMO_SEGMENTS; i++)
    {
        const float a0 = TAU * (float)i / (float)GIZMO_SEGMENTS;
        const float a1 = TAU * (float)(i + 1u) / (float)GIZMO_SEGMENTS;

        vec3 r0 = {0};
        vec3 r1 = {0};
        _gizmo_radial_at(u, v, a0, geometry->shaft_radius, r0);
        _gizmo_radial_at(u, v, a1, geometry->shaft_radius, r1);

        vec3 n0 = {r0[0], r0[1], r0[2]};
        vec3 n1 = {r1[0], r1[1], r1[2]};
        _normalize3(n0);
        _normalize3(n1);

        vec3 p00 = {0};
        vec3 p01 = {0};
        vec3 p10 = {0};
        vec3 p11 = {0};
        _gizmo_axis_point(dir, r0, 0.0f, p00);
        _gizmo_axis_point(dir, r1, 0.0f, p01);
        _gizmo_axis_point(dir, r0, geometry->shaft_length, p10);
        _gizmo_axis_point(dir, r1, geometry->shaft_length, p11);

        if (!_gizmo_triangle(mesh, p00, n0, p11, n1, p10, n0, color))
            return false;
        if (!_gizmo_triangle(mesh, p00, n0, p01, n1, p11, n1, color))
            return false;
    }
    return true;
}


/**
 * Append one colored conical arrow tip.
 *
 * @param mesh target mesh arrays
 * @param geometry gizmo geometry controls
 * @param axis axis index
 * @return true when all cone triangles were appended
 */
static bool _gizmo_append_tip(GizmoMesh* mesh, const GizmoGeometry* geometry, uint32_t axis)
{
    ANN(mesh);
    ANN(geometry);

    vec3 dir = {0};
    vec3 u = {0};
    vec3 v = {0};
    _gizmo_axis_basis(axis, dir, u, v);
    const DvzColor color = geometry->colors[axis % GIZMO_AXIS_COUNT];

    vec3 apex = {0};
    vec3 base_center = {0};
    _gizmo_axis_point(dir, (vec3){0}, geometry->shaft_length + geometry->tip_length, apex);
    _gizmo_axis_point(dir, (vec3){0}, geometry->shaft_length, base_center);

    for (uint32_t i = 0; i < GIZMO_SEGMENTS; i++)
    {
        const float a0 = TAU * (float)i / (float)GIZMO_SEGMENTS;
        const float a1 = TAU * (float)(i + 1u) / (float)GIZMO_SEGMENTS;

        vec3 r0 = {0};
        vec3 r1 = {0};
        _gizmo_radial_at(u, v, a0, geometry->tip_radius, r0);
        _gizmo_radial_at(u, v, a1, geometry->tip_radius, r1);

        vec3 p0 = {0};
        vec3 p1 = {0};
        _gizmo_axis_point(dir, r0, geometry->shaft_length, p0);
        _gizmo_axis_point(dir, r1, geometry->shaft_length, p1);

        vec3 n0 = {0};
        vec3 n1 = {0};
        _vec3_add_scaled(
            r0, 1.0f / geometry->tip_radius, dir, geometry->tip_radius / geometry->tip_length,
            n0);
        _vec3_add_scaled(
            r1, 1.0f / geometry->tip_radius, dir, geometry->tip_radius / geometry->tip_length,
            n1);
        _normalize3(n0);
        _normalize3(n1);

        vec3 apex_normal = {0};
        _vec3_add_scaled(n0, 0.5f, n1, 0.5f, apex_normal);
        _normalize3(apex_normal);

        vec3 cap_normal = {-dir[0], -dir[1], -dir[2]};
        if (!_gizmo_triangle(mesh, p0, n0, p1, n1, apex, apex_normal, color))
            return false;
        if (!_gizmo_triangle(mesh, p1, cap_normal, p0, cap_normal, base_center, cap_normal, color))
            return false;
    }
    return true;
}


/**
 * Append the small white cube at the gizmo origin.
 *
 * @param mesh target mesh arrays
 * @param geometry gizmo geometry controls
 * @return true when all hub triangles were appended
 */
static bool _gizmo_append_hub(GizmoMesh* mesh, const GizmoGeometry* geometry)
{
    ANN(mesh);
    ANN(geometry);

    const float s = geometry->hub_half_size;
    const DvzColor color = {238, 240, 244, 255};
    const vec3 p[8] = {
        {-s, -s, -s}, {+s, -s, -s}, {+s, +s, -s}, {-s, +s, -s},
        {-s, -s, +s}, {+s, -s, +s}, {+s, +s, +s}, {-s, +s, +s},
    };
    const uint32_t faces[6][4] = {
        {1, 5, 6, 2}, {4, 0, 3, 7}, {2, 6, 7, 3}, {4, 5, 1, 0}, {5, 4, 7, 6}, {0, 1, 2, 3},
    };
    const vec3 normals[6] = {
        {+1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, +1.0f, 0.0f},
        {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, +1.0f}, {0.0f, 0.0f, -1.0f},
    };

    for (uint32_t i = 0; i < 6u; i++)
    {
        const uint32_t* f = faces[i];
        if (!_gizmo_triangle(
                mesh, p[f[0]], normals[i], p[f[1]], normals[i], p[f[2]], normals[i], color))
            return false;
        if (!_gizmo_triangle(
                mesh, p[f[0]], normals[i], p[f[2]], normals[i], p[f[3]], normals[i], color))
            return false;
    }
    return true;
}


/**
 * Append one path point to the gizmo rings.
 *
 * @param path target path arrays
 * @param position point position
 * @param color point color
 * @param stroke_width_px stroke width in pixels
 * @return true when the point was appended
 */
static bool
_gizmo_path_point(GizmoPath* path, const vec3 position, const DvzColor color, float stroke_width_px)
{
    ANN(path);
    ANN(position);
    if (path->count >= path->capacity)
        return false;

    dvz_memcpy(path->positions[path->count], sizeof(vec3), position, sizeof(vec3));
    path->colors[path->count] = color;
    path->stroke_widths[path->count] = stroke_width_px;
    path->count++;
    return true;
}


/**
 * Fill the retained mesh buffers for the lit gizmo axes and hub.
 *
 * @param mesh target mesh arrays
 * @param geometry gizmo geometry controls
 * @return true when the mesh was fully generated
 */
static bool _gizmo_build_axes(GizmoMesh* mesh, const GizmoGeometry* geometry)
{
    ANN(mesh);
    ANN(geometry);

    mesh->count = 0;

    for (uint32_t axis = 0; axis < GIZMO_AXIS_COUNT; axis++)
    {
        if (!_gizmo_append_shaft(mesh, geometry, axis))
            return false;
        if (!_gizmo_append_tip(mesh, geometry, axis))
            return false;
    }

    if (!_gizmo_append_hub(mesh, geometry))
        return false;

    return mesh->count == GIZMO_AXES_VERTICES;
}


/**
 * Fill the retained path buffers for the orientation rings.
 *
 * @param path target path arrays
 * @param geometry gizmo geometry controls
 * @return true when the path was fully generated
 */
static bool _gizmo_build_rings(GizmoPath* path, const GizmoGeometry* geometry)
{
    ANN(path);
    ANN(geometry);

    path->count = 0;

    for (uint32_t axis = 0; axis < GIZMO_AXIS_COUNT; axis++)
    {
        vec3 dir = {0};
        vec3 u = {0};
        vec3 v = {0};
        _gizmo_axis_basis(axis, dir, u, v);
        DvzColor color = geometry->colors[axis % GIZMO_AXIS_COUNT];
        color.a = 255;

        for (uint32_t i = 0; i < GIZMO_RING_POINTS; i++)
        {
            const float angle =
                TAU * (float)(i % GIZMO_RING_SEGMENTS) / (float)GIZMO_RING_SEGMENTS;
            vec3 p = {0};
            _gizmo_radial_at(u, v, angle, geometry->ring_radius, p);
            if (!_gizmo_path_point(path, p, color, geometry->ring_stroke_width))
                return false;
        }
    }
    return path->count == GIZMO_RINGS_POINTS;
}


/**
 * Release cached geometry arrays for one gizmo.
 *
 * @param gizmo orientation gizmo
 */
static void _orientation_gizmo_free_geometry(DvzOrientationGizmo* gizmo)
{
    ANN(gizmo);
    dvz_free(gizmo->axes_positions);
    dvz_free(gizmo->axes_normals);
    dvz_free(gizmo->axes_colors);
    dvz_free(gizmo->ring_positions);
    dvz_free(gizmo->ring_colors);
    dvz_free(gizmo->ring_widths);
    gizmo->axes_positions = NULL;
    gizmo->axes_normals = NULL;
    gizmo->axes_colors = NULL;
    gizmo->ring_positions = NULL;
    gizmo->ring_colors = NULL;
    gizmo->ring_widths = NULL;
}


/**
 * Allocate cached geometry arrays for one gizmo.
 *
 * @param gizmo orientation gizmo
 * @return true on success
 */
static bool _orientation_gizmo_alloc_geometry(DvzOrientationGizmo* gizmo)
{
    ANN(gizmo);
    gizmo->axes_positions = (vec3*)dvz_calloc(GIZMO_AXES_VERTICES, sizeof(vec3));
    gizmo->axes_normals = (vec3*)dvz_calloc(GIZMO_AXES_VERTICES, sizeof(vec3));
    gizmo->axes_colors = (DvzColor*)dvz_calloc(GIZMO_AXES_VERTICES, sizeof(DvzColor));
    gizmo->ring_positions = (vec3*)dvz_calloc(GIZMO_RINGS_POINTS, sizeof(vec3));
    gizmo->ring_colors = (DvzColor*)dvz_calloc(GIZMO_RINGS_POINTS, sizeof(DvzColor));
    gizmo->ring_widths = (float*)dvz_calloc(GIZMO_RINGS_POINTS, sizeof(float));

    if (gizmo->axes_positions != NULL && gizmo->axes_normals != NULL &&
        gizmo->axes_colors != NULL && gizmo->ring_positions != NULL &&
        gizmo->ring_colors != NULL && gizmo->ring_widths != NULL)
        return true;

    _orientation_gizmo_free_geometry(gizmo);
    return false;
}


/**
 * Refresh one gizmo inset panel layout from its placement.
 *
 * @param gizmo orientation gizmo
 * @return whether the layout is valid
 */
static bool _orientation_gizmo_refresh_layout(DvzOrientationGizmo* gizmo)
{
    ANN(gizmo);
    if (!gizmo->active || gizmo->source_panel == NULL || gizmo->panel == NULL)
        return false;

    DvzPanelDesc desc = {0};
    if (!_orientation_gizmo_panel_desc(gizmo->source_panel, &gizmo->desc.placement, &desc))
        return false;
    if (!dvz_panel_set_desc(gizmo->panel, desc))
        return false;

    if (gizmo->panel->camera != NULL)
    {
        float width = 0.0f;
        float height = 0.0f;
        _scene_panel_pixel_size(gizmo->panel, &width, &height);
        dvz_camera_resize(gizmo->panel->camera, width, height);
    }
    return true;
}


/**
 * Populate the retained mesh and path visuals for one gizmo.
 *
 * @param gizmo orientation gizmo
 * @return whether data upload succeeded
 */
static bool _orientation_gizmo_update_geometry(DvzOrientationGizmo* gizmo)
{
    ANN(gizmo);
    ANN(gizmo->axes_visual);
    ANN(gizmo->rings_visual);
    ANN(gizmo->axes_positions);
    ANN(gizmo->axes_normals);
    ANN(gizmo->axes_colors);
    ANN(gizmo->ring_positions);
    ANN(gizmo->ring_colors);
    ANN(gizmo->ring_widths);

    const GizmoGeometry geometry = _gizmo_geometry(&gizmo->desc);
    GizmoMesh axes = {
        .positions = gizmo->axes_positions,
        .normals = gizmo->axes_normals,
        .colors = gizmo->axes_colors,
        .capacity = GIZMO_AXES_VERTICES,
    };
    GizmoPath rings = {
        .positions = gizmo->ring_positions,
        .colors = gizmo->ring_colors,
        .stroke_widths = gizmo->ring_widths,
        .capacity = GIZMO_RINGS_POINTS,
    };
    if (!_gizmo_build_axes(&axes, &geometry))
        return false;
    if (!_gizmo_build_rings(&rings, &geometry))
        return false;

    DvzVisualDataUpdate axes_updates[] = {
        {.attr_name = "position", .data = gizmo->axes_positions, .item_count = axes.count},
        {.attr_name = "normal", .data = gizmo->axes_normals, .item_count = axes.count},
        {.attr_name = "color", .data = gizmo->axes_colors, .item_count = axes.count},
    };
    if (dvz_visual_set_data_many(gizmo->axes_visual, axes_updates, 3) != 0)
        return false;

    DvzVisualDataUpdate ring_updates[] = {
        {.attr_name = "position", .data = gizmo->ring_positions, .item_count = rings.count},
        {.attr_name = "color", .data = gizmo->ring_colors, .item_count = rings.count},
        {.attr_name = "stroke_width_px", .data = gizmo->ring_widths, .item_count = rings.count},
    };
    if (dvz_visual_set_data_many(gizmo->rings_visual, ring_updates, 3) != 0)
        return false;

    const uint32_t ring_subpaths[GIZMO_AXIS_COUNT] = {
        GIZMO_RING_POINTS,
        GIZMO_RING_POINTS,
        GIZMO_RING_POINTS,
    };
    return dvz_path_set_subpaths(gizmo->rings_visual, GIZMO_AXIS_COUNT, ring_subpaths) == 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the default orientation-gizmo descriptor.
 *
 * @return default orientation-gizmo descriptor
 */
DvzOrientationGizmoDesc dvz_orientation_gizmo_desc(void)
{
    return (DvzOrientationGizmoDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzOrientationGizmoDesc),
        .placement =
            (DvzPlacement){
                .space = DVZ_PLACEMENT_SPACE_PANEL,
                .horizontal_anchor = DVZ_HORIZONTAL_ANCHOR_RIGHT,
                .vertical_anchor = DVZ_VERTICAL_ANCHOR_BOTTOM,
                .offset_x_px = -16.0f,
                .offset_y_px = -16.0f,
                .width_px = 150.0f,
                .height_px = 150.0f,
            },
        .show_axes = true,
        .axis_length = 0.82f,
        .axis_width_px = 5.0f,
        .x_color = {242, 80, 86, 255},
        .y_color = {86, 196, 126, 255},
        .z_color = {78, 150, 250, 255},
    };
}


/**
 * Create a passive orientation gizmo attached to one source panel.
 *
 * @param panel source panel
 * @param desc descriptor, or NULL for defaults
 * @return the orientation gizmo, or NULL on validation/allocation error
 */
DvzOrientationGizmo* dvz_orientation_gizmo(
    DvzPanel* panel, const DvzOrientationGizmoDesc* desc)
{
    if (panel == NULL || panel->figure == NULL || panel->figure->scene == NULL)
        return NULL;
    if (!_orientation_gizmo_desc_validate(desc))
        return NULL;
    DvzOrientationGizmoDesc resolved = desc != NULL ? *desc : dvz_orientation_gizmo_desc();
    if (!isfinite(resolved.axis_length) || resolved.axis_length <= 0.0f ||
        !isfinite(resolved.axis_width_px) || resolved.axis_width_px <= 0.0f)
        return NULL;

    DvzScene* scene = panel->figure->scene;

    DvzOrientationGizmo* gizmo = NULL;
    for (uint32_t i = 0; i < scene->orientation_gizmo_count; i++)
    {
        if (!scene->orientation_gizmos[i].active)
        {
            gizmo = &scene->orientation_gizmos[i];
            break;
        }
    }
    if (gizmo == NULL)
    {
        if (scene->orientation_gizmo_count >= DVZ_SCENE_MAX_ORIENTATION_GIZMOS)
            return NULL;
        gizmo = &scene->orientation_gizmos[scene->orientation_gizmo_count++];
    }
    dvz_memset(gizmo, sizeof(DvzOrientationGizmo), 0, sizeof(DvzOrientationGizmo));
    gizmo->scene = scene;
    gizmo->source_panel = panel;
    gizmo->desc = resolved;
    gizmo->active = true;
    gizmo->visible = true;
    gizmo->version = 1;

    DvzPanelDesc panel_desc = {0};
    if (!_orientation_gizmo_panel_desc(panel, &resolved.placement, &panel_desc))
        goto fail;
    gizmo->panel = dvz_panel(panel->figure, panel_desc);
    if (gizmo->panel == NULL)
        goto fail;

    DvzCameraDesc camera = dvz_camera_desc();
    camera.eye[0] = 0.0f;
    camera.eye[1] = 0.0f;
    camera.eye[2] = 3.0f;
    camera.target[0] = 0.0f;
    camera.target[1] = 0.0f;
    camera.target[2] = 0.0f;
    camera.fov_y = 0.64f;
    camera.near_clip = 0.05f;
    camera.far_clip = 20.0f;
    if (dvz_panel_set_camera(gizmo->panel, &camera) == NULL)
        goto fail;
    dvz_panel_set_background_color(gizmo->panel, dvz_color_from_unit(0.0f, 0.0f, 0.0f, 0.0f));

    if (!_orientation_gizmo_alloc_geometry(gizmo))
        goto fail;

    gizmo->axes_visual = dvz_mesh(scene, 0);
    if (gizmo->axes_visual == NULL)
        goto fail;
    gizmo->rings_visual = dvz_path(scene, 0);
    if (gizmo->rings_visual == NULL)
        goto fail;
    if (dvz_path_set_caps(gizmo->rings_visual, DVZ_SEGMENT_CAP_NONE, DVZ_SEGMENT_CAP_NONE) != 0)
        goto fail;
    if (dvz_path_set_join(gizmo->rings_visual, DVZ_PATH_JOIN_MITER, 4.0f) != 0)
        goto fail;
    if (!_orientation_gizmo_update_geometry(gizmo))
        goto fail;

    DvzMaterialDesc material = dvz_phong_material_desc();
    material.light_direction[0] = 0.35f;
    material.light_direction[1] = 0.45f;
    material.light_direction[2] = 0.82f;
    material.phong.ambient = 0.24f;
    material.phong.diffuse = 0.82f;
    material.phong.specular = 0.38f;
    material.phong.shininess = 48.0f;
    if (dvz_visual_set_material(gizmo->axes_visual, &material) != 0)
        goto fail;

    dvz_visual_set_visible(gizmo->axes_visual, resolved.show_axes);
    dvz_visual_set_visible(gizmo->rings_visual, resolved.show_axes);
    DvzVisualAttachDesc attach = dvz_visual_attach_desc();
    attach.controller_mode = DVZ_CONTROLLER_APPLY_VIEW_PROJ;
    attach.coord_space = DVZ_COORD_VIEW;
    if (dvz_panel_add_visual(gizmo->panel, gizmo->rings_visual, &attach) != 0)
        goto fail;
    if (dvz_panel_add_visual(gizmo->panel, gizmo->axes_visual, &attach) != 0)
        goto fail;

    if (!_orientation_gizmo_sync_transform(gizmo))
        goto fail;

    _scene_notify_request_frame(panel->figure);
    return gizmo;

fail:
    dvz_orientation_gizmo_destroy(gizmo);
    return NULL;
}


/**
 * Destroy an orientation gizmo.
 *
 * @param gizmo the orientation gizmo
 */
void dvz_orientation_gizmo_destroy(DvzOrientationGizmo* gizmo)
{
    if (gizmo == NULL || !gizmo->active)
        return;
    DvzFigure* figure = gizmo->source_panel != NULL ? gizmo->source_panel->figure : NULL;
    if (gizmo->axes_visual != NULL)
        dvz_visual_set_visible(gizmo->axes_visual, false);
    if (gizmo->rings_visual != NULL)
        dvz_visual_set_visible(gizmo->rings_visual, false);
    if (gizmo->panel != NULL)
        dvz_panel_destroy(gizmo->panel);
    _orientation_gizmo_free_geometry(gizmo);
    dvz_memset(gizmo, sizeof(DvzOrientationGizmo), 0, sizeof(DvzOrientationGizmo));
    _scene_notify_request_frame(figure);
}


/**
 * Set orientation-gizmo visibility.
 *
 * @param gizmo the orientation gizmo
 * @param visible whether the gizmo should be visible
 */
void dvz_orientation_gizmo_set_visible(DvzOrientationGizmo* gizmo, bool visible)
{
    if (gizmo == NULL || !gizmo->active)
        return;
    if (gizmo->visible == visible)
        return;
    gizmo->visible = visible;
    if (gizmo->axes_visual != NULL)
        dvz_visual_set_visible(gizmo->axes_visual, visible && gizmo->desc.show_axes);
    if (gizmo->rings_visual != NULL)
        dvz_visual_set_visible(gizmo->rings_visual, visible && gizmo->desc.show_axes);
    gizmo->version = gizmo->version == UINT64_MAX ? 1 : gizmo->version + 1;
    _scene_notify_request_frame(gizmo->source_panel != NULL ? gizmo->source_panel->figure : NULL);
}


/**
 * Refresh layout for all active orientation gizmos owned by one figure.
 *
 * @param figure the figure
 */
void _scene_prepare_orientation_gizmos(DvzFigure* figure)
{
    if (figure == NULL || figure->scene == NULL)
        return;
    DvzScene* scene = figure->scene;
    for (uint32_t i = 0; i < scene->orientation_gizmo_count; i++)
    {
        DvzOrientationGizmo* gizmo = &scene->orientation_gizmos[i];
        if (!gizmo->active || gizmo->source_panel == NULL || gizmo->source_panel->figure != figure)
            continue;
        (void)_orientation_gizmo_refresh_layout(gizmo);
        (void)_orientation_gizmo_sync_transform(gizmo);
    }
}
