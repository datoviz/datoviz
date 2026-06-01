/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* instanced_cubes - live lit cube grid using mesh instance transforms.
 *
 * Build:  just example-c visuals/instanced_cubes
 * Run:    ./build/examples/c/visuals/instanced_cubes
 * Smoke:  ./build/examples/c/visuals/instanced_cubes 60
 * DVZR:   DVZ_CAPTURE=dvzr ./build/examples/c/visuals/instanced_cubes 60
 * Video:  DVZ_CAPTURE=mp4 ./build/examples/c/visuals/instanced_cubes 60
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "datoviz/app.h"
#include "datoviz/geom.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1600
#define HEIGHT 1200

#define GRID_X 13u
#define GRID_Y 9u
#define GRID_Z 5u

#define CUBE_SIZE 0.28

#define ROTATION_SPEED_RAD_PER_SEC 0.45f

static const float TAU = 6.28318530718f;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Fill one column-major rotation/scale/translation transform.
 *
 * @param transform output mat4 storage
 * @param tx translation on X
 * @param ty translation on Y
 * @param tz translation on Z
 * @param scale uniform scale
 * @param angle_z rotation around Z in radians
 * @param angle_y rotation around Y in radians
 */
static void _cube_transform(
    float transform[16], float tx, float ty, float tz, float scale, float angle_z, float angle_y)
{
    ANN(transform);

    const float cz = cosf(angle_z);
    const float sz = sinf(angle_z);
    const float cy = cosf(angle_y);
    const float sy = sinf(angle_y);

    transform[0] = scale * cz * cy;
    transform[1] = scale * sz * cy;
    transform[2] = scale * -sy;
    transform[3] = 0.0f;
    transform[4] = scale * -sz;
    transform[5] = scale * cz;
    transform[6] = 0.0f;
    transform[7] = 0.0f;
    transform[8] = scale * cz * sy;
    transform[9] = scale * sz * sy;
    transform[10] = scale * cy;
    transform[11] = 0.0f;
    transform[12] = tx;
    transform[13] = ty;
    transform[14] = tz;
    transform[15] = 1.0f;
}



/**
 * Allocate and fill the cube instance transform grid.
 *
 * @param out_count output transform count
 * @return heap-allocated transform array, or NULL on failure
 */
static float (*_make_cube_transforms(uint32_t* out_count))[16]
{
    ANN(out_count);

    const uint32_t count = GRID_X * GRID_Y * GRID_Z;
    float (*transforms)[16] = (float(*)[16])dvz_calloc(count, sizeof(*transforms));
    if (transforms == NULL)
        return NULL;

    const float spacing = 0.42f;
    const float half_x = 0.5f * (float)(GRID_X - 1u);
    const float half_y = 0.5f * (float)(GRID_Y - 1u);
    const float half_z = 0.5f * (float)(GRID_Z - 1u);
    uint32_t idx = 0;
    for (uint32_t z = 0; z < GRID_Z; z++)
    {
        for (uint32_t y = 0; y < GRID_Y; y++)
        {
            for (uint32_t x = 0; x < GRID_X; x++)
            {
                const float fx = ((float)x - half_x) * spacing;
                const float fy = ((float)y - half_y) * spacing;
                const float fz = ((float)z - half_z) * spacing;
                const float wave =
                    0.5f + 0.5f * sinf(TAU * ((float)x / (float)GRID_X + (float)z * 0.09f));
                const float scale = 0.58f + 0.42f * wave;
                const float angle_z = 0.12f * (float)x + 0.19f * (float)y;
                const float angle_y = 0.21f * (float)z - 0.08f * (float)y;
                _cube_transform(transforms[idx++], fx, fy, fz, scale, angle_z, angle_y);
            }
        }
    }

    *out_count = count;
    return transforms;
}



int main(int argc, char** argv)
{
    uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("instanced_cubes");
    bool video_enabled = (capture.flags & DVZ_APP_CAPTURE_VIDEO) != 0;

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    DvzGeometry* cube = NULL;
    float (*transforms)[16] = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[0] = 0.0f;
    camera_desc.eye[1] = -5.8f;
    camera_desc.eye[2] = 5.0f;
    camera_desc.up[1] = 0.0f;
    camera_desc.up[2] = 1.0f;
    camera_desc.fov_y = 0.78539816339f;
    camera_desc.near = 0.1f;
    camera_desc.far = 100.0f;
    bool ok = dvz_panel_set_camera(panel, &camera_desc);
    EXAMPLE_CHECK(ok, "dvz_panel_set_camera() failed");

    DvzVisual* visual = dvz_mesh(scene, 0);
    EXAMPLE_CHECK(visual != NULL, "dvz_mesh() failed");

    const DvzColor face_colors[DVZ_GEOM_CUBE_FACE_COUNT] = {
        {229, 57, 53, 255},
        {30, 136, 229, 255},
        {67, 160, 71, 255},
        {251, 192, 45, 255},
        {142, 68, 173, 255},
        {244, 81, 30, 255},
    };
    cube = dvz_geom_cube(&(DvzGeometryCubeDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometryCubeDesc),
        .size = CUBE_SIZE,
        .face_colors = face_colors,
        .face_color_count = DVZ_GEOM_CUBE_FACE_COUNT,
    });
    EXAMPLE_CHECK(cube != NULL, "dvz_geom_cube() failed");

    ok = example_mesh_geometry(visual, cube);
    EXAMPLE_CHECK(ok, "example_mesh_geometry() failed");
    dvz_geometry_destroy(cube);
    cube = NULL;

    uint32_t instance_count = 0;
    transforms = _make_cube_transforms(&instance_count);
    EXAMPLE_CHECK(transforms != NULL && instance_count > 0, "failed to allocate cube transforms");

    int rc = dvz_visual_set_data(visual, "instance_transform", transforms, instance_count);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data(instance_transform) failed");

    DvzMaterialDesc material = dvz_phong_material_desc();
    material.light_direction[0] = 0.25f;
    material.light_direction[1] = -0.55f;
    material.light_direction[2] = 0.80f;
    material.phong.ambient = 0.22f;
    material.phong.diffuse = 0.80f;
    material.phong.specular = 0.45f;
    material.phong.shininess = 48.0f;
    rc = dvz_visual_set_material(visual, &material);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_material() failed");

    rc = dvz_panel_add_visual(panel, visual, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");
    dvz_panel_set_background_color(panel, 0.04f, 0.045f, 0.052f, 1.0f);

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "instanced cubes");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzArcball* arcball = dvz_view_arcball(win, panel, NULL);
    EXAMPLE_CHECK(arcball != NULL, "failed to create or bind arcball controller");
    dvz_arcball_set(arcball, (vec3){+0.70f, 0.0f, +0.28f});

    dvz_scene_set_clock_mode(scene, video_enabled ? DVZ_CLOCK_OFFLINE : DVZ_CLOCK_REALTIME);
    dvz_scene_set_fps(scene, 60.0);

    rc = dvz_view_capture_start(win, &capture);
    EXAMPLE_CHECK(rc == 0, "dvz_view_capture_start() failed");

    DvzAnimation* spin = dvz_anim_arcball_spin(
        scene, arcball, (vec3){0.0f, 0.0f, 1.0f}, ROTATION_SPEED_RAD_PER_SEC,
        DVZ_ARCBALL_SPIN_FLAGS_PAUSE_ON_INTERACTION);
    EXAMPLE_CHECK(spin != NULL, "dvz_anim_arcball_spin() failed");
    dvz_anim_start(spin, 0.0);

    dvz_app_run(app, frame_count);
    rc = dvz_view_capture_stop(win);
    EXAMPLE_CHECK(rc == 0, "dvz_view_capture_stop() failed");
    ret = 0;

cleanup:
    dvz_free(transforms);
    if (cube != NULL)
        dvz_geometry_destroy(cube);
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
