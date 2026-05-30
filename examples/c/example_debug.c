/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Example debug shortcuts                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "example_debug.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "_assertions.h"
#include "_compat.h"
#include "datoviz/input.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return a non-empty label.
 *
 * @param name optional label
 * @param fallback fallback label
 * @return label
 */
static const char* _debug_name(const char* name, const char* fallback)
{
    return name != NULL && name[0] != '\0' ? name : fallback;
}



/**
 * Dump one arcball as pasteable C snippets.
 *
 * @param name arcball label
 * @param arcball arcball controller
 */
static void _debug_dump_arcball(const char* name, DvzArcball* arcball)
{
    ANN(arcball);

    vec3 angles = {0};
    mat4 model = {0};
    dvz_arcball_angles(arcball, angles);
    dvz_arcball_model(arcball, model);

    dvz_fprintf(stderr, "arcball %s:\n", _debug_name(name, "unnamed"));
    dvz_fprintf(
        stderr, "dvz_arcball_initial(arcball, (vec3){%+.6ff, %+.6ff, %+.6ff});\n",
        angles[0], angles[1], angles[2]);
    dvz_fprintf(stderr, "dvz_arcball_zoom(arcball, %.6ff);\n", arcball->zoom);
    dvz_fprintf(
        stderr, "dvz_arcball_pan(arcball, (vec2){%+.6ff, %+.6ff});\n", arcball->pan[0],
        arcball->pan[1]);

    dvz_fprintf(stderr, "arcball model matrix:\n");
    for (uint32_t row = 0; row < 4; row++)
    {
        dvz_fprintf(
            stderr, "  {%+.6ff, %+.6ff, %+.6ff, %+.6ff}\n", model[0][row], model[1][row],
            model[2][row], model[3][row]);
    }
}



/**
 * Dump one panzoom as pasteable C snippets.
 *
 * @param name panzoom label
 * @param panzoom panzoom controller
 */
static void _debug_dump_panzoom(const char* name, DvzPanzoom* panzoom)
{
    ANN(panzoom);

    dvz_fprintf(stderr, "panzoom %s:\n", _debug_name(name, "unnamed"));
    dvz_fprintf(
        stderr, "dvz_panzoom_pan(panzoom, (vec2){%+.6ff, %+.6ff});\n", panzoom->pan[0],
        panzoom->pan[1]);
    dvz_fprintf(
        stderr, "dvz_panzoom_zoom(panzoom, (vec2){%.6ff, %.6ff});\n", panzoom->zoom[0],
        panzoom->zoom[1]);

    float extent[4] = {0};
    if (dvz_panzoom_extent(panzoom, extent))
    {
        dvz_fprintf(
            stderr, "panzoom extent: xmin=%+.6f xmax=%+.6f ymin=%+.6f ymax=%+.6f\n",
            extent[0], extent[1], extent[2], extent[3]);
    }
}



/**
 * Dump one camera descriptor as pasteable C snippets.
 *
 * @param name camera label
 * @param camera camera descriptor
 */
static void _debug_dump_camera(const char* name, const DvzCameraDesc* camera)
{
    ANN(camera);

    dvz_fprintf(stderr, "camera %s:\n", _debug_name(name, "unnamed"));
    dvz_fprintf(stderr, "DvzCameraDesc camera_desc = dvz_camera_desc();\n");
    dvz_fprintf(stderr, "camera_desc.type = %d;\n", (int)camera->type);
    dvz_fprintf(
        stderr,
        "camera_desc.eye[0] = %+.6ff; camera_desc.eye[1] = %+.6ff; "
        "camera_desc.eye[2] = %+.6ff;\n",
        camera->eye[0], camera->eye[1], camera->eye[2]);
    dvz_fprintf(
        stderr,
        "camera_desc.target[0] = %+.6ff; camera_desc.target[1] = %+.6ff; "
        "camera_desc.target[2] = %+.6ff;\n",
        camera->target[0], camera->target[1], camera->target[2]);
    dvz_fprintf(
        stderr,
        "camera_desc.up[0] = %+.6ff; camera_desc.up[1] = %+.6ff; "
        "camera_desc.up[2] = %+.6ff;\n",
        camera->up[0], camera->up[1], camera->up[2]);
    dvz_fprintf(
        stderr,
        "camera_desc.fov_y = %.6ff; camera_desc.near = %.6ff; "
        "camera_desc.far = %.6ff;\n",
        camera->fov_y, camera->near, camera->far);
    dvz_fprintf(stderr, "camera_desc.ortho_height = %.6ff;\n", camera->ortho_height);
}



/**
 * Dump every registered controller and camera.
 *
 * @param debug debug state
 */
static void _debug_dump(const ExampleDebug* debug)
{
    ANN(debug);

    dvz_fprintf(stderr, "\nexample debug dump:\n");
    for (uint32_t i = 0; i < debug->arcball_count; i++)
    {
        if (debug->arcballs[i].arcball != NULL)
            _debug_dump_arcball(debug->arcballs[i].name, debug->arcballs[i].arcball);
    }
    for (uint32_t i = 0; i < debug->panzoom_count; i++)
    {
        if (debug->panzooms[i].panzoom != NULL)
            _debug_dump_panzoom(debug->panzooms[i].name, debug->panzooms[i].panzoom);
    }
    for (uint32_t i = 0; i < debug->camera_count; i++)
        _debug_dump_camera(debug->cameras[i].name, &debug->cameras[i].camera);
    dvz_fprintf(stderr, "\n");
}



/**
 * Save the current view to a numbered debug PNG beside the example executable.
 *
 * @param debug debug state
 */
static void _debug_screenshot(ExampleDebug* debug)
{
    ANN(debug);
    ANN(debug->view);

    char name[128] = {0};
    char path[1024] = {0};
    dvz_snprintf(
        name, sizeof(name), "%s_debug_%04" PRIu32 ".png", _debug_name(debug->basename, "example"),
        debug->screenshot_index++);
    example_outpath(debug->exe, name, path, sizeof(path));

    if (dvz_view_capture_png(debug->view, path) == 0)
        dvz_fprintf(stderr, "example debug: saved %s\n", path);
    else
        dvz_fprintf(stderr, "example debug: failed to save %s\n", path);
}



/**
 * Reset all registered controllers.
 *
 * @param debug debug state
 */
static void _debug_reset(ExampleDebug* debug)
{
    ANN(debug);
    for (uint32_t i = 0; i < debug->arcball_count; i++)
    {
        if (debug->arcballs[i].arcball != NULL)
            dvz_arcball_reset(debug->arcballs[i].arcball);
    }
    for (uint32_t i = 0; i < debug->panzoom_count; i++)
    {
        if (debug->panzooms[i].panzoom != NULL)
            dvz_panzoom_reset(debug->panzooms[i].panzoom);
    }
    dvz_fprintf(stderr, "example debug: reset registered controllers\n");
}



/**
 * Handle example debug keyboard shortcuts.
 *
 * @param router input router
 * @param event keyboard event
 * @param user_data ExampleDebug pointer
 */
static void
_debug_keyboard(DvzInputRouter* router, const DvzKeyboardEvent* event, void* user_data)
{
    (void)router;
    ExampleDebug* debug = (ExampleDebug*)user_data;
    if (debug == NULL || event == NULL || event->type != DVZ_KEYBOARD_EVENT_PRESS)
        return;

    switch (event->key)
    {
    case DVZ_KEY_D:
        _debug_dump(debug);
        break;
    case DVZ_KEY_S:
        _debug_screenshot(debug);
        break;
    case DVZ_KEY_R:
        _debug_reset(debug);
        break;
    default:
        break;
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return whether a command-line token enables example debug shortcuts.
 *
 * @param arg command-line token
 * @return true when the token enables debug mode
 */
bool example_debug_arg(const char* arg)
{
    return arg != NULL && strcmp(arg, "--debug") == 0;
}



/**
 * Return whether example debug shortcuts were requested.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return whether debug shortcuts should be installed
 */
bool example_debug_requested(int argc, char** argv)
{
    if (argc >= 2 && argv != NULL)
    {
        for (int i = 1; i < argc; i++)
        {
            if (example_debug_arg(argv[i]))
                return true;
        }
    }

    const char* env = getenv("DVZ_EXAMPLE_DEBUG");
    return env != NULL && env[0] != '\0' && strcmp(env, "0") != 0 && strcmp(env, "false") != 0 &&
           strcmp(env, "off") != 0;
}



/**
 * Initialize example debug state.
 *
 * @param view app view
 * @param exe executable path, typically argv[0]
 * @param basename screenshot filename prefix
 * @return initialized debug state
 */
ExampleDebug example_debug(DvzView* view, const char* exe, const char* basename)
{
    return (ExampleDebug){
        .view = view,
        .exe = exe,
        .basename = basename,
    };
}



/**
 * Register an arcball controller for dump and reset shortcuts.
 *
 * @param debug debug state
 * @param name controller label
 * @param arcball arcball controller
 */
void example_debug_arcball(ExampleDebug* debug, const char* name, DvzArcball* arcball)
{
    if (debug == NULL || arcball == NULL || debug->arcball_count >= EXAMPLE_DEBUG_MAX_ARCBALLS)
        return;
    debug->arcballs[debug->arcball_count++] =
        (ExampleDebugArcball){.name = name, .arcball = arcball};
}



/**
 * Register a panzoom controller for dump and reset shortcuts.
 *
 * @param debug debug state
 * @param name controller label
 * @param panzoom panzoom controller
 */
void example_debug_panzoom(ExampleDebug* debug, const char* name, DvzPanzoom* panzoom)
{
    if (debug == NULL || panzoom == NULL || debug->panzoom_count >= EXAMPLE_DEBUG_MAX_PANZOOMS)
        return;
    debug->panzooms[debug->panzoom_count++] =
        (ExampleDebugPanzoom){.name = name, .panzoom = panzoom};
}



/**
 * Register a camera descriptor for dump shortcuts.
 *
 * @param debug debug state
 * @param name camera label
 * @param camera_desc camera descriptor
 */
void example_debug_camera(ExampleDebug* debug, const char* name, const DvzCameraDesc* camera_desc)
{
    if (debug == NULL || camera_desc == NULL || debug->camera_count >= EXAMPLE_DEBUG_MAX_CAMERAS)
        return;
    debug->cameras[debug->camera_count++] =
        (ExampleDebugCamera){.name = name, .camera = *camera_desc};
}



/**
 * Install keyboard shortcuts if requested by argv or DVZ_EXAMPLE_DEBUG.
 *
 * @param debug debug state
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return true when shortcuts were installed
 */
bool example_debug_install(ExampleDebug* debug, int argc, char** argv)
{
    if (debug == NULL || debug->view == NULL || !example_debug_requested(argc, argv))
        return false;

    debug->input = dvz_view_input(debug->view);
    if (debug->input == NULL)
        return false;

    dvz_input_subscribe_keyboard(debug->input, _debug_keyboard, debug);
    debug->installed = true;
    dvz_fprintf(stderr, "example debug: D dump state, S save PNG, R reset controllers\n");
    return true;
}



/**
 * Uninstall keyboard shortcuts.
 *
 * @param debug debug state
 */
void example_debug_uninstall(ExampleDebug* debug)
{
    if (debug == NULL || !debug->installed || debug->input == NULL)
        return;
    dvz_input_unsubscribe_keyboard(debug->input, _debug_keyboard, debug);
    debug->installed = false;
    debug->input = NULL;
}
