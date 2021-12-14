/*************************************************************************************************/
/*  Input                                                                                        */
/*************************************************************************************************/

#include "input.h"
#include "_glfw_input.h"
#include "_time.h"
#include "common.h"
#include "window.h"



/*************************************************************************************************/
/*  Util functions                                                                               */
/*************************************************************************************************/

// Find the Deq queue index by inspecting the input type used for registering the callback.
static inline uint32_t _deq_from_input_type(DvzEventType type)
{
    uint32_t idx = DVZ_INPUT_DEQ_MOUSE;
    if (type == DVZ_EVENT_KEYBOARD_PRESS || type == DVZ_EVENT_KEYBOARD_RELEASE ||
        type == DVZ_EVENT_KEYBOARD_STROKE)
        idx = DVZ_INPUT_DEQ_KEYBOARD;
    return idx;
}



static void _deq_callback(DvzDeq* deq, void* item, void* user_data)
{
    ASSERT(deq != NULL);

    DvzEventPayload* payload = (DvzEventPayload*)user_data;
    ASSERT(payload != NULL);

    DvzInput* input = payload->input;
    ASSERT(input != NULL);

    DvzEvent* ev = (DvzEvent*)item;
    ASSERT(ev != NULL);

    payload->callback(input, *ev, payload->user_data);

    return;
}



// Process the async events in a background thread.
static int _input_thread(void* user_data)
{
    DvzInput* input = (DvzInput*)user_data;
    ASSERT(input != NULL);
    // Process all events in that thread.
    dvz_deq_dequeue_loop(&input->deq, 0);
    return 0;
}



/*************************************************************************************************/
/*  Input functions                                                                              */
/*************************************************************************************************/

DvzInput dvz_input(void)
{
    DvzInput input = {0};

    dvz_mouse_reset(&input.mouse);
    dvz_keyboard_reset(&input.keyboard);

    input.timers =
        dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzTimer), DVZ_OBJECT_TYPE_TIMER);

    input.clock = dvz_clock();

    // Random number generator, used to generate UUIDs for timers.
    input.prng = dvz_prng();

    // Queues: mouse, keyboard, timer.
    input.deq = dvz_deq(3);

    // A single proc handling all events.
    dvz_deq_proc(
        &input.deq, 0, 3,
        (uint32_t[]){DVZ_INPUT_DEQ_MOUSE, DVZ_INPUT_DEQ_KEYBOARD, DVZ_INPUT_DEQ_TIMER});

    return input;
}



void dvz_input_callback(
    DvzInput* input, DvzEventType type, DvzEventCallback callback, void* user_data)
{
    ASSERT(input != NULL);
    ASSERT(input->callback_count < DVZ_INPUT_MAX_CALLBACKS);

    // HACK: create the thread here, because we need to pass a pointer to the thread function.
    if (!dvz_obj_is_created(&input->thread.obj))
    {
        log_trace("creating the input thread");
        input->thread = dvz_thread(_input_thread, input);
    }

    DvzEventPayload* payload = &input->callbacks[input->callback_count++];
    payload->input = input;
    payload->user_data = user_data;
    payload->callback = callback;
    dvz_deq_callback(&input->deq, _deq_from_input_type(type), (int)type, _deq_callback, payload);
}



void dvz_input_event(DvzInput* input, DvzEventType type, DvzEvent ev)
{
    ASSERT(input != NULL);

    // Do not enqueue input events if the Input instance is blocked.
    if (input->is_blocked)
        return;

    uint32_t deq_idx = _deq_from_input_type(type);

    // // NOTE: prevent input enqueueing for mouse and keyboard if this input is inactive.
    // if (deq_idx == DVZ_INPUT_DEQ_MOUSE && !input->mouse.is_active)
    //     return;
    // if (deq_idx == DVZ_INPUT_DEQ_KEYBOARD && !input->keyboard.is_active)
    //     return;

    DvzEvent* pev = calloc(1, sizeof(DvzEvent));
    *pev = ev;
    pev->type = type;
    dvz_deq_enqueue(&input->deq, deq_idx, (int)type, pev);
}



void dvz_input_attach(DvzInput* input, DvzWindow* window)
{
    ASSERT(input != NULL);
    ASSERT(window != NULL);

#if HAS_GLFW
    backend_attach_mouse(&input->mouse, window);
    backend_attach_keyboard(&input->keyboard, window);
#endif
}



void dvz_input_block(DvzInput* input, bool block)
{
    ASSERT(input != NULL);
    // input->mouse.is_active = !block;
    // input->keyboard.is_active = !block;
    input->is_blocked = block;
}



void dvz_input_destroy(DvzInput* input)
{
    ASSERT(input != NULL);

    // Destroy the PRNG.
    dvz_prng_destroy(input->prng);

    // Stop the event thread.
    if (dvz_obj_is_created(&input->thread.obj))
    {
        log_trace("stopping the thread");

        // Enqueue a STOP task to stop the UL and DL threads.
        dvz_deq_enqueue(&input->deq, DVZ_EVENT_NONE, 0, NULL);

        // Join the thread.
        dvz_thread_join(&input->thread);
    }

    // Destroy the Deq.
    dvz_deq_destroy(&input->deq);
}



/*************************************************************************************************/
/*  Mouse functions                                                                              */
/*************************************************************************************************/

DvzMouse* dvz_mouse(DvzInput* input)
{
    ASSERT(input != NULL);
    return &input->mouse;
}



void dvz_mouse_reset(DvzMouse* mouse)
{
    ASSERT(mouse != NULL);
    memset(mouse, 0, sizeof(DvzMouse));
    mouse->button = DVZ_MOUSE_BUTTON_NONE;
    glm_vec2_zero(mouse->cur_pos);
    glm_vec2_zero(mouse->press_pos);
    glm_vec2_zero(mouse->last_pos);
    mouse->cur_state = DVZ_MOUSE_STATE_INACTIVE;
    mouse->press_time = DVZ_NEVER;
    mouse->click_time = DVZ_NEVER;
    mouse->last_move = DVZ_NEVER;
    // mouse->is_active = true;
}



/*************************************************************************************************/
/*  Keyboard functions                                                                           */
/*************************************************************************************************/

DvzKeyboard* dvz_keyboard(DvzInput* input)
{
    ASSERT(input != NULL);
    return &input->keyboard;
}



void dvz_keyboard_reset(DvzKeyboard* keyboard)
{
    ASSERT(keyboard != NULL);
    memset(keyboard, 0, sizeof(DvzKeyboard));
    keyboard->key_count = 0;
    keyboard->modifiers = 0;
    keyboard->press_time = DVZ_NEVER;
    // keyboard->is_active = true;
}



/*************************************************************************************************/
/*  Timer functions                                                                              */
/*************************************************************************************************/

DvzTimer* dvz_timer(DvzInput* input, int64_t max_count, uint32_t after_ms, uint32_t period_ms)
{
    ASSERT(input != NULL);

    DvzTimer* timer = (DvzTimer*)dvz_container_alloc(&input->timers);
    ASSERT(timer != NULL);

    timer->input = input;
    timer->obj.id = dvz_prng_uuid(input->prng);
    timer->max_count = max_count;
    timer->is_running = true;

    timer->after = after_ms;
    timer->period = period_ms;

    timer->start_time = dvz_clock_get(&input->clock);

    dvz_obj_created(&timer->obj);
    return timer;
}



void dvz_timer_toggle(DvzTimer* timer, bool is_running)
{
    ASSERT(timer != NULL);
    timer->is_running = is_running;
}



void dvz_timer_destroy(DvzTimer* timer)
{
    ASSERT(timer != NULL);
    dvz_obj_destroyed(&timer->obj);
}
