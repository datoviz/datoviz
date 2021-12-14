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
    log_info("mouse position: %.0fx%.0f", ev.content.m.pos[0], ev.content.m.pos[1]);

    ASSERT(user_data != NULL);
    vec2* pos = (vec2*)user_data;
    pos[0][0] = ev.content.m.pos[0];
    pos[0][1] = ev.content.m.pos[1];
}

int test_input_1(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzInput input = dvz_input();
    DvzEvent ev = {0};

    // Mouse position.
    vec2 pos_exp = {0};
    dvz_input_callback(&input, DVZ_EVENT_MOUSE_MOVE, _on_mouse_move, &pos_exp);

    ev.content.m.pos[0] = .5;
    ev.content.m.pos[1] = -.5;
    dvz_input_event(&input, DVZ_EVENT_MOUSE_MOVE, ev);
    dvz_sleep(10);

    dvz_input_destroy(&input);
    return 0;
}
