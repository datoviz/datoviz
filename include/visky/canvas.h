#ifndef VKL_CANVAS_HEADER
#define VKL_CANVAS_HEADER

#include "keycode.h"
#include "vklite2.h"


/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    VKL_CANVAS_CALLBACK_INIT,      // called before the first frame
    VKL_CANVAS_CALLBACK_EVENT,     // called at every frame, before event enqueue
    VKL_CANVAS_CALLBACK_FRAME,     // called at every frame, after event enqueue
    VKL_CANVAS_CALLBACK_RESIZE,    // called at every resize
    VKL_CANVAS_CALLBACK_POST_SEND, // called after sending the commands buffers
    VKL_CANVAS_CALLBACK_DESTROY,   // called before destruction
} VklCanvasCallbackType;



typedef enum
{
    VKL_CANVAS_SIZE_SCREEN,
    VKL_CANVAS_SIZE_FRAMEBUFFER,
} VklCanvasSizeType;



/*************************************************************************************************/
/*  Event system                                                                                 */
/*************************************************************************************************/

typedef enum
{
    VKL_EVENT_NONE,
    VKL_EVENT_MOUSE,
    VKL_EVENT_KEY,
    VKL_EVENT_FRAME,
    VKL_EVENT_TIMER
} VklEventType;



typedef enum
{
    VKL_KEY_RELEASE,
    VKL_KEY_PRESS,
} VklKeyType;



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
/*  Type definitions */
/*************************************************************************************************/

typedef struct VklFrame VklFrame;
typedef struct VklMouse VklMouse;
typedef struct VklMouseState VklMouseState;
typedef struct VklKey VklKey;
typedef struct VklKeyState VklKeyState;
typedef struct VklEvent VklEvent;

typedef void (*VklCanvasCallback)(VklCanvas*, VklCanvasCallbackType, void*);
typedef void (*VklEventCallback)(VklCanvas*, VklEventType, void*);
typedef void (*VklCanvasRefill)(VklCanvas*, VklCommands*, uint32_t idx, void*);



/*************************************************************************************************/
/*  Event structs                                                                                */
/*************************************************************************************************/

struct VklMouse
{
    uint64_t idx;
    VklMouseButton button;
    uvec2 pos;
};



struct VklKey
{
    uint64_t idx;
    VklKeyType type;
    VklKeyCode key_code;
    char key_char;
};



struct VklFrame
{
    uint64_t idx;
    double time;
    double interval;
};



struct VklEvent
{
    VklEventType type;
    union
    {
        VklMouse m;
        VklKey k;
        VklFrame f;
    } u;
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
    VklContext* ctx;

    VklWindow* window;
    uint32_t width, height;

    uint32_t max_commands;
    VklCommands* commands;

    VklSwapchain* swapchain;
    VklImages* images[VKL_MAX_SWAPCHAIN_IMAGES]; // swapchain images
    VklImages* depth_image;

    uint32_t max_renderpasses;
    VklRenderpass* renderpasses;

    uint32_t max_swapchains;
    VklSwapchain* swapchains;

    uint32_t max_framebuffers;
    VklFramebuffers* framebuffers;

    uint32_t max_semaphores;
    VklSemaphores* semaphores;

    uint32_t max_fences;
    VklFences* fences;

    // TODO: event system
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



/*************************************************************************************************/
/*  Canvas misc                                                                                  */
/*************************************************************************************************/

VKY_EXPORT void vkl_canvas_size(VklCanvas* canvas, VklCanvasSizeType type, uvec2 size);

VKY_EXPORT void vkl_canvas_close_on_esc(VklCanvas* canvas, bool value);



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

VKY_EXPORT void vkl_canvas_callback(
    VklCanvas* canvas, VklCanvasCallbackType type, VklCanvasCallback* callback, void* user_data);

VKY_EXPORT void vkl_canvas_refill(VklCanvas* canvas, VklCanvasRefill* callback, void* user_data);

// these callbacks are called in the background thread
// they can access VklMouse and VklKeyboard which are owned by the background thread
VKY_EXPORT void vkl_event_callback(
    VklCanvas* canvas, VklEventType type, VklEventCallback* callback, void* user_data);



/*************************************************************************************************/
/*  State changes                                                                                */
/*************************************************************************************************/

VKY_EXPORT void vkl_canvas_to_refill(VklCanvas* canvas, bool value);

VKY_EXPORT void vkl_canvas_to_close(VklCanvas* canvas, bool value);



/*************************************************************************************************/
/*  Screenshot                                                                                   */
/*************************************************************************************************/

VKY_EXPORT void vkl_screenshot(VklCanvas* canvas, void* data);

VKY_EXPORT void vkl_screenshot_file(VklCanvas* canvas, const char* filepath);



/*************************************************************************************************/
/*  Prompt                                                                                       */
/*************************************************************************************************/

VKY_EXPORT void vkl_prompt(VklCanvas* canvas);

VKY_EXPORT char* vkl_prompt_get(VklCanvas* canvas);

VKY_EXPORT void vkl_prompt_hide(VklCanvas* canvas);



/*************************************************************************************************/
/*  Event system                                                                                 */
/*************************************************************************************************/

VKY_EXPORT void vkl_event_enqueue(VklCanvas* canvas, VklEvent event);

VKY_EXPORT void vkl_event_mouse(VklCanvas* canvas, VklMouseButton button, uvec2 pos);

VKY_EXPORT void vkl_event_key(VklCanvas* canvas, VklKeyType type, VklKey key_code, char key_char);

VKY_EXPORT void vkl_event_frame(VklCanvas* canvas, uint64_t idx, double time, double interval);

VKY_EXPORT void vkl_event_timer(VklCanvas* canvas, uint64_t idx, double time, double interval);

VKY_EXPORT VklEvent vkl_event_dequeue(VklCanvas* canvas, bool wait);

// send a null event to the queue which causes the dequeue awaiting thread to end
VKY_EXPORT void vkl_event_stop(VklCanvas* canvas);



/*************************************************************************************************/
/*  Event loop                                                                                   */
/*************************************************************************************************/

// call EVENT callbacks (for backends only), which may enqueue some events
// FRAME callbacks (rarely used)
// check canvas.need_refill (atomic)
// if refill needed, wait for current fence, and call the refill callbacks
VKY_EXPORT void vkl_canvas_frame(VklCanvas* canvas);

// loop over all canvas commands on the RENDER queue (skip inactive ones)
// add them to a new Submit
// send the command associated to the current swapchain image
// if resize, call RESIZE callback before cmd_reset
// between send and present, call POST_SEND callback
VKY_EXPORT void vkl_canvas_frame_submit(VklCanvas* canvas);

// main loop over frames
// in each iteration, loop over the canvas
// for each canvas, call canvas_frame and frame_submit
// vkl_context_transfer_loop(no wait)
// if present queue different from render queue, present queue wait
// close canvases to close
// if no canvases remaining, exit the loop
VKY_EXPORT void vkl_app_run(VklApp* app, uint32_t frame_count);



#endif
