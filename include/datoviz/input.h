/*************************************************************************************************/
/*  Input                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_INPUT
#define DVZ_HEADER_INPUT



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_prng.h"
#include "common.h"
#include "fifo.h"
#include "keycode.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_INPUT_DEQ_MOUSE    0
#define DVZ_INPUT_DEQ_KEYBOARD 1
#define DVZ_INPUT_DEQ_TIMER    2

#define DVZ_INPUT_MAX_CALLBACKS 64
#define DVZ_INPUT_MAX_KEYS      8
#define DVZ_INPUT_MAX_TIMERS    16



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Input type.
typedef enum
{
    DVZ_EVENT_NONE,
    DVZ_EVENT_INIT,

    DVZ_EVENT_MOUSE_MOVE,
    DVZ_EVENT_MOUSE_PRESS,
    DVZ_EVENT_MOUSE_RELEASE,
    DVZ_EVENT_MOUSE_CLICK,
    DVZ_EVENT_MOUSE_DOUBLE_CLICK,
    DVZ_EVENT_MOUSE_WHEEL,
    DVZ_EVENT_MOUSE_DRAG_BEGIN,
    DVZ_EVENT_MOUSE_DRAG,
    DVZ_EVENT_MOUSE_DRAG_END,

    DVZ_EVENT_KEYBOARD_PRESS,
    DVZ_EVENT_KEYBOARD_RELEASE,
    DVZ_EVENT_KEYBOARD_STROKE, // unused for now

    DVZ_EVENT_TIMER_TICK,
} DvzEventType;



// Key mods
// NOTE: must match GLFW values! no mapping is done for now
typedef enum
{
    DVZ_KEY_MODIFIER_NONE = 0x00000000,
    DVZ_KEY_MODIFIER_SHIFT = 0x00000001,
    DVZ_KEY_MODIFIER_CONTROL = 0x00000002,
    DVZ_KEY_MODIFIER_ALT = 0x00000004,
    DVZ_KEY_MODIFIER_SUPER = 0x00000008,
} DvzKeyModifiers;



// Mouse button
typedef enum
{
    DVZ_MOUSE_BUTTON_NONE,
    DVZ_MOUSE_BUTTON_LEFT,
    DVZ_MOUSE_BUTTON_MIDDLE,
    DVZ_MOUSE_BUTTON_RIGHT,
} DvzMouseButton;



// Mouse state type
typedef enum
{
    DVZ_MOUSE_STATE_INACTIVE,
    DVZ_MOUSE_STATE_DRAG,
    DVZ_MOUSE_STATE_WHEEL,
    DVZ_MOUSE_STATE_CLICK,
    DVZ_MOUSE_STATE_DOUBLE_CLICK,
    DVZ_MOUSE_STATE_CAPTURE,
} DvzMouseStateType;



// Key state type
typedef enum
{
    DVZ_KEYBOARD_STATE_INACTIVE,
    DVZ_KEYBOARD_STATE_ACTIVE,
    DVZ_KEYBOARD_STATE_CAPTURE,
} DvzKeyboardStateType;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzMouse DvzMouse;
typedef struct DvzKeyboard DvzKeyboard;
typedef struct DvzTimer DvzTimer;
typedef struct DvzInput DvzInput;
typedef struct DvzEvent DvzEvent;
typedef union DvzEventContent DvzEventContent;

// Forward declarations.
typedef struct DvzWindow DvzWindow;

// typedef struct DvzEventKey DvzEventKey;
// typedef struct DvzEventMouse DvzEventMouse;
// typedef struct DvzEventTimer DvzEventTimer;

typedef struct DvzEventPayload DvzEventPayload;
typedef void (*DvzEventCallback)(DvzInput*, DvzEvent, void*);



/*************************************************************************************************/
/*  Event structs                                                                                */
/*************************************************************************************************/

struct DvzEventPayload
{
    DvzInput* input;
    DvzEventCallback callback;
    void* user_data;
};



union DvzEventContent
{
    // Mouse move.
    struct
    {
        vec2 pos;
    } m;

    // Mouse button.
    struct
    {
        DvzMouseButton button;
    } b;

    // Mouse wheel.
    struct
    {
        vec2 pos, dir;
    } w;

    // Mouse drag.
    struct
    {
        DvzMouseButton button;
        vec2 pos;
    } d;

    // Mouse click.
    struct
    {
        DvzMouseButton button;
        vec2 pos;
        bool double_click;
    } c;

    // Key.
    struct
    {
        DvzKeyCode key_code;
    } k;

    // Timer.
    struct
    {
        uint64_t id;     // timer UUID
        uint64_t tick;   // tick idx
        double time;     // time of emission
        double interval; // interval since last event emission
    } t;
};



struct DvzEvent
{
    DvzEventType type;
    DvzEventContent content;
    int mods;
    void* event_data;
};



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzMouse
{
    DvzMouseButton button;
    vec2 press_pos;
    vec2 last_pos;
    vec2 cur_pos;
    vec2 wheel_delta;
    float shift_length;
    int mods;

    DvzMouseStateType prev_state;
    DvzMouseStateType cur_state;

    double press_time;
    double click_time;
    double last_move;
    // bool is_active;
};



struct DvzKeyboard
{
    uint32_t key_count;                  // number of keys currently pressed
    DvzKeyCode keys[DVZ_INPUT_MAX_KEYS]; // which keys are currently pressed
    int mods;

    DvzKeyboardStateType prev_state;
    DvzKeyboardStateType cur_state;

    double press_time;
    // bool is_active;
};



struct DvzTimer
{
    DvzObject obj;
    DvzInput* input;

    bool is_running; // whether the timer is running or paused

    // uint32_t timer_id; // unique ID of this timer among all timers registered in a given input
    uint64_t tick;      // current tick number
    uint64_t max_count; // specified maximum number of ticks for this timer
    uint32_t after;     // number of milliseconds before the first tick
    uint32_t period;    // expected number of milliseconds between ticks

    double start_time;   // the time of the last checkpoint
    uint64_t start_tick; // tick number at the last checkpoint

    // DvzEventCallback callback;
};



struct DvzInput
{
    DvzObject obj;

    // Event queues for mouse, keyboard, and timer.
    DvzDeq deq;

    // Mouse and keyboard.
    DvzMouse mouse;
    DvzKeyboard keyboard;

    // Timers.
    // uint32_t timer_count;
    // DvzTimer timers[DVZ_INPUT_MAX_TIMERS];
    DvzContainer timers;

    // Callbacks.
    uint32_t callback_count;
    DvzEventPayload callbacks[DVZ_INPUT_MAX_CALLBACKS];

    // Thread and clock.
    DvzThread thread; // background thread processing the input events
    DvzClock clock;
    DvzPrng* prng;
    bool is_blocked;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Input functions                                                                              */
/*************************************************************************************************/

/**
 * Create an input.
 *
 * @returns an input struct
 */
DVZ_EXPORT DvzInput dvz_input(void);



/**
 * Register an input callback.
 *
 * @param input the input
 * @param type the event type
 * @param callback the callback function
 * @param user_data pointer to custom user data
 */
DVZ_EXPORT void
dvz_input_callback(DvzInput* input, DvzEventType type, DvzEventCallback callback, void* user_data);



/**
 * Raise an input event.
 *
 * @param input the input
 * @param type the event type
 * @param ev the event object
 * @param enqueue_first whether to enqueue the event at the top or bottom of the queue
 */
DVZ_EXPORT void
dvz_input_event(DvzInput* input, DvzEventType type, DvzEvent ev, bool enqueue_first);



/**
 * Attach an input to a window.
 *
 * @param input the input
 * @param window the window
 */
DVZ_EXPORT void dvz_input_attach(DvzInput* input, DvzWindow* window);



/**
 * Block or unblock an input.
 *
 * @param input the input
 * @param block whether to block the input or not
 */
DVZ_EXPORT void dvz_input_block(DvzInput* input, bool block);



/**
 * Destroy an input.
 *
 * @param input the input
 */
DVZ_EXPORT void dvz_input_destroy(DvzInput* input);



/*************************************************************************************************/
/*  Mouse functions                                                                              */
/*************************************************************************************************/

/**
 * Return a mouse instance.
 *
 * @param input the input
 * @returns the mouse
 */
DVZ_EXPORT DvzMouse* dvz_mouse(DvzInput* input);



/**
 * Reset a mouse.
 *
 * @param mouse the mouse
 */
DVZ_EXPORT void dvz_mouse_reset(DvzMouse* mouse);



/**
 * Update the mouse state after every mouse event.
 *
 * @param mouse the input
 * @param type the event type
 * @param ev the mouse event
 */
DVZ_EXPORT void dvz_mouse_update(DvzInput* input, DvzEventType type, DvzEvent* ev);



/*************************************************************************************************/
/*  Keyboard functions                                                                           */
/*************************************************************************************************/

/**
 * Return a keyboard instance.
 *
 * @param input the input
 * @returns the keyboard
 */
DVZ_EXPORT DvzKeyboard* dvz_keyboard(DvzInput* input);



/**
 * Reset a keyboard.
 *
 * @param keyboard the keyboard
 */
DVZ_EXPORT void dvz_keyboard_reset(DvzKeyboard* keyboard);



/**
 * Update the keyboard state after every mouse event.
 *
 * @param keyboard the input
 * @param type the event type
 * @param ev the keyboard event
 */
DVZ_EXPORT void dvz_keyboard_update(DvzInput* input, DvzEventType type, DvzEvent* ev);



/*************************************************************************************************/
/*  Timer functions                                                                              */
/*************************************************************************************************/

/**
 * Create a timer.
 *
 * @param input the input
 * @returns the timer
 */
DVZ_EXPORT DvzTimer*
dvz_timer(DvzInput* input, uint64_t max_count, uint32_t after_ms, uint32_t period_ms);



/**
 * Pause or continue a timer.
 *
 * @param timer the timer
 * @param bool whether the timer should be running
 */
DVZ_EXPORT void dvz_timer_toggle(DvzTimer* timer, bool is_running);



/**
 * Destroy a timer.
 *
 * @param timer the timer
 */
DVZ_EXPORT void dvz_timer_destroy(DvzTimer* timer);



EXTERN_C_OFF

#endif
