/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing mouse                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_mouse.h"
#include "datoviz.h"
#include "mouse.h"
#include "test.h"
#include "testing.h"



/*************************************************************************************************/
/*  Mouse tests                                                                                  */
/*************************************************************************************************/

static void _on_mouse_move(DvzMouse* mouse, DvzMouseEvent* ev)
{
    ANN(mouse);
    ANN(ev->user_data);

    ASSERT(ev->type == DVZ_MOUSE_EVENT_MOVE);
    *((float*)ev->user_data) = ev->pos[0];
}

int test_mouse_move(TstSuite* suite)
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



static void _on_mouse_press(DvzMouse* mouse, DvzMouseEvent* ev)
{
    ANN(mouse);
    ANN(ev->user_data);

    ASSERT(ev->type == DVZ_MOUSE_EVENT_PRESS);
    *((int*)ev->user_data) = (int)ev->button;
}

static void _on_mouse_click(DvzMouse* mouse, DvzMouseEvent* ev)
{
    ANN(mouse);
    ANN(ev->user_data);

    ASSERT(ev->type == DVZ_MOUSE_EVENT_CLICK);
    *((float*)ev->user_data) = ev->pos[0];
}

typedef struct MouseOrder
{
    int idx;
    DvzMouseEventType order[4];
} MouseOrder;

static void _on_mouse_order(DvzMouse* mouse, DvzMouseEvent* ev)
{
    ANN(mouse);
    ANN(ev->user_data);

    MouseOrder* order = (MouseOrder*)ev->user_data;
    if (order->idx < (int)(sizeof(order->order) / sizeof(order->order[0])))
        order->order[order->idx++] = ev->type;
}

int test_mouse_press(TstSuite* suite)
{
    DvzMouse* mouse = dvz_mouse();

    int res = 0;
    float resp = 0;
    MouseOrder order = {0};
    dvz_mouse_callback(mouse, DVZ_MOUSE_EVENT_PRESS, _on_mouse_press, &res);
    dvz_mouse_callback(mouse, DVZ_MOUSE_EVENT_CLICK, _on_mouse_click, &resp);
    dvz_mouse_callback(mouse, DVZ_MOUSE_EVENT_CLICK, _on_mouse_order, &order);
    dvz_mouse_callback(mouse, DVZ_MOUSE_EVENT_RELEASE, _on_mouse_order, &order);

    // Mouse press.
    DvzMouseButton button = DVZ_MOUSE_BUTTON_LEFT;
    dvz_mouse_press(mouse, button, 0);
    AT(res == (int)button);

    // Time.
    dvz_mouse_tick(mouse, .01);

    // Minor mouse movement.
    vec2 pos = {2, 1};
    dvz_mouse_move(mouse, pos, 0);

    // Mouse release.
    dvz_mouse_release(mouse, button, 0);

    // Should trigger click.
    AT(resp == 2);
    AT(order.idx == 2);
    AT(order.order[0] == DVZ_MOUSE_EVENT_CLICK);
    AT(order.order[1] == DVZ_MOUSE_EVENT_RELEASE);
    order.idx = 0;

    // Minor mouse movement.
    pos[0] = 1;
    dvz_mouse_move(mouse, pos, 0);

    // Time.
    dvz_mouse_tick(mouse, .5);

    // New click, but not close enough to do a double-click.
    dvz_mouse_press(mouse, button, 0);
    dvz_mouse_release(mouse, button, 0);
    AT(resp == 1);
    AT(order.idx == 2);
    AT(order.order[0] == DVZ_MOUSE_EVENT_CLICK);
    AT(order.order[1] == DVZ_MOUSE_EVENT_RELEASE);

    // Destroy the resources.
    dvz_mouse_destroy(mouse);
    return 0;
}



static void _on_mouse_wheel(DvzMouse* mouse, DvzMouseEvent* ev)
{
    ANN(mouse);
    ANN(ev->user_data);

    ASSERT(ev->type == DVZ_MOUSE_EVENT_WHEEL);
    *((float*)ev->user_data) = ev->content.w.dir[1];
}

int test_mouse_wheel(TstSuite* suite)
{
    DvzMouse* mouse = dvz_mouse();

    float res = 0;
    dvz_mouse_callback(mouse, DVZ_MOUSE_EVENT_WHEEL, _on_mouse_wheel, &res);

    // Mouse move.
    vec2 pos = {100, 200};
    dvz_mouse_move(mouse, pos, 0);
    AT(res == 0);

    // Mouse wheel.
    dvz_mouse_wheel(mouse, (vec2){1, 2}, 0);
    AT(res == 2);

    // Destroy the resources.
    dvz_mouse_destroy(mouse);
    return 0;
}



static void _on_mouse_drag(DvzMouse* mouse, DvzMouseEvent* ev)
{
    ANN(mouse);
    ANN(ev->user_data);

    // ASSERT(ev->type == DVZ_MOUSE_EVENT_DRAG);
    *((float*)ev->user_data) = ev->pos[0];
}

static void _on_mouse_drag_stop(DvzMouse* mouse, DvzMouseEvent* ev)
{
    ANN(mouse);
    ANN(ev->user_data);

    ASSERT(ev->type == DVZ_MOUSE_EVENT_DRAG_STOP);
    *((int*)ev->user_data) = (int)ev->button;
}

int test_mouse_drag(TstSuite* suite)
{
    DvzMouse* mouse = dvz_mouse();

    float res = 0;
    int resi = 0;
    MouseOrder order = {0};
    dvz_mouse_callback(mouse, DVZ_MOUSE_EVENT_DRAG, _on_mouse_drag, &res);
    dvz_mouse_callback(mouse, DVZ_MOUSE_EVENT_DRAG_START, _on_mouse_drag, &res);
    dvz_mouse_callback(mouse, DVZ_MOUSE_EVENT_DRAG_STOP, _on_mouse_drag_stop, &resi);
    dvz_mouse_callback(mouse, DVZ_MOUSE_EVENT_DRAG_STOP, _on_mouse_order, &order);
    dvz_mouse_callback(mouse, DVZ_MOUSE_EVENT_RELEASE, _on_mouse_order, &order);

    // Mouse press.
    DvzMouseButton button = DVZ_MOUSE_BUTTON_LEFT;
    dvz_mouse_press(mouse, button, 0);

    // Time.
    dvz_mouse_tick(mouse, .01);

    // Minor mouse movement, dragging not started.
    vec2 pos = {2, 1};
    dvz_mouse_move(mouse, pos, 0);
    AT(res == 0);

    // Large mouse movement, dragging started.
    pos[0] = 20;
    pos[1] = 50;
    dvz_mouse_move(mouse, pos, 0);
    AT(res == 20);

    // Mouse release.
    dvz_mouse_release(mouse, button, 0);
    AT(resi == (int)DVZ_MOUSE_BUTTON_LEFT);
    AT(order.idx == 2);
    AT(order.order[0] == DVZ_MOUSE_EVENT_DRAG_STOP);
    AT(order.order[1] == DVZ_MOUSE_EVENT_RELEASE);

    // Destroy the resources.
    dvz_mouse_destroy(mouse);
    return 0;
}
