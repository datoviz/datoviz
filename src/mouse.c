/*************************************************************************************************/
/*  Mouse                                                                                        */
/*************************************************************************************************/

#include "mouse.h"
#include "common.h"



/*************************************************************************************************/
/*  Mouse util functions                                                                         */
/*************************************************************************************************/

static void _callbacks(DvzMouse* mouse, DvzMouseEvent event)
{
    DvzMousePayload* payload = NULL;
    uint32_t n = dvz_list_count(&mouse->callbacks);
    for (uint32_t i = 0; i < n; i++)
    {
        payload = (DvzMousePayload*)dvz_list_get(&mouse->callbacks, i).p;
        if (payload->type == event.type)
        {
            payload->callback(mouse, event, payload->user_data);
        }
    }
}



/*************************************************************************************************/
/*  Mouse state transitions                                                                      */
/*************************************************************************************************/

static DvzMouseEvent _after_release(DvzMouse* mouse)
{
    ASSERT(mouse != NULL);
    DvzMouseState state = mouse->state;

    DvzMouseEvent ev = {0};
    ev.type = DVZ_MOUSE_EVENT_RELEASE;

    switch (state)
    {
    case DVZ_MOUSE_STATE_RELEASE:
        break;

    default:
        break;
    }

    return ev;
}



static DvzMouseEvent _after_press(DvzMouse* mouse)
{
    ASSERT(mouse != NULL);
    DvzMouseState state = mouse->state;

    DvzMouseEvent ev = {0};
    ev.type = DVZ_MOUSE_EVENT_PRESS;

    switch (state)
    {
    case DVZ_MOUSE_STATE_RELEASE:
        break;

    default:
        break;
    }

    return ev;
}



static DvzMouseEvent _after_move(DvzMouse* mouse)
{
    ASSERT(mouse != NULL);
    DvzMouseState state = mouse->state;

    DvzMouseEvent ev = {0};
    ev.type = DVZ_MOUSE_EVENT_PRESS;

    switch (state)
    {
    case DVZ_MOUSE_STATE_RELEASE:
        break;

    default:
        break;
    }

    return ev;
}



static DvzMouseEvent _after_wheel(DvzMouse* mouse)
{
    ASSERT(mouse != NULL);
    DvzMouseState state = mouse->state;

    DvzMouseEvent ev = {0};
    ev.type = DVZ_MOUSE_EVENT_PRESS;

    switch (state)
    {
    case DVZ_MOUSE_STATE_RELEASE:
        break;

    default:
        break;
    }

    return ev;
}



/*************************************************************************************************/
/*  Mouse functions                                                                              */
/*************************************************************************************************/

DvzMouse* dvz_mouse()
{
    DvzMouse* mouse = calloc(1, sizeof(DvzMouse));
    mouse->callbacks = dvz_list();
    return mouse;
}



void dvz_mouse_move(DvzMouse* mouse, vec2 pos, int mods)
{
    ASSERT(mouse != NULL);

    // This call may change the mouse state, and return an output transition.
    DvzMouseEvent ev = _after_move(mouse);
    _callbacks(mouse, ev);
}



void dvz_mouse_press(DvzMouse* mouse, DvzMouseButton button, int mods)
{
    ASSERT(mouse != NULL);

    // This call may change the mouse state, and return an output transition.
    DvzMouseEvent ev = _after_press(mouse);
    _callbacks(mouse, ev);
}



void dvz_mouse_release(DvzMouse* mouse, DvzMouseButton button)
{
    ASSERT(mouse != NULL);

    // This call may change the mouse state, and return an output transition.
    DvzMouseEvent ev = _after_release(mouse);
    _callbacks(mouse, ev);
}



void dvz_mouse_wheel(DvzMouse* mouse, vec2 dir, int mods)
{
    ASSERT(mouse != NULL);

    // This call may change the mouse state, and return an output transition.
    DvzMouseEvent ev = _after_wheel(mouse);
    _callbacks(mouse, ev);
}



void dvz_mouse_tick(DvzMouse* mouse, double time)
{
    ASSERT(mouse != NULL);
    mouse->time = time;
}



void dvz_mouse_callback(
    DvzMouse* mouse, DvzMouseEventType type, DvzMouseCallback callback, void* user_data)
{
    ASSERT(mouse != NULL);
    DvzMousePayload* payload = (DvzMousePayload*)calloc(1, sizeof(DvzMousePayload));
    payload->type = type;
    payload->callback = callback;
    payload->user_data = user_data;
    dvz_list_append(&mouse->callbacks, (DvzListItem){.p = (void*)payload});
}



void dvz_mouse_destroy(DvzMouse* mouse)
{
    ASSERT(mouse != NULL);

    // Free the callback payloads.
    DvzMousePayload* payload = NULL;
    for (uint32_t i = 0; i < mouse->callbacks.count; i++)
    {
        payload = (DvzMousePayload*)(dvz_list_get(&mouse->callbacks, i).p);
        ASSERT(payload != NULL);
        FREE(payload);
    }
    dvz_list_destroy(&mouse->callbacks);

    FREE(mouse);
}
