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
    AT(interact.u.p.camera_pos[1] == 0);

    dvz_event_mouse_move(canvas, (vec2){100, 20}, 0);
    AT(mouse->cur_state == DVZ_MOUSE_STATE_DRAG);
    dvz_interact_update(&interact, canvas->viewport, &canvas->mouse, &canvas->keyboard);
    AIN(interact.u.p.camera_pos[0], -.9, -.1);
    AT(interact.u.p.camera_pos[1] != 0);

    dvz_event_mouse_release(canvas, DVZ_MOUSE_BUTTON_LEFT, 0);
    AT(mouse->cur_state == DVZ_MOUSE_STATE_INACTIVE);


    // Zoom with right drag.
    dvz_event_mouse_move(canvas, (vec2){10, 10}, 0);
    dvz_event_mouse_press(canvas, DVZ_MOUSE_BUTTON_RIGHT, 0);
    AT(mouse->cur_state == DVZ_MOUSE_STATE_INACTIVE);
    AT(interact.u.p.zoom[0] == 1);
    AT(interact.u.p.zoom[1] == 1);

    dvz_event_mouse_move(canvas, (vec2){100, 20}, 0);
    AT(mouse->cur_state == DVZ_MOUSE_STATE_DRAG);
    dvz_interact_update(&interact, canvas->viewport, &canvas->mouse, &canvas->keyboard);
    AT(interact.u.p.zoom[0] > 1);
    AT(interact.u.p.zoom[1] < 1);

    dvz_event_mouse_release(canvas, DVZ_MOUSE_BUTTON_LEFT, 0);
    AT(mouse->cur_state == DVZ_MOUSE_STATE_INACTIVE);


    // Reset with double-click.
    dvz_event_mouse_double_click(canvas, (vec2){10, 10}, DVZ_MOUSE_BUTTON_LEFT, 0);
    dvz_interact_update(&interact, canvas->viewport, &canvas->mouse, &canvas->keyboard);
    dvz_interact_update(&interact, canvas->viewport, &canvas->mouse, &canvas->keyboard);
    AT(interact.u.p.camera_pos[0] == 0);
    AT(interact.u.p.camera_pos[1] == 0);
    AT(interact.u.p.zoom[0] == 1);
    AT(interact.u.p.zoom[1] == 1);


    dvz_interact_destroy(&interact);
    return 0;
}



/*************************************************************************************************/
/*  Arcball tests                                                                                */
/*************************************************************************************************/

int test_interact_arcball(TestContext* context)
{
    ASSERT(context != NULL);

    DvzCanvas* canvas = context->canvas;

    DvzInteract interact = dvz_interact_builtin(canvas, DVZ_INTERACT_ARCBALL);
    DvzMouse* mouse = &canvas->mouse;


    // Rotate with left drag.
    dvz_event_mouse_move(canvas, (vec2){10, 10}, 0);
    dvz_event_mouse_press(canvas, DVZ_MOUSE_BUTTON_LEFT, 0);
    AT(mouse->cur_state == DVZ_MOUSE_STATE_INACTIVE);
    // glm_mat4_print(interact.u.a.mat, stdout);
    AC(glm_mat4_trace(interact.u.a.mat), 4, 1e-6);

    dvz_event_mouse_move(canvas, (vec2){100, 20}, 0);
    AT(mouse->cur_state == DVZ_MOUSE_STATE_DRAG);
    dvz_interact_update(&interact, canvas->viewport, &canvas->mouse, &canvas->keyboard);
    // glm_mat4_print(interact.u.a.mat, stdout);
    AIN(interact.u.a.mat[0][0], .9, 1);
    AIN(interact.u.a.mat[1][0], .1, .5);
    AIN(interact.u.a.mat[0][1], -.5, -.1);
    AIN(interact.u.a.mat[1][1], .9, 1);

    dvz_event_mouse_release(canvas, DVZ_MOUSE_BUTTON_LEFT, 0);
    AT(mouse->cur_state == DVZ_MOUSE_STATE_INACTIVE);


    // // Pan with right drag.
    // dvz_event_mouse_move(canvas, (vec2){10, 10}, 0);
    // dvz_event_mouse_press(canvas, DVZ_MOUSE_BUTTON_RIGHT, 0);
    // AT(mouse->cur_state == DVZ_MOUSE_STATE_INACTIVE);
    // glm_mat4_print(interact.u.a.mat, stdout);

    // dvz_event_mouse_move(canvas, (vec2){100, 20}, 0);
    // AT(mouse->cur_state == DVZ_MOUSE_STATE_DRAG);
    // dvz_interact_update(&interact, canvas->viewport, &canvas->mouse, &canvas->keyboard);
    // glm_mat4_print(interact.u.a.mat, stdout);

    // dvz_event_mouse_release(canvas, DVZ_MOUSE_BUTTON_LEFT, 0);
    // AT(mouse->cur_state == DVZ_MOUSE_STATE_INACTIVE);


    // Reset with double-click.
    dvz_event_mouse_double_click(canvas, (vec2){10, 10}, DVZ_MOUSE_BUTTON_LEFT, 0);
    dvz_interact_update(&interact, canvas->viewport, &canvas->mouse, &canvas->keyboard);
    dvz_interact_update(&interact, canvas->viewport, &canvas->mouse, &canvas->keyboard);
    // glm_mat4_print(interact.u.a.mat, stdout);
    AC(glm_mat4_trace(interact.u.a.mat), 4, 1e-6);


    dvz_interact_destroy(&interact);
    return 0;
}



/*************************************************************************************************/
/*  Camera tests                                                                                 */
/*************************************************************************************************/

int test_interact_camera(TestContext* context)
{
    ASSERT(context != NULL);

    DvzCanvas* canvas = context->canvas;

    DvzInteract interact = dvz_interact_builtin(canvas, DVZ_INTERACT_FLY);
    DvzMouse* mouse = &canvas->mouse;

    float eps = 1e-6;
    vec3 forward = {0, 0, -1};


    // Move camera target with left drag.
    dvz_event_mouse_move(canvas, (vec2){10, 10}, 0);
    dvz_event_mouse_press(canvas, DVZ_MOUSE_BUTTON_LEFT, 0);
    AT(mouse->cur_state == DVZ_MOUSE_STATE_INACTIVE);
    // glm_vec3_print(interact.u.c.forward, stdout);
    AT(glm_vec3_distance2(interact.u.c.forward, forward) < eps);

    dvz_event_mouse_move(canvas, (vec2){100, 20}, 0);
    AT(mouse->cur_state == DVZ_MOUSE_STATE_DRAG);
    dvz_interact_update(&interact, canvas->viewport, &canvas->mouse, &canvas->keyboard);
    AIN(interact.u.c.forward[0], .1, .5);
    AIN(interact.u.c.forward[1], -.1, -.01);
    AIN(interact.u.c.forward[2], -1, -.9);

    dvz_event_mouse_release(canvas, DVZ_MOUSE_BUTTON_LEFT, 0);
    AT(mouse->cur_state == DVZ_MOUSE_STATE_INACTIVE);


    // Reset with double-click.
    dvz_event_mouse_double_click(canvas, (vec2){10, 10}, DVZ_MOUSE_BUTTON_LEFT, 0);
    dvz_interact_update(&interact, canvas->viewport, &canvas->mouse, &canvas->keyboard);
    dvz_interact_update(&interact, canvas->viewport, &canvas->mouse, &canvas->keyboard);
    AT(glm_vec3_distance2(interact.u.c.forward, forward) < eps);
    dvz_event_mouse_release(canvas, DVZ_MOUSE_BUTTON_LEFT, 0);
    AT(mouse->cur_state == DVZ_MOUSE_STATE_INACTIVE);


    // Move camera position.
    canvas->clock.elapsed = .1;
    canvas->clock.interval = .01;
    AT(interact.u.c.target[0] == 0);
    AT(interact.u.c.target[1] == 0);
    AT(interact.u.c.target[2] == 4);
    dvz_event_key_press(canvas, DVZ_KEY_UP, 0);
    dvz_interact_update(&interact, canvas->viewport, &canvas->mouse, &canvas->keyboard);
    dvz_event_key_release(canvas, DVZ_KEY_UP, 0);
    dvz_interact_update(&interact, canvas->viewport, &canvas->mouse, &canvas->keyboard);
    // glm_vec3_print(interact.u.c.target, stdout);
    AT(interact.u.c.target[0] == 0);
    AT(interact.u.c.target[1] == 0);
    AIN(interact.u.c.target[2], 3.5, 3.99);


    dvz_interact_destroy(&interact);
    return 0;
}
