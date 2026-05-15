/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* hello_mesh_glfw — live rotating lit cube mesh via dvz_mesh + scene/app.
 *
 * Opens a GLFW window showing one indexed cube mesh with explicit face normals, per-face colours,
 * depth testing, and a perspective camera. A scene timer advances the panel arcball while the
 * user is idle so the retained 3D scene exercises the live scene -> DRP2 -> vklite/canvas path
 * continuously without fighting interactive arcball gestures.
 *
 * Build:  just example-c hello_mesh_glfw
 * Run:    ./build/examples/c/hello_mesh_glfw
 * Smoke:  ./build/examples/c/hello_mesh_glfw 60
 * DVZR:   ./build/examples/c/hello_mesh_glfw record
 * Video:  ./build/examples/c/hello_mesh_glfw video
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "_alloc.h"
#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/canvas.h"
#include "datoviz/scene.h"
#include "datoviz/video.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  800
#define HEIGHT 600

#define ROTATION_SPEED_RAD_PER_SEC 0.9f



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct MeshGlfwState MeshGlfwState;

struct MeshGlfwState
{
    DvzArcball* arcball;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Build an indexed cube with duplicated vertices and per-face normals.
 *
 * @param positions output vertex positions
 * @param colors output vertex colors
 * @param normals output vertex normals
 * @param indices output triangle-list indices
 */
static void _build_cube(
    float positions[24][3], DvzColor colors[24], float normals[24][3], DvzIndex indices[36])
{
    const float s = 0.58f;
    const float face_positions[6][4][3] = {
        {{-s, -s, +s}, {+s, -s, +s}, {+s, +s, +s}, {-s, +s, +s}},
        {{+s, -s, -s}, {-s, -s, -s}, {-s, +s, -s}, {+s, +s, -s}},
        {{-s, -s, -s}, {-s, -s, +s}, {-s, +s, +s}, {-s, +s, -s}},
        {{+s, -s, +s}, {+s, -s, -s}, {+s, +s, -s}, {+s, +s, +s}},
        {{-s, +s, +s}, {+s, +s, +s}, {+s, +s, -s}, {-s, +s, -s}},
        {{-s, -s, -s}, {+s, -s, -s}, {+s, -s, +s}, {-s, -s, +s}},
    };
    const float face_normals[6][3] = {
        {0.0f, 0.0f, +1.0f},
        {0.0f, 0.0f, -1.0f},
        {-1.0f, 0.0f, 0.0f},
        {+1.0f, 0.0f, 0.0f},
        {0.0f, +1.0f, 0.0f},
        {0.0f, -1.0f, 0.0f},
    };
    const DvzColor face_colors[6] = {
        {239, 83, 80, 255},
        {66, 165, 245, 255},
        {102, 187, 106, 255},
        {255, 202, 40, 255},
        {171, 71, 188, 255},
        {255, 112, 67, 255},
    };

    for (uint32_t face = 0; face < 6; face++)
    {
        for (uint32_t corner = 0; corner < 4; corner++)
        {
            const uint32_t vertex = 4 * face + corner;
            positions[vertex][0] = face_positions[face][corner][0];
            positions[vertex][1] = face_positions[face][corner][1];
            positions[vertex][2] = face_positions[face][corner][2];
            dvz_memcpy(colors[vertex], sizeof(DvzColor), face_colors[face], sizeof(DvzColor));
            normals[vertex][0] = face_normals[face][0];
            normals[vertex][1] = face_normals[face][1];
            normals[vertex][2] = face_normals[face][2];
        }

        const uint32_t base = 4 * face;
        indices[6 * face + 0] = base + 0;
        indices[6 * face + 1] = base + 1;
        indices[6 * face + 2] = base + 2;
        indices[6 * face + 3] = base + 0;
        indices[6 * face + 4] = base + 2;
        indices[6 * face + 5] = base + 3;
    }
}



/**
 * Parse an optional bounded frame count from the command line.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return requested frame count, or 0 for the interactive loop
 */
static uint32_t _frame_count(int argc, char** argv)
{
    if (argc < 2 || argv == NULL)
        return 0;

    for (int i = 1; i < argc; i++)
    {
        if (argv[i] == NULL)
            continue;
        char* end = NULL;
        unsigned long value = strtoul(argv[i], &end, 10);
        if (end == argv[i] || (end != NULL && *end != '\0'))
            continue;
        if (value > UINT32_MAX)
            return UINT32_MAX;
        return (uint32_t)value;
    }
    return 0;
}



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
/*  Callbacks                                                                                    */
/*************************************************************************************************/

/**
 * Advance the cube orientation from the scene clock.
 *
 * @param animation timer animation
 * @param t current scene-clock time
 * @param dt elapsed scene-clock time since the previous step
 * @param user_data mesh GLFW example state
 */
static void _mesh_glfw_timer(DvzAnimation* animation, double t, double dt, void* user_data)
{
    (void)animation;
    (void)t;
    MeshGlfwState* state = (MeshGlfwState*)user_data;
    if (state == NULL || state->arcball == NULL)
        return;

    if (!dvz_arcball_is_interacting(state->arcball))
    {
        dvz_arcball_rotate_axis(
            state->arcball, ROTATION_SPEED_RAD_PER_SEC * (float)dt,
            (vec3){0.0f, 1.0f, 0.0f});
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    bool video_enabled = _video_enabled(argc, argv);
    uint32_t frame_count = _frame_count(argc, argv);
    char default_dvzr_path[512] = {0};
    char dvzr_path[512] = {0};
    _outpath(argv[0], "hello_mesh_glfw.dvzr", default_dvzr_path, sizeof(default_dvzr_path));
    bool recording_enabled =
        _recording_path(argc, argv, default_dvzr_path, dvzr_path, sizeof(dvzr_path));

    DvzScene* scene = dvz_scene();
    if (scene == NULL)
    {
        dvz_fprintf(stderr, "dvz_scene() failed\n");
        return 1;
    }

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    if (figure == NULL)
    {
        dvz_fprintf(stderr, "dvz_figure() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzPanel* panel =
        dvz_panel(figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    if (panel == NULL)
    {
        dvz_fprintf(stderr, "dvz_panel() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.eye[2] = 3.0f;
    camera_desc.up[1] = 1.0f;
    camera_desc.fov_y = 0.78539816339f;
    camera_desc.near = 0.1f;
    camera_desc.far = 100.0f;
    if (!dvz_panel_set_camera(panel, &camera_desc))
    {
        dvz_fprintf(stderr, "dvz_panel_set_camera() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzVisual* visual = dvz_mesh(scene, 0);
    if (visual == NULL)
    {
        dvz_fprintf(stderr, "dvz_mesh() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    float positions[24][3] = {0};
    DvzColor colors[24] = {0};
    float normals[24][3] = {0};
    DvzIndex indices[36] = {0};
    _build_cube(positions, colors, normals, indices);

    DvzSceneBuffer* index_buffer = dvz_scene_buffer(
        scene, &(DvzSceneBufferDesc){
                   .usage = DVZ_SCENE_BUFFER_USAGE_INDEX,
                   .stride = sizeof(DvzIndex),
               });
    if (index_buffer == NULL)
    {
        dvz_fprintf(stderr, "dvz_scene_buffer() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    if (!dvz_scene_buffer_set_data(index_buffer, indices, sizeof(indices)))
    {
        dvz_fprintf(stderr, "dvz_scene_buffer_set_data() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    dvz_visual_set_data(visual, "position", positions, 24);
    dvz_visual_set_data(visual, "color", colors, 24);
    dvz_visual_set_data(visual, "normal", normals, 24);
    dvz_visual_set_buffer(visual, "index", index_buffer);
    dvz_visual_set_primitive_shading(
        visual,
        &(DvzPrimitiveShadingDesc){
            .light_direction = {0.35f, 0.55f, 0.75f},
            .ambient = 0.25f,
            .diffuse = 0.85f,
        });
    dvz_panel_add_visual(panel, visual, NULL);
    dvz_panel_set_background_color(panel, 0.05f, 0.05f, 0.08f, 1.0f);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        dvz_fprintf(stderr, "dvz_app() failed (no GPU or display?)\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzAppWindow* win = dvz_app_window_glfw(app, figure, WIDTH, HEIGHT, "hello_mesh_glfw");
    if (win == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_glfw() failed (GLFW unavailable?)\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }

    dvz_panel_set_arcball(panel, dvz_app_window_input(win), 0);
    DvzArcball* arcball = dvz_panel_arcball(panel);
    if (arcball == NULL)
    {
        dvz_fprintf(stderr, "dvz_panel_set_arcball() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
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
        _outpath(argv[0], "hello_mesh_glfw.mp4", mp4_path, sizeof(mp4_path));
        video.encoder.mp4_path = mp4_path;

        if (dvz_canvas_configure_video_sink(dvz_app_window_canvas(win), true, &video) != 0)
        {
            dvz_fprintf(stderr, "dvz_canvas_configure_video_sink() failed\n");
            dvz_app_destroy(app);
            dvz_scene_destroy(scene);
            return 1;
        }
    }
    if (recording_enabled && dvz_app_window_record_start(win, dvzr_path) != 0)
    {
        dvz_fprintf(stderr, "dvz_app_window_record_start() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }

    MeshGlfwState state = {.arcball = arcball};
    DvzAnimation* spin = dvz_anim_timer(scene, 0.0, _mesh_glfw_timer, &state);
    if (spin == NULL)
    {
        dvz_fprintf(stderr, "dvz_anim_timer() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_anim_start(spin, 0.0);

    dvz_app_run(app, frame_count);
    if (recording_enabled)
    {
        if (dvz_app_window_record_stop(win) != 0)
            dvz_fprintf(stderr, "dvz_app_window_record_stop() failed\n");
        else
            dvz_fprintf(stdout, "hello_mesh_glfw: saved %s\n", dvzr_path);
    }
    if (video_enabled)
        dvz_fprintf(stdout, "hello_mesh_glfw: saved %s\n", mp4_path);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
