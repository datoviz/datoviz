/*************************************************************************************************/
/*  Testing input                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_input.h"
#include "input.h"
#include "test.h"
#include "testing.h"



/*************************************************************************************************/
/*  Mouse tests                                                                                  */
/*************************************************************************************************/

static void _on_mouse_move(DvzInput* input, DvzEvent ev, void* user_data)
{
    ASSERT(input != NULL);
    log_debug("mouse position: %.0fx%.0f", ev.content.m.pos[0], ev.content.m.pos[1]);

    ASSERT(user_data != NULL);
    vec2* pos = (vec2*)user_data;
    pos[0][0] = ev.content.m.pos[0];
    pos[0][1] = ev.content.m.pos[1];
}

static void _on_mouse_button(DvzInput* input, DvzEvent ev, void* user_data)
{
    ASSERT(input != NULL);
    log_debug("mouse button: %d", ev.content.b.button);

    ASSERT(user_data != NULL);
    int* button = (int*)user_data;
    *button = (int)ev.content.b.button;
}

static void _on_mouse_wheel(DvzInput* input, DvzEvent ev, void* user_data)
{
    ASSERT(input != NULL);
    log_debug("mouse wheel: %.1fx%.1f", ev.content.w.dir[0], ev.content.w.dir[1]);

    ASSERT(user_data != NULL);
    vec2* dir = (vec2*)user_data;
    dir[0][0] = ev.content.w.dir[0];
    dir[0][1] = ev.content.w.dir[1];
}

int test_input_mouse_1(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzInput input = dvz_input();
    DvzEvent ev = {0};


    // Mouse move.
    vec2 pos_exp = {0};
    dvz_input_callback(&input, DVZ_EVENT_MOUSE_MOVE, _on_mouse_move, &pos_exp);

    ev.content.m.pos[0] = .5;
    ev.content.m.pos[1] = -.5;
    dvz_input_event(&input, DVZ_EVENT_MOUSE_MOVE, ev, false);
    dvz_sleep(10);
    AT(pos_exp[0] == .5);
    AT(pos_exp[1] == -.5);


    // Mouse button press.
    int button = 0;
    dvz_input_callback(&input, DVZ_EVENT_MOUSE_PRESS, _on_mouse_button, &button);

    ev.content.b.button = DVZ_MOUSE_BUTTON_LEFT;
    dvz_input_event(&input, DVZ_EVENT_MOUSE_PRESS, ev, false);
    // HACK: wait for the background thread to process the mouse press callback and modify the
    // button variable.
    dvz_sleep(10);
    AT(button == (int)DVZ_MOUSE_BUTTON_LEFT);


    // Mouse button release.
    ev.content.b.button = DVZ_MOUSE_BUTTON_RIGHT;
    dvz_input_callback(&input, DVZ_EVENT_MOUSE_RELEASE, _on_mouse_button, &button);
    dvz_input_event(&input, DVZ_EVENT_MOUSE_RELEASE, ev, false);
    // HACK: wait for the background thread to process the mouse press callback and modify the
    // button variable.
    dvz_sleep(10);
    AT(button == (int)DVZ_MOUSE_BUTTON_RIGHT);


    // Mouse wheel.
    vec2 dir = {0};
    dvz_input_callback(&input, DVZ_EVENT_MOUSE_WHEEL, _on_mouse_wheel, dir);
    ev.content.w.dir[0] = 0;
    ev.content.w.dir[1] = 1;
    dvz_input_event(&input, DVZ_EVENT_MOUSE_WHEEL, ev, false);
    // HACK: wait for the background thread to process the mouse press callback and modify the
    // button variable.
    dvz_sleep(10);
    AT(dir[1] == 1);


    dvz_input_destroy(&input);
    return 0;
}



static void _on_mouse_drag_begin(DvzInput* input, DvzEvent ev, void* user_data)
{
    ASSERT(input != NULL);
    log_debug("BEGIN mouse drag button %d, mods %d", ev.content.d.button, ev.mods);
}

static void _on_mouse_drag(DvzInput* input, DvzEvent ev, void* user_data)
{
    ASSERT(input != NULL);
    log_debug(
        "mouse drag button %d %.0fx%.0f, mods %d", //
        ev.content.d.button, ev.content.d.pos[0], ev.content.d.pos[1], ev.mods);
}

static void _on_mouse_drag_end(DvzInput* input, DvzEvent ev, void* user_data)
{
    ASSERT(input != NULL);
    log_debug("END mouse drag");
    ASSERT(user_data != NULL);
    *((bool*)user_data) = true;
}

int test_input_drag_1(TstSuite* suite)
{
    // Create an input and window.
    DvzInput input = dvz_input();

    bool dragged = false;
    dvz_input_callback(&input, DVZ_EVENT_MOUSE_DRAG_BEGIN, _on_mouse_drag_begin, NULL);
    dvz_input_callback(&input, DVZ_EVENT_MOUSE_DRAG, _on_mouse_drag, NULL);
    dvz_input_callback(&input, DVZ_EVENT_MOUSE_DRAG_END, _on_mouse_drag_end, &dragged);

    DvzEvent ev = {0};
    ev.content.k.key_code = DVZ_KEY_LEFT_CONTROL;
    dvz_input_event(&input, DVZ_EVENT_KEYBOARD_PRESS, ev, false);
    ev.content.m.pos[0] = 10;
    ev.content.m.pos[1] = 10;
    dvz_input_event(&input, DVZ_EVENT_MOUSE_MOVE, ev, false);
    ev.content.b.button = DVZ_MOUSE_BUTTON_LEFT;
    dvz_input_event(&input, DVZ_EVENT_MOUSE_PRESS, ev, false);
    ev.content.m.pos[0] = 50;
    ev.content.m.pos[1] = 50;
    dvz_input_event(&input, DVZ_EVENT_MOUSE_MOVE, ev, false);
    ev.content.m.pos[0] = 100;
    ev.content.m.pos[1] = 100;
    dvz_input_event(&input, DVZ_EVENT_MOUSE_MOVE, ev, false);
    ev.content.b.button = DVZ_MOUSE_BUTTON_LEFT;
    dvz_input_event(&input, DVZ_EVENT_MOUSE_RELEASE, ev, false);
    // NOTE: wait for the background thread to process
    dvz_deq_wait(&input.deq, 0);
    AT(dragged);

    // Destroy the resources.
    dvz_input_destroy(&input);
    return 0;
}



static void _on_mouse_click(DvzInput* input, DvzEvent ev, void* user_data)
{
    ASSERT(input != NULL);
    log_debug("click");
    ASSERT(user_data != NULL);
    *((bool*)user_data) = true;
}

static void _on_mouse_double_click(DvzInput* input, DvzEvent ev, void* user_data)
{
    ASSERT(input != NULL);
    log_debug("double click");
    ASSERT(user_data != NULL);
    *((bool*)user_data) = true;
}

int test_input_click_1(TstSuite* suite)
{
    // Create an input and window.
    DvzInput input = dvz_input();
    DvzEvent ev = {0};
    bool click = false, dbl_click = false;

    dvz_input_callback(&input, DVZ_EVENT_MOUSE_CLICK, _on_mouse_click, &click);
    dvz_input_callback(&input, DVZ_EVENT_MOUSE_DOUBLE_CLICK, _on_mouse_double_click, &dbl_click);

    ev.content.m.pos[0] = 10;
    ev.content.m.pos[1] = 10;
    dvz_input_event(&input, DVZ_EVENT_MOUSE_MOVE, ev, false);

    // Simulate a click.
    ev.content.b.button = DVZ_MOUSE_BUTTON_LEFT;
    dvz_input_event(&input, DVZ_EVENT_MOUSE_PRESS, ev, false);
    dvz_sleep(5);
    dvz_input_event(&input, DVZ_EVENT_MOUSE_RELEASE, ev, false);

    dvz_deq_wait(&input.deq, 0);
    AT(click);
    AT(!dbl_click);

    // Simulate a double click.
    for (uint32_t i = 0; i < 2; i++)
    {
        dvz_input_event(&input, DVZ_EVENT_MOUSE_PRESS, ev, false);
        dvz_sleep(5);
        dvz_input_event(&input, DVZ_EVENT_MOUSE_RELEASE, ev, false);
        dvz_sleep(5);
    }

    dvz_deq_wait(&input.deq, 0);
    AT(dbl_click);

    // Destroy the resources.
    dvz_input_destroy(&input);
    return 0;
}



/*************************************************************************************************/
/*  Keyboard tests                                                                               */
/*************************************************************************************************/

static void _on_key_press(DvzInput* input, DvzEvent ev, void* user_data)
{
    ASSERT(input != NULL);
    log_debug("key press %d, modifiers %d", ev.content.k.key_code, ev.mods);
    DvzKeyCode* k = input->keyboard.keys;
    log_debug("%d key(s) pressed: %d %d %d %d", input->keyboard.key_count, k[0], k[1], k[2], k[3]);
    ASSERT(user_data != NULL);
    *((int*)user_data) = ev.content.k.key_code;
}

static void _on_key_release(DvzInput* input, DvzEvent ev, void* user_data)
{
    ASSERT(input != NULL);
    log_debug("key release %d", ev.content.k.key_code);
    ASSERT(user_data != NULL);
    *((int*)user_data) = DVZ_KEY_NONE;
}

int test_input_keyboard_1(TstSuite* suite)
{
    // Create an input and window.
    DvzInput input = dvz_input();
    DvzEvent ev = {0};

    // Keyboard callbacks.
    DvzKeyCode key = {0};
    dvz_input_callback(&input, DVZ_EVENT_KEYBOARD_PRESS, _on_key_press, &key);
    dvz_input_callback(&input, DVZ_EVENT_KEYBOARD_RELEASE, _on_key_release, &key);

    // Simulate a key stroke.
    ev.content.k.key_code = DVZ_KEY_A;
    dvz_input_event(&input, DVZ_EVENT_KEYBOARD_PRESS, ev, false);
    dvz_deq_wait(&input.deq, 0);
    dvz_sleep(10);
    AT(key == DVZ_KEY_A);

    dvz_input_event(&input, DVZ_EVENT_KEYBOARD_RELEASE, ev, false);
    dvz_deq_wait(&input.deq, 0);
    dvz_sleep(10);
    AT(key == DVZ_KEY_NONE);

    // Destroy the resources.
    dvz_input_destroy(&input);
    return 0;
}
