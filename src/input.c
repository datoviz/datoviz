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



static void
_input_proc_pre_callback(DvzDeq* deq, uint32_t deq_idx, int type, void* item, void* user_data)
{
    ASSERT(deq != NULL);
    if (item == NULL)
        return;

    DvzInput* input = (DvzInput*)user_data;
    ASSERT(input != NULL);

    // Update the mouse state after every mouse event.
    if (deq_idx == DVZ_INPUT_DEQ_MOUSE)
    {
        dvz_mouse_update(input, (DvzEventType)type, (DvzEvent*)item);
    }

    else if (deq_idx == DVZ_INPUT_DEQ_KEYBOARD)
    {
        dvz_keyboard_update(input, (DvzEventType)type, (DvzEvent*)item);
    }
}



static void
_input_proc_post_callback(DvzDeq* deq, uint32_t deq_idx, int type, void* item, void* user_data)
{
    ASSERT(deq != NULL);
    if (item == NULL)
        return;

    DvzInput* input = (DvzInput*)user_data;
    ASSERT(input != NULL);

    // Reset wheel event.
    if (input->mouse.cur_state == DVZ_MOUSE_STATE_WHEEL)
    {
        // log_debug("reset wheel state %d", canvas->frame_idx);
        input->mouse.cur_state = DVZ_MOUSE_STATE_INACTIVE;
    }
}



/*************************************************************************************************/
/*  Key util functions                                                                           */
/*************************************************************************************************/

static bool _is_key_modifier(DvzKeyCode key)
{
    return (
        key == DVZ_KEY_LEFT_SHIFT || key == DVZ_KEY_RIGHT_SHIFT || key == DVZ_KEY_LEFT_CONTROL ||
        key == DVZ_KEY_RIGHT_CONTROL || key == DVZ_KEY_LEFT_ALT || key == DVZ_KEY_RIGHT_ALT ||
        key == DVZ_KEY_LEFT_SUPER || key == DVZ_KEY_RIGHT_SUPER);
}



static int _key_modifiers(int key_code)
{
    int mods = 0;
    if (key_code == DVZ_KEY_LEFT_CONTROL || key_code == DVZ_KEY_RIGHT_CONTROL)
        mods |= DVZ_KEY_MODIFIER_CONTROL;
    if (key_code == DVZ_KEY_LEFT_SHIFT || key_code == DVZ_KEY_RIGHT_SHIFT)
        mods |= DVZ_KEY_MODIFIER_SHIFT;
    if (key_code == DVZ_KEY_LEFT_ALT || key_code == DVZ_KEY_RIGHT_ALT)
        mods |= DVZ_KEY_MODIFIER_ALT;
    if (key_code == DVZ_KEY_LEFT_SUPER || key_code == DVZ_KEY_RIGHT_SUPER)
        mods |= DVZ_KEY_MODIFIER_SUPER;
    return mods;
}



// Return the position of the key pressed if it is pressed, otherwise return -1.
static int _is_key_pressed(DvzKeyboard* keyboard, DvzKeyCode key_code)
{
    ASSERT(keyboard != NULL);
    for (uint32_t i = 0; i < keyboard->key_count; i++)
    {
        if (keyboard->keys[i] == key_code)
            return (int)i;
    }
    return -1;
}



static void _remove_key(DvzKeyboard* keyboard, DvzKeyCode key_code, uint32_t pos)
{
    ASSERT(keyboard != NULL);
    // ASSERT(pos >= 0);
    ASSERT(pos < DVZ_INPUT_MAX_KEYS);
    ASSERT(keyboard->key_count > 0);
    ASSERT(pos < keyboard->key_count);
    ASSERT(keyboard->keys[pos] == key_code);

    // When an element is removed, need to shift all keys after one position to the left.
    for (uint32_t i = pos; i < (uint32_t)MIN((int)key_code, DVZ_INPUT_MAX_KEYS - 1); i++)
    {
        keyboard->keys[i] = keyboard->keys[i + 1];
    }
    keyboard->key_count--;

    // Reset the unset positions in the array.
    // log_trace(
    //     "reset %d keys after pos %d", DVZ_INPUT_MAX_KEYS - keyboard->key_count,
    //     keyboard->key_count);
    memset(
        &keyboard->keys[keyboard->key_count], 0,
        (DVZ_INPUT_MAX_KEYS - keyboard->key_count) * sizeof(DvzKeyCode));
}



/*************************************************************************************************/
/*  Timer                                                                                        */
/*************************************************************************************************/

static DvzTimer* _timer_get(DvzInput* input, uint32_t timer_id)
{
    ASSERT(input != NULL);

    // Look for the timer with the passed timer idx, and mark it as destroyed.
    DvzContainerIterator iter = dvz_container_iterator(&input->timers);
    DvzTimer* timer = NULL;
    while (iter.item != NULL)
    {
        timer = (DvzTimer*)iter.item;
        ASSERT(timer != NULL);
        if (timer->obj.id == timer_id)
            return timer;
        dvz_container_iter(&iter);
    }
    return NULL;
}



static bool _timer_should_tick(DvzTimer* timer)
{
    ASSERT(timer != NULL);
    ASSERT(timer->input != NULL);

    // Dead or paused timers should not tick.
    if (!dvz_obj_is_created(&timer->obj) || !timer->is_running)
        return false;

    // If the numbers of ticks was exceeded, stop the timer.
    if (timer->max_count > 0 && timer->tick >= timer->max_count)
    {
        timer->is_running = false;
        return false;
    }

    // Go through all TIMER callbacks
    double cur_time = dvz_clock_get(&timer->input->clock) - timer->start_time;

    // Wait until "after" ms to start the timer.
    if (timer->after > 0 && cur_time < timer->after / 1000.0)
    {
        return false;
    }

    // When is the next expected time?
    ASSERT(timer->start_tick <= timer->tick + 1);
    double expected_time =
        ((timer->tick - timer->start_tick + 1) * timer->period - timer->after) / 1000.0;

    // If we reached the expected time, we raise the TIMER event immediately.
    if (cur_time >= expected_time)
        return true;
    else
        return false;
}



static void _timer_tick(DvzTimer* timer)
{
    ASSERT(timer != NULL);
    ASSERT(timer->input != NULL);

    DvzEvent ev = {0};
    ev.content.t.id = timer->obj.id;
    // ev.content.t.period = timer->period;
    // ev.content.t.after = timer->after;
    // ev.content.t.max_count = timer->max_count;

    double cur_time = dvz_clock_get(&timer->input->clock) - timer->start_time;
    // At what time was the last TIMER event for this callback?
    double last_time = ((timer->tick - timer->start_tick) * timer->period - timer->after) / 1000.0;

    ev.content.t.time = cur_time;
    ev.content.t.tick = timer->tick;
    // NOTE: this is the time since the last *expected* time of the previous TIMER
    // event, not the actual time.
    ev.content.t.interval = cur_time - last_time;

    // HACK: release the lock before enqueuing a TIMER event, because the lock is currently
    // acquired. Here, we are in a proc wait callback, called while waiting for the queue to be
    // non-empty. The waiting acquires the lock.
    pthread_mutex_unlock(&timer->input->deq.procs[0].lock);
    dvz_input_event(timer->input, DVZ_EVENT_TIMER_TICK, ev, false);
    pthread_mutex_lock(&timer->input->deq.procs[0].lock);

    timer->tick++;
}



static void _timer_ticks(DvzDeq* deq, void* user_data)
{
    ASSERT(deq != NULL);
    DvzInput* input = (DvzInput*)user_data;
    ASSERT(input != NULL);

    // Update the clock struct every 1 ms (proc wait callback).
    dvz_clock_tick(&input->clock);

    DvzContainerIterator iter = dvz_container_iterator(&input->timers);
    DvzTimer* timer = NULL;
    while (iter.item != NULL)
    {
        timer = (DvzTimer*)iter.item;
        ASSERT(timer != NULL);
        if (_timer_should_tick(timer))
            _timer_tick(timer);
        dvz_container_iter(&iter);
    }
}



/*************************************************************************************************/
/*  Input functions                                                                              */
/*************************************************************************************************/

static void _input_init(DvzInput* input)
{
    ASSERT(input != NULL);

    log_trace("creating the input thread");
    input->thread = dvz_thread(_input_thread, input);

    // Register a proc callback to update the mouse and keyboard state after every event.
    dvz_deq_proc_callback(
        &input->deq, 0, DVZ_DEQ_PROC_CALLBACK_PRE, _input_proc_pre_callback, input);

    // Register a proc callback to update the mouse and keyboard state after each dequeue.
    dvz_deq_proc_callback(
        &input->deq, 0, DVZ_DEQ_PROC_CALLBACK_POST, _input_proc_post_callback, input);

    // In the input thread, while waiting for input events, every millisecond, check if there
    // are active timers, and fire TIMER_TICK events if needed.
    dvz_deq_proc_wait_delay(&input->deq, 0, 1);
    dvz_deq_proc_wait_callback(&input->deq, 0, _timer_ticks, input);
}

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
        _input_init(input);
    }

    DvzEventPayload* payload = &input->callbacks[input->callback_count++];
    payload->input = input;
    payload->user_data = user_data;
    payload->callback = callback;
    dvz_deq_callback(&input->deq, _deq_from_input_type(type), (int)type, _deq_callback, payload);
}



void dvz_input_event(DvzInput* input, DvzEventType type, DvzEvent ev, bool enqueue_first)
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
    if (enqueue_first)
        dvz_deq_enqueue_first(&input->deq, deq_idx, (int)type, pev);
    else
        dvz_deq_enqueue(&input->deq, deq_idx, (int)type, pev);
}



void dvz_input_attach(DvzInput* input, DvzWindow* window)
{
    ASSERT(input != NULL);
    ASSERT(window != NULL);

#if HAS_GLFW
    backend_attach(input, window);
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



// Called after every mouse callback.
void dvz_mouse_update(DvzInput* input, DvzEventType type, DvzEvent* pev)
{
    ASSERT(input != NULL);

    DvzMouse* mouse = &input->mouse;
    ASSERT(mouse != NULL);

    // Manually-set keyboard mods, if bypassing glfw.
    int mods = input->keyboard.mods;
    mouse->mods |= mods;

    DvzEvent ev = {0};
    ev = *pev;

    // log_debug("mouse event %d", canvas->frame_idx);
    mouse->prev_state = mouse->cur_state;

    double time = dvz_clock_get(&input->clock);

    // Update the last pos.
    glm_vec2_copy(mouse->cur_pos, mouse->last_pos);

    // Reset click events as soon as the next loop iteration after they were raised.
    if (mouse->cur_state == DVZ_MOUSE_STATE_CLICK ||
        mouse->cur_state == DVZ_MOUSE_STATE_DOUBLE_CLICK)
    {
        mouse->cur_state = DVZ_MOUSE_STATE_INACTIVE;
        mouse->button = DVZ_MOUSE_BUTTON_NONE;
    }

    // Net distance in pixels since the last press event.
    vec2 shift = {0};

    switch (type)
    {

    case DVZ_EVENT_MOUSE_PRESS:

        // Press event.
        if (mouse->press_time == DVZ_NEVER)
        {
            glm_vec2_copy(mouse->cur_pos, mouse->press_pos);
            mouse->press_time = time;
            mouse->button = ev.content.b.button;
            // Keep track of the mods used for the press event.
            mouse->mods = mods | ev.mods;
        }
        mouse->shift_length = 0;
        break;

    case DVZ_EVENT_MOUSE_RELEASE:
        // Release event.

        // End drag.
        if (mouse->cur_state == DVZ_MOUSE_STATE_DRAG)
        {
            log_trace("end drag event");
            mouse->button = DVZ_MOUSE_BUTTON_NONE;
            mouse->mods = 0; // Reset the mouse key mods

            // dvz_event_mouse_drag_end(canvas, mouse->cur_pos, mouse->button, mouse->mods);
            ev.mods = mouse->mods;
            ev.content.d.button = mouse->button;
            ev.content.d.pos[0] = mouse->cur_pos[0];
            ev.content.d.pos[1] = mouse->cur_pos[1];
            dvz_input_event(input, DVZ_EVENT_MOUSE_DRAG_END, ev, true);
            mouse->cur_state = DVZ_MOUSE_STATE_INACTIVE;
        }

        // Double click event.
        else if (time - mouse->click_time < DVZ_MOUSE_DOUBLE_CLICK_MAX_DELAY)
        {
            // NOTE: when releasing, current button is NONE so we must use the previously set
            // button in mouse->button.
            log_trace("double click event on button %d", mouse->button);
            mouse->click_time = time;

            ev.mods = mouse->mods;
            ev.content.c.button = mouse->button;
            ev.content.c.pos[0] = mouse->cur_pos[0];
            ev.content.c.pos[1] = mouse->cur_pos[1];
            dvz_input_event(input, DVZ_EVENT_MOUSE_DOUBLE_CLICK, ev, true);
        }
        // Click event.
        else if (
            time - mouse->press_time < DVZ_MOUSE_CLICK_MAX_DELAY &&
            mouse->shift_length < DVZ_MOUSE_CLICK_MAX_SHIFT)
        {
            log_trace("click event on button %d", mouse->button);
            mouse->cur_state = DVZ_MOUSE_STATE_CLICK;
            mouse->click_time = time;

            ev.mods = mouse->mods;
            ev.content.c.button = mouse->button;
            ev.content.c.pos[0] = mouse->cur_pos[0];
            ev.content.c.pos[1] = mouse->cur_pos[1];
            dvz_input_event(input, DVZ_EVENT_MOUSE_CLICK, ev, true);
        }

        else
        {
            // Reset the mouse button state.
            mouse->button = DVZ_MOUSE_BUTTON_NONE;
        }

        mouse->press_time = DVZ_NEVER;
        mouse->shift_length = 0;
        break;


    case DVZ_EVENT_MOUSE_MOVE:
        glm_vec2_copy(ev.content.m.pos, mouse->cur_pos);

        // Update the distance since the last press position.
        if (mouse->button != DVZ_MOUSE_BUTTON_NONE)
        {
            glm_vec2_sub(mouse->cur_pos, mouse->press_pos, shift);
            mouse->shift_length = glm_vec2_norm(shift);
        }

        // Mouse move: start drag.
        // NOTE: do not DRAG if we are clicking, with short press time and shift length
        if (mouse->cur_state == DVZ_MOUSE_STATE_INACTIVE &&
            mouse->button != DVZ_MOUSE_BUTTON_NONE &&
            !(time - mouse->press_time < DVZ_MOUSE_CLICK_MAX_DELAY &&
              mouse->shift_length < DVZ_MOUSE_CLICK_MAX_SHIFT))
        {

            ev.mods = mouse->mods;
            ev.content.d.button = mouse->button;
            ev.content.d.pos[0] = mouse->cur_pos[0];
            ev.content.d.pos[1] = mouse->cur_pos[1];
            dvz_input_event(input, DVZ_EVENT_MOUSE_DRAG_BEGIN, ev, true);

            mouse->cur_state = DVZ_MOUSE_STATE_DRAG;
            break; // HACK: avoid enqueueing a DRAG event *after* the DRAG_BEGIN event.
        }

        // Mouse move: is dragging.
        if (mouse->cur_state == DVZ_MOUSE_STATE_DRAG)
        {
            ev.content.d.pos[0] = mouse->cur_pos[0];
            ev.content.d.pos[1] = mouse->cur_pos[1];
            ev.content.d.button = mouse->button;
            ev.mods = mouse->mods;
            dvz_input_event(input, DVZ_EVENT_MOUSE_DRAG, ev, true);
        }

        // log_trace("mouse mouse %.1fx%.1f", mouse->cur_pos[0], mouse->cur_pos[1]);
        break;


    case DVZ_EVENT_MOUSE_WHEEL:
        glm_vec2_copy(ev.content.w.pos, mouse->cur_pos);
        glm_vec2_copy(ev.content.w.dir, mouse->wheel_delta);
        mouse->cur_state = DVZ_MOUSE_STATE_WHEEL;
        mouse->mods = ev.mods;
        break;

    default:
        break;
    }
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
    keyboard->mods = 0;
    keyboard->press_time = DVZ_NEVER;
    // keyboard->is_active = true;
}



void dvz_keyboard_update(DvzInput* input, DvzEventType type, DvzEvent* pev)
{
    ASSERT(input != NULL);

    DvzKeyboard* keyboard = &input->keyboard;
    ASSERT(keyboard != NULL);

    // TODO?: if input capture, do nothing

    DvzEvent ev = {0};
    ev = *pev;

    keyboard->prev_state = keyboard->cur_state;

    double time = dvz_clock_get(&input->clock);
    DvzKeyCode key = ev.content.k.key_code;
    int is_pressed = 0;

    if (type == DVZ_EVENT_KEYBOARD_PRESS && time - keyboard->press_time > .025)
    {
        // Find out if the key is already pressed.
        is_pressed = _is_key_pressed(keyboard, key);
        // Make sure we don't reach the max number of keys pressed simultaneously.
        // Also, do not add mod keys in the list of keys pressed.
        if (is_pressed < 0 && keyboard->key_count < DVZ_INPUT_MAX_KEYS && !_is_key_modifier(key))
        {
            // Need to register the key in the keyboard state.
            keyboard->keys[keyboard->key_count++] = key;
        }

        // Register the key modifier in the keyboard state.
        if (_is_key_modifier(key))
        {
            keyboard->mods |= _key_modifiers(key);
        }

        // Here, we've ensured that the keyboard state has been updated to register the key
        // pressed, except if the maximum number of keys pressed simultaneously has been reached.
        log_trace("key pressed %d mods %d", key, ev.mods);
        keyboard->mods |= ev.mods;
        keyboard->press_time = time;
        if (keyboard->cur_state == DVZ_KEYBOARD_STATE_INACTIVE)
            keyboard->cur_state = DVZ_KEYBOARD_STATE_ACTIVE;
    }
    else if (type == DVZ_EVENT_KEYBOARD_RELEASE)
    {
        // HACK
        // keyboard->key_count = 0;

        is_pressed = _is_key_pressed(keyboard, key);
        // If the key released was pressed, remove it from the keyboard state.
        // log_debug("is pressed %d", is_pressed);
        if (is_pressed >= 0)
        {
            ASSERT(is_pressed < DVZ_INPUT_MAX_KEYS);
            _remove_key(keyboard, key, (uint32_t)is_pressed);
        }

        // Remove the key modifier in the keyboard state.
        if (_is_key_modifier(key))
        {
            keyboard->mods &= ~_key_modifiers(key);
        }

        if (keyboard->cur_state == DVZ_KEYBOARD_STATE_ACTIVE)
            keyboard->cur_state = DVZ_KEYBOARD_STATE_INACTIVE;
    }
}



/*************************************************************************************************/
/*  Timer functions                                                                              */
/*************************************************************************************************/

DvzTimer* dvz_timer(DvzInput* input, uint64_t max_count, uint32_t after_ms, uint32_t period_ms)
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
    ASSERT(timer->input != NULL);
    timer->is_running = is_running;
    if (is_running)
    {
        timer->start_time = dvz_clock_get(&timer->input->clock);
        timer->start_tick = timer->tick;
    }
}



void dvz_timer_destroy(DvzTimer* timer)
{
    ASSERT(timer != NULL);
    dvz_obj_destroyed(&timer->obj);
}
