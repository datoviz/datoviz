#include "../include/datoviz/interact.h"
#include "proto.h"
#include "tests.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Panzoom tests                                                                                */
/*************************************************************************************************/

int test_interact_panzoom(TestContext* context)
{
    ASSERT(context != NULL);

    DvzCanvas* canvas = context->canvas;

    DvzInteract interact = dvz_interact_builtin(canvas, DVZ_INTERACT_PANZOOM);
    DvzMouse* mouse = &canvas->mouse;


    // Pan with left drag.
    dvz_event_mouse_move(canvas, (vec2){10, 10}, 0);
    dvz_event_mouse_press(canvas, DVZ_MOUSE_BUTTON_LEFT, 0);
    AT(mouse->cur_state == DVZ_MOUSE_STATE_INACTIVE);
    AT(interact.u.p.camera_pos[0] == 0);

    dvz_event_mouse_move(canvas, (vec2){100, 10}, 0);
    AT(mouse->cur_state == DVZ_MOUSE_STATE_DRAG);
    dvz_interact_update(&interact, canvas->viewport, &canvas->mouse, &canvas->keyboard);
    AIN(interact.u.p.camera_pos[0], -.9, -.1);

    dvz_event_mouse_release(canvas, DVZ_MOUSE_BUTTON_LEFT, 0);
    AT(mouse->cur_state == DVZ_MOUSE_STATE_INACTIVE);
    AIN(interact.u.p.camera_pos[0], -.9, -.1);


    dvz_interact_destroy(&interact);
    return 0;
}
