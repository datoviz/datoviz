/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* input_events - live native keyboard, pointer, wheel, and resize event logging.
 *
 * Scenario: feature.input_events
 * Style: features, native app
 *
 * Build:  just example-c features/input_events
 * Run:    ./build/examples/c/features/input_events
 * Smoke:  ./build/examples/c/features/input_events --synthetic
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "_assertions.h"
#include "datoviz/app.h"
#include "datoviz/canvas/enums.h"
#include "datoviz/input.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  640u
#define HEIGHT 480u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct InputEventsState
{
    uint32_t pointer_count;
    uint32_t keyboard_count;
    uint32_t resize_count;
    bool verbose;
} InputEventsState;



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

/**
 * Count pointer events received by the view input router.
 *
 * @param router input router
 * @param event pointer event
 * @param user_data input example state
 */
static void
_input_events_pointer(DvzInputRouter* router, const DvzPointerEvent* event, void* user_data)
{
    (void)router;
    InputEventsState* state = (InputEventsState*)user_data;
    if (state == NULL || event == NULL)
        return;
    state->pointer_count++;
    if (state->verbose)
    {
        if (event->type == DVZ_POINTER_EVENT_WHEEL)
            dvz_fprintf(
                stdout, "input_events: wheel dx=%.2f dy=%.2f at x=%.1f y=%.1f\n",
                event->content.w.dir[0], event->content.w.dir[1], event->pos[0], event->pos[1]);
        else
            dvz_fprintf(
                stdout, "input_events: pointer type=%d button=%d x=%.1f y=%.1f\n",
                (int)event->type, (int)event->button, event->pos[0], event->pos[1]);
    }
}



/**
 * Count keyboard events received by the view input router.
 *
 * @param router input router
 * @param event keyboard event
 * @param user_data input example state
 */
static void
_input_events_keyboard(DvzInputRouter* router, const DvzKeyboardEvent* event, void* user_data)
{
    (void)router;
    InputEventsState* state = (InputEventsState*)user_data;
    if (state == NULL || event == NULL)
        return;
    state->keyboard_count++;
    if (state->verbose)
        dvz_fprintf(
            stdout, "input_events: key type=%d key=%d mods=%d\n", (int)event->type,
            (int)event->key, (int)event->mods);
}



/**
 * Count resize events received by the view input router.
 *
 * @param router input router
 * @param event resize event
 * @param user_data input example state
 */
static void
_input_events_resize(DvzInputRouter* router, const DvzInputResizeEvent* event, void* user_data)
{
    (void)router;
    InputEventsState* state = (InputEventsState*)user_data;
    if (state == NULL || event == NULL)
        return;
    state->resize_count++;
    if (state->verbose)
        dvz_fprintf(
            stdout, "input_events: resize framebuffer=%ux%u window=%ux%u scale=%.2fx%.2f\n",
            event->framebuffer_width, event->framebuffer_height, event->window_width,
            event->window_height, event->content_scale_x, event->content_scale_y);
}



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return whether a command-line flag is present.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @param flag flag to find
 * @return whether the flag is present
 */
static bool _has_arg(int argc, char** argv, const char* flag)
{
    for (int i = 1; i < argc; i++)
        if (strcmp(argv[i], flag) == 0)
            return true;
    return false;
}



/**
 * Subscribe all event callbacks used by the example.
 *
 * @param router input router
 * @param state event counter state
 */
static void _subscribe_events(DvzInputRouter* router, InputEventsState* state)
{
    ANN(router);
    ANN(state);
    dvz_input_subscribe_pointer(router, _input_events_pointer, state);
    dvz_input_subscribe_keyboard(router, _input_events_keyboard, state);
    dvz_input_subscribe_resize(router, _input_events_resize, state);
}



/**
 * Unsubscribe all event callbacks used by the example.
 *
 * @param router input router
 * @param state event counter state
 */
static void _unsubscribe_events(DvzInputRouter* router, InputEventsState* state)
{
    ANN(router);
    ANN(state);
    dvz_input_unsubscribe_pointer(router, _input_events_pointer, state);
    dvz_input_unsubscribe_keyboard(router, _input_events_keyboard, state);
    dvz_input_unsubscribe_resize(router, _input_events_resize, state);
}


/**
 * Add a small scene so the live input window has an obvious event target.
 *
 * @param scene scene owning the visual
 * @param panel target panel
 * @return whether setup succeeded
 */
static bool _add_points(DvzScene* scene, DvzPanel* panel)
{
    ANN(scene);
    ANN(panel);

    DvzVisual* points = dvz_point(scene, 0);
    if (points == NULL)
        return false;
    vec3 positions[5] = {
        {-0.60f, -0.30f, 0.0f},
        {-0.30f, +0.25f, 0.0f},
        {+0.00f, -0.05f, 0.0f},
        {+0.35f, +0.32f, 0.0f},
        {+0.62f, -0.24f, 0.0f},
    };
    DvzColor colors[5] = {
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ERROR),
    };
    float sizes[5] = {22.0f, 34.0f, 42.0f, 30.0f, 24.0f};
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = 5},
        {.attr_name = "color", .data = colors, .item_count = 5},
        {.attr_name = "diameter", .data = sizes, .item_count = 5},
    };
    if (dvz_visual_set_data_many(points, updates, 3) != 0)
        return false;
    return dvz_panel_add_visual(panel, points, NULL) == 0;
}



/**
 * Run the synthetic emitted-event smoke path.
 *
 * @return process exit code
 */
static int _run_synthetic(void)
{
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    DvzPanel* panel = figure != NULL ? dvz_panel_full(figure) : NULL;
    EXAMPLE_CHECK(figure != NULL && panel != NULL, "failed to create scene objects");
    example_graphite_cyan_set_panel_background(panel);
    EXAMPLE_CHECK(_add_points(scene, panel), "failed to add point visual");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU?)");

    DvzView* view = dvz_view_offscreen(app, figure, WIDTH, HEIGHT);
    EXAMPLE_CHECK(view != NULL, "dvz_view_offscreen() failed");

    DvzInputRouter* router = dvz_view_input(view);
    EXAMPLE_CHECK(router != NULL, "dvz_view_input() failed");
    InputEventsState state = {.verbose = true};
    _subscribe_events(router, &state);

    EXAMPLE_CHECK(
        dvz_view_emit_resize(view, WIDTH, HEIGHT, WIDTH, HEIGHT, 1.0f, 1.0f) == 0,
        "dvz_view_emit_resize() failed");
    EXAMPLE_CHECK(
        dvz_view_emit_pointer(
            view, DVZ_POINTER_EVENT_MOVE, 140.0f, 160.0f, WIDTH, HEIGHT, DVZ_POINTER_BUTTON_NONE,
            DVZ_KEY_MODIFIER_NONE) == 0,
        "dvz_view_emit_pointer(move) failed");
    EXAMPLE_CHECK(
        dvz_view_emit_pointer(
            view, DVZ_POINTER_EVENT_PRESS, 140.0f, 160.0f, WIDTH, HEIGHT, DVZ_POINTER_BUTTON_LEFT,
            DVZ_KEY_MODIFIER_NONE) == 0,
        "dvz_view_emit_pointer(press) failed");
    EXAMPLE_CHECK(
        dvz_view_emit_wheel(
            view, 140.0f, 160.0f, WIDTH, HEIGHT, 0.0f, +1.0f, DVZ_KEY_MODIFIER_NONE) == 0,
        "dvz_view_emit_wheel() failed");
    EXAMPLE_CHECK(
        dvz_view_emit_key(view, DVZ_KEYBOARD_EVENT_PRESS, DVZ_KEY_A, DVZ_KEY_MODIFIER_NONE) == 0,
        "dvz_view_emit_key(press) failed");
    EXAMPLE_CHECK(
        dvz_view_emit_key(view, DVZ_KEYBOARD_EVENT_RELEASE, DVZ_KEY_A, DVZ_KEY_MODIFIER_NONE) == 0,
        "dvz_view_emit_key(release) failed");

    EXAMPLE_CHECK(
        dvz_view_render_once(view) == DVZ_CANVAS_FRAME_READY, "dvz_view_render_once() failed");
    EXAMPLE_CHECK(
        state.pointer_count >= 3 && state.keyboard_count == 2 && state.resize_count >= 1,
        "input event callbacks did not receive the emitted events");

    _unsubscribe_events(router, &state);
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}



/**
 * Run the live native event logging path.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
static int _run_live(int argc, char** argv)
{
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    DvzPanel* panel = figure != NULL ? dvz_panel_full(figure) : NULL;
    EXAMPLE_CHECK(figure != NULL && panel != NULL, "failed to create scene objects");
    example_graphite_cyan_set_panel_background(panel);
    EXAMPLE_CHECK(_add_points(scene, panel), "failed to add point visual");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU?)");

    DvzView* view = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "input_events");
    EXAMPLE_CHECK(view != NULL, "dvz_view_glfw() failed");

    DvzInputRouter* router = dvz_view_input(view);
    EXAMPLE_CHECK(router != NULL, "dvz_view_input() failed");
    InputEventsState state = {.verbose = true};
    _subscribe_events(router, &state);

    dvz_fprintf(
        stdout,
        "input_events: move/click/scroll/resize/type in the window; close it to exit\n");
    dvz_app_run(app, example_frame_count_any(argc, argv));
    _unsubscribe_events(router, &state);
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    if (_has_arg(argc, argv, "--synthetic") || _has_arg(argc, argv, "--png"))
        return _run_synthetic();
    return _run_live(argc, argv);
}
