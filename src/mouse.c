/*************************************************************************************************/
/*  Mouse                                                                                        */
/*************************************************************************************************/

#include "mouse.h"
#include "common.h"
#include "datoviz_types.h"



/*************************************************************************************************/
/*  Defines                                                                                      */
/*************************************************************************************************/

#define NULL_EVENT (DvzMouseEvent){0};



/*************************************************************************************************/
/*  Mouse util functions                                                                         */
/*************************************************************************************************/

static void _callbacks(DvzMouse* mouse, DvzMouseEvent event)
{
    ANN(mouse);
    ANN(mouse->callbacks);

    DvzMousePayload* payload = NULL;
    uint32_t n = dvz_list_count(mouse->callbacks);
    for (uint32_t i = 0; i < n; i++)
    {
        payload = (DvzMousePayload*)dvz_list_get(mouse->callbacks, i).p;
        event.user_data = payload->user_data;
        if (payload->type == event.type || payload->type == DVZ_MOUSE_EVENT_ALL)
        {
            payload->callback(mouse, event);
        }
    }
}



/*************************************************************************************************/
/*  Mouse state transitions                                                                      */
/*************************************************************************************************/

static DvzMouseEvent _after_release(DvzMouse* mouse, DvzMouseButton button, int mods)
{
    ANN(mouse);

    // Old state.
    DvzMouseState state = mouse->state;

    // Save the button.
    mouse->button = DVZ_MOUSE_BUTTON_NONE;

    // Delay since the last click.
    double delay = mouse->time - mouse->last_press;

    // Generate the press event, may be modified below.
    DvzMouseEvent ev = {0};
    ev.type = DVZ_MOUSE_EVENT_RELEASE;
    ev.mods = mods;
    ev.content.b.button = button;

    switch (state)
    {
    case DVZ_MOUSE_STATE_RELEASE:
    case DVZ_MOUSE_STATE_CLICK:
        return NULL_EVENT;
        break;

    case DVZ_MOUSE_STATE_DOUBLE_CLICK:
        mouse->state = DVZ_MOUSE_STATE_RELEASE;
        break;

    case DVZ_MOUSE_STATE_DRAGGING:
        mouse->state = DVZ_MOUSE_STATE_RELEASE;
        ev.type = DVZ_MOUSE_EVENT_DRAG_STOP;
        ev.content.d.button = button;
        glm_vec2_copy(mouse->cur_pos, ev.content.d.cur_pos);
        break;

    case DVZ_MOUSE_STATE_PRESS:
    case DVZ_MOUSE_STATE_CLICK_PRESS:
        if (delay <= DVZ_MOUSE_CLICK_MAX_DELAY)
        {
            mouse->state = DVZ_MOUSE_STATE_CLICK;
            ev.type = state == DVZ_MOUSE_STATE_CLICK_PRESS ? DVZ_MOUSE_EVENT_DOUBLE_CLICK
                                                           : DVZ_MOUSE_EVENT_CLICK;
            ev.content.c.button = button;
            glm_vec2_copy(mouse->cur_pos, ev.pos);

            // Record the time of the last click.
            mouse->last_click = mouse->time;
        }
        else
        {
            mouse->state = DVZ_MOUSE_STATE_RELEASE;
        }
        break;

    default:
        break;
    }

    return ev;
}



static DvzMouseEvent _after_press(DvzMouse* mouse, DvzMouseButton button, int mods)
{
    ANN(mouse);

    // Old state.
    DvzMouseState state = mouse->state;

    // Save the button.
    mouse->button = button;

    // Copy the press position and time.
    glm_vec2_copy(mouse->cur_pos, mouse->press_pos);

    // Delay since the last press.
    double delay = mouse->time - mouse->last_press;
    mouse->last_press = mouse->time;

    // Generate the press event, may be modified below.
    DvzMouseEvent ev = {0};
    ev.type = DVZ_MOUSE_EVENT_PRESS;
    ev.mods = mods;
    ev.content.b.button = button;

    switch (state)
    {
    case DVZ_MOUSE_STATE_PRESS:
    case DVZ_MOUSE_STATE_DRAGGING:
        return NULL_EVENT;
        break;

    case DVZ_MOUSE_STATE_RELEASE:
    case DVZ_MOUSE_STATE_DOUBLE_CLICK:
        mouse->state = DVZ_MOUSE_STATE_PRESS;
        break;

    case DVZ_MOUSE_STATE_CLICK:
        if (delay <= DVZ_MOUSE_DOUBLE_CLICK_MAX_DELAY)
        {
            mouse->state = DVZ_MOUSE_STATE_CLICK_PRESS;
        }
        else
        {
            mouse->state = DVZ_MOUSE_STATE_PRESS;
        }
        break;

    case DVZ_MOUSE_STATE_CLICK_PRESS:
    default:
        break;
    }

    return ev;
}



static DvzMouseEvent _after_move(DvzMouse* mouse, vec2 pos, int mods)
{
    ANN(mouse);

    // Old state.
    DvzMouseState state = mouse->state;

    // Copy the current position.
    glm_vec2_copy(pos, mouse->cur_pos);

    // Delay since the last click.
    float delta = glm_vec2_distance(mouse->press_pos, mouse->cur_pos);

    // Generate the press event, may be modified below.
    DvzMouseEvent ev = {0};
    ev.type = DVZ_MOUSE_EVENT_MOVE;
    ev.mods = mods;
    glm_vec2_copy(pos, ev.pos);

    switch (state)
    {
    case DVZ_MOUSE_STATE_RELEASE:
        break;

    case DVZ_MOUSE_STATE_PRESS:
    case DVZ_MOUSE_STATE_CLICK_PRESS:
        if (delta > DVZ_MOUSE_CLICK_MAX_SHIFT)
        {
            mouse->state = DVZ_MOUSE_STATE_DRAGGING;
            ev.type = DVZ_MOUSE_EVENT_DRAG_START;
            ev.content.d.button = mouse->button;
            glm_vec2_copy(pos, ev.content.d.cur_pos);
        }
        break;

    case DVZ_MOUSE_STATE_CLICK:
        if (delta > DVZ_MOUSE_CLICK_MAX_SHIFT)
        {
            mouse->state = DVZ_MOUSE_STATE_RELEASE;
        }
        break;

    case DVZ_MOUSE_STATE_DOUBLE_CLICK:
        mouse->state = DVZ_MOUSE_STATE_RELEASE;
        break;

    case DVZ_MOUSE_STATE_DRAGGING:
        ev.type = DVZ_MOUSE_EVENT_DRAG;
        glm_vec2_copy(pos, ev.content.d.cur_pos);
        // Shift between the press position and the current position.
        glm_vec2_sub(pos, mouse->press_pos, ev.content.d.shift);
        glm_vec2_copy(mouse->press_pos, ev.pos); // NOTE: ev.pos is the dragging press position
        ev.content.d.button = mouse->button;
        break;

    default:
        break;
    }

    return ev;
}



static DvzMouseEvent _after_wheel(DvzMouse* mouse, vec2 dir, int mods)
{
    ANN(mouse);

    // Old state.
    DvzMouseState state = mouse->state;

    // Generate the press event, may be modified below.
    DvzMouseEvent ev = {0};
    ev.type = DVZ_MOUSE_EVENT_WHEEL;
    ev.mods = mods;
    glm_vec2_copy(dir, ev.content.w.dir);
    glm_vec2_copy(mouse->cur_pos, ev.pos);

    if (state == DVZ_MOUSE_STATE_DOUBLE_CLICK)
    {
        mouse->state = DVZ_MOUSE_STATE_RELEASE;
    }

    return ev;
}



/*************************************************************************************************/
/*  Mouse functions                                                                              */
/*************************************************************************************************/

DvzMouse* dvz_mouse(void)
{
    DvzMouse* mouse = calloc(1, sizeof(DvzMouse));
    mouse->callbacks = dvz_list();
    return mouse;
}



void dvz_mouse_move(DvzMouse* mouse, vec2 pos, int mods)
{
    ANN(mouse);

    // This call may change the mouse state, and return an output transition.
    DvzMouseEvent ev = _after_move(mouse, pos, mods);
    _callbacks(mouse, ev);
}



void dvz_mouse_press(DvzMouse* mouse, DvzMouseButton button, int mods)
{
    ANN(mouse);

    // This call may change the mouse state, and return an output transition.
    DvzMouseEvent ev = _after_press(mouse, button, mods);
    _callbacks(mouse, ev);
}



void dvz_mouse_release(DvzMouse* mouse, DvzMouseButton button, int mods)
{
    ANN(mouse);

    // This call may change the mouse state, and return an output transition.
    DvzMouseEvent ev = _after_release(mouse, button, mods);
    _callbacks(mouse, ev);
}



void dvz_mouse_wheel(DvzMouse* mouse, vec2 dir, int mods)
{
    ANN(mouse);

    // This call may change the mouse state, and return an output transition.
    DvzMouseEvent ev = _after_wheel(mouse, dir, mods);
    _callbacks(mouse, ev);
}



void dvz_mouse_tick(DvzMouse* mouse, double time)
{
    ANN(mouse);
    mouse->time = time;
}



void dvz_mouse_callback(
    DvzMouse* mouse, DvzMouseEventType type, DvzMouseCallback callback, void* user_data)
{
    ANN(mouse);

    DvzMousePayload* payload = (DvzMousePayload*)calloc(1, sizeof(DvzMousePayload));
    payload->type = type;
    payload->callback = callback;
    payload->user_data = user_data;
    dvz_list_append(mouse->callbacks, (DvzListItem){.p = (void*)payload});
}



void dvz_mouse_destroy(DvzMouse* mouse)
{
    ANN(mouse);
    ANN(mouse->callbacks);

    // Free the callback payloads.
    DvzMousePayload* payload = NULL;
    for (uint32_t i = 0; i < mouse->callbacks->count; i++)
    {
        payload = (DvzMousePayload*)(dvz_list_get(mouse->callbacks, i).p);
        ANN(payload);
        FREE(payload);
    }
    dvz_list_destroy(mouse->callbacks);

    FREE(mouse);
}
