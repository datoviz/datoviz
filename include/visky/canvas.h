/*************************************************************************************************/
/*  Batteries-included Vulkan-aware bare window with swapchain and event system                  */
/*************************************************************************************************/

#ifndef VKL_CANVAS_HEADER
#define VKL_CANVAS_HEADER

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

#define VKL_MAX_EVENT_CALLBACKS 32
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
#define VKL_DEFAULT_DPI_SCALING 1.0f
#define VKL_DEFAULT_PRESENT_MODE                                                                  \
    (getenv("VKL_FPS") != NULL ? VK_PRESENT_MODE_IMMEDIATE_KHR : VK_PRESENT_MODE_FIFO_KHR)
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

// Canvas creation flags.
typedef enum
{
    VKL_CANVAS_FLAGS_NONE = 0x0000,
    VKL_CANVAS_FLAGS_IMGUI = 0x0001,
    VKL_CANVAS_FLAGS_FPS = 0x0003, // NOTE: 1 bit for ImGUI, 1 bit for FPS
} VklCanvasFlags;



// Canvas size type
typedef enum
{
    VKL_CANVAS_SIZE_SCREEN,
    VKL_CANVAS_SIZE_FRAMEBUFFER,
} VklCanvasSizeType;



// Viewport type.
// NOTE: must correspond to values in common.glsl
typedef enum
{
    VKL_VIEWPORT_FULL,
    VKL_VIEWPORT_INNER,
    VKL_VIEWPORT_OUTER,
    VKL_VIEWPORT_OUTER_BOTTOM,
    VKL_VIEWPORT_OUTER_LEFT,
} VklViewportClip;



// Transform axis
// NOTE: must correspond to values in common.glsl
typedef enum
{
    VKL_INTERACT_FIXED_AXIS_DEFAULT = 0x0000,
    VKL_INTERACT_FIXED_AXIS_X = 0x1000,
    VKL_INTERACT_FIXED_AXIS_Y = 0x2000,
    VKL_INTERACT_FIXED_AXIS_Z = 0x4000,
    VKL_INTERACT_FIXED_AXIS_XY = 0x3000,
    VKL_INTERACT_FIXED_AXIS_XZ = 0x5000,
    VKL_INTERACT_FIXED_AXIS_YZ = 0x6000,
    VKL_INTERACT_FIXED_AXIS_ALL = 0x7000,
    VKL_INTERACT_FIXED_AXIS_NONE = 0x8000,
} VklInteractAxis;



// Mouse state type
typedef enum
{
    VKL_MOUSE_STATE_INACTIVE,
    VKL_MOUSE_STATE_DRAG,
    VKL_MOUSE_STATE_WHEEL,
    VKL_MOUSE_STATE_CLICK,
    VKL_MOUSE_STATE_DOUBLE_CLICK,
    VKL_MOUSE_STATE_CAPTURE,
} VklMouseStateType;



// Key state type
typedef enum
{
    VKL_KEYBOARD_STATE_INACTIVE,
    VKL_KEYBOARD_STATE_ACTIVE,
    VKL_KEYBOARD_STATE_CAPTURE,
} VklKeyboardStateType;



// Transfer status.
typedef enum
{
    VKL_TRANSFER_STATUS_NONE,
    VKL_TRANSFER_STATUS_PROCESSING,
    VKL_TRANSFER_STATUS_DONE,
} VklTransferStatus;



// Canvas refill status.
typedef enum
{
    VKL_REFILL_NONE,
    VKL_REFILL_REQUESTED,
    VKL_REFILL_PROCESSING,
} VklRefillStatus;



// Screencast status.
typedef enum
{
    VKL_SCREENCAST_NONE,
    VKL_SCREENCAST_IDLE,
    VKL_SCREENCAST_AWAIT_COPY,
    VKL_SCREENCAST_AWAIT_TRANSFER,
} VklScreencastStatus;



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
    VKL_EVENT_NONE,               //
    VKL_EVENT_INIT,               // called before the first frame
    VKL_EVENT_REFILL,             // called every time the command buffers need to be recreated
    VKL_EVENT_INTERACT,           // called at every frame, before event enqueue
    VKL_EVENT_FRAME,              // called at every frame, after event enqueue
    VKL_EVENT_IMGUI,              // called at every frame, after event enqueue
    VKL_EVENT_SCREENCAST,         // called when a screenshot has been downloaded
    VKL_EVENT_TIMER,              // called every X ms in the main thread, just after FRAME
    VKL_EVENT_MOUSE_BUTTON,       // called when a mouse button is pressed or released
    VKL_EVENT_MOUSE_MOVE,         // called when the mouse moves
    VKL_EVENT_MOUSE_WHEEL,        // called when the mouse wheel is used
    VKL_EVENT_MOUSE_DRAG_BEGIN,   // called when a drag event starts
    VKL_EVENT_MOUSE_DRAG_END,     // called when a drag event stops
    VKL_EVENT_MOUSE_CLICK,        // called after a click (called once during a double click)
    VKL_EVENT_MOUSE_DOUBLE_CLICK, // called after a double click
    VKL_EVENT_KEY,                // called after a keyboard key pressed or released
    VKL_EVENT_RESIZE,             // called at every resize
    VKL_EVENT_PRE_SEND,           // called before sending the commands buffers
    VKL_EVENT_POST_SEND,          // called after sending the commands buffers
    VKL_EVENT_DESTROY,            // called before destruction
} VklEventType;



// Event mode (sync/async)
typedef enum
{
    VKL_EVENT_MODE_SYNC,
    VKL_EVENT_MODE_ASYNC,
} VklEventMode;



// Key type
typedef enum
{
    VKL_KEY_RELEASE,
    VKL_KEY_PRESS,
} VklKeyType;



// Mouse button type
typedef enum
{
    VKL_MOUSE_RELEASE,
    VKL_MOUSE_PRESS,
} VklMouseButtonType;



// Key modifiers
// NOTE: must match GLFW values! no mapping is done for now
typedef enum
{
    VKL_KEY_MODIFIER_NONE = 0x00000000,
    VKL_KEY_MODIFIER_SHIFT = 0x00000001,
    VKL_KEY_MODIFIER_CONTROL = 0x00000002,
    VKL_KEY_MODIFIER_ALT = 0x00000004,
    VKL_KEY_MODIFIER_SUPER = 0x00000008,
} VklKeyModifiers;



// Mouse button
typedef enum
{
    VKL_MOUSE_BUTTON_NONE,
    VKL_MOUSE_BUTTON_LEFT,
    VKL_MOUSE_BUTTON_MIDDLE,
    VKL_MOUSE_BUTTON_RIGHT,
} VklMouseButton;



/*************************************************************************************************/
/*  Type definitions                                                                             */
/*************************************************************************************************/

typedef struct VklScene VklScene;
typedef struct VklMouse VklMouse;
typedef struct VklKeyboard VklKeyboard;
typedef struct VklMouseLocal VklMouseLocal;

// Events structures.
typedef struct VklEvent VklEvent;
typedef struct VklFrameEvent VklFrameEvent;
typedef struct VklKeyEvent VklKeyEvent;
typedef struct VklMouseButtonEvent VklMouseButtonEvent;
typedef struct VklMouseClickEvent VklMouseClickEvent;
typedef struct VklMouseDragEvent VklMouseDragEvent;
typedef struct VklMouseMoveEvent VklMouseMoveEvent;
typedef struct VklMouseWheelEvent VklMouseWheelEvent;
typedef struct VklRefillEvent VklRefillEvent;
typedef struct VklResizeEvent VklResizeEvent;
typedef struct VklScreencastEvent VklScreencastEvent;
typedef struct VklSubmitEvent VklSubmitEvent;
typedef struct VklTimerEvent VklTimerEvent;
typedef struct VklViewport VklViewport;
typedef union VklEventUnion VklEventUnion;

typedef void (*VklEventCallback)(VklCanvas*, VklEvent);
typedef struct VklEventCallbackRegister VklEventCallbackRegister;

typedef struct VklScreencast VklScreencast;
typedef struct VklPendingRefill VklPendingRefill;



/*************************************************************************************************/
/*  Mouse and keyboard structs                                                                   */
/*************************************************************************************************/

struct VklMouse
{
    VklMouseButton button;
    vec2 press_pos;
    vec2 last_pos;
    vec2 cur_pos;
    vec2 wheel_delta;
    float shift_length;

    VklMouseStateType prev_state;
    VklMouseStateType cur_state;

    double press_time;
    double click_time;
};



// In normalize coordinates [-1, +1]
struct VklMouseLocal
{
    vec2 press_pos;
    vec2 last_pos;
    vec2 cur_pos;
    // vec2 delta; // delta between the last and current pos
    // vec2 press_delta; // delta between t
};



struct VklKeyboard
{
    VklKeyCode key_code;
    int modifiers;

    VklKeyboardStateType prev_state;
    VklKeyboardStateType cur_state;

    double press_time;
};



/*************************************************************************************************/
/*  Viewport struct                                                                              */
/*************************************************************************************************/

// NOTE: must correspond to the shader structure in common.glsl
struct VklViewport
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
    VklViewportClip clip; // used by the GPU for viewport clipping

    // Used to discard transform on one axis
    int32_t interact_axis;

    float dpi_scaling;

    // TODO: aspect ratio
};



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
    vec2 pos;
};



struct VklMouseWheelEvent
{
    vec2 dir;
};



struct VklMouseDragEvent
{
    vec2 pos;
    VklMouseButton button;
};



struct VklMouseClickEvent
{
    vec2 pos;
    VklMouseButton button;
    bool double_click;
};



struct VklKeyEvent
{
    VklKeyType type;
    VklKeyCode key_code;
    int modifiers;
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
    uint32_t width;
    uint32_t height;
    uint8_t* rgba;
};



struct VklRefillEvent
{
    uint32_t img_idx;
    uint32_t cmd_count;
    VklCommands* cmds[32];
    VklViewport viewport;
    VkClearColorValue clear_color;
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



union VklEventUnion
{
    VklFrameEvent f;       // for FRAME events
    VklFrameEvent t;       // for TIMER events
    VklKeyEvent k;         // for KEY events
    VklMouseButtonEvent b; // for MOUSE_BUTTON events
    VklMouseClickEvent c;  // for DRAG events
    VklMouseDragEvent d;   // for DRAG events
    VklMouseMoveEvent m;   // for MOUSE_MOVE events
    VklMouseWheelEvent w;  // for WHEEL events
    VklRefillEvent rf;     // for REFILL events
    VklResizeEvent r;      // for RESIZE events
    VklScreencastEvent sc; // for SCREENCAST events
    VklSubmitEvent s;      // for SUBMIT events
};



struct VklEvent
{
    VklEventType type;
    void* user_data;
    VklEventUnion u;
};



struct VklEventCallbackRegister
{
    VklEventType type;
    uint64_t idx; // used by TIMER events: increases every time the TIMER event is raised
    double param;
    VklEventMode mode;
    VklEventCallback callback;
    void* user_data;
};



/*************************************************************************************************/
/*  Misc structs                                                                                 */
/*************************************************************************************************/

struct VklScreencast
{
    VklObject obj;

    VklCanvas* canvas;
    VklCommands cmds;
    VklSemaphores semaphore;
    VklFences fence;
    VklImages staging;
    VklSubmit submit;
    uint64_t frame_idx;
    VklClock clock;
    VklScreencastStatus status;
};



struct VklPendingRefill
{
    bool completed[VKL_MAX_SWAPCHAIN_IMAGES];
    atomic(VklRefillStatus, status);
};



/*************************************************************************************************/
/*  Canvas struct                                                                                */
/*************************************************************************************************/

struct VklCanvas
{
    VklObject obj;
    VklApp* app;
    VklGpu* gpu;

    bool offscreen;
    bool overlay;
    bool resized;
    int flags;
    void* user_data;

    // This thread-safe variable is used by the background thread to
    // safely communicate a status change of the canvas
    atomic(VklObjectStatus, cur_status);
    atomic(bool, to_close);

    VklWindow* window;

    // Swapchain
    VklSwapchain swapchain;
    VklImages depth_image;
    VklFramebuffers framebuffers;
    VklFramebuffers framebuffers_overlay; // used by the overlay renderpass
    VklSubmit submit;

    uint32_t cur_frame; // current frame within the images in flight
    uint64_t frame_idx;
    VklClock clock;
    float fps;

    // Renderpasses.
    VklRenderpass renderpass;         // default renderpass
    VklRenderpass renderpass_overlay; // GUI overlay renderpass

    // Synchronization events.
    VklSemaphores sem_img_available;
    VklSemaphores sem_render_finished;
    VklSemaphores* present_semaphores;
    VklFences fences_render_finished;
    VklFences fences_flight;

    // Default command buffers.
    VklCommands cmds_transfer;
    VklCommands cmds_render;

    // Other command buffers.
    VklContainer commands;

    // Graphics pipelines.
    VklContainer graphics;

    // Data transfers.
    VklFifo transfers;

    // Event callbacks, running in the background thread, may be slow, for end-users.
    uint32_t callbacks_count;
    VklEventCallbackRegister callbacks[VKL_MAX_EVENT_CALLBACKS];

    // Event queue.
    VklFifo event_queue;
    VklEvent events[VKL_MAX_FIFO_CAPACITY];
    VklThread event_thread;
    bool enable_lock;
    atomic(VklEventType, event_processing);
    VklMouse mouse;
    VklKeyboard keyboard;

    VklScreencast* screencast;
    VklPendingRefill refills;

    VklViewport viewport;
    VklScene* scene;
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
VKY_EXPORT VklCanvas* vkl_canvas(VklGpu* gpu, uint32_t width, uint32_t height, int flags);

/**
 * Create an offscreen canvas.
 *
 * @param gpu the GPU to use for swapchain presentation
 * @param width the canvas width, in pixels
 * @param height the canvas height, in pixels
 * @param flags the creation flags for the canvas
 */
VKY_EXPORT VklCanvas*
vkl_canvas_offscreen(VklGpu* gpu, uint32_t width, uint32_t height, int flags);

/**
 * Recreate the canvas GPU resources and swapchain.
 *
 * @param canvas the canvas to recreate
 */
VKY_EXPORT void vkl_canvas_recreate(VklCanvas* canvas);

/**
 * Create a set of Vulkan command buffers on a given GPU queue.
 *
 * @param canvas the canvas
 * @param queue_idx the index of the GPU queue within the GPU context
 * @param count number of command buffers to create
 * @returns the set of created command buffers
 */
VKY_EXPORT VklCommands* vkl_canvas_commands(VklCanvas* canvas, uint32_t queue_idx, uint32_t count);



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
 * @param color the background color
 */
VKY_EXPORT void vkl_canvas_clear_color(VklCanvas* canvas, VkClearColorValue color);

/**
 * Get the canvas size.
 *
 * @param canvas the canvas
 * @param type the unit of the requested screen size
 * @param[out] size the size vector filled by this function
 */
VKY_EXPORT void vkl_canvas_size(VklCanvas* canvas, VklCanvasSizeType type, uvec2 size);

/**
 * Whether the canvas should close when Escape is pressed.
 *
 * @param canvas the canvas
 * @param value the boolean value
 */
VKY_EXPORT void vkl_canvas_close_on_esc(VklCanvas* canvas, bool value);

// screen coordinates
static inline bool _pos_in_viewport(VklViewport viewport, vec2 screen_pos)
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
VKY_EXPORT VklViewport vkl_viewport_full(VklCanvas* canvas);



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

/**
 * Register a callback for canvas events.
 *
 * These user callbacks run either in the main thread (*sync* callbacks) or in the background
 * thread * (*async* callbacks). Callbacks can access the `VklMouse` and `VklKeyboard` structures
 * with the current state of the mouse and keyboard.
 *
 * Callback function signature: `void(VklCanvas*, VklEvent)`
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
VKY_EXPORT void vkl_event_callback(
    VklCanvas* canvas, VklEventType type, double param, VklEventMode mode, //
    VklEventCallback callback, void* user_data);



/*************************************************************************************************/
/*  State changes                                                                                */
/*************************************************************************************************/

/**
 * Trigger a canvas refill at the next frame.
 *
 * @param canvas the canvas
 */
VKY_EXPORT void vkl_canvas_to_refill(VklCanvas* canvas);

/**
 * Close the canvas at the next frame.
 *
 * @param canvas the canvas
 */
VKY_EXPORT void vkl_canvas_to_close(VklCanvas* canvas);



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
VKY_EXPORT void vkl_screencast(VklCanvas* canvas, double interval);

/**
 * Destroy the screencast.
 *
 * @param canvas the canvas
 */
VKY_EXPORT void vkl_screencast_destroy(VklCanvas* canvas);

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
VKY_EXPORT uint8_t* vkl_screenshot(VklCanvas* canvas);

/**
 * Make a screenshot and save it to a PNG or PPM file.
 *
 * @param canvas the canvas
 * @param png_path the path to the PNG file to create
 */
VKY_EXPORT void vkl_screenshot_file(VklCanvas* canvas, const char* png_path);



/*************************************************************************************************/
/*  Mouse and keyboard                                                                           */
/*************************************************************************************************/

/**
 * Create the mouse object holding the current mouse state.
 *
 * @returns mouse object
 */
VKY_EXPORT VklMouse vkl_mouse(void);

/**
 * Reset the mouse state.
 *
 * @param mouse the mouse object
 */
VKY_EXPORT void vkl_mouse_reset(VklMouse* mouse);

/**
 * Emit a mouse event.
 *
 * @param mouse the mouse object
 * @param canvas the canvas
 * @param ev the mouse event
 */
VKY_EXPORT void vkl_mouse_event(VklMouse* mouse, VklCanvas* canvas, VklEvent ev);

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
VKY_EXPORT void vkl_mouse_local(
    VklMouse* mouse, VklMouseLocal* mouse_local, VklCanvas* canvas, VklViewport viewport);

/**
 * Create the keyboard object holding the current keyboard state.
 *
 * @returns keyboard object
 */
VKY_EXPORT VklKeyboard vkl_keyboard(void);

/**
 * Reset the keyboard state
 *
 * @returns keyboard object
 */
VKY_EXPORT void vkl_keyboard_reset(VklKeyboard* keyboard);

/**
 * Emit a keyboard event.
 *
 * @param keyboard the keyboard object
 * @param canvas the canvas
 * @param ev the keyboard event
 */
VKY_EXPORT void vkl_keyboard_event(VklKeyboard* keyboard, VklCanvas* canvas, VklEvent ev);



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
VKY_EXPORT void vkl_event_mouse_button(
    VklCanvas* canvas, VklMouseButtonType type, VklMouseButton button, int modifiers);

/**
 * Emit a mouse move event.
 *
 * @param canvas the canvas
 * @param pos the current mouse position, in pixels
 */
VKY_EXPORT void vkl_event_mouse_move(VklCanvas* canvas, vec2 pos);

/**
 * Emit a mouse wheel event.
 *
 * @param canvas the canvas
 * @param dir the mouse wheel direction
 */
VKY_EXPORT void vkl_event_mouse_wheel(VklCanvas* canvas, vec2 dir);

/**
 * Emit a mouse click event.
 *
 * @param canvas the canvas
 * @param pos the click position
 * @param button the mouse button
 */
VKY_EXPORT void vkl_event_mouse_click(VklCanvas* canvas, vec2 pos, VklMouseButton button);

/**
 * Emit a mouse double-click event.
 *
 * @param canvas the canvas
 * @param pos the double-click position
 * @param button the mouse button
 */
VKY_EXPORT void vkl_event_mouse_double_click(VklCanvas* canvas, vec2 pos, VklMouseButton button);

/**
 * Emit a mouse drag event.
 *
 * @param canvas the canvas
 * @param pos the drag start position
 * @param button the mouse button
 */
VKY_EXPORT void vkl_event_mouse_drag(VklCanvas* canvas, vec2 pos, VklMouseButton button);

/**
 * Emit a mouse drag end event.
 *
 * @param canvas the canvas
 * @param pos the drag end position
 * @param button the mouse button
 */
VKY_EXPORT void vkl_event_mouse_drag_end(VklCanvas* canvas, vec2 pos, VklMouseButton button);

/**
 * Emit a keyboard event.
 *
 * @param canvas the canvas
 * @param type press or release
 * @param key_code the key
 * @param modifiers flags with the active keyboard modifiers
 */
VKY_EXPORT void
vkl_event_key(VklCanvas* canvas, VklKeyType type, VklKeyCode key_code, int modifiers);

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
VKY_EXPORT void vkl_event_frame(VklCanvas* canvas, uint64_t idx, double time, double interval);

/**
 * Emit a timer event.
 *
 * @param canvas the canvas
 * @param idx the timer event index
 * @param time the current time
 * @param interval the interval since the last timer event
 */
VKY_EXPORT void vkl_event_timer(VklCanvas* canvas, uint64_t idx, double time, double interval);

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
VKY_EXPORT int vkl_event_pending(VklCanvas* canvas, VklEventType type);

/**
 * Stop the background event loop.
 *
 * This function sends a special "closing" event to the event loop, causing it to stop.
 *
 * @param canvas the canvas
 */
VKY_EXPORT void vkl_event_stop(VklCanvas* canvas);



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
VKY_EXPORT void vkl_canvas_frame(VklCanvas* canvas);

/**
 * Submit the rendered frame to the swapchain system.
 *
 * @param canvas the canvas
 */
VKY_EXPORT void vkl_canvas_frame_submit(VklCanvas* canvas);

/**
 * Start the main event loop.
 *
 * Every loop iteration processes one frame of all open canvases.
 *
 * @param app the app
 * @param frame_count number of frames to process (0 for infinite loop)
 */
VKY_EXPORT void vkl_app_run(VklApp* app, uint64_t frame_count);



/*************************************************************************************************/
/*  Event system                                                                                 */
/*************************************************************************************************/

// Enqueue an event.
static void _event_enqueue(VklCanvas* canvas, VklEvent event)
{
    ASSERT(canvas != NULL);
    VklFifo* fifo = &canvas->event_queue;
    ASSERT(fifo != NULL);
    VklEvent* ev = (VklEvent*)calloc(1, sizeof(VklEvent));
    *ev = event;
    vkl_fifo_enqueue(fifo, ev);
}



// Dequeue an event, immediately, or waiting until an event is available.
static VklEvent _event_dequeue(VklCanvas* canvas, bool wait)
{
    ASSERT(canvas != NULL);
    VklFifo* fifo = &canvas->event_queue;
    ASSERT(fifo != NULL);
    VklEvent* item = (VklEvent*)vkl_fifo_dequeue(fifo, wait);
    VklEvent out;
    out.type = VKL_EVENT_NONE;
    if (item == NULL)
        return out;
    ASSERT(item != NULL);
    out = *item;
    FREE(item);
    return out;
}



// Whether there is at least one async callback.
static bool _has_async_callbacks(VklCanvas* canvas, VklEventType type)
{
    ASSERT(canvas != NULL);
    for (uint32_t i = 0; i < canvas->callbacks_count; i++)
    {
        if (canvas->callbacks[i].type == type && canvas->callbacks[i].mode == VKL_EVENT_MODE_ASYNC)
            return true;
    }
    return false;
}



// Whether there is at least one event callback.
static bool _has_event_callbacks(VklCanvas* canvas, VklEventType type)
{
    ASSERT(canvas != NULL);
    if (type == VKL_EVENT_NONE || type == VKL_EVENT_INIT)
        return true;
    for (uint32_t i = 0; i < canvas->callbacks_count; i++)
        if (canvas->callbacks[i].type == type)
            return true;
    return false;
}



// Consume an event, return the number of callbacks called.
static int _event_consume(VklCanvas* canvas, VklEvent ev, VklEventMode mode)
{
    ASSERT(canvas != NULL);

    if (canvas->enable_lock)
        vkl_thread_lock(&canvas->event_thread);

    // HACK: we first call the callbacks with no param, then we call the callbacks with a non-zero
    // param. This is a way to use the param as a priority value. This is used by the scene FRAME
    // callback so that it occurs after the user callbacks.
    int n_callbacks = 0;
    VklEventCallbackRegister* r = NULL;
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
        vkl_thread_unlock(&canvas->event_thread);

    return n_callbacks;
}



// Produce an event, call the sync callbacks, and enqueue the event if there is at least one async
// callback.
static int _event_produce(VklCanvas* canvas, VklEvent ev)
{
    ASSERT(canvas != NULL);

    // Call the sync callbacks directly.
    int n_callbacks = _event_consume(canvas, ev, VKL_EVENT_MODE_SYNC);

    // Enqueue the event only if there is at least one async callback for that event type.
    if (_has_async_callbacks(canvas, ev.type))
        _event_enqueue(canvas, ev);

    return n_callbacks;
}



// Event loop running in the background thread, waiting for events and dequeuing them.
static void* _event_thread(void* p_canvas)
{
    VklCanvas* canvas = (VklCanvas*)p_canvas;
    ASSERT(canvas != NULL);
    log_debug("starting event thread");

    VklEvent ev;
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
        if (ev.type == VKL_EVENT_NONE)
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
        n_callbacks = _event_consume(canvas, ev, VKL_EVENT_MODE_ASYNC);
        elapsed = _clock_get(&canvas->clock) - elapsed;
        // NOTE: avoid division by zero.
        if (n_callbacks > 0)
            elapsed /= n_callbacks; // average duration of the events

        // Update the average event time.
        avg_event_time = ((avg_event_time * counter) + elapsed) / (counter + 1);
        if (avg_event_time > 0)
        {
            events_to_keep =
                CLIP(VKL_MAX_EVENT_DURATION / avg_event_time, 1, VKL_MAX_FIFO_CAPACITY);
            if (events_to_keep == VKL_MAX_FIFO_CAPACITY)
                events_to_keep = 0;
        }

        // Handle event queue overloading: if events are enqueued faster than
        // they are consumed, we should discard the older events so that the
        // queue doesn't keep filling up.
        vkl_fifo_discard(&canvas->event_queue, events_to_keep);

        canvas->event_processing = VKL_EVENT_NONE;
        counter++;
    }
    log_debug("end event thread");

    return NULL;
}



#ifdef __cplusplus
}
#endif

#endif
