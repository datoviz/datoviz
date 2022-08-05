/*************************************************************************************************/
/*  Testing mouse                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_mouse.h"
#include "mouse.h"
#include "test.h"
#include "testing.h"



/*************************************************************************************************/
/*  Mouse tests                                                                                  */
/*************************************************************************************************/

static void _on_mouse_move(DvzMouse* mouse, DvzMouseEvent ev, void* user_data)
{
    ASSERT(mouse != NULL);
    ASSERT(user_data != NULL);

    ASSERT(ev.type == DVZ_MOUSE_EVENT_MOVE);
    *((float*)user_data) = ev.content.m.pos[0];
}

int test_mouse_1(TstSuite* suite)
{
    DvzMouse* mouse = dvz_mouse();

    float res = 0;
    dvz_mouse_callback(mouse, DVZ_MOUSE_EVENT_MOVE, _on_mouse_move, &res);

    // Mouse move.
    vec2 pos = {100, 200};
    dvz_mouse_move(mouse, pos, 0);
    AT(res == 100);

    // Destroy the resources.
    dvz_mouse_destroy(mouse);
    return 0;
}
