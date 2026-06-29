/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* arcball_gizmo - cube arcball with a synchronized bottom-right orientation gizmo.
 *
 * Build:  just example-c techniques/arcball_gizmo
 * Run:    ./build/examples/c/techniques/arcball_gizmo
 * Smoke:  ./build/examples/c/techniques/arcball_gizmo 120
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/app.h"
#include "datoviz/geom.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1200u
#define HEIGHT 900u

#define CUBE_SIZE 1.18

#define GIZMO_SEGMENTS             40u
#define GIZMO_AXIS_COUNT           3u
#define GIZMO_SHAFT_LENGTH         1.22f
#define GIZMO_TIP_LENGTH           0.34f
#define GIZMO_SHAFT_RADIUS         0.035f
#define GIZMO_TIP_RADIUS           0.105f
#define GIZMO_HUB_HALF_SIZE        0.075f
#define GIZMO_RING_RADIUS          0.92f
#define GIZMO_RING_SEGMENTS        96u
#define GIZMO_RING_STROKE_WIDTH    4.0f
#define ROTATION_SPEED_RAD_PER_SEC 0.42f

#define INSET_LEFT   0.765f
#define INSET_TOP    0.685f
#define INSET_WIDTH  0.205f
#define INSET_HEIGHT 0.275f

#define GIZMO_CYLINDER_VERTEX_COUNT (GIZMO_SEGMENTS * 6u)
#define GIZMO_CONE_VERTEX_COUNT     (GIZMO_SEGMENTS * 6u)
#define GIZMO_RING_POINT_COUNT      (GIZMO_RING_SEGMENTS + 1u)
#define GIZMO_HUB_VERTEX_COUNT      36u
#define GIZMO_AXES_VERTEX_COUNT                                                                  \
    (GIZMO_AXIS_COUNT * (GIZMO_CYLINDER_VERTEX_COUNT + GIZMO_CONE_VERTEX_COUNT) +                 \
     GIZMO_HUB_VERTEX_COUNT)
#define GIZMO_RINGS_POINT_COUNT (GIZMO_AXIS_COUNT * GIZMO_RING_POINT_COUNT)

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
} GizmoGeometry;


typedef struct ArcballGizmoState
{
    DvzScene* scene;
    DvzPanel* gizmo_panel;
    DvzVisual* gizmo_axes_visual;
    DvzVisual* gizmo_rings_visual;
    DvzExampleVisualSpin spin;
    GizmoMesh* axes_mesh;
    GizmoPath* rings_path;
    GizmoGeometry geometry;
    DvzPanelDesc inset_desc;
    bool auto_rotate;
    bool show_axes;
    bool show_rings;
    bool show_gizmo;
    float spin_speed;
    float light_direction[3];
    float ambient;
    float diffuse;
    float specular;
    float shininess;
} ArcballGizmoState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill the orthonormal basis for one gizmo axis.
 *
 * @param axis axis index, where 0 is X, 1 is Y, and 2 is Z
 * @param dir output axis direction
 * @param u output first radial basis vector
 * @param v output second radial basis vector
 */
static void _axis_basis(uint32_t axis, vec3 dir, vec3 u, vec3 v)
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
 * Return the display color for one gizmo axis.
 *
 * @param axis axis index
 * @return RGBA color
 */
static DvzColor _axis_color(uint32_t axis)
{
    const DvzColor colors[GIZMO_AXIS_COUNT] = {
        {242, 80, 86, 255},
        {86, 196, 126, 255},
        {78, 150, 250, 255},
    };
    return colors[axis % GIZMO_AXIS_COUNT];
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
static void _radial_at(const vec3 u, const vec3 v, float angle, float radius, vec3 out)
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
 * Compute a point on one oriented axis cylinder.
 *
 * @param dir axis direction
 * @param radial radial offset
 * @param distance distance along the axis
 * @param out output point
 */
static void _axis_point(const vec3 dir, const vec3 radial, float distance, vec3 out)
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
static bool _append_shaft(GizmoMesh* mesh, const GizmoGeometry* geometry, uint32_t axis)
{
    ANN(mesh);
    ANN(geometry);

    vec3 dir = {0};
    vec3 u = {0};
    vec3 v = {0};
    _axis_basis(axis, dir, u, v);
    DvzColor color = _axis_color(axis);

    for (uint32_t i = 0; i < GIZMO_SEGMENTS; i++)
    {
        const float a0 = TAU * (float)i / (float)GIZMO_SEGMENTS;
        const float a1 = TAU * (float)(i + 1u) / (float)GIZMO_SEGMENTS;

        vec3 r0 = {0};
        vec3 r1 = {0};
        _radial_at(u, v, a0, geometry->shaft_radius, r0);
        _radial_at(u, v, a1, geometry->shaft_radius, r1);

        vec3 n0 = {r0[0], r0[1], r0[2]};
        vec3 n1 = {r1[0], r1[1], r1[2]};
        _normalize3(n0);
        _normalize3(n1);

        vec3 p00 = {0};
        vec3 p01 = {0};
        vec3 p10 = {0};
        vec3 p11 = {0};
        _axis_point(dir, r0, 0.0f, p00);
        _axis_point(dir, r1, 0.0f, p01);
        _axis_point(dir, r0, geometry->shaft_length, p10);
        _axis_point(dir, r1, geometry->shaft_length, p11);

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
static bool _append_tip(GizmoMesh* mesh, const GizmoGeometry* geometry, uint32_t axis)
{
    ANN(mesh);
    ANN(geometry);

    vec3 dir = {0};
    vec3 u = {0};
    vec3 v = {0};
    _axis_basis(axis, dir, u, v);
    DvzColor color = _axis_color(axis);

    vec3 apex = {0};
    vec3 base_center = {0};
    _axis_point(dir, (vec3){0}, geometry->shaft_length + geometry->tip_length, apex);
    _axis_point(dir, (vec3){0}, geometry->shaft_length, base_center);

    for (uint32_t i = 0; i < GIZMO_SEGMENTS; i++)
    {
        const float a0 = TAU * (float)i / (float)GIZMO_SEGMENTS;
        const float a1 = TAU * (float)(i + 1u) / (float)GIZMO_SEGMENTS;

        vec3 r0 = {0};
        vec3 r1 = {0};
        _radial_at(u, v, a0, geometry->tip_radius, r0);
        _radial_at(u, v, a1, geometry->tip_radius, r1);

        vec3 p0 = {0};
        vec3 p1 = {0};
        _axis_point(dir, r0, geometry->shaft_length, p0);
        _axis_point(dir, r1, geometry->shaft_length, p1);

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
static bool _append_hub(GizmoMesh* mesh, const GizmoGeometry* geometry)
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
static bool _gizmo_path_point(
    GizmoPath* path, const vec3 position, const DvzColor color, float stroke_width_px)
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
static bool _build_gizmo_axes(GizmoMesh* mesh, const GizmoGeometry* geometry)
{
    ANN(mesh);
    ANN(geometry);

    mesh->count = 0;

    for (uint32_t axis = 0; axis < GIZMO_AXIS_COUNT; axis++)
    {
        if (!_append_shaft(mesh, geometry, axis))
            return false;
        if (!_append_tip(mesh, geometry, axis))
            return false;
    }

    if (!_append_hub(mesh, geometry))
        return false;

    return mesh->count == GIZMO_AXES_VERTEX_COUNT;
}



/**
 * Fill the retained path buffers for the orientation rings.
 *
 * @param path target path arrays
 * @param geometry gizmo geometry controls
 * @return true when the path was fully generated
 */
static bool _build_gizmo_rings(GizmoPath* path, const GizmoGeometry* geometry)
{
    ANN(path);
    ANN(geometry);

    path->count = 0;

    for (uint32_t axis = 0; axis < GIZMO_AXIS_COUNT; axis++)
    {
        vec3 dir = {0};
        vec3 u = {0};
        vec3 v = {0};
        _axis_basis(axis, dir, u, v);
        DvzColor color = _axis_color(axis);
        color.a = 255;

        for (uint32_t i = 0; i < GIZMO_RING_POINT_COUNT; i++)
        {
            const float angle = TAU * (float)(i % GIZMO_RING_SEGMENTS) /
                                (float)GIZMO_RING_SEGMENTS;
            vec3 p = {0};
            _radial_at(u, v, angle, geometry->ring_radius, p);
            if (!_gizmo_path_point(path, p, color, geometry->ring_stroke_width))
                return false;
        }
    }
    return path->count == GIZMO_RINGS_POINT_COUNT;
}



/**
 * Return default geometry controls for the inset gizmo.
 *
 * @return default geometry controls
 */
static GizmoGeometry _gizmo_geometry_defaults(void)
{
    return (GizmoGeometry){
        .shaft_length = GIZMO_SHAFT_LENGTH,
        .tip_length = GIZMO_TIP_LENGTH,
        .shaft_radius = GIZMO_SHAFT_RADIUS,
        .tip_radius = GIZMO_TIP_RADIUS,
        .hub_half_size = GIZMO_HUB_HALF_SIZE,
        .ring_radius = GIZMO_RING_RADIUS,
        .ring_stroke_width = GIZMO_RING_STROKE_WIDTH,
    };
}



/**
 * Apply the retained mesh/path payloads for the current gizmo geometry.
 *
 * @param state live GUI state
 * @return whether the visual data updates succeeded
 */
static bool _gizmo_apply_geometry(ArcballGizmoState* state)
{
    ANN(state);
    ANN(state->axes_mesh);
    ANN(state->rings_path);
    ANN(state->gizmo_axes_visual);
    ANN(state->gizmo_rings_visual);

    if (!_build_gizmo_axes(state->axes_mesh, &state->geometry))
        return false;
    if (!_build_gizmo_rings(state->rings_path, &state->geometry))
        return false;

    DvzVisualDataUpdate axes_updates[] = {
        {
            .attr_name = "position",
            .data = state->axes_mesh->positions,
            .item_count = state->axes_mesh->count,
        },
        {
            .attr_name = "normal",
            .data = state->axes_mesh->normals,
            .item_count = state->axes_mesh->count,
        },
        {
            .attr_name = "color",
            .data = state->axes_mesh->colors,
            .item_count = state->axes_mesh->count,
        },
    };
    if (dvz_visual_set_data_many(state->gizmo_axes_visual, axes_updates, 3) != 0)
        return false;

    DvzVisualDataUpdate ring_updates[] = {
        {
            .attr_name = "position",
            .data = state->rings_path->positions,
            .item_count = state->rings_path->count,
        },
        {
            .attr_name = "color",
            .data = state->rings_path->colors,
            .item_count = state->rings_path->count,
        },
        {
            .attr_name = "stroke_width_px",
            .data = state->rings_path->stroke_widths,
            .item_count = state->rings_path->count,
        },
    };
    if (dvz_visual_set_data_many(state->gizmo_rings_visual, ring_updates, 3) != 0)
        return false;

    const uint32_t ring_subpaths[GIZMO_AXIS_COUNT] = {
        GIZMO_RING_POINT_COUNT,
        GIZMO_RING_POINT_COUNT,
        GIZMO_RING_POINT_COUNT,
    };
    return dvz_path_set_subpaths(state->gizmo_rings_visual, GIZMO_AXIS_COUNT, ring_subpaths) == 0;
}



/**
 * Apply the current lit material controls to the gizmo axes.
 *
 * @param state live GUI state
 * @return whether the material update succeeded
 */
static bool _gizmo_apply_material(ArcballGizmoState* state)
{
    ANN(state);
    ANN(state->gizmo_axes_visual);

    DvzMaterialDesc material = dvz_phong_material_desc();
    material.light_direction[0] = state->light_direction[0];
    material.light_direction[1] = state->light_direction[1];
    material.light_direction[2] = state->light_direction[2];
    material.phong.ambient = state->ambient;
    material.phong.diffuse = state->diffuse;
    material.phong.specular = state->specular;
    material.phong.shininess = state->shininess;
    return dvz_visual_set_material(state->gizmo_axes_visual, &material) == 0;
}



/**
 * Apply current gizmo visual visibility flags.
 *
 * @param state live GUI state
 */
static void _gizmo_apply_visibility(ArcballGizmoState* state)
{
    ANN(state);
    ANN(state->gizmo_axes_visual);
    ANN(state->gizmo_rings_visual);

    dvz_visual_set_visible(state->gizmo_axes_visual, state->show_gizmo && state->show_axes);
    dvz_visual_set_visible(state->gizmo_rings_visual, state->show_gizmo && state->show_rings);
}



/**
 * Apply the current inset panel rectangle.
 *
 * @param state live GUI state
 */
static void _gizmo_apply_layout(ArcballGizmoState* state)
{
    ANN(state);
    ANN(state->gizmo_panel);
    (void)dvz_panel_set_desc(state->gizmo_panel, state->inset_desc);
}



/**
 * Apply current auto-rotation controls.
 *
 * @param state live GUI state
 */
static void _gizmo_apply_spin(ArcballGizmoState* state)
{
    ANN(state);
    if (state->spin.animation == NULL)
        return;
    example_visual_spin_set_speed(&state->spin, state->spin_speed);
    if (state->auto_rotate)
        example_visual_spin_start(&state->spin, 0.0);
    else
        example_visual_spin_stop(&state->spin);
}



/**
 * Reset user-facing controls to the polished example defaults.
 *
 * @param state live GUI state
 */
static void _gizmo_reset_controls(ArcballGizmoState* state)
{
    ANN(state);

    state->geometry = _gizmo_geometry_defaults();
    state->inset_desc = (DvzPanelDesc){
        .x = INSET_LEFT,
        .y = INSET_TOP,
        .width = INSET_WIDTH,
        .height = INSET_HEIGHT,
    };
    state->auto_rotate = true;
    state->show_axes = true;
    state->show_rings = true;
    state->show_gizmo = true;
    state->spin_speed = ROTATION_SPEED_RAD_PER_SEC;
    state->light_direction[0] = 0.25f;
    state->light_direction[1] = 0.52f;
    state->light_direction[2] = 0.82f;
    state->ambient = 0.62f;
    state->diffuse = 0.44f;
    state->specular = 0.68f;
    state->shininess = 76.0f;
}



/**
 * Build live GUI controls for the arcball gizmo example.
 *
 * @param gui GUI overlay
 * @param win view
 * @param user_data example state
 */
static void _arcball_gizmo_gui(DvzGui* gui, DvzView* win, void* user_data)
{
    (void)win;
    ArcballGizmoState* state = (ArcballGizmoState*)user_data;
    if (state == NULL)
        return;

    bool geometry_changed = false;
    bool material_changed = false;
    bool layout_changed = false;
    bool visibility_changed = false;
    bool spin_changed = false;
    bool reset = false;

    if (dvz_gui_begin(gui, "Arcball Gizmo", NULL, 0))
    {
        dvz_gui_separator_text(gui, "Animation");
        spin_changed |= dvz_gui_checkbox(gui, "Auto rotate", &state->auto_rotate);
        spin_changed |= dvz_gui_slider_float_format(
            gui, "Rotation speed", &state->spin_speed, 0.0f, 1.8f, "%.2f rad/s");

        dvz_gui_separator_text(gui, "Inset panel");
        layout_changed |= dvz_gui_slider_float(gui, "X", &state->inset_desc.x, 0.0f, 0.95f);
        layout_changed |= dvz_gui_slider_float(gui, "Y", &state->inset_desc.y, 0.0f, 0.95f);
        layout_changed |=
            dvz_gui_slider_float(gui, "Width", &state->inset_desc.width, 0.08f, 0.45f);
        layout_changed |=
            dvz_gui_slider_float(gui, "Height", &state->inset_desc.height, 0.08f, 0.45f);

        dvz_gui_separator_text(gui, "Visibility");
        visibility_changed |= dvz_gui_checkbox(gui, "Show gizmo", &state->show_gizmo);
        visibility_changed |= dvz_gui_checkbox(gui, "Show axes", &state->show_axes);
        visibility_changed |= dvz_gui_checkbox(gui, "Show rings", &state->show_rings);

        dvz_gui_separator_text(gui, "Geometry");
        geometry_changed |= dvz_gui_slider_float(
            gui, "Shaft length", &state->geometry.shaft_length, 0.45f, 1.80f);
        geometry_changed |= dvz_gui_slider_float_format(
            gui, "Shaft radius", &state->geometry.shaft_radius, 0.012f, 0.080f, "%.3f");
        geometry_changed |=
            dvz_gui_slider_float(gui, "Tip length", &state->geometry.tip_length, 0.10f, 0.70f);
        geometry_changed |= dvz_gui_slider_float_format(
            gui, "Tip radius", &state->geometry.tip_radius, 0.040f, 0.180f, "%.3f");
        geometry_changed |= dvz_gui_slider_float_format(
            gui, "Hub half-size", &state->geometry.hub_half_size, 0.035f, 0.160f, "%.3f");
        geometry_changed |=
            dvz_gui_slider_float(gui, "Ring radius", &state->geometry.ring_radius, 0.45f, 1.30f);
        geometry_changed |= dvz_gui_slider_float_format(
            gui, "Ring width", &state->geometry.ring_stroke_width, 1.0f, 8.0f, "%.1f px");

        dvz_gui_separator_text(gui, "Lighting");
        material_changed |=
            dvz_gui_slider_float3(gui, "Light direction", state->light_direction, -1.0f, 1.0f);
        material_changed |= dvz_gui_slider_float(gui, "Ambient", &state->ambient, 0.0f, 1.0f);
        material_changed |= dvz_gui_slider_float(gui, "Diffuse", &state->diffuse, 0.0f, 1.5f);
        material_changed |= dvz_gui_slider_float(gui, "Specular", &state->specular, 0.0f, 1.5f);
        material_changed |= dvz_gui_slider_float(gui, "Shininess", &state->shininess, 1.0f, 160.0f);

        reset = dvz_gui_button(gui, "Reset");
    }
    dvz_gui_end(gui);

    if (reset)
    {
        _gizmo_reset_controls(state);
        geometry_changed = true;
        material_changed = true;
        layout_changed = true;
        visibility_changed = true;
        spin_changed = true;
    }
    if (geometry_changed && !_gizmo_apply_geometry(state))
        return;
    if (material_changed)
        (void)_gizmo_apply_material(state);
    if (layout_changed)
        _gizmo_apply_layout(state);
    if (visibility_changed)
        _gizmo_apply_visibility(state);
    if (spin_changed)
        _gizmo_apply_spin(state);
}


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("arcball_gizmo");
    bool video_enabled = (capture.flags & DVZ_APP_CAPTURE_VIDEO) != 0;

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    DvzGeometry* cube = NULL;
    GizmoMesh axes_mesh = {0};
    GizmoPath rings_path = {0};
    ArcballGizmoState gui_state = {0};
    _gizmo_reset_controls(&gui_state);

    axes_mesh.capacity = GIZMO_AXES_VERTEX_COUNT;
    axes_mesh.positions = (vec3*)dvz_calloc(axes_mesh.capacity, sizeof(vec3));
    axes_mesh.normals = (vec3*)dvz_calloc(axes_mesh.capacity, sizeof(vec3));
    axes_mesh.colors = (DvzColor*)dvz_calloc(axes_mesh.capacity, sizeof(DvzColor));
    EXAMPLE_CHECK(
        axes_mesh.positions != NULL && axes_mesh.normals != NULL && axes_mesh.colors != NULL,
        "gizmo axes mesh allocation failed");

    rings_path.capacity = GIZMO_RINGS_POINT_COUNT;
    rings_path.positions = (vec3*)dvz_calloc(rings_path.capacity, sizeof(vec3));
    rings_path.colors = (DvzColor*)dvz_calloc(rings_path.capacity, sizeof(DvzColor));
    rings_path.stroke_widths = (float*)dvz_calloc(rings_path.capacity, sizeof(float));
    EXAMPLE_CHECK(
        rings_path.positions != NULL && rings_path.colors != NULL &&
            rings_path.stroke_widths != NULL,
        "gizmo rings path allocation failed");

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* main_panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(main_panel != NULL, "dvz_panel_full() failed");

    DvzPanel* gizmo_panel = dvz_panel(figure, gui_state.inset_desc);
    EXAMPLE_CHECK(gizmo_panel != NULL, "inset dvz_panel() failed");

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.view.eye[0] = 2.35f;
    camera_desc.view.eye[1] = -2.60f;
    camera_desc.view.eye[2] = 1.90f;
    camera_desc.view.target[0] = 0.0f;
    camera_desc.view.target[1] = 0.0f;
    camera_desc.view.target[2] = 0.0f;
    camera_desc.view.up[0] = 0.0f;
    camera_desc.view.up[1] = 0.0f;
    camera_desc.view.up[2] = 1.0f;
    camera_desc.projection.fov_y = 0.72f;
    camera_desc.projection.near_clip = 0.05f;
    camera_desc.projection.far_clip = 100.0f;
    bool ok = dvz_panel_set_camera(main_panel, &camera_desc);
    EXAMPLE_CHECK(ok, "dvz_panel_set_camera(main_panel) failed");

    DvzCameraDesc gizmo_camera_desc = dvz_camera_desc();
    gizmo_camera_desc.view.target[0] = 0.0f;
    gizmo_camera_desc.view.target[1] = 0.0f;
    gizmo_camera_desc.view.target[2] = 0.0f;
    gizmo_camera_desc.view.eye[0] =
        gizmo_camera_desc.view.target[0] + camera_desc.view.eye[0];
    gizmo_camera_desc.view.eye[1] =
        gizmo_camera_desc.view.target[1] + camera_desc.view.eye[1];
    gizmo_camera_desc.view.eye[2] =
        gizmo_camera_desc.view.target[2] + camera_desc.view.eye[2];
    gizmo_camera_desc.view.up[0] = 0.0f;
    gizmo_camera_desc.view.up[1] = 0.0f;
    gizmo_camera_desc.view.up[2] = 1.0f;
    gizmo_camera_desc.projection.fov_y = 0.76f;
    gizmo_camera_desc.projection.near_clip = 0.05f;
    gizmo_camera_desc.projection.far_clip = 100.0f;
    ok = dvz_panel_set_camera(gizmo_panel, &gizmo_camera_desc);
    EXAMPLE_CHECK(ok, "dvz_panel_set_camera(gizmo_panel) failed");

    DvzVisual* cube_visual = dvz_mesh(scene, 0);
    DvzVisual* gizmo_axes_visual = dvz_mesh(scene, 0);
    DvzVisual* gizmo_rings_visual = dvz_path(scene, 0);
    EXAMPLE_CHECK(
        cube_visual != NULL && gizmo_axes_visual != NULL && gizmo_rings_visual != NULL,
        "visual creation failed");

    const DvzColor face_colors[DVZ_GEOM_CUBE_FACE_COUNT] = {
        {240, 82, 82, 255},  {78, 154, 246, 255},  {96, 190, 126, 255},
        {250, 202, 70, 255}, {172, 105, 235, 255}, {236, 112, 74, 255},
    };
    cube = dvz_geom_cube(&(DvzGeometryCubeDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometryCubeDesc),
        .size = CUBE_SIZE,
        .face_colors = face_colors,
        .face_color_count = DVZ_GEOM_CUBE_FACE_COUNT,
    });
    EXAMPLE_CHECK(cube != NULL, "dvz_geom_cube() failed");

    ok = dvz_mesh_set_geometry(cube_visual, cube) == 0;
    EXAMPLE_CHECK(ok, "dvz_mesh_set_geometry() failed for cube");
    dvz_geometry_destroy(cube);
    cube = NULL;

    gui_state.scene = scene;
    gui_state.gizmo_panel = gizmo_panel;
    gui_state.gizmo_axes_visual = gizmo_axes_visual;
    gui_state.gizmo_rings_visual = gizmo_rings_visual;
    gui_state.axes_mesh = &axes_mesh;
    gui_state.rings_path = &rings_path;

    ok = _gizmo_apply_geometry(&gui_state);
    EXAMPLE_CHECK(ok, "failed to initialize gizmo geometry");

    int rc = 0;
    rc = dvz_path_set_caps(gizmo_rings_visual, DVZ_SEGMENT_CAP_NONE, DVZ_SEGMENT_CAP_NONE);
    EXAMPLE_CHECK(rc == 0, "dvz_path_set_caps() failed for gizmo rings");
    rc = dvz_path_set_join(gizmo_rings_visual, DVZ_PATH_JOIN_MITER, 4.0f);
    EXAMPLE_CHECK(rc == 0, "dvz_path_set_join() failed for gizmo rings");

    DvzMaterialDesc cube_material = dvz_phong_material_desc();
    cube_material.light_direction[0] = 0.35f;
    cube_material.light_direction[1] = 0.45f;
    cube_material.light_direction[2] = 0.82f;
    cube_material.phong.ambient = 0.24f;
    cube_material.phong.diffuse = 0.82f;
    cube_material.phong.specular = 0.38f;
    cube_material.phong.shininess = 48.0f;
    rc = dvz_visual_set_material(cube_visual, &cube_material);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_material() failed for cube");

    ok = _gizmo_apply_material(&gui_state);
    EXAMPLE_CHECK(ok, "dvz_visual_set_material() failed for gizmo axes");
    _gizmo_apply_visibility(&gui_state);

    rc = dvz_panel_add_visual(main_panel, cube_visual, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed for cube");
    rc = dvz_panel_add_visual(gizmo_panel, gizmo_rings_visual, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed for gizmo rings");
    rc = dvz_panel_add_visual(gizmo_panel, gizmo_axes_visual, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed for gizmo axes");
    dvz_panel_set_background_color(main_panel, dvz_color_from_unit(0.035f, 0.038f, 0.046f, 1.0f));
    dvz_panel_set_background_color(gizmo_panel, dvz_color_from_unit(0.080f, 0.087f, 0.100f, 1.0f));

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "arcball gizmo");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzGui* gui = dvz_view_gui(win, NULL);
    EXAMPLE_CHECK(gui != NULL, "dvz_view_gui() failed");
    dvz_view_set_gui_callback(win, _arcball_gizmo_gui, &gui_state);

    DvzArcball* arcball = dvz_view_arcball(win, main_panel, NULL);
    EXAMPLE_CHECK(arcball != NULL, "failed to create or bind arcball controller");
    dvz_arcball_set(arcball, (vec3){+0.62f, -0.18f, +0.42f});
    DvzController* main_controller = dvz_panel_controller(main_panel, DVZ_DIM_X);
    EXAMPLE_CHECK(main_controller != NULL, "failed to resolve main arcball controller");

    DvzController* gizmo_controller = dvz_arcball(scene, NULL);
    DvzArcball* gizmo_arcball = dvz_controller_arcball(gizmo_controller);
    EXAMPLE_CHECK(gizmo_arcball != NULL, "failed to create inset arcball controller");
    rc = dvz_panel_bind_controller(gizmo_panel, gizmo_controller, DVZ_DIM_MASK_XYZ);
    EXAMPLE_CHECK(rc == 0, "failed to bind arcball controller to inset gizmo panel");
    DvzControllerLink* gizmo_link = dvz_controller_link(
        scene, main_controller, gizmo_controller, DVZ_CONTROLLER_LINK_ROTATION,
        DVZ_CONTROLLER_LINK_ONE_WAY);
    EXAMPLE_CHECK(gizmo_link != NULL, "failed to link main arcball rotation to inset gizmo");

    dvz_scene_set_clock_mode(scene, video_enabled ? DVZ_CLOCK_OFFLINE : DVZ_CLOCK_REALTIME);
    dvz_scene_set_fps(scene, 60.0);

    rc = dvz_view_capture_start(win, &capture);
    EXAMPLE_CHECK(rc == 0, "dvz_view_capture_start() failed");

    EXAMPLE_CHECK(
        example_visual_spin(
            scene, cube_visual, (vec3){0.0f, 0.0f, 1.0f}, ROTATION_SPEED_RAD_PER_SEC, NULL,
            &gui_state.spin),
        "example_visual_spin() failed");
    _gizmo_apply_spin(&gui_state);

    dvz_app_run(app, frame_count);
    rc = dvz_view_capture_stop(win);
    EXAMPLE_CHECK(rc == 0, "dvz_view_capture_stop() failed");
    ret = 0;

cleanup:
    if (cube != NULL)
        dvz_geometry_destroy(cube);
    if (app != NULL)
        dvz_app_destroy(app);
    example_visual_spin_destroy(&gui_state.spin);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    dvz_free(axes_mesh.positions);
    dvz_free(axes_mesh.normals);
    dvz_free(axes_mesh.colors);
    dvz_free(rings_path.positions);
    dvz_free(rings_path.colors);
    dvz_free(rings_path.stroke_widths);
    return ret;
}
