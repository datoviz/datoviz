/*************************************************************************************************/
/*  Batteries-included Vulkan-aware bare window with swapchain and event system                  */
/*************************************************************************************************/

#ifndef DVZ_CANVAS_HEADER
#define DVZ_CANVAS_HEADER

#include "context.h"
#include "fifo.h"
#include "keycode.h"
#include "transfers.h"
#include "vklite.h"

#ifdef __cplusplus
extern "C" {
#endif



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_MAX_EVENT_CALLBACKS 32
// Maximum acceptable duration for the pending events in the event queue, in seconds
#define DVZ_MAX_EVENT_DURATION .5
#define DVZ_DEFAULT_BACKGROUND                                                                    \
    (VkClearColorValue)                                                                           \
    {                                                                                             \
        {                                                                                         \
            0, .03, .07, 1.0f                                                                     \
        }                                                                                         \
    }
#define DVZ_DEFAULT_IMAGE_FORMAT      VK_FORMAT_B8G8R8A8_UNORM
#define DVZ_DEFAULT_DPI_SCALING       1.0f
#define DVZ_MIN_SWAPCHAIN_IMAGE_COUNT 3
#define DVZ_SEMAPHORE_IMG_AVAILABLE   0
#define DVZ_SEMAPHORE_RENDER_FINISHED 1
#define DVZ_FENCE_RENDER_FINISHED     0
#define DVZ_FENCES_FLIGHT             1
#define DVZ_DEFAULT_COMMANDS_TRANSFER 0
#define DVZ_DEFAULT_COMMANDS_RENDER   1
#define DVZ_MAX_FRAMES_IN_FLIGHT      2



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Canvas creation flags.
typedef enum
{
    DVZ_CANVAS_FLAGS_NONE = 0x0000,
    DVZ_CANVAS_FLAGS_IMGUI = 0x0001,
    DVZ_CANVAS_FLAGS_FPS = 0x0003, // NOTE: 1 bit for ImGUI, 1 bit for FPS
} DvzCanvasFlags;



// Canvas size type
typedef enum
{
    DVZ_CANVAS_SIZE_SCREEN,
    DVZ_CANVAS_SIZE_FRAMEBUFFER,
} DvzCanvasSizeType;



// Viewport type.
// NOTE: must correspond to values in common.glsl
typedef enum
{
    DVZ_VIEWPORT_FULL,
    DVZ_VIEWPORT_INNER,
    DVZ_VIEWPORT_OUTER,
    DVZ_VIEWPORT_OUTER_BOTTOM,
    DVZ_VIEWPORT_OUTER_LEFT,
} DvzViewportClip;



// Transform axis
// NOTE: must correspond to values in common.glsl
typedef enum
{
    DVZ_INTERACT_FIXED_AXIS_DEFAULT = 0x0000,
    DVZ_INTERACT_FIXED_AXIS_X = 0x1000,
    DVZ_INTERACT_FIXED_AXIS_Y = 0x2000,
    DVZ_INTERACT_FIXED_AXIS_Z = 0x4000,
    DVZ_INTERACT_FIXED_AXIS_XY = 0x3000,
    DVZ_INTERACT_FIXED_AXIS_XZ = 0x5000,
    DVZ_INTERACT_FIXED_AXIS_YZ = 0x6000,
    DVZ_INTERACT_FIXED_AXIS_ALL = 0x7000,
    DVZ_INTERACT_FIXED_AXIS_NONE = 0x8000,
} DvzInteractAxis;



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



// Transfer status.
typedef enum
{
    DVZ_TRANSFER_STATUS_NONE,
    DVZ_TRANSFER_STATUS_PROCESSING,
    DVZ_TRANSFER_STATUS_DONE,
} DvzTransferStatus;



// Canvas refill status.
typedef enum
{
    DVZ_REFILL_NONE,
    DVZ_REFILL_REQUESTED,
    DVZ_REFILL_PROCESSING,
} DvzRefillStatus;



// Screencast status.
typedef enum
{
    DVZ_SCREENCAST_NONE,
    DVZ_SCREENCAST_IDLE,
    DVZ_SCREENCAST_AWAIT_COPY,
    DVZ_SCREENCAST_AWAIT_TRANSFER,
} DvzScreencastStatus;



/*************************************************************************************************/
/*  Event system                                                                                 */
/*************************************************************************************************/

/**
 * Event types.
 *
 * Events are emitted and consumed either in the main thread or in the background thread.
 *
 */
// Event types
typedef enum
{
    DVZ_EVENT_NONE,               //
    DVZ_EVENT_INIT,               // called before the first frame
    DVZ_EVENT_REFILL,             // called every time the command buffers need to be recreated
    DVZ_EVENT_INTERACT,           // called at every frame, before event enqueue
    DVZ_EVENT_FRAME,              // called at every frame, after event enqueue
    DVZ_EVENT_IMGUI,              // called at every frame, after event enqueue
    DVZ_EVENT_SCREENCAST,         // called when a screenshot has been downloaded
    DVZ_EVENT_TIMER,              // called every X ms in the main thread, just after FRAME
    DVZ_EVENT_MOUSE_BUTTON,       // called when a mouse button is pressed or released
    DVZ_EVENT_MOUSE_MOVE,         // called when the mouse moves
    DVZ_EVENT_MOUSE_WHEEL,        // called when the mouse wheel is used
    DVZ_EVENT_MOUSE_DRAG_BEGIN,   // called when a drag event starts
    DVZ_EVENT_MOUSE_DRAG_END,     // called when a drag event stops
    DVZ_EVENT_MOUSE_CLICK,        // called after a click (called once during a double click)
    DVZ_EVENT_MOUSE_DOUBLE_CLICK, // called after a double click
    DVZ_EVENT_KEY,                // called after a keyboard key pressed or released
    DVZ_EVENT_RESIZE,             // called at every resize
    DVZ_EVENT_PRE_SEND,           // called before sending the commands buffers
    DVZ_EVENT_POST_SEND,          // called after sending the commands buffers
    DVZ_EVENT_DESTROY,            // called before destruction
} DvzEventType;



// Event mode (sync/async)
typedef enum
{
    DVZ_EVENT_MODE_SYNC,
    DVZ_EVENT_MODE_ASYNC,
} DvzEventMode;



// Key type
typedef enum
{
    DVZ_KEY_RELEASE,
    DVZ_KEY_PRESS,
} DvzKeyType;



// Mouse button type
typedef enum
{
    DVZ_MOUSE_RELEASE,
    DVZ_MOUSE_PRESS,
} DvzMouseButtonType;



// Key modifiers
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



/*************************************************************************************************/
/*  Type definitions                                                                             */
/*************************************************************************************************/

typedef struct DvzScene DvzScene;
typedef struct DvzMouse DvzMouse;
typedef struct DvzKeyboard DvzKeyboard;
typedef struct DvzMouseLocal DvzMouseLocal;

// Events structures.
typedef struct DvzEvent DvzEvent;
typedef struct DvzFrameEvent DvzFrameEvent;
typedef struct DvzKeyEvent DvzKeyEvent;
typedef struct DvzMouseButtonEvent DvzMouseButtonEvent;
typedef struct DvzMouseClickEvent DvzMouseClickEvent;
typedef struct DvzMouseDragEvent DvzMouseDragEvent;
typedef struct DvzMouseMoveEvent DvzMouseMoveEvent;
typedef struct DvzMouseWheelEvent DvzMouseWheelEvent;
typedef struct DvzRefillEvent DvzRefillEvent;
typedef struct DvzResizeEvent DvzResizeEvent;
typedef struct DvzScreencastEvent DvzScreencastEvent;
typedef struct DvzSubmitEvent DvzSubmitEvent;
typedef struct DvzTimerEvent DvzTimerEvent;
typedef struct DvzViewport DvzViewport;
typedef union DvzEventUnion DvzEventUnion;

typedef void (*DvzEventCallback)(DvzCanvas*, DvzEvent);
typedef struct DvzEventCallbackRegister DvzEventCallbackRegister;

typedef struct DvzScreencast DvzScreencast;
typedef struct DvzPendingRefill DvzPendingRefill;



/*************************************************************************************************/
/*  Mouse and keyboard structs                                                                   */
/*************************************************************************************************/

struct DvzMouse
{
    DvzMouseButton button;
    vec2 press_pos;
    vec2 last_pos;
    vec2 cur_pos;
    vec2 wheel_delta;
    float shift_length;

    DvzMouseStateType prev_state;
    DvzMouseStateType cur_state;

    double press_time;
    double click_time;
};



// In normalize coordinates [-1, +1]
struct DvzMouseLocal
{
    vec2 press_pos;
    vec2 last_pos;
    vec2 cur_pos;
    // vec2 delta; // delta between the last and current pos
    // vec2 press_delta; // delta between t
};



struct DvzKeyboard
{
    DvzKeyCode key_code;
    int modifiers;

    DvzKeyboardStateType prev_state;
    DvzKeyboardStateType cur_state;

    double press_time;
};



/*************************************************************************************************/
/*  Viewport struct                                                                              */
/*************************************************************************************************/

// NOTE: must correspond to the shader structure in common.glsl
struct DvzViewport
{
    VkViewport viewport; // Vulkan viewport
    vec4 margins;

    // Position and size of the viewport in screen coordinates.
    uvec2 offset_screen;
    uvec2 size_screen;

    // Position and size of the viewport in framebuffer coordinates.
    uvec2 offset_framebuffer;
    uvec2 size_framebuffer;

    // Options
    // Viewport clipping.
    DvzViewportClip clip; // used by the GPU for viewport clipping

    // Used to discard transform on one axis
    int32_t interact_axis;

    float dpi_scaling;

    // TODO: aspect ratio
};



/*************************************************************************************************/
/*  Event structs                                                                                */
/*************************************************************************************************/

struct DvzMouseButtonEvent
{
    DvzMouseButton button;
    DvzMouseButtonType type;
    int modifiers;
};



struct DvzMouseMoveEvent
{
    vec2 pos;
};



struct DvzMouseWheelEvent
{
    vec2 dir;
};



struct DvzMouseDragEvent
{
    vec2 pos;
    DvzMouseButton button;
};



struct DvzMouseClickEvent
{
    vec2 pos;
    DvzMouseButton button;
    bool double_click;
};



struct DvzKeyEvent
{
    DvzKeyType type;
    DvzKeyCode key_code;
    int modifiers;
};



struct DvzFrameEvent
{
    uint64_t idx;    // frame index
    double time;     // current time
    double interval; // interval since last event
};



struct DvzTimerEvent
{
    uint64_t idx;    // event index
    double time;     // current time
    double interval; // interval since last event
};



struct DvzScreencastEvent
{
    uint64_t idx;
    double time;
    double interval;
    uint32_t width;
    uint32_t height;
    uint8_t* rgba;
};



struct DvzRefillEvent
{
    uint32_t img_idx;
    uint32_t cmd_count;
    DvzCommands* cmds[32];
    DvzViewport viewport;
    VkClearColorValue clear_color;
};



struct DvzResizeEvent
{
    uvec2 size_screen;
    uvec2 size_framebuffer;
};



struct DvzSubmitEvent
{
    DvzSubmit* submit;
};



union DvzEventUnion
{
    DvzFrameEvent f;       // for FRAME events
    DvzFrameEvent t;       // for TIMER events
    DvzKeyEvent k;         // for KEY events
    DvzMouseButtonEvent b; // for MOUSE_BUTTON events
    DvzMouseClickEvent c;  // for DRAG events
    DvzMouseDragEvent d;   // for DRAG events
    DvzMouseMoveEvent m;   // for MOUSE_MOVE events
    DvzMouseWheelEvent w;  // for WHEEL events
    DvzRefillEvent rf;     // for REFILL events
    DvzResizeEvent r;      // for RESIZE events
    DvzScreencastEvent sc; // for SCREENCAST events
    DvzSubmitEvent s;      // for SUBMIT events
};



struct DvzEvent
{
    DvzEventType type;
    void* user_data;
    DvzEventUnion u;
};



struct DvzEventCallbackRegister
{
    DvzEventType type;
    uint64_t idx; // used by TIMER events: increases every time the TIMER event is raised
    double param;
    DvzEventMode mode;
    DvzEventCallback callback;
    void* user_data;
};



/*************************************************************************************************/
/*  Misc structs                                                                                 */
/*************************************************************************************************/

struct DvzScreencast
{
    DvzObject obj;

    DvzCanvas* canvas;
    DvzCommands cmds;
    DvzSemaphores semaphore;
    DvzFences fence;
    DvzImages staging;
    DvzSubmit submit;
    uint64_t frame_idx;
    DvzClock clock;
    DvzScreencastStatus status;
};



struct DvzPendingRefill
{
    bool completed[DVZ_MAX_SWAPCHAIN_IMAGES];
    atomic(DvzRefillStatus, status);
};



/*************************************************************************************************/
/*  Canvas struct                                                                                */
/*************************************************************************************************/

struct DvzCanvas
{
    DvzObject obj;
    DvzApp* app;
    DvzGpu* gpu;

    bool offscreen;
    bool overlay;
    bool resized;
    int flags;
    void* user_data;

    // This thread-safe variable is used by the background thread to
    // safely communicate a status change of the canvas
    atomic(DvzObjectStatus, cur_status);
    atomic(bool, to_close);

    DvzWindow* window;

    // Swapchain
    DvzSwapchain swapchain;
    DvzImages depth_image;
    DvzFramebuffers framebuffers;
    DvzFramebuffers framebuffers_overlay; // used by the overlay renderpass
    DvzSubmit submit;

    uint32_t cur_frame; // current frame within the images in flight
    uint64_t frame_idx;
    DvzClock clock;
    float fps;

    // Renderpasses.
    DvzRenderpass renderpass;         // default renderpass
    DvzRenderpass renderpass_overlay; // GUI overlay renderpass

    // Synchronization events.
    DvzSemaphores sem_img_available;
    DvzSemaphores sem_render_finished;
    DvzSemaphores* present_semaphores;
    DvzFences fences_render_finished;
    DvzFences fences_flight;

    // Default command buffers.
    DvzCommands cmds_transfer;
    DvzCommands cmds_render;

    // Other command buffers.
    DvzContainer commands;

    // Graphics pipelines.
    DvzContainer graphics;

    // Data transfers.
    DvzFifo transfers;

    // Event callbacks, running in the background thread, may be slow, for end-users.
    uint32_t callbacks_count;
    DvzEventCallbackRegister callbacks[DVZ_MAX_EVENT_CALLBACKS];

    // Event queue.
    DvzFifo event_queue;
    DvzEvent events[DVZ_MAX_FIFO_CAPACITY];
    DvzThread event_thread;
    bool enable_lock;
    atomic(DvzEventType, event_processing);
    DvzMouse mouse;
    DvzKeyboard keyboard;

    DvzScreencast* screencast;
    DvzPendingRefill refills;

    DvzViewport viewport;
    DvzScene* scene;
};



/*************************************************************************************************/
/*  Canvas                                                                                       */
/*************************************************************************************************/

/**
 * Create a canvas.
 *
 * @param gpu the GPU to use for swapchain presentation
 * @param width the initial window width, in pixels
 * @param height the initial window height, in pixels
 * @param flags the creation flags for the canvas
 */
DVZ_EXPORT DvzCanvas* dvz_canvas(DvzGpu* gpu, uint32_t width, uint32_t height, int flags);

/**
 * Create an offscreen canvas.
 *
 * @param gpu the GPU to use for swapchain presentation
 * @param width the canvas width, in pixels
 * @param height the canvas height, in pixels
 * @param flags the creation flags for the canvas
 */
DVZ_EXPORT DvzCanvas*
dvz_canvas_offscreen(DvzGpu* gpu, uint32_t width, uint32_t height, int flags);

/**
 * Recreate the canvas GPU resources and swapchain.
 *
 * @param canvas the canvas to recreate
 */
DVZ_EXPORT void dvz_canvas_recreate(DvzCanvas* canvas);

/**
 * Create a set of Vulkan command buffers on a given GPU queue.
 *
 * @param canvas the canvas
 * @param queue_idx the index of the GPU queue within the GPU context
 * @param count number of command buffers to create
 * @returns the set of created command buffers
 */
DVZ_EXPORT DvzCommands* dvz_canvas_commands(DvzCanvas* canvas, uint32_t queue_idx, uint32_t count);



/*************************************************************************************************/
/*  Canvas misc                                                                                  */
/*************************************************************************************************/

/**
 * Change the background color of a canvas.
 *
 * !!! note
 *     A command buffer refill will be triggered so as to record them again with the updated clear
 *     color value.
 *
 * @param canvas the canvas
 * @param red the red component, between 0 and 1
 * @param green the green component, between 0 and 1
 * @param blue the blue component, between 0 and 1
 */
DVZ_EXPORT void dvz_canvas_clear_color(DvzCanvas* canvas, float red, float green, float blue);

/**
 * Get the canvas size.
 *
 * @param canvas the canvas
 * @param type the unit of the requested screen size
 * @param[out] size the size vector filled by this function
 */
DVZ_EXPORT void dvz_canvas_size(DvzCanvas* canvas, DvzCanvasSizeType type, uvec2 size);

/**
 * Whether the canvas should close when Escape is pressed.
 *
 * @param canvas the canvas
 * @param value the boolean value
 */
DVZ_EXPORT void dvz_canvas_close_on_esc(DvzCanvas* canvas, bool value);

// screen coordinates
static inline bool _pos_in_viewport(DvzViewport viewport, vec2 screen_pos)
{
    ASSERT(viewport.size_screen[0] > 0);
    return (
        viewport.offset_screen[0] <= screen_pos[0] &&                           //
        viewport.offset_screen[1] <= screen_pos[1] &&                           //
        screen_pos[0] <= viewport.offset_screen[0] + viewport.size_screen[0] && //
        screen_pos[1] <= viewport.offset_screen[1] + viewport.size_screen[1]    //
    );
}

/**
 * Get the viewport corresponding to the full canvas.
 *
 * @param canvas the canvas
 * @returns the viewport
 */
DVZ_EXPORT DvzViewport dvz_viewport_full(DvzCanvas* canvas);



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

/**
 * Register a callback for canvas events.
 *
 * These user callbacks run either in the main thread (*sync* callbacks) or in the background
 * thread * (*async* callbacks). Callbacks can access the `DvzMouse` and `DvzKeyboard` structures
 * with the current state of the mouse and keyboard.
 *
 * Callback function signature: `void(DvzCanvas*, DvzEvent)`
 *
 * The event object has a field with the user-specified pointer `user_data`.
 *
 * @param canvas the canvas
 * @param type the event type
 * @param param time interval for TIMER events, in seconds
 * @param mode whether the callback is sync or async
 * @param callback the callback function
 * @param user_data a pointer to arbitrary user data
 *
 */
DVZ_EXPORT void dvz_event_callback(
    DvzCanvas* canvas, DvzEventType type, double param, DvzEventMode mode, //
    DvzEventCallback callback, void* user_data);



/*************************************************************************************************/
/*  State changes                                                                                */
/*************************************************************************************************/

/**
 * Trigger a canvas refill at the next frame.
 *
 * @param canvas the canvas
 */
DVZ_EXPORT void dvz_canvas_to_refill(DvzCanvas* canvas);

/**
 * Close the canvas at the next frame.
 *
 * @param canvas the canvas
 */
DVZ_EXPORT void dvz_canvas_to_close(DvzCanvas* canvas);



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
 * This command creates a host-coherent GPU image with the same size as the current framebuffer
 * size.
 *
 * If the interval is non-zero, the canvas will raise periodic SCREENCAST events every  `interval`
 * seconds. The event payload will contain a pointer to the grabbed framebuffer image.
 *
 * @param canvas the canvas
 * @param interval screencast events interval
 */
DVZ_EXPORT void dvz_screencast(DvzCanvas* canvas, double interval);

/**
 * Destroy the screencast.
 *
 * @param canvas the canvas
 */
DVZ_EXPORT void dvz_screencast_destroy(DvzCanvas* canvas);

/**
 * Make a screenshot.
 *
 * This function creates a screencast if there isn't one already. It is implemented with hard
 * synchronization commands so this command should *not* be used for creating many successive
 * screenshots. For that, one should register a SCREENCAST event callback.
 *
 * !!! error
 *     This function is not yet implemented.
 *
 * @param canvas the canvas
 * @returns A pointer to the 24-bit RGB framebuffer.
 */
DVZ_EXPORT uint8_t* dvz_screenshot(DvzCanvas* canvas);

/**
 * Make a screenshot and save it to a PNG file.
 *
 * !!! note
 *     This function uses full GPU synchronization methods so it is relatively inefficient. More
 *      efficient methods are not yet implemented.
 *
 * @param canvas the canvas
 * @param png_path the path to the PNG file to create
 */
DVZ_EXPORT void dvz_screenshot_file(DvzCanvas* canvas, const char* png_path);



/*************************************************************************************************/
/*  Mouse and keyboard                                                                           */
/*************************************************************************************************/

/**
 * Create the mouse object holding the current mouse state.
 *
 * @returns mouse object
 */
DVZ_EXPORT DvzMouse dvz_mouse(void);

/**
 * Reset the mouse state.
 *
 * @param mouse the mouse object
 */
DVZ_EXPORT void dvz_mouse_reset(DvzMouse* mouse);

/**
 * Emit a mouse event.
 *
 * @param mouse the mouse object
 * @param canvas the canvas
 * @param ev the mouse event
 */
DVZ_EXPORT void dvz_mouse_event(DvzMouse* mouse, DvzCanvas* canvas, DvzEvent ev);

/**
 * Convert mouse coordinates from global to local.
 *
 * * Global coordinates: in pixels, origin at the top-left corner of the window.
 * * Local coordinates: in normalize coordinates [-1, 1], origin at the center of a given viewport,
 *   taking viewport margins into account
 *
 * @param mouse the mouse object
 * @param mouse_local the mouse local object
 * @param canvas the canvas
 * @param viewport the viewport defining the local coordinates
 */
DVZ_EXPORT void dvz_mouse_local(
    DvzMouse* mouse, DvzMouseLocal* mouse_local, DvzCanvas* canvas, DvzViewport viewport);

/**
 * Create the keyboard object holding the current keyboard state.
 *
 * @returns keyboard object
 */
DVZ_EXPORT DvzKeyboard dvz_keyboard(void);

/**
 * Reset the keyboard state
 *
 * @returns keyboard object
 */
DVZ_EXPORT void dvz_keyboard_reset(DvzKeyboard* keyboard);

/**
 * Emit a keyboard event.
 *
 * @param keyboard the keyboard object
 * @param canvas the canvas
 * @param ev the keyboard event
 */
DVZ_EXPORT void dvz_keyboard_event(DvzKeyboard* keyboard, DvzCanvas* canvas, DvzEvent ev);



/*************************************************************************************************/
/*  Event system                                                                                 */
/*************************************************************************************************/

/**
 * Emit a mouse button event.
 *
 * @param canvas the canvas
 * @param type whether this is a press or release event
 * @param button the mouse button
 * @param modifiers flags with the active keyboard modifiers
 */
DVZ_EXPORT void dvz_event_mouse_button(
    DvzCanvas* canvas, DvzMouseButtonType type, DvzMouseButton button, int modifiers);

/**
 * Emit a mouse move event.
 *
 * @param canvas the canvas
 * @param pos the current mouse position, in pixels
 */
DVZ_EXPORT void dvz_event_mouse_move(DvzCanvas* canvas, vec2 pos);

/**
 * Emit a mouse wheel event.
 *
 * @param canvas the canvas
 * @param dir the mouse wheel direction
 */
DVZ_EXPORT void dvz_event_mouse_wheel(DvzCanvas* canvas, vec2 dir);

/**
 * Emit a mouse click event.
 *
 * @param canvas the canvas
 * @param pos the click position
 * @param button the mouse button
 */
DVZ_EXPORT void dvz_event_mouse_click(DvzCanvas* canvas, vec2 pos, DvzMouseButton button);

/**
 * Emit a mouse double-click event.
 *
 * @param canvas the canvas
 * @param pos the double-click position
 * @param button the mouse button
 */
DVZ_EXPORT void dvz_event_mouse_double_click(DvzCanvas* canvas, vec2 pos, DvzMouseButton button);

/**
 * Emit a mouse drag event.
 *
 * @param canvas the canvas
 * @param pos the drag start position
 * @param button the mouse button
 */
DVZ_EXPORT void dvz_event_mouse_drag(DvzCanvas* canvas, vec2 pos, DvzMouseButton button);

/**
 * Emit a mouse drag end event.
 *
 * @param canvas the canvas
 * @param pos the drag end position
 * @param button the mouse button
 */
DVZ_EXPORT void dvz_event_mouse_drag_end(DvzCanvas* canvas, vec2 pos, DvzMouseButton button);

/**
 * Emit a keyboard event.
 *
 * @param canvas the canvas
 * @param type press or release
 * @param key_code the key
 * @param modifiers flags with the active keyboard modifiers
 */
DVZ_EXPORT void
dvz_event_key(DvzCanvas* canvas, DvzKeyType type, DvzKeyCode key_code, int modifiers);

/**
 * Emit a frame event.
 *
 * Typically raised at every canvas frame.
 *
 * @param canvas the canvas
 * @param idx the frame index
 * @param time the current time
 * @param interval the interval since the last frame event
 */
DVZ_EXPORT void dvz_event_frame(DvzCanvas* canvas, uint64_t idx, double time, double interval);

/**
 * Emit a timer event.
 *
 * @param canvas the canvas
 * @param idx the timer event index
 * @param time the current time
 * @param interval the interval since the last timer event
 */
DVZ_EXPORT void dvz_event_timer(DvzCanvas* canvas, uint64_t idx, double time, double interval);

/**
 * Return the number of pending events.
 *
 * This is the number of events of the given type that are still being processed or pending in the
 * queue.
 *
 * @param canvas the canvas
 * @param type the event type
 * @returns the number of pending events
 */
DVZ_EXPORT int dvz_event_pending(DvzCanvas* canvas, DvzEventType type);

/**
 * Stop the background event loop.
 *
 * This function sends a special "closing" event to the event loop, causing it to stop.
 *
 * @param canvas the canvas
 */
DVZ_EXPORT void dvz_event_stop(DvzCanvas* canvas);



/*************************************************************************************************/
/*  Main canvas event loop                                                                       */
/*************************************************************************************************/

/**
 * Process a single frame in the event loop.
 *
 * This function probably never needs to be called directly, unless writing a custom backend.
 *
 * @param canvas the canvas
 */
DVZ_EXPORT void dvz_canvas_frame(DvzCanvas* canvas);

/**
 * Submit the rendered frame to the swapchain system.
 *
 * @param canvas the canvas
 */
DVZ_EXPORT void dvz_canvas_frame_submit(DvzCanvas* canvas);

/**
 * Start the main event loop.
 *
 * Every loop iteration processes one frame of all open canvases.
 *
 * @param app the app
 * @param frame_count number of frames to process (0 for infinite loop)
 */
DVZ_EXPORT void dvz_app_run(DvzApp* app, uint64_t frame_count);



/*************************************************************************************************/
/*  Event system                                                                                 */
/*************************************************************************************************/

// Enqueue an event.
static void _event_enqueue(DvzCanvas* canvas, DvzEvent event)
{
    ASSERT(canvas != NULL);
    DvzFifo* fifo = &canvas->event_queue;
    ASSERT(fifo != NULL);
    DvzEvent* ev = (DvzEvent*)calloc(1, sizeof(DvzEvent));
    *ev = event;
    dvz_fifo_enqueue(fifo, ev);
}



// Dequeue an event, immediately, or waiting until an event is available.
static DvzEvent _event_dequeue(DvzCanvas* canvas, bool wait)
{
    ASSERT(canvas != NULL);
    DvzFifo* fifo = &canvas->event_queue;
    ASSERT(fifo != NULL);
    DvzEvent* item = (DvzEvent*)dvz_fifo_dequeue(fifo, wait);
    DvzEvent out;
    out.type = DVZ_EVENT_NONE;
    if (item == NULL)
        return out;
    ASSERT(item != NULL);
    out = *item;
    FREE(item);
    return out;
}



// Whether there is at least one async callback.
static bool _has_async_callbacks(DvzCanvas* canvas, DvzEventType type)
{
    ASSERT(canvas != NULL);
    for (uint32_t i = 0; i < canvas->callbacks_count; i++)
    {
        if (canvas->callbacks[i].type == type && canvas->callbacks[i].mode == DVZ_EVENT_MODE_ASYNC)
            return true;
    }
    return false;
}



// Whether there is at least one event callback.
static bool _has_event_callbacks(DvzCanvas* canvas, DvzEventType type)
{
    ASSERT(canvas != NULL);
    if (type == DVZ_EVENT_NONE || type == DVZ_EVENT_INIT)
        return true;
    for (uint32_t i = 0; i < canvas->callbacks_count; i++)
        if (canvas->callbacks[i].type == type)
            return true;
    return false;
}



// Consume an event, return the number of callbacks called.
static int _event_consume(DvzCanvas* canvas, DvzEvent ev, DvzEventMode mode)
{
    ASSERT(canvas != NULL);

    if (canvas->enable_lock)
        dvz_thread_lock(&canvas->event_thread);

    // HACK: we first call the callbacks with no param, then we call the callbacks with a non-zero
    // param. This is a way to use the param as a priority value. This is used by the scene FRAME
    // callback so that it occurs after the user callbacks.
    int n_callbacks = 0;
    DvzEventCallbackRegister* r = NULL;
    for (uint32_t pass = 0; pass < 2; pass++)
    {
        for (uint32_t i = 0; i < canvas->callbacks_count; i++)
        {
            r = &canvas->callbacks[i];
            // Will pass the user_data that was registered, to the callback function.
            ev.user_data = r->user_data;

            // Only call the callbacks registered for the specified type.
            if ((r->type == ev.type) &&              //
                (r->mode == mode) &&                 //
                (((pass == 0) && (r->param == 0)) || //
                 ((pass == 1) && (r->param > 0))))   //
            {
                r->callback(canvas, ev);
                n_callbacks++;
            }
        }
    }

    if (canvas->enable_lock)
        dvz_thread_unlock(&canvas->event_thread);

    return n_callbacks;
}



// Produce an event, call the sync callbacks, and enqueue the event if there is at least one async
// callback.
static int _event_produce(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);

    // Call the sync callbacks directly.
    int n_callbacks = _event_consume(canvas, ev, DVZ_EVENT_MODE_SYNC);

    // Enqueue the event only if there is at least one async callback for that event type.
    if (_has_async_callbacks(canvas, ev.type))
        _event_enqueue(canvas, ev);

    return n_callbacks;
}



// Event loop running in the background thread, waiting for events and dequeuing them.
static void* _event_thread(void* p_canvas)
{
    DvzCanvas* canvas = (DvzCanvas*)p_canvas;
    ASSERT(canvas != NULL);
    log_debug("starting event thread");

    DvzEvent ev;
    double avg_event_time = 0; // average event callback time across all event types
    double elapsed = 0;        // average time of the event callbacks in the current iteration
    int n_callbacks = 0;       // number of event callbacks in the current event loop iteration
    int counter = 0;           // number of iterations in the event loop
    int events_to_keep = 0;    // maximum number of pending events to keep in the queue

    while (true)
    {
        // log_trace("event thread awaits for events...");
        // Wait until an event is available
        ev = _event_dequeue(canvas, true);
        canvas->event_processing = ev.type; // type of the event being processed
        if (ev.type == DVZ_EVENT_NONE)
        {
            log_trace("received empty event, stopping the event thread");
            break;
        }

        // Logic to discard some events if the queue is getting overloaded because of long-running
        // callbacks.

        // TODO: there are ways to improve the mechanism dropping events from the queue when the
        // queue is getting overloaded. Doing it on a per-type basis, better estimating the avg
        // time taken by each callback, etc.

        // log_trace("event dequeued type %d, processing it...", ev.type);
        // process the dequeued task
        elapsed = _clock_get(&canvas->clock);
        n_callbacks = _event_consume(canvas, ev, DVZ_EVENT_MODE_ASYNC);
        elapsed = _clock_get(&canvas->clock) - elapsed;
        // NOTE: avoid division by zero.
        if (n_callbacks > 0)
            elapsed /= n_callbacks; // average duration of the events

        // Update the average event time.
        avg_event_time = ((avg_event_time * counter) + elapsed) / (counter + 1);
        if (avg_event_time > 0)
        {
            events_to_keep =
                CLIP(DVZ_MAX_EVENT_DURATION / avg_event_time, 1, DVZ_MAX_FIFO_CAPACITY);
            if (events_to_keep == DVZ_MAX_FIFO_CAPACITY)
                events_to_keep = 0;
        }

        // Handle event queue overloading: if events are enqueued faster than
        // they are consumed, we should discard the older events so that the
        // queue doesn't keep filling up.
        dvz_fifo_discard(&canvas->event_queue, events_to_keep);

        canvas->event_processing = DVZ_EVENT_NONE;
        counter++;
    }
    log_debug("end event thread");

    return NULL;
}



#ifdef __cplusplus
}
#endif

#endif
