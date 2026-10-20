/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../input/_input_router.h"
#include "datoviz/input/keyboard.h"
#include "window_internal.h"
#include <float.h>
#include <math.h>

/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Validate one original before any state or callback changes.
 * @param event borrowed observation
 * @return whether all dispatched fields are valid
 */
static bool _host_input_valid(const DvzHostInputEvent* event)
{
    if (event == NULL || event->type < DVZ_HOST_INPUT_MOVE || event->type > DVZ_HOST_INPUT_CANCEL)
        return false;
    if (!isfinite(event->x) || !isfinite(event->y) || !isfinite(event->dx) ||
        !isfinite(event->dy) || fabs(event->x) > FLT_MAX || fabs(event->y) > FLT_MAX ||
        fabs(event->dx) > FLT_MAX || fabs(event->dy) > FLT_MAX)
        return false;
    switch (event->type)
    {
    case DVZ_HOST_INPUT_BUTTON:
        return event->button >= 0 && event->button < 8 &&
               (event->action == 0 || event->action == 1);
    case DVZ_HOST_INPUT_KEY:
        return event->key >= -1 && event->key <= 348 && event->logical_key >= -1 &&
               event->logical_key <= 348 && event->action >= 0 && event->action <= 2;
    case DVZ_HOST_INPUT_TEXT:
        return event->byte_size <= 1024 * 1024 &&
               _dvz_input_utf8_valid(event->utf8, event->byte_size);
    case DVZ_HOST_INPUT_METRICS:
        return isfinite(event->metrics.device_scale.x) && event->metrics.device_scale.x > 0 &&
               isfinite(event->metrics.device_scale.y) && event->metrics.device_scale.y > 0 &&
               isfinite(event->metrics.native_to_logical.x) &&
               event->metrics.native_to_logical.x > 0 &&
               isfinite(event->metrics.native_to_logical.y) &&
               event->metrics.native_to_logical.y > 0 &&
               isfinite(event->metrics.framebuffer_scale.x) &&
               event->metrics.framebuffer_scale.x > 0 &&
               isfinite(event->metrics.framebuffer_scale.y) &&
               event->metrics.framebuffer_scale.y > 0 &&
               isfinite(event->metrics.content_scale.x) && event->metrics.content_scale.x > 0 &&
               isfinite(event->metrics.content_scale.y) && event->metrics.content_scale.y > 0;
    default:
        return true;
    }
}



/**
 * Emit cancellation without a successful button release or click.
 * @param window input host
 * @param timestamp_ns logical cancellation time
 */
static void _host_input_cancel(DvzWindow* window, uint64_t timestamp_ns)
{
    window->input_state.buttons = 0;
    window->input_state.held_key_count = 0;
    window->input_state.mods = 0;
    window->glfw_mods = 0;
    for (uint32_t key = 0; key < 349; key++)
    {
        if (window->input_keys[key])
            dvz_keyboard_emit(
                window->router, DVZ_KEYBOARD_EVENT_RELEASE, (DvzKeyCode)key, 0, window->user_data);
        window->input_keys[key] = false;
    }
    DvzPointerEvent cancel = {
        .type = DVZ_POINTER_EVENT_CANCEL,
        .timestamp_ns = timestamp_ns,
        .user_data = window->user_data};
    dvz_input_emit_pointer(window->router, &cancel);
}



/**
 * Preserve legacy raw-integration callbacks after host observers.
 * @param window input host
 * @param event validated original
 * @return whether the integration consumed the original
 */
static bool _host_input_legacy(DvzWindow* window, const DvzHostInputEvent* event)
{
    const DvzWindowGlfwInputCallbacks* c = &window->glfw_input_callbacks;
    void* data = window->glfw_input_user_data;
    switch (event->type)
    {
    case DVZ_HOST_INPUT_MOVE:
        return c->cursor_pos != NULL && c->cursor_pos(window, event->x, event->y, data);
    case DVZ_HOST_INPUT_BUTTON:
        return c->mouse_button != NULL &&
               c->mouse_button(window, event->button, event->action, event->mods, data);
    case DVZ_HOST_INPUT_WHEEL:
        return c->scroll != NULL && c->scroll(window, event->dx, event->dy, data);
    case DVZ_HOST_INPUT_KEY:
        return c->key != NULL &&
               c->key(window, event->key, event->scancode, event->action, event->mods, data);
    case DVZ_HOST_INPUT_TEXT:
    {
        if (c->character == NULL)
            return false;
        uint32_t i = 0;
        bool consumed = false;
        while (i < event->byte_size)
        {
            uint8_t first = (uint8_t)event->utf8[i++];
            uint32_t value = first;
            uint32_t extra = 0;
            if (first >= 0xf0)
            {
                value = first & 7;
                extra = 3;
            }
            else if (first >= 0xe0)
            {
                value = first & 15;
                extra = 2;
            }
            else if (first >= 0xc0)
            {
                value = first & 31;
                extra = 1;
            }
            for (uint32_t j = 0; j < extra; j++)
                value = (value << 6) | ((uint8_t)event->utf8[i++] & 63);
            consumed = c->character(window, value, data) || consumed;
        }
        return consumed;
    }
    default:
        return false;
    }
}



/**
 * Route one unconsumed original, normalizing coordinates and wheel sign exactly once.
 * @param window input host
 * @param event validated original
 * @return dispatch result
 */
static DvzResult _host_input_route(DvzWindow* window, const DvzHostInputEvent* event)
{
    DvzInputRouter* router = window->router;
    const DvzWindowMetrics* m = &window->metrics;
    float x = (float)(event->x * m->native_to_logical.x);
    float y = (float)(event->y * m->native_to_logical.y);
    float width = m->logical_size.width > 0 ? (float)m->logical_size.width : 1.0f;
    float height = m->logical_size.height > 0 ? (float)m->logical_size.height : 1.0f;
    switch (event->type)
    {
    case DVZ_HOST_INPUT_MOVE:
    case DVZ_HOST_INPUT_BUTTON:
        dvz_pointer_emit_position(
            router,
            event->type == DVZ_HOST_INPUT_MOVE ? DVZ_POINTER_EVENT_MOVE
            : event->action == 1               ? DVZ_POINTER_EVENT_PRESS
                                               : DVZ_POINTER_EVENT_RELEASE,
            x, y, width, height,
            event->type == DVZ_HOST_INPUT_MOVE ? DVZ_POINTER_BUTTON_NONE
                                               : dvz_pointer_button_from_glfw(event->button),
            event->mods, window->surface.scale_x, event->timestamp_ns, window->user_data);
        break;
    case DVZ_HOST_INPUT_WHEEL:
    {
        double dy = event->dy;
#if defined(__APPLE__)
        dy = -dy;
#endif
        dvz_pointer_emit_wheel(
            router, x, y, width, height, (float)event->dx, (float)dy, event->mods,
            window->surface.scale_x, event->timestamp_ns, window->user_data);
        break;
    }
    case DVZ_HOST_INPUT_KEY:
        dvz_keyboard_emit(
            router,
            event->action == 0   ? DVZ_KEYBOARD_EVENT_RELEASE
            : event->action == 1 ? DVZ_KEYBOARD_EVENT_PRESS
                                 : DVZ_KEYBOARD_EVENT_REPEAT,
            (DvzKeyCode)event->key, event->mods, window->user_data);
        break;
    case DVZ_HOST_INPUT_TEXT:
    {
        DvzInputTextEvent text = {
            .utf8 = event->utf8,
            .byte_size = event->byte_size,
            .mods = event->mods,
            .user_data = window->user_data};
        return dvz_input_emit_text(router, &text);
    }
    default:
        break;
    }
    return DVZ_OK;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Subscribe an owner-thread observer before GUI arbitration.
 * @param window input host
 * @param callback borrowed observer function
 * @param user_data borrowed callback data
 * @return subscription ID or zero
 */
DvzCallbackId
dvz_window_subscribe_host_input(DvzWindow* window, DvzHostInputCallback callback, void* user_data)
{
    if (window == NULL || callback == NULL || window->input_next_id == UINT64_MAX)
        return 0;
    uint32_t index = 0;
    while (index < window->input_subscription_count && window->input_subscriptions[index].id != 0)
        index++;
    if (index == window->input_subscription_count)
    {
        if (index >= UINT32_MAX / sizeof(DvzHostInputSubscription))
            return 0;
        DvzHostInputSubscription* grown = (DvzHostInputSubscription*)dvz_calloc(
            (size_t)index + 1, sizeof(DvzHostInputSubscription));
        if (grown == NULL)
            return 0;
        if (index > 0)
            dvz_memcpy(
                grown, ((size_t)index + 1) * sizeof(*grown), window->input_subscriptions,
                (size_t)index * sizeof(*grown));
        dvz_free(window->input_subscriptions);
        window->input_subscriptions = grown;
        window->input_subscription_count++;
    }
    DvzCallbackId id = ++window->input_next_id;
    window->input_subscriptions[index] = (DvzHostInputSubscription){id, callback, user_data};
    return id;
}



/**
 * Remove one host input observer.
 * @param window input host
 * @param id subscription ID
 * @return whether the subscription existed
 */
bool dvz_window_unsubscribe_host_input(DvzWindow* window, DvzCallbackId id)
{
    if (window == NULL || id == 0)
        return false;
    for (uint32_t i = 0; i < window->input_subscription_count; i++)
        if (window->input_subscriptions[i].id == id)
        {
            window->input_subscriptions[i] = (DvzHostInputSubscription){0};
            return true;
        }
    return false;
}



/**
 * Dispatch one validated original through the shared host path.
 * @param window input host
 * @param event borrowed original
 * @param origin live or replay ownership
 * @return dispatch result
 */
DvzResult dvz_window_emit_host_input(
    DvzWindow* window, const DvzHostInputEvent* event, DvzHostInputOrigin origin)
{
    if (window == NULL || window->input_dispatching || !_host_input_valid(event) ||
        (origin != DVZ_HOST_INPUT_LIVE && origin != DVZ_HOST_INPUT_REPLAY) ||
        window->input_state.replaying != (origin == DVZ_HOST_INPUT_REPLAY))
        return DVZ_ERROR;
    if (origin == DVZ_HOST_INPUT_REPLAY && window->input_dispatch == NULL &&
        (window->glfw_input_callbacks.cursor_pos != NULL ||
         window->glfw_input_callbacks.mouse_button != NULL ||
         window->glfw_input_callbacks.scroll != NULL || window->glfw_input_callbacks.key != NULL ||
         window->glfw_input_callbacks.character != NULL))
        return DVZ_ERROR;
    DvzHostInputEvent copy = *event;
    window->input_dispatching = true;
    DvzHostInputState* state = &window->input_state;
    state->timestamp_ns = copy.timestamp_ns;
    if (copy.type == DVZ_HOST_INPUT_MOVE || copy.type == DVZ_HOST_INPUT_BUTTON ||
        copy.type == DVZ_HOST_INPUT_WHEEL)
    {
        state->x = copy.x;
        state->y = copy.y;
    }
    if (copy.type == DVZ_HOST_INPUT_KEY || copy.type == DVZ_HOST_INPUT_BUTTON ||
        copy.type == DVZ_HOST_INPUT_TEXT)
        window->glfw_mods = state->mods = copy.mods;
    if (copy.type == DVZ_HOST_INPUT_BUTTON)
    {
        uint32_t mask = 1u << copy.button;
        state->buttons = copy.action == 1 ? state->buttons | mask : state->buttons & ~mask;
    }
    if (copy.type == DVZ_HOST_INPUT_KEY && copy.key >= 0)
    {
        bool was_down = window->input_keys[copy.key];
        bool down = copy.action != 0;
        if (down && !was_down)
            state->held_key_count++;
        else if (!down && was_down)
            state->held_key_count--;
        window->input_keys[copy.key] = down;
    }
    if (copy.type == DVZ_HOST_INPUT_FOCUS)
    {
        state->focused = copy.focused;
        if (!copy.focused)
        {
            window->input_cancelled_buttons = state->buttons;
            for (uint32_t key = 0; key < 349; key++)
                window->input_cancelled_keys[key] = window->input_keys[key];
        }
    }
    if (copy.type == DVZ_HOST_INPUT_ENTER)
        state->entered = copy.entered;

    uint64_t last_id = window->input_next_id;
    uint32_t count = window->input_subscription_count;
    // Visit by monotonically increasing ID even when callbacks reuse vacant storage slots.
    uint64_t previous_id = 0;
    for (uint32_t n = 0; n < count; n++)
    {
        DvzHostInputSubscription next = {0};
        for (uint32_t i = 0; i < window->input_subscription_count; i++)
        {
            DvzHostInputSubscription sub = window->input_subscriptions[i];
            if (sub.id > previous_id && sub.id <= last_id && (next.id == 0 || sub.id < next.id))
                next = sub;
        }
        if (next.id == 0)
            break;
        previous_id = next.id;
        next.callback(window, &copy, origin, next.user_data);
    }
    bool consumed = window->input_dispatch != NULL
                        ? window->input_dispatch(window, &copy, window->input_dispatch_data)
                        : _host_input_legacy(window, &copy);
    DvzResult result = DVZ_OK;
    if (copy.type == DVZ_HOST_INPUT_CANCEL || (copy.type == DVZ_HOST_INPUT_FOCUS && !copy.focused))
        _host_input_cancel(window, copy.timestamp_ns);
    else if (copy.type == DVZ_HOST_INPUT_METRICS)
        _dvz_window_apply_metrics(window, &copy.metrics);
    else if (!consumed)
        result = _host_input_route(window, &copy);
    window->input_dispatching = false;
    dvz_window_host_request_frame(window->host, window);
    return result;
}



/**
 * Acquire or release exclusive replay input after cancelling held state.
 * @param window input host
 * @param replay requested ownership mode
 * @return ownership transition result
 */
DvzResult dvz_window_input_replay(DvzWindow* window, bool replay)
{
    if (window == NULL || window->input_dispatching)
        return DVZ_ERROR;
    if (window->input_state.replaying == replay)
        return DVZ_OK;
    DvzHostInputEvent cancel = {
        .type = DVZ_HOST_INPUT_CANCEL, .timestamp_ns = window->input_state.timestamp_ns};
    DvzResult result = dvz_window_emit_host_input(
        window, &cancel,
        window->input_state.replaying ? DVZ_HOST_INPUT_REPLAY : DVZ_HOST_INPUT_LIVE);
    window->input_state.replaying = replay;
    if (window->input_mode != NULL)
        window->input_mode(replay, window->input_dispatch_data);
    if (replay)
        window->input_state.unsupported_operation = false;
    else
        _dvz_window_glfw_refresh_input(window);
    window->input_state.delta_seconds = 1.0 / 60.0;
    return result;
}



/**
 * Supply replay GUI processing time without replacing the benchmark clock.
 * @param window replay-owned host
 * @param timestamp_ns logical processing timestamp
 * @param delta_seconds fixed positive GUI step, or zero for paced wall time
 * @return update result
 */
DvzResult dvz_window_input_frame(DvzWindow* window, uint64_t timestamp_ns, double delta_seconds)
{
    if (window == NULL || !window->input_state.replaying || window->input_dispatching ||
        !isfinite(delta_seconds) || delta_seconds < 0 || delta_seconds > 3600)
        return DVZ_ERROR;
    window->input_state.timestamp_ns = timestamp_ns;
    window->input_state.delta_seconds = delta_seconds;
    return DVZ_OK;
}



/**
 * Return the borrowed virtual input state.
 * @param window input host
 * @return borrowed state or NULL
 */
const DvzHostInputState* dvz_window_host_input_state(const DvzWindow* window)
{
    return window != NULL ? &window->input_state : NULL;
}



/**
 * Install one borrowed GUI arbitration callback; clear before freeing its data.
 * @param window input host
 * @param callback borrowed GUI integration
 * @param pending borrowed queued-input query
 * @param mode borrowed replay ownership transition handler
 * @param user_data borrowed integration data
 */
void _dvz_window_set_host_input_dispatch(
    DvzWindow* window, DvzHostInputDispatch callback, uint32_t (*pending)(void*),
    void (*mode)(bool, void*), void* user_data)
{
    window->input_dispatch = callback;
    window->input_pending = pending;
    window->input_mode = mode;
    window->input_dispatch_data = user_data;
}



/**
 * Query pending GUI observations without consuming them.
 * @param window input host
 * @return queued observation count
 */
uint32_t dvz_window_input_pending(DvzWindow* window)
{
    return window != NULL && window->input_pending != NULL
               ? window->input_pending(window->input_dispatch_data)
               : 0;
}



/**
 * Return the borrowed host for controlled polling without entering rendering scheduling.
 * @param window input host
 * @return owning host or NULL
 */
DvzWindowHost* dvz_window_host_for_window(DvzWindow* window)
{
    return window != NULL ? window->host : NULL;
}



/**
 * Consume one GLFW-generated release caused by an already-recorded focus cancellation.
 * @param window input host
 * @param keyboard whether token is a key rather than a mouse button
 * @param token native key/button token
 * @return whether this release is a cancellation consequence rather than a new original
 */
bool _dvz_window_cancelled_release(DvzWindow* window, bool keyboard, int token)
{
    if (window == NULL || token < 0)
        return false;
    if (keyboard)
    {
        if (token >= 349 || !window->input_cancelled_keys[token])
            return false;
        window->input_cancelled_keys[token] = false;
        return true;
    }
    if (token >= 8 || (window->input_cancelled_buttons & (1u << token)) == 0)
        return false;
    window->input_cancelled_buttons &= ~(1u << token);
    return true;
}
