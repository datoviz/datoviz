/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* mesh — live rotating lit cube mesh via dvz_mesh + scene/app.
 *
 * Opens a GLFW window showing one indexed cube mesh with explicit face normals, per-face colours,
 * depth testing, and a perspective camera. A scene arcball spin animation keeps the retained 3D
 * scene exercising the live scene -> DRP2 -> vklite/canvas path while the user is idle.
 *
 * Build:  just example-c visuals/mesh
 * Run:    ./build/examples/c/visuals/mesh
 * Smoke:  ./build/examples/c/visuals/mesh 60
 * DVZR:   DVZ_CAPTURE=dvzr ./build/examples/c/visuals/mesh 60
 * Video:  DVZ_CAPTURE=mp4 ./build/examples/c/visuals/mesh 60
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/geom.h"
#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  1600
#define HEIGHT 1200

#define MESH_CUBE_SIZE 1.16

#define ROTATION_SPEED_RAD_PER_SEC 0.9f



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    uint32_t frame_count = example_frame_count_any(argc, argv);
    DvzAppCaptureConfig capture = dvz_app_capture_config_from_env("mesh");
    bool video_enabled = (capture.flags & DVZ_APP_CAPTURE_VIDEO) != 0;

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    DvzGeometry* cube = NULL;
    DvzExampleVisualSpin spin = {0};

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel() failed");

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.view.eye[2] = 3.0f;
    camera_desc.view.up[1] = 1.0f;
    camera_desc.projection.fov_y = 0.78539816339f;
    camera_desc.projection.near_clip = 0.1f;
    camera_desc.projection.far_clip = 100.0f;
    bool ok = dvz_panel_set_camera(panel, &camera_desc);
    EXAMPLE_CHECK(ok, "dvz_panel_set_camera() failed");

    DvzVisual* visual = dvz_mesh(scene, 0);
    EXAMPLE_CHECK(visual != NULL, "dvz_mesh() failed");

    const DvzColor face_colors[DVZ_GEOM_CUBE_FACE_COUNT] = {
        {239, 83, 80, 255},
        {66, 165, 245, 255},
        {102, 187, 106, 255},
        {255, 202, 40, 255},
        {171, 71, 188, 255},
        {255, 112, 67, 255},
    };
    cube = dvz_geom_cube(&(DvzGeometryCubeDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometryCubeDesc),
        .size = MESH_CUBE_SIZE,
        .face_colors = face_colors,
        .face_color_count = DVZ_GEOM_CUBE_FACE_COUNT,
    });
    EXAMPLE_CHECK(cube != NULL, "dvz_geom_cube() failed");

    bool uploaded = dvz_mesh_set_geometry(visual, cube) == 0;
    EXAMPLE_CHECK(uploaded, "dvz_mesh_set_geometry() failed");
    dvz_geometry_destroy(cube);
    cube = NULL;

    DvzMaterialDesc material = dvz_phong_material_desc();
    material.light_direction[0] = 0.35f;
    material.light_direction[1] = 0.55f;
    material.light_direction[2] = 0.75f;
    material.phong.ambient = 0.25f;
    material.phong.diffuse = 0.85f;
    dvz_visual_set_material(visual, &material);
    int rc = dvz_panel_add_visual(panel, visual, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.05f, 0.05f, 0.08f, 1.0f));

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "mesh");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzArcball* arcball = dvz_view_arcball(win, panel, NULL);
    EXAMPLE_CHECK(arcball != NULL, "failed to create or bind arcball controller");
    dvz_arcball_set(arcball, (vec3){+0.65f, 0.0f, +0.35f});

    dvz_scene_set_clock_mode(scene, video_enabled ? DVZ_CLOCK_OFFLINE : DVZ_CLOCK_REALTIME);
    dvz_scene_set_fps(scene, 60.0);

    rc = dvz_view_capture_start(win, &capture);
    EXAMPLE_CHECK(rc == 0, "dvz_view_capture_start() failed");

    EXAMPLE_CHECK(
        example_visual_spin(
            scene, visual, (vec3){0.0f, 1.0f, 0.0f}, ROTATION_SPEED_RAD_PER_SEC, NULL,
            &spin),
        "example_visual_spin() failed");
    example_visual_spin_start(&spin, 0.0);

    dvz_app_run(app, frame_count);
    rc = dvz_view_capture_stop(win);
    EXAMPLE_CHECK(rc == 0, "dvz_view_capture_stop() failed");
    ret = 0;

cleanup:
    if (cube != NULL)
        dvz_geometry_destroy(cube);
    if (app != NULL)
        dvz_app_destroy(app);
    example_visual_spin_destroy(&spin);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
