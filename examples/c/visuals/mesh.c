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
 * DVZR:   ./build/examples/c/visuals/mesh record
 * Video:  ./build/examples/c/visuals/mesh video
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "_compat.h"
#include "datoviz/geom.h"
#include "datoviz/app.h"
#include "datoviz/canvas.h"
#include "datoviz/scene.h"
#include "datoviz/video.h"
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

/**
 * Return whether the example should enable live MP4 recording.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return whether video recording was requested
 */
static bool _video_enabled(int argc, char** argv)
{
    if (argc < 2 || argv == NULL)
        return false;
    for (int i = 1; i < argc; i++)
    {
        if (argv[i] != NULL && strcmp(argv[i], "video") == 0)
            return true;
    }
    return false;
}



/**
 * Return whether the example should record a DVZR stream.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @param default_path default path next to the executable
 * @param out output recording path
 * @param out_size output buffer size
 * @return whether DVZR recording was requested
 */
static bool _recording_path(
    int argc, char** argv, const char* default_path, char* out, size_t out_size)
{
    if (argc < 2 || argv == NULL || out == NULL || out_size == 0)
        return false;
    for (int i = 1; i < argc; i++)
    {
        if (argv[i] == NULL)
            continue;
        if (strcmp(argv[i], "record") == 0)
        {
            dvz_snprintf(out, out_size, "%s", default_path);
            return true;
        }
        if (strncmp(argv[i], "record=", 7) == 0)
        {
            dvz_snprintf(out, out_size, "%s", argv[i] + 7);
            return true;
        }
        if (strcmp(argv[i], "--record") == 0 && i + 1 < argc && argv[i + 1] != NULL)
        {
            dvz_snprintf(out, out_size, "%s", argv[i + 1]);
            return true;
        }
    }
    return false;
}



/**
 * Build an output path next to the example executable.
 *
 * @param exe executable path from argv[0]
 * @param name output file name
 * @param out destination path buffer
 * @param size destination path buffer size
 */
static void _outpath(const char* exe, const char* name, char* out, size_t size)
{
    const char* slash = exe != NULL ? strrchr(exe, '/') : NULL;
    if (slash != NULL)
        dvz_snprintf(out, size, "%.*s/%s", (int)(slash - exe), exe, name);
    else
        dvz_snprintf(out, size, "%s", name);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    bool video_enabled = _video_enabled(argc, argv);
    uint32_t frame_count = example_frame_count_any(argc, argv);
    char default_dvzr_path[512] = {0};
    char dvzr_path[512] = {0};
    _outpath(argv[0], "mesh.dvzr", default_dvzr_path, sizeof(default_dvzr_path));
    bool recording_enabled =
        _recording_path(argc, argv, default_dvzr_path, dvzr_path, sizeof(dvzr_path));

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    DvzGeometry* cube = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel() failed");

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[2] = 3.0f;
    camera_desc.up[1] = 1.0f;
    camera_desc.fov_y = 0.78539816339f;
    camera_desc.near = 0.1f;
    camera_desc.far = 100.0f;
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
        .size = MESH_CUBE_SIZE,
        .face_colors = face_colors,
        .face_color_count = DVZ_GEOM_CUBE_FACE_COUNT,
    });
    EXAMPLE_CHECK(cube != NULL, "dvz_geom_cube() failed");

    int rc = dvz_mesh_geometry(visual, cube);
    EXAMPLE_CHECK(rc == 0, "dvz_mesh_geometry() failed");
    dvz_geometry_destroy(cube);
    cube = NULL;

    DvzMaterialDesc material = dvz_phong_material_desc();
    material.light_direction[0] = 0.35f;
    material.light_direction[1] = 0.55f;
    material.light_direction[2] = 0.75f;
    material.phong.ambient = 0.25f;
    material.phong.diffuse = 0.85f;
    dvz_visual_set_material(visual, &material);
    dvz_panel_add_visual(panel, visual, NULL);
    dvz_panel_set_background_color(panel, 0.05f, 0.05f, 0.08f, 1.0f);

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzAppWindow* win = dvz_app_window_glfw(app, figure, WIDTH, HEIGHT, "mesh");
    EXAMPLE_CHECK(win != NULL, "dvz_app_window_glfw() failed (GLFW unavailable?)");

    DvzArcball* arcball = dvz_app_window_panel_arcball(win, panel, NULL);
    EXAMPLE_CHECK(arcball != NULL, "failed to create or bind arcball controller");
    dvz_arcball_set(arcball, (vec3){+0.65f, 0.0f, +0.35f});

    dvz_scene_set_clock_mode(scene, video_enabled ? DVZ_CLOCK_OFFLINE : DVZ_CLOCK_REALTIME);
    dvz_scene_set_fps(scene, 60.0);

    char mp4_path[512] = {0};
    if (video_enabled)
    {
        DvzVideoSinkConfig video = dvz_video_sink_default_config();
        video.encoder.backend = "nvenc";
        video.encoder.width = WIDTH;
        video.encoder.height = HEIGHT;
        video.encoder.fps = 60;
        _outpath(argv[0], "mesh.mp4", mp4_path, sizeof(mp4_path));
        video.encoder.mp4_path = mp4_path;

        rc = dvz_canvas_configure_video_sink(dvz_app_window_canvas(win), true, &video);
        EXAMPLE_CHECK(rc == 0, "dvz_canvas_configure_video_sink() failed");
    }
    if (recording_enabled && dvz_app_window_record_start(win, dvzr_path) != 0)
        EXAMPLE_CHECK(false, "dvz_app_window_record_start() failed");

    DvzAnimation* spin = dvz_anim_arcball_spin(
        scene, arcball, (vec3){0.0f, 1.0f, 0.0f}, ROTATION_SPEED_RAD_PER_SEC,
        DVZ_ARCBALL_SPIN_FLAGS_PAUSE_ON_INTERACTION);
    EXAMPLE_CHECK(spin != NULL, "dvz_anim_arcball_spin() failed");
    dvz_anim_start(spin, 0.0);

    dvz_app_run(app, frame_count);
    if (recording_enabled)
    {
        if (dvz_app_window_record_stop(win) != 0)
            dvz_fprintf(stderr, "dvz_app_window_record_stop() failed\n");
        else
            dvz_fprintf(stdout, "mesh: saved %s\n", dvzr_path);
    }
    if (video_enabled)
        dvz_fprintf(stdout, "mesh: saved %s\n", mp4_path);
    ret = 0;

cleanup:
    if (cube != NULL)
        dvz_geometry_destroy(cube);
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
