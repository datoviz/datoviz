/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* replay_dvzr — replay a DVZR recording into a live GLFW window.
 *
 * Build:  just build
 * Run:    ./build/examples/c/tools/replay_dvzr path/to/recording.dvzr
 * Loop:   ./build/examples/c/tools/replay_dvzr --loop path/to/recording.dvzr
 * Fast:   ./build/examples/c/tools/replay_dvzr --fast path/to/recording.dvzr
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  800u
#define HEIGHT 600u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ReplayArgs ReplayArgs;

struct ReplayArgs
{
    const char* path;
    bool loop;
    bool fast;
    double speed;
    uint32_t frame_count;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Print command-line usage.
 *
 * @param argv0 executable path
 */
static void _usage(const char* argv0)
{
    dvz_fprintf(
        stderr,
        "usage: %s [--loop] [--fast] [--speed N] [--frames N] recording.dvzr\n",
        argv0 != NULL ? argv0 : "replay_dvzr");
}



/**
 * Parse an unsigned integer argument.
 *
 * @param text input string
 * @param out parsed value
 * @return whether parsing succeeded
 */
static bool _parse_u32(const char* text, uint32_t* out)
{
    if (text == NULL || out == NULL)
        return false;
    char* end = NULL;
    unsigned long value = strtoul(text, &end, 10);
    if (end == text || (end != NULL && *end != '\0') || value > UINT32_MAX)
        return false;
    *out = (uint32_t)value;
    return true;
}



/**
 * Parse a positive floating-point argument.
 *
 * @param text input string
 * @param out parsed value
 * @return whether parsing succeeded
 */
static bool _parse_speed(const char* text, double* out)
{
    if (text == NULL || out == NULL)
        return false;
    char* end = NULL;
    double value = strtod(text, &end);
    if (end == text || (end != NULL && *end != '\0') || value <= 0)
        return false;
    *out = value;
    return true;
}



/**
 * Parse replay command-line arguments.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @param args output arguments
 * @return whether parsing succeeded
 */
static bool _parse_args(int argc, char** argv, ReplayArgs* args)
{
    if (args == NULL)
        return false;
    *args = (ReplayArgs){.speed = 1.0};

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--loop") == 0)
        {
            args->loop = true;
            continue;
        }
        if (strcmp(argv[i], "--fast") == 0)
        {
            args->fast = true;
            continue;
        }
        if (strcmp(argv[i], "--speed") == 0 && i + 1 < argc)
        {
            if (!_parse_speed(argv[++i], &args->speed))
                return false;
            continue;
        }
        if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc)
        {
            if (!_parse_u32(argv[++i], &args->frame_count))
                return false;
            continue;
        }
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
            return false;
        if (args->path == NULL)
        {
            args->path = argv[i];
            continue;
        }
        return false;
    }

    return args->path != NULL;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    ReplayArgs args = {0};
    if (!_parse_args(argc, argv, &args))
    {
        _usage(argv != NULL ? argv[0] : NULL);
        return 1;
    }

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

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        dvz_fprintf(stderr, "dvz_app() failed (no GPU?)\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzAppWindow* win = dvz_app_window_glfw(app, figure, WIDTH, HEIGHT, "Datoviz DVZR replay");
    if (win == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_glfw() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }

    if (dvz_app_window_replay_start(win, args.path) != 0)
    {
        dvz_fprintf(stderr, "failed to start replay from %s\n", args.path);
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_app_window_replay_set_loop(win, args.loop);
    dvz_app_window_replay_set_paced(win, !args.fast);
    dvz_app_window_replay_set_speed(win, args.speed);

    uint32_t frame_count = args.frame_count;
    if (!args.loop && frame_count == 0)
        frame_count = dvz_app_window_replay_frame_count(win);

    dvz_fprintf(
        stdout, "replay_dvzr: replaying %s (%s%s, speed %.3g)\n", args.path,
        args.loop ? "loop" : "once", args.fast ? ", fast" : "", args.speed);
    dvz_app_run(app, frame_count);

    (void)dvz_app_window_replay_stop(win);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
