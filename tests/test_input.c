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
/*  Input tests                                                                                  */
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

int test_input_1(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzInput input = dvz_input();
    DvzEvent ev = {0};


    // Mouse move.
    vec2 pos_exp = {0};
    dvz_input_callback(&input, DVZ_EVENT_MOUSE_MOVE, _on_mouse_move, &pos_exp);

    ev.content.m.pos[0] = .5;
    ev.content.m.pos[1] = -.5;
    dvz_input_event(&input, DVZ_EVENT_MOUSE_MOVE, ev);
    dvz_sleep(10);
    AT(pos_exp[0] == .5);
    AT(pos_exp[1] == -.5);


    // Mouse button press.
    int button = 0;
    dvz_input_callback(&input, DVZ_EVENT_MOUSE_PRESS, _on_mouse_button, &button);

    ev.content.b.button = DVZ_MOUSE_BUTTON_LEFT;
    dvz_input_event(&input, DVZ_EVENT_MOUSE_PRESS, ev);
    // HACK: wait for the background thread to process the mouse press callback and modify the
    // button variable.
    dvz_sleep(10);
    AT(button == (int)DVZ_MOUSE_BUTTON_LEFT);


    dvz_input_destroy(&input);
    return 0;
}
