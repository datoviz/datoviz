#ifndef VKL_CANVAS_HEADER
#define VKL_CANVAS_HEADER

#ifndef __STDC_NO_ATOMICS__
#include <stdatomic.h>
#endif

#include <stdatomic.h>

#include "../include/visky/context.h"
#include "keycode.h"
#include "vklite2.h"


/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define VKL_MAX_EVENT_CALLBACKS 256
// Maximum acceptable duration for the pending events in the event queue, in seconds
#define VKL_MAX_EVENT_DURATION .5
#define VKL_DEFAULT_BACKGROUND                                                                    \
    (VkClearColorValue)                                                                           \
    {                                                                                             \
        {                                                                                         \
            0, .03, .07, 1.0f                                                                     \
        }                                                                                         \
    }
#define VKL_DEFAULT_IMAGE_FORMAT VK_FORMAT_B8G8R8A8_UNORM
// #define VKL_DEFAULT_PRESENT_MODE VK_PRESENT_MODE_FIFO_KHR
#define VKL_DEFAULT_PRESENT_MODE      VK_PRESENT_MODE_IMMEDIATE_KHR
#define VKL_MIN_SWAPCHAIN_IMAGE_COUNT 3
#define VKL_SEMAPHORE_IMG_AVAILABLE   0
#define VKL_SEMAPHORE_RENDER_FINISHED 1
#define VKL_FENCE_RENDER_FINISHED     0
#define VKL_FENCES_FLIGHT             1
#define VKL_DEFAULT_COMMANDS_TRANSFER 0
#define VKL_DEFAULT_COMMANDS_RENDER   1
#define VKL_MAX_FRAMES_IN_FLIGHT      2



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

/**
 * Private event types.
 *
 * Private events are emitted and consumed in the main thread, typically by the canvas itself.
 * They are rarely used by end-users.
 *
 */
typedef enum
{
    VKL_PRIVATE_EVENT_INIT,      // called before the first frame
    VKL_PRIVATE_EVENT_REFILL,    // called every time the command buffers need to be recreated
    VKL_PRIVATE_EVENT_INTERACT,  // called at every frame, before event enqueue
    VKL_PRIVATE_EVENT_FRAME,     // called at every frame, after event enqueue
    VKL_PRIVATE_EVENT_TIMER,     // called every X ms in the main thread, just after FRAME
    VKL_PRIVATE_EVENT_RESIZE,    // called at every resize
    VKL_PRIVATE_EVENT_PRE_SEND,  // called before sending the commands buffers
    VKL_PRIVATE_EVENT_POST_SEND, // called after sending the commands buffers
    VKL_PRIVATE_EVENT_DESTROY,   // called before destruction
} VklPrivateEventType;



typedef enum
{
    VKL_CANVAS_SIZE_SCREEN,
    VKL_CANVAS_SIZE_FRAMEBUFFER,
} VklCanvasSizeType;



/*************************************************************************************************/
/*  Event system                                                                                 */
/*************************************************************************************************/

/**
 * Public event types.
 *
 * Public events (also just called "events") are emitted in the main thread and consumed in the
 * background thread by user callbacks.
 */
typedef enum
{
    VKL_EVENT_NONE,
    VKL_EVENT_INIT,
    VKL_EVENT_MOUSE_BUTTON,
    VKL_EVENT_MOUSE_MOVE,
    VKL_EVENT_MOUSE_WHEEL,
    VKL_EVENT_KEY,
    VKL_EVENT_FRAME,
    // VKL_EVENT_TIMER,   // TODO later
    // VKL_EVENT_ONESHOT, // TODO later
    VKL_EVENT_SCREENCAST,
} VklEventType;



typedef enum
{
    VKL_KEY_RELEASE,
    VKL_KEY_PRESS,
} VklKeyType;



typedef enum
{
    VKL_MOUSE_RELEASE,
    VKL_MOUSE_PRESS,
} VklMouseButtonType;



// NOTE: must match GLFW values!
typedef enum
{
    VKL_KEY_MODIFIER_NONE = 0x00000000,
    VKL_KEY_MODIFIER_SHIFT = 0x00000001,
    VKL_KEY_MODIFIER_CONTROL = 0x00000002,
    VKL_KEY_MODIFIER_ALT = 0x00000004,
    VKL_KEY_MODIFIER_SUPER = 0x00000008,
} VklKeyModifiers;



typedef enum
{
    VKL_MOUSE_BUTTON_NONE,
    VKL_MOUSE_BUTTON_LEFT,
    VKL_MOUSE_BUTTON_MIDDLE,
    VKL_MOUSE_BUTTON_RIGHT,
} VklMouseButton;



typedef enum
{
    VKL_MOUSE_STATE_INACTIVE,
    VKL_MOUSE_STATE_DRAG,
    VKL_MOUSE_STATE_WHEEL,
    VKL_MOUSE_STATE_CLICK,
    VKL_MOUSE_STATE_DOUBLE_CLICK,
    VKL_MOUSE_STATE_CAPTURE,
} VklMouseStateType;



typedef enum
{
    VKL_KEYBOARD_STATE_INACTIVE,
    VKL_KEYBOARD_STATE_ACTIVE,
    VKL_KEYBOARD_STATE_CAPTURE,
} VklKeyStateType;



/*************************************************************************************************/
/*  Type definitions                                                                             */
/*************************************************************************************************/

// Public events (background thread).
typedef struct VklKeyEvent VklKeyEvent;
typedef struct VklMouseButtonEvent VklMouseButtonEvent;
typedef struct VklMouseMoveEvent VklMouseMoveEvent;
typedef struct VklMouseWheelEvent VklMouseWheelEvent;
typedef struct VklScreencastEvent VklScreencastEvent;
typedef struct VklFrameEvent VklFrameEvent;
typedef struct VklEvent VklEvent;

// Private events (main thread).
typedef struct VklTimerEvent VklTimerEvent;
typedef struct VklResizeEvent VklResizeEvent;
typedef struct VklRefillEvent VklRefillEvent;
typedef struct VklSubmitEvent VklSubmitEvent;
typedef struct VklPrivateEvent VklPrivateEvent;

typedef struct VklMouseState VklMouseState;
typedef struct VklKeyState VklKeyState;

typedef void (*VklCanvasCallback)(VklCanvas*, VklPrivateEvent);
typedef void (*VklEventCallback)(VklCanvas*, VklEvent);

typedef struct VklCanvasCallbackRegister VklCanvasCallbackRegister;
typedef struct VklEventCallbackRegister VklEventCallbackRegister;



/*************************************************************************************************/
/*  Event structs                                                                                */
/*************************************************************************************************/

struct VklMouseButtonEvent
{
    VklMouseButton button;
    VklMouseButtonType type;
    int modifiers;
};



struct VklMouseMoveEvent
{
    dvec2 pos;
};



struct VklMouseWheelEvent
{
    dvec2 dir;
};



struct VklKeyEvent
{
    VklKeyType type;
    VklKeyCode key_code;
};



struct VklFrameEvent
{
    uint64_t idx;    // frame index
    double time;     // current time
    double interval; // interval since last event
};



struct VklTimerEvent
{
    uint64_t idx;    // event index
    double time;     // current time
    double interval; // interval since last event
};



struct VklScreencastEvent
{
    uint64_t idx;
    double time;
    double interval;
    uint8_t* rgb;
};



struct VklRefillEvent
{
    uint32_t img_idx;
    uint32_t cmd_count;
    VklCommands* cmds[VKL_MAX_COMMANDS];
};



struct VklResizeEvent
{
    uvec2 size_screen;
    uvec2 size_framebuffer;
};



struct VklSubmitEvent
{
    VklSubmit* submit;
};



struct VklPrivateEvent
{
    VklPrivateEventType type;
    void* user_data;
    union
    {
        VklRefillEvent rf; // for REFILL private events
        VklResizeEvent r;  // for RESIZE private events
        VklFrameEvent t;   // for FRAME private events
        VklFrameEvent f;   // for TIMER private events
        VklSubmitEvent s;  // for SUBMIT private events
    } u;
};



struct VklEvent
{
    VklEventType type;
    void* user_data;
    union
    {
        VklMouseButtonEvent b; // for MOUSE_BUTTON public events
        VklMouseMoveEvent m;   // for MOUSE_MOVE public events
        VklMouseWheelEvent w;  // for WHEEL public events
        VklKeyEvent k;         // for KEY public events
        VklFrameEvent f;       // for FRAME public event
        // VklTimerEvent t;       // for TIMER, ONESHOT public events
        VklScreencastEvent s; // for SCREENCAST public events
    } u;
};



struct VklCanvasCallbackRegister
{
    VklPrivateEventType type;
    uint64_t idx; // used by TIMER events: increases every time the TIMER event is raised
    double param;
    void* user_data;
    VklCanvasCallback callback;
};



struct VklEventCallbackRegister
{
    VklEventType type;
    // uint64_t idx; // used by TIMER events: increases every time the TIMER event is raised
    double param;
    void* user_data;
    VklEventCallback callback;
};



/*************************************************************************************************/
/*  Mouse and keyboard states                                                                    */
/*************************************************************************************************/

struct VklMouseState
{
    VklMouseButton button;
    vec2 press_pos;
    vec2 last_pos;
    vec2 cur_pos;
    vec2 wheel_delta;

    VklMouseStateType prev_state;
    VklMouseStateType cur_state;

    double press_time;
    double click_time;
};



struct VklKeyState
{
    VklKeyCode key_code;
    uint32_t modifiers;

    VklKeyStateType prev_state;
    VklKeyStateType cur_state;

    double press_time;
};



/*************************************************************************************************/
/*  Canvas struct                                                                                */
/*************************************************************************************************/

struct VklCanvas
{
    VklObject obj;
    VklApp* app;
    VklGpu* gpu;

    void* user_data;

    // This thread-safe variable is used by the background thread to
    // safely communicate a status change of the canvas
    _Atomic VklObjectStatus cur_status;
    _Atomic VklObjectStatus next_status;

    VklWindow* window;

    // Swapchain
    VklSwapchain swapchain;
    VklImages depth_image;
    VklFramebuffers framebuffers;
    VklSubmit submit;

    uint32_t cur_frame; // current frame within the images in flight
    uint64_t frame_idx;
    VklClock clock;

    uint32_t max_commands;
    VklCommands* commands;
    // when refilling command buffers, keep track of which img_idx were updated until we stop
    // calling the REFILL callbackks
    bool img_updated[VKL_MAX_SWAPCHAIN_IMAGES];

    // Synchronization events.
    uint32_t max_renderpasses;
    VklRenderpass* renderpasses;

    uint32_t max_semaphores;
    VklSemaphores* semaphores;

    uint32_t max_fences;
    VklFences* fences;

    uint32_t max_graphics;
    VklGraphics* graphics;

    // FAST transfers
    VklFifo fifo_fast; // fast transfers queue
    VklTransfer transfers_fast[VKL_MAX_TRANSFERS];
    VklTransfer* cur_transfer_fast;
    bool cur_transfer_updated[VKL_MAX_SWAPCHAIN_IMAGES];

    uint32_t canvas_callbacks_count;
    VklCanvasCallbackRegister canvas_callbacks[VKL_MAX_EVENT_CALLBACKS];

    uint32_t event_callbacks_count;
    VklEventCallbackRegister event_callbacks[VKL_MAX_EVENT_CALLBACKS];

    // Event queue.
    VklFifo event_queue;
    VklEvent events[VKL_MAX_FIFO_CAPACITY];
    VklThread event_thread;
    _Atomic VklEventType event_processing;
};



/*************************************************************************************************/
/*  Canvas                                                                                       */
/*************************************************************************************************/

// adds callbacks as a function of the backend
// GLFW ex: init_canvas_glfw(VklCanvas* canvas);
// start a background thread that:
// - dequeue event queue (wait)
// - switch the event type
// - call the relevant event callbacks
VKY_EXPORT VklCanvas* vkl_canvas(VklGpu* gpu, uint32_t width, uint32_t height);

VKY_EXPORT VklCanvas* vkl_canvas_offscreen(VklGpu* gpu, uint32_t width, uint32_t height);

VKY_EXPORT void vkl_canvas_recreate(VklCanvas* canvas);



/*************************************************************************************************/
/*  Canvas misc                                                                                  */
/*************************************************************************************************/

VKY_EXPORT void vkl_canvas_clear_color(VklCanvas* canvas, VkClearColorValue color);

VKY_EXPORT void vkl_canvas_size(VklCanvas* canvas, VklCanvasSizeType type, uvec2 size);

VKY_EXPORT void vkl_canvas_close_on_esc(VklCanvas* canvas, bool value);



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

/**
 * Register a callback for private events.
 */
VKY_EXPORT void vkl_canvas_callback(
    VklCanvas* canvas, VklPrivateEventType type, double param, //
    VklCanvasCallback callback, void* user_data);



/**
 * Register a callback for public events.
 *
 * These user callbacks run in the background thread and can access the VklMouse and VklKeyboard
 * structures with the current state of the mouse and keyboard.
 *
 *
 * @par TIMER public events:
 *
 * Callbacks registered with TIMER public events need to specify as `param` the delay, in seconds,
 * between successive TIMER events.
 *
 * TIMER public events are raised by a special thread and enqueued in the Canvas event queue.
 * They are consumed in the background thread (which is a different thread than the TIMER thread).
 *
 */
VKY_EXPORT void vkl_event_callback(
    VklCanvas* canvas, VklEventType type, double param, //
    VklEventCallback callback, void* user_data);



/*************************************************************************************************/
/*  State changes                                                                                */
/*************************************************************************************************/

VKY_EXPORT void vkl_canvas_set_status(VklCanvas* canvas, VklObjectStatus status);

VKY_EXPORT void vkl_canvas_to_refill(VklCanvas* canvas, bool value);

VKY_EXPORT void vkl_canvas_to_close(VklCanvas* canvas, bool value);



/*************************************************************************************************/
/*  Fast transfers                                                                               */
/*************************************************************************************************/

VKY_EXPORT void vkl_upload_buffers_fast(
    VklCanvas* canvas, VklBufferRegions* regions, bool update_all_regions, //
    VkDeviceSize offset, VkDeviceSize size, void* data);



/*************************************************************************************************/
/*  Screencast                                                                                   */
/*************************************************************************************************/

/**
 * Prepare the canvas for a screencast.
 *
 * A **screencast** is a live record of one or several frames of the canvas during the interactive
 * execution of the app. Creating a screencast is required for:
 * - screenshots,
 * - video records (requires ffmpeg)
 *
 * @param canvas
 * @param interval If non-zero, the Canvas will raise periodic SCREENCAST private events every
 *      `interval` seconds. The private event payload will contain a pointer to the grabbed
 *      framebuffer image.
 * @param rgb If NULL, the Canvas will create a CPU buffer with the appropriate size. Otherwise,
 *      the images will be copied to the provided buffer. The caller must ensure the buffer is
 *      allocated with enough memory to store the image. Providing a pointer disables resize
 *      support (the swapchain and GPU images will not be recreated upon resize).
 *
 * This command creates a host-coherent GPU image with the same size as the current framebuffer
 * size.
 *
 */

/*
 Implementation details:

- need a VklCommands*, a VklSemaphores*, and a VklFences*

- register a TIMER private event callback with the following:
take special screencast cmd buf
    constructed once, when creating the screencast
    transition to SRC layout
    copy image to screencast image
    transition to previous image layout
new submit object
wait for "image_ready" semaphore
signal screencast_finished semaphore
send screencast cmd buf to transfer queue
signal screencast fence when submitting
present must wait for that semaphore instead of the image_ready semaphore

- register a FRAME private event callback with the following:
check screencast fence state
if screencast fence is signaled:
if vkl_event_pending(SCREENCAST) is >0, discard this frame because the callback is not ready to
process the current frame
map/unmap the screencast image and copy to the user-provided CPU buffer
enqueue a special SCREENCAST public event with a pointer to the CPU buffer
user callbacks registered for SCREENCAST public events and running in the background thread have
access to the framebuffer RGB image

*/

VKY_EXPORT void vkl_screencast(VklCanvas* canvas, double interval, uint8_t* rgb);



/**
 * Destroy the screencast.
 */
VKY_EXPORT void vkl_screencast_destroy(VklCanvas* canvas);



/**
 * Make a screenshot.
 *
 * This function creates a screencast if there isn't one already. It is implemented with hard
 * synchronization commands so this command should *not* be used for creating many successive
 * screenshots. For that, one should register a SCREENCAST private event callback.
 *
 * @param canvas
 * @return A pointer to the 24-bit RGB framebuffer.
 *
 */
VKY_EXPORT uint8_t* vkl_screenshot(VklCanvas* canvas);



/**
 * Make a screenshot and save it to a PNG or PPM file.
 *
 * @param canvas
 * @param filename Path to the screenshot image.
 *
 */
VKY_EXPORT void vkl_screenshot_file(VklCanvas* canvas, const char* filename);



/*************************************************************************************************/
/*  Event system                                                                                 */
/*************************************************************************************************/

VKY_EXPORT void vkl_event_enqueue(VklCanvas* canvas, VklEvent event);

VKY_EXPORT void vkl_event_mouse_button(
    VklCanvas* canvas, VklMouseButtonType type, VklMouseButton button, int modifiers);

VKY_EXPORT void vkl_event_mouse_move(VklCanvas* canvas, dvec2 pos);

VKY_EXPORT void vkl_event_mouse_wheel(VklCanvas* canvas, dvec2 dir);

VKY_EXPORT void vkl_event_key(VklCanvas* canvas, VklKeyType type, VklKeyCode key_code);

VKY_EXPORT void vkl_event_frame(VklCanvas* canvas, uint64_t idx, double time, double interval);

VKY_EXPORT void vkl_event_timer(VklCanvas* canvas, uint64_t idx, double time, double interval);

VKY_EXPORT VklEvent* vkl_event_dequeue(VklCanvas* canvas, bool wait);

// Return the number of events of the given type that are still being processed or pending in the
// queue.
VKY_EXPORT int vkl_event_pending(VklCanvas* canvas, VklEventType type);

VKY_EXPORT void vkl_event_stop(VklCanvas* canvas);



/*************************************************************************************************/
/*  Event loop                                                                                   */
/*************************************************************************************************/

VKY_EXPORT void vkl_canvas_frame(VklCanvas* canvas);

VKY_EXPORT void vkl_canvas_frame_submit(VklCanvas* canvas);

VKY_EXPORT void vkl_app_run(VklApp* app, uint64_t frame_count);



#endif
