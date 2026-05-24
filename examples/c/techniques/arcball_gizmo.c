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
#define GIZMO_RING_WIDTH           0.012f
#define GIZMO_RING_SEGMENTS        96u
#define ROTATION_SPEED_RAD_PER_SEC 0.42f

#define INSET_LEFT   0.765f
#define INSET_TOP    0.685f
#define INSET_WIDTH  0.205f
#define INSET_HEIGHT 0.275f

#define GIZMO_CYLINDER_VERTEX_COUNT (GIZMO_SEGMENTS * 6u)
#define GIZMO_CONE_VERTEX_COUNT     (GIZMO_SEGMENTS * 6u)
#define GIZMO_RING_VERTEX_COUNT     (GIZMO_RING_SEGMENTS * 6u)
#define GIZMO_HUB_VERTEX_COUNT      36u
#define GIZMO_VERTEX_COUNT                                                                        \
    (GIZMO_AXIS_COUNT * (GIZMO_CYLINDER_VERTEX_COUNT + GIZMO_CONE_VERTEX_COUNT) +                 \
     GIZMO_AXIS_COUNT * GIZMO_RING_VERTEX_COUNT + GIZMO_HUB_VERTEX_COUNT)

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


typedef struct GizmoSyncState
{
    DvzArcball* main_arcball;
    DvzArcball* gizmo_arcball;
} GizmoSyncState;



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
 * @param out output RGBA color
 */
static void _axis_color(uint32_t axis, DvzColor out)
{
    ANN(out);

    const DvzColor colors[GIZMO_AXIS_COUNT] = {
        {242, 80, 86, 255},
        {86, 196, 126, 255},
        {78, 150, 250, 255},
    };
    dvz_memcpy(out, sizeof(DvzColor), colors[axis % GIZMO_AXIS_COUNT], sizeof(DvzColor));
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
    ANN(color);

    if (mesh->count >= mesh->capacity)
        return false;

    dvz_memcpy(mesh->positions[mesh->count], sizeof(vec3), position, sizeof(vec3));
    dvz_memcpy(mesh->normals[mesh->count], sizeof(vec3), normal, sizeof(vec3));
    dvz_memcpy(mesh->colors[mesh->count], sizeof(DvzColor), color, sizeof(DvzColor));
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
    ANN(color);

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
 * @param axis axis index
 * @return true when all shaft triangles were appended
 */
static bool _append_shaft(GizmoMesh* mesh, uint32_t axis)
{
    ANN(mesh);

    vec3 dir = {0};
    vec3 u = {0};
    vec3 v = {0};
    DvzColor color = {0};
    _axis_basis(axis, dir, u, v);
    _axis_color(axis, color);

    for (uint32_t i = 0; i < GIZMO_SEGMENTS; i++)
    {
        const float a0 = TAU * (float)i / (float)GIZMO_SEGMENTS;
        const float a1 = TAU * (float)(i + 1u) / (float)GIZMO_SEGMENTS;

        vec3 r0 = {0};
        vec3 r1 = {0};
        _radial_at(u, v, a0, GIZMO_SHAFT_RADIUS, r0);
        _radial_at(u, v, a1, GIZMO_SHAFT_RADIUS, r1);

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
        _axis_point(dir, r0, GIZMO_SHAFT_LENGTH, p10);
        _axis_point(dir, r1, GIZMO_SHAFT_LENGTH, p11);

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
 * @param axis axis index
 * @return true when all cone triangles were appended
 */
static bool _append_tip(GizmoMesh* mesh, uint32_t axis)
{
    ANN(mesh);

    vec3 dir = {0};
    vec3 u = {0};
    vec3 v = {0};
    DvzColor color = {0};
    _axis_basis(axis, dir, u, v);
    _axis_color(axis, color);

    vec3 apex = {0};
    vec3 base_center = {0};
    _axis_point(dir, (vec3){0}, GIZMO_SHAFT_LENGTH + GIZMO_TIP_LENGTH, apex);
    _axis_point(dir, (vec3){0}, GIZMO_SHAFT_LENGTH, base_center);

    for (uint32_t i = 0; i < GIZMO_SEGMENTS; i++)
    {
        const float a0 = TAU * (float)i / (float)GIZMO_SEGMENTS;
        const float a1 = TAU * (float)(i + 1u) / (float)GIZMO_SEGMENTS;

        vec3 r0 = {0};
        vec3 r1 = {0};
        _radial_at(u, v, a0, GIZMO_TIP_RADIUS, r0);
        _radial_at(u, v, a1, GIZMO_TIP_RADIUS, r1);

        vec3 p0 = {0};
        vec3 p1 = {0};
        _axis_point(dir, r0, GIZMO_SHAFT_LENGTH, p0);
        _axis_point(dir, r1, GIZMO_SHAFT_LENGTH, p1);

        vec3 n0 = {0};
        vec3 n1 = {0};
        _vec3_add_scaled(
            r0, 1.0f / GIZMO_TIP_RADIUS, dir, GIZMO_TIP_RADIUS / GIZMO_TIP_LENGTH, n0);
        _vec3_add_scaled(
            r1, 1.0f / GIZMO_TIP_RADIUS, dir, GIZMO_TIP_RADIUS / GIZMO_TIP_LENGTH, n1);
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
 * Append one thin orientation ring in the plane perpendicular to an axis.
 *
 * @param mesh target mesh arrays
 * @param axis axis index
 * @return true when all ring triangles were appended
 */
static bool _append_ring(GizmoMesh* mesh, uint32_t axis)
{
    ANN(mesh);

    vec3 dir = {0};
    vec3 u = {0};
    vec3 v = {0};
    DvzColor color = {0};
    _axis_basis(axis, dir, u, v);
    _axis_color(axis, color);
    color[3] = 170;

    for (uint32_t i = 0; i < GIZMO_RING_SEGMENTS; i++)
    {
        const float a0 = TAU * (float)i / (float)GIZMO_RING_SEGMENTS;
        const float a1 = TAU * (float)(i + 1u) / (float)GIZMO_RING_SEGMENTS;

        vec3 p00 = {0};
        vec3 p01 = {0};
        vec3 p10 = {0};
        vec3 p11 = {0};
        _radial_at(u, v, a0, GIZMO_RING_RADIUS - GIZMO_RING_WIDTH, p00);
        _radial_at(u, v, a1, GIZMO_RING_RADIUS - GIZMO_RING_WIDTH, p01);
        _radial_at(u, v, a0, GIZMO_RING_RADIUS + GIZMO_RING_WIDTH, p10);
        _radial_at(u, v, a1, GIZMO_RING_RADIUS + GIZMO_RING_WIDTH, p11);

        if (!_gizmo_triangle(mesh, p00, dir, p11, dir, p10, dir, color))
            return false;
        if (!_gizmo_triangle(mesh, p00, dir, p01, dir, p11, dir, color))
            return false;
    }
    return true;
}



/**
 * Append the small white cube at the gizmo origin.
 *
 * @param mesh target mesh arrays
 * @return true when all hub triangles were appended
 */
static bool _append_hub(GizmoMesh* mesh)
{
    ANN(mesh);

    const float s = GIZMO_HUB_HALF_SIZE;
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
 * Fill the retained mesh buffers for the axis gizmo.
 *
 * @param mesh target mesh arrays
 * @return true when the mesh was fully generated
 */
static bool _build_gizmo(GizmoMesh* mesh)
{
    ANN(mesh);

    for (uint32_t axis = 0; axis < GIZMO_AXIS_COUNT; axis++)
    {
        if (!_append_shaft(mesh, axis))
            return false;
        if (!_append_tip(mesh, axis))
            return false;
        if (!_append_ring(mesh, axis))
            return false;
    }

    if (!_append_hub(mesh))
        return false;

    return mesh->count == GIZMO_VERTEX_COUNT;
}


/**
 * Synchronize the inset orientation gizmo without inheriting cube pan.
 *
 * @param animation timer animation handle
 * @param t scene clock time
 * @param dt scene clock delta
 * @param user_data sync state
 */
static void _sync_gizmo_arcball(DvzAnimation* animation, double t, double dt, void* user_data)
{
    (void)animation;
    (void)t;
    (void)dt;
    GizmoSyncState* state = (GizmoSyncState*)user_data;
    if (state == NULL || state->main_arcball == NULL || state->gizmo_arcball == NULL)
        return;

    DvzArcball* src = state->main_arcball;
    DvzArcball* dst = state->gizmo_arcball;
    dvz_memcpy(dst->mat, sizeof(dst->mat), src->mat, sizeof(src->mat));
    dvz_memcpy(dst->rotation, sizeof(dst->rotation), src->rotation, sizeof(src->rotation));
    dst->zoom = 1.0f;
    dst->pan[0] = 0.0f;
    dst->pan[1] = 0.0f;
    dst->pan_center[0] = 0.0f;
    dst->pan_center[1] = 0.0f;
    dst->interacting = src->interacting;
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
    GizmoMesh mesh = {0};
    GizmoSyncState sync = {0};

    mesh.capacity = GIZMO_VERTEX_COUNT;
    mesh.positions = (vec3*)dvz_calloc(mesh.capacity, sizeof(vec3));
    mesh.normals = (vec3*)dvz_calloc(mesh.capacity, sizeof(vec3));
    mesh.colors = (DvzColor*)dvz_calloc(mesh.capacity, sizeof(DvzColor));
    EXAMPLE_CHECK(
        mesh.positions != NULL && mesh.normals != NULL && mesh.colors != NULL,
        "gizmo mesh allocation failed");

    bool ok = _build_gizmo(&mesh);
    EXAMPLE_CHECK(ok, "gizmo mesh generation failed");

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* main_panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(main_panel != NULL, "dvz_panel_full() failed");

    DvzPanel* gizmo_panel = dvz_panel(
        figure, (DvzPanelDesc){
                    .x = INSET_LEFT,
                    .y = INSET_TOP,
                    .width = INSET_WIDTH,
                    .height = INSET_HEIGHT,
                });
    EXAMPLE_CHECK(gizmo_panel != NULL, "inset dvz_panel() failed");

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[0] = 2.35f;
    camera_desc.eye[1] = -2.60f;
    camera_desc.eye[2] = 1.90f;
    camera_desc.target[0] = 0.0f;
    camera_desc.target[1] = 0.0f;
    camera_desc.target[2] = 0.0f;
    camera_desc.up[0] = 0.0f;
    camera_desc.up[1] = 0.0f;
    camera_desc.up[2] = 1.0f;
    camera_desc.fov_y = 0.72f;
    camera_desc.near = 0.05f;
    camera_desc.far = 100.0f;
    ok = dvz_panel_set_camera(main_panel, &camera_desc);
    EXAMPLE_CHECK(ok, "dvz_panel_set_camera(main_panel) failed");

    DvzCameraDesc gizmo_camera_desc = dvz_camera_desc();
    gizmo_camera_desc.target[0] = 0.46f;
    gizmo_camera_desc.target[1] = 0.46f;
    gizmo_camera_desc.target[2] = 0.46f;
    gizmo_camera_desc.eye[0] = gizmo_camera_desc.target[0] + camera_desc.eye[0];
    gizmo_camera_desc.eye[1] = gizmo_camera_desc.target[1] + camera_desc.eye[1];
    gizmo_camera_desc.eye[2] = gizmo_camera_desc.target[2] + camera_desc.eye[2];
    gizmo_camera_desc.up[0] = 0.0f;
    gizmo_camera_desc.up[1] = 0.0f;
    gizmo_camera_desc.up[2] = 1.0f;
    gizmo_camera_desc.fov_y = 0.76f;
    gizmo_camera_desc.near = 0.05f;
    gizmo_camera_desc.far = 100.0f;
    ok = dvz_panel_set_camera(gizmo_panel, &gizmo_camera_desc);
    EXAMPLE_CHECK(ok, "dvz_panel_set_camera(gizmo_panel) failed");

    DvzVisual* cube_visual = dvz_mesh(scene, 0);
    DvzVisual* gizmo_visual = dvz_mesh(scene, 0);
    EXAMPLE_CHECK(cube_visual != NULL && gizmo_visual != NULL, "dvz_mesh() failed");

    const DvzColor face_colors[DVZ_GEOM_CUBE_FACE_COUNT] = {
        {240, 82, 82, 255},  {78, 154, 246, 255},  {96, 190, 126, 255},
        {250, 202, 70, 255}, {172, 105, 235, 255}, {236, 112, 74, 255},
    };
    cube = dvz_geom_cube(&(DvzGeometryCubeDesc){
        .size = CUBE_SIZE,
        .face_colors = face_colors,
        .face_color_count = DVZ_GEOM_CUBE_FACE_COUNT,
    });
    EXAMPLE_CHECK(cube != NULL, "dvz_geom_cube() failed");

    ok = example_mesh_geometry(cube_visual, cube);
    EXAMPLE_CHECK(ok, "example_mesh_geometry() failed for cube");
    dvz_geometry_destroy(cube);
    cube = NULL;

    DvzVisualDataUpdate gizmo_updates[] = {
        {.attr_name = "position", .data = mesh.positions, .item_count = mesh.count},
        {.attr_name = "normal", .data = mesh.normals, .item_count = mesh.count},
        {.attr_name = "color", .data = mesh.colors, .item_count = mesh.count},
    };
    int rc = dvz_visual_set_data_many(gizmo_visual, gizmo_updates, 3);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data_many() failed for gizmo");

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

    DvzMaterialDesc gizmo_material = dvz_phong_material_desc();
    gizmo_material.light_direction[0] = 0.25f;
    gizmo_material.light_direction[1] = 0.52f;
    gizmo_material.light_direction[2] = 0.82f;
    gizmo_material.phong.ambient = 0.34f;
    gizmo_material.phong.diffuse = 0.78f;
    gizmo_material.phong.specular = 0.42f;
    gizmo_material.phong.shininess = 58.0f;
    rc = dvz_visual_set_material(gizmo_visual, &gizmo_material);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_material() failed for gizmo");

    rc = dvz_panel_add_visual(main_panel, cube_visual, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed for cube");
    rc = dvz_panel_add_visual(gizmo_panel, gizmo_visual, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed for gizmo");
    dvz_panel_set_background_color(main_panel, 0.035f, 0.038f, 0.046f, 1.0f);
    dvz_panel_set_background_color(gizmo_panel, 0.080f, 0.087f, 0.100f, 1.0f);

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "arcball gizmo");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzArcball* arcball = dvz_view_arcball(win, main_panel, NULL);
    EXAMPLE_CHECK(arcball != NULL, "failed to create or bind arcball controller");
    dvz_arcball_set(arcball, (vec3){+0.62f, -0.18f, +0.42f});

    DvzController* gizmo_controller = dvz_arcball(scene, NULL);
    DvzArcball* gizmo_arcball = dvz_controller_arcball(gizmo_controller);
    EXAMPLE_CHECK(gizmo_arcball != NULL, "failed to create inset arcball controller");
    rc = dvz_panel_bind_controller(gizmo_panel, gizmo_controller, DVZ_DIM_MASK_XYZ);
    EXAMPLE_CHECK(rc == 0, "failed to bind arcball controller to inset gizmo panel");
    sync.main_arcball = arcball;
    sync.gizmo_arcball = gizmo_arcball;
    _sync_gizmo_arcball(NULL, 0.0, 0.0, &sync);

    dvz_scene_set_clock_mode(scene, video_enabled ? DVZ_CLOCK_OFFLINE : DVZ_CLOCK_REALTIME);
    dvz_scene_set_fps(scene, 60.0);

    rc = dvz_view_capture_start(win, &capture);
    EXAMPLE_CHECK(rc == 0, "dvz_view_capture_start() failed");

    DvzAnimation* spin = dvz_anim_arcball_spin(
        scene, arcball, (vec3){0.0f, 0.0f, 1.0f}, ROTATION_SPEED_RAD_PER_SEC,
        DVZ_ARCBALL_SPIN_FLAGS_PAUSE_ON_INTERACTION);
    EXAMPLE_CHECK(spin != NULL, "dvz_anim_arcball_spin() failed");
    dvz_anim_start(spin, 0.0);

    DvzAnimation* sync_anim = dvz_anim_timer(scene, 0.0, _sync_gizmo_arcball, &sync);
    EXAMPLE_CHECK(sync_anim != NULL, "dvz_anim_timer() failed for inset gizmo sync");
    dvz_anim_start(sync_anim, 0.0);

    dvz_app_run(app, frame_count);
    rc = dvz_view_capture_stop(win);
    EXAMPLE_CHECK(rc == 0, "dvz_view_capture_stop() failed");
    ret = 0;

cleanup:
    if (cube != NULL)
        dvz_geometry_destroy(cube);
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    dvz_free(mesh.positions);
    dvz_free(mesh.normals);
    dvz_free(mesh.colors);
    return ret;
}
