/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* input_events - hosted input event emission and router subscriptions.
 *
 * Scenario: feature.input_events
 * Style: features, native app
 *
 * Build:  just example-c features/input_events
 * Run:    ./build/examples/c/features/input_events
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

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
    dvz_fprintf(
        stdout, "input_events: pointer type=%d x=%.1f y=%.1f\n", (int)event->type, event->pos[0],
        event->pos[1]);
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
    dvz_fprintf(stdout, "input_events: key type=%d key=%d\n", (int)event->type, (int)event->key);
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
    dvz_fprintf(
        stdout, "input_events: resize framebuffer=%ux%u window=%ux%u\n", event->framebuffer_width,
        event->framebuffer_height, event->window_width, event->window_height);
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    DvzPanel* panel = figure != NULL ? dvz_panel_full(figure) : NULL;
    EXAMPLE_CHECK(figure != NULL && panel != NULL, "failed to create scene objects");
    example_graphite_cyan_set_panel_background(panel);

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU?)");

    DvzView* view = dvz_view_offscreen(app, figure, WIDTH, HEIGHT);
    EXAMPLE_CHECK(view != NULL, "dvz_view_offscreen() failed");

    DvzInputRouter* router = dvz_view_input(view);
    EXAMPLE_CHECK(router != NULL, "dvz_view_input() failed");
    InputEventsState state = {0};
    dvz_input_subscribe_pointer(router, _input_events_pointer, &state);
    dvz_input_subscribe_keyboard(router, _input_events_keyboard, &state);
    dvz_input_subscribe_resize(router, _input_events_resize, &state);

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

    dvz_input_unsubscribe_pointer(router, _input_events_pointer, &state);
    dvz_input_unsubscribe_keyboard(router, _input_events_keyboard, &state);
    dvz_input_unsubscribe_resize(router, _input_events_resize, &state);
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
