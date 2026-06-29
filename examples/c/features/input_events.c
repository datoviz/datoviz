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
#include <stdio.h>
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

#define WIDTH  EXAMPLE_WINDOW_WIDTH
#define HEIGHT EXAMPLE_WINDOW_HEIGHT



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
 * Return a human-readable pointer event type label.
 *
 * @param type pointer event type
 * @return stable label
 */
static const char* _pointer_event_label(DvzPointerEventType type)
{
    switch (type)
    {
    case DVZ_POINTER_EVENT_RELEASE:
        return "release";
    case DVZ_POINTER_EVENT_PRESS:
        return "press";
    case DVZ_POINTER_EVENT_MOVE:
        return "move";
    case DVZ_POINTER_EVENT_CLICK:
        return "click";
    case DVZ_POINTER_EVENT_DOUBLE_CLICK:
        return "double-click";
    case DVZ_POINTER_EVENT_DRAG_START:
        return "drag-start";
    case DVZ_POINTER_EVENT_DRAG:
        return "drag";
    case DVZ_POINTER_EVENT_DRAG_STOP:
        return "drag-stop";
    case DVZ_POINTER_EVENT_WHEEL:
        return "wheel";
    case DVZ_POINTER_EVENT_NONE:
        return "none";
    case DVZ_POINTER_EVENT_ALL:
        return "all";
    default:
        return "unknown";
    }
}



/**
 * Return a human-readable pointer button label.
 *
 * @param button pointer button
 * @return stable label
 */
static const char* _pointer_button_label(DvzPointerButton button)
{
    switch (button)
    {
    case DVZ_POINTER_BUTTON_NONE:
        return "none";
    case DVZ_POINTER_BUTTON_LEFT:
        return "left";
    case DVZ_POINTER_BUTTON_MIDDLE:
        return "middle";
    case DVZ_POINTER_BUTTON_RIGHT:
        return "right";
    default:
        return "unknown";
    }
}



/**
 * Return a human-readable keyboard event type label.
 *
 * @param type keyboard event type
 * @return stable label
 */
static const char* _keyboard_event_label(DvzKeyboardEventType type)
{
    switch (type)
    {
    case DVZ_KEYBOARD_EVENT_NONE:
        return "none";
    case DVZ_KEYBOARD_EVENT_PRESS:
        return "press";
    case DVZ_KEYBOARD_EVENT_REPEAT:
        return "repeat";
    case DVZ_KEYBOARD_EVENT_RELEASE:
        return "release";
    default:
        return "unknown";
    }
}



/**
 * Return a human-readable key label for common Datoviz key codes.
 *
 * @param key key code
 * @return stable label
 */
static const char* _key_label(DvzKeyCode key)
{
    if (key >= DVZ_KEY_A && key <= DVZ_KEY_Z)
    {
        static const char* const letters[] = {
            "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
            "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"};
        return letters[key - DVZ_KEY_A];
    }
    if (key >= DVZ_KEY_0 && key <= DVZ_KEY_9)
    {
        static const char* const digits[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
        return digits[key - DVZ_KEY_0];
    }
    if (key >= DVZ_KEY_F1 && key <= DVZ_KEY_F25)
    {
        static const char* const function_keys[] = {
            "F1",  "F2",  "F3",  "F4",  "F5",  "F6",  "F7",  "F8",  "F9",
            "F10", "F11", "F12", "F13", "F14", "F15", "F16", "F17", "F18",
            "F19", "F20", "F21", "F22", "F23", "F24", "F25"};
        return function_keys[key - DVZ_KEY_F1];
    }
    if (key >= DVZ_KEY_KP_0 && key <= DVZ_KEY_KP_9)
    {
        static const char* const keypad_digits[] = {
            "keypad-0", "keypad-1", "keypad-2", "keypad-3", "keypad-4",
            "keypad-5", "keypad-6", "keypad-7", "keypad-8", "keypad-9"};
        return keypad_digits[key - DVZ_KEY_KP_0];
    }

    switch (key)
    {
    case DVZ_KEY_UNKNOWN:
        return "unknown";
    case DVZ_KEY_NONE:
        return "none";
    case DVZ_KEY_SPACE:
        return "space";
    case DVZ_KEY_APOSTROPHE:
        return "'";
    case DVZ_KEY_COMMA:
        return ",";
    case DVZ_KEY_MINUS:
        return "-";
    case DVZ_KEY_PERIOD:
        return ".";
    case DVZ_KEY_SLASH:
        return "/";
    case DVZ_KEY_SEMICOLON:
        return ";";
    case DVZ_KEY_EQUAL:
        return "=";
    case DVZ_KEY_LEFT_BRACKET:
        return "[";
    case DVZ_KEY_BACKSLASH:
        return "\\";
    case DVZ_KEY_RIGHT_BRACKET:
        return "]";
    case DVZ_KEY_GRAVE_ACCENT:
        return "`";
    case DVZ_KEY_WORLD_1:
        return "world-1";
    case DVZ_KEY_WORLD_2:
        return "world-2";
    case DVZ_KEY_ESCAPE:
        return "escape";
    case DVZ_KEY_ENTER:
        return "enter";
    case DVZ_KEY_TAB:
        return "tab";
    case DVZ_KEY_BACKSPACE:
        return "backspace";
    case DVZ_KEY_INSERT:
        return "insert";
    case DVZ_KEY_DELETE:
        return "delete";
    case DVZ_KEY_RIGHT:
        return "right";
    case DVZ_KEY_LEFT:
        return "left";
    case DVZ_KEY_DOWN:
        return "down";
    case DVZ_KEY_UP:
        return "up";
    case DVZ_KEY_PAGE_UP:
        return "page-up";
    case DVZ_KEY_PAGE_DOWN:
        return "page-down";
    case DVZ_KEY_HOME:
        return "home";
    case DVZ_KEY_END:
        return "end";
    case DVZ_KEY_CAPS_LOCK:
        return "caps-lock";
    case DVZ_KEY_SCROLL_LOCK:
        return "scroll-lock";
    case DVZ_KEY_NUM_LOCK:
        return "num-lock";
    case DVZ_KEY_PRINT_SCREEN:
        return "print-screen";
    case DVZ_KEY_PAUSE:
        return "pause";
    case DVZ_KEY_KP_DECIMAL:
        return "keypad-decimal";
    case DVZ_KEY_KP_DIVIDE:
        return "keypad-divide";
    case DVZ_KEY_KP_MULTIPLY:
        return "keypad-multiply";
    case DVZ_KEY_KP_SUBTRACT:
        return "keypad-subtract";
    case DVZ_KEY_KP_ADD:
        return "keypad-add";
    case DVZ_KEY_KP_ENTER:
        return "keypad-enter";
    case DVZ_KEY_KP_EQUAL:
        return "keypad-equal";
    case DVZ_KEY_LEFT_SHIFT:
        return "left-shift";
    case DVZ_KEY_LEFT_CONTROL:
        return "left-control";
    case DVZ_KEY_LEFT_ALT:
        return "left-alt";
    case DVZ_KEY_LEFT_SUPER:
        return "left-super";
    case DVZ_KEY_RIGHT_SHIFT:
        return "right-shift";
    case DVZ_KEY_RIGHT_CONTROL:
        return "right-control";
    case DVZ_KEY_RIGHT_ALT:
        return "right-alt";
    case DVZ_KEY_RIGHT_SUPER:
        return "right-super";
    case DVZ_KEY_MENU:
        return "menu";
    default:
        return "unknown";
    }
}



/**
 * Format a modifier mask as a readable joined label.
 *
 * @param mods modifier mask
 * @param out output string
 * @param out_size output string size
 */
static void _format_mods(int mods, char* out, size_t out_size)
{
    ANN(out);
    ASSERT(out_size > 0);
    out[0] = '\0';

    if (mods == DVZ_KEY_MODIFIER_NONE)
    {
        snprintf(out, out_size, "none");
        return;
    }

    bool first = true;
#define ADD_MOD(bit, label)                                                                        \
    do                                                                                             \
    {                                                                                              \
        if ((mods & (bit)) != 0)                                                                   \
        {                                                                                          \
            snprintf(out + strlen(out), out_size - strlen(out), "%s%s", first ? "" : "+", label); \
            first = false;                                                                         \
        }                                                                                          \
    } while (0)

    ADD_MOD(DVZ_KEY_MODIFIER_SHIFT, "shift");
    ADD_MOD(DVZ_KEY_MODIFIER_CONTROL, "control");
    ADD_MOD(DVZ_KEY_MODIFIER_ALT, "alt");
    ADD_MOD(DVZ_KEY_MODIFIER_SUPER, "super");

#undef ADD_MOD

    int known = DVZ_KEY_MODIFIER_SHIFT | DVZ_KEY_MODIFIER_CONTROL | DVZ_KEY_MODIFIER_ALT |
                DVZ_KEY_MODIFIER_SUPER;
    if ((mods & ~known) != 0)
        snprintf(
            out + strlen(out), out_size - strlen(out), "%s0x%x", first ? "" : "+", mods & ~known);
}

/**
 * Count and log a routed pointer event received by the view input router.
 *
 * @param event pointer event
 * @param state input example state
 */
static void _input_events_pointer(const DvzPointerEvent* event, InputEventsState* state)
{
    if (state == NULL || event == NULL)
        return;
    state->pointer_count++;
    if (state->verbose)
    {
        char mods[64] = {0};
        _format_mods(event->mods, mods, sizeof(mods));
        if (event->type == DVZ_POINTER_EVENT_WHEEL)
            dvz_fprintf(
                stdout, "input_events: pointer wheel dx=%.2f dy=%.2f x=%.1f y=%.1f mods=%s\n",
                event->content.w.dir[0], event->content.w.dir[1], event->pos[0], event->pos[1],
                mods);
        else
            dvz_fprintf(
                stdout, "input_events: pointer %s button=%s x=%.1f y=%.1f mods=%s\n",
                _pointer_event_label(event->type), _pointer_button_label(event->button),
                event->pos[0], event->pos[1], mods);
    }
}



/**
 * Count and log a routed keyboard event received by the view input router.
 *
 * @param event keyboard event
 * @param state input example state
 */
static void _input_events_keyboard(const DvzKeyboardEvent* event, InputEventsState* state)
{
    if (state == NULL || event == NULL)
        return;
    state->keyboard_count++;
    if (state->verbose)
    {
        char mods[64] = {0};
        _format_mods(event->mods, mods, sizeof(mods));
        dvz_fprintf(
            stdout, "input_events: key %s key=%s mods=%s\n", _keyboard_event_label(event->type),
            _key_label(event->key), mods);
    }
}



/**
 * Count and log a routed resize event received by the view input router.
 *
 * @param event resize event
 * @param state input example state
 */
static void _input_events_resize(const DvzInputResizeEvent* event, InputEventsState* state)
{
    if (state == NULL || event == NULL)
        return;
    state->resize_count++;
    if (state->verbose)
        dvz_fprintf(
            stdout, "input_events: resize framebuffer=%ux%u window=%ux%u scale=%.2fx%.2f\n",
            event->framebuffer_width, event->framebuffer_height, event->window_width,
            event->window_height, event->content_scale_x, event->content_scale_y);
}



/**
 * Count routed input events received by the view input router.
 *
 * The union event stream is the canonical stream for consumers that need high-level pointer
 * gestures: the raw pointer callbacks receive backend-normalized pointer events, while the gesture
 * handler emits click, double-click, drag-start, drag, and drag-stop events here.
 *
 * @param router input router
 * @param event routed input event
 * @param user_data input example state
 */
static void _input_events_event(DvzInputRouter* router, const DvzInputEvent* event, void* user_data)
{
    (void)router;
    InputEventsState* state = (InputEventsState*)user_data;
    if (state == NULL || event == NULL)
        return;

    switch (event->type)
    {
    case DVZ_INPUT_EVENT_POINTER:
        _input_events_pointer(&event->content.pointer, state);
        break;
    case DVZ_INPUT_EVENT_KEYBOARD:
        _input_events_keyboard(&event->content.keyboard, state);
        break;
    case DVZ_INPUT_EVENT_RESIZE:
        _input_events_resize(&event->content.resize, state);
        break;
    default:
        break;
    }
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
    dvz_input_subscribe_event(router, _input_events_event, state);
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
    dvz_input_unsubscribe_event(router, _input_events_event, state);
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
        {.attr_name = "diameter_px", .data = sizes, .item_count = 5},
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
