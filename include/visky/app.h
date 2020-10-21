#ifndef VKY_APP_HEADER
#define VKY_APP_HEADER

#ifdef __cplusplus
extern "C" {
#endif

#include "vklite.h"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    VKY_KEY_MODIFIER_NONE = 0x00000000,
    VKY_KEY_MODIFIER_SHIFT = 0x00000001,
    VKY_KEY_MODIFIER_CONTROL = 0x00000002,
    VKY_KEY_MODIFIER_ALT = 0x00000004,
    VKY_KEY_MODIFIER_SUPER = 0x00000008,
} VkyKeyModifiers;

typedef enum
{
    VKY_MOUSE_BUTTON_NONE = 0,
    VKY_MOUSE_BUTTON_LEFT = 1,
    VKY_MOUSE_BUTTON_MIDDLE = 2,
    VKY_MOUSE_BUTTON_RIGHT = 3,
} VkyMouseButton;

typedef enum
{
    VKY_MOUSE_STATE_STATIC = 0,
    VKY_MOUSE_STATE_DRAG = 1,
    VKY_MOUSE_STATE_WHEEL = 2,
    VKY_MOUSE_STATE_CLICK = 3,
    VKY_MOUSE_STATE_DOUBLE_CLICK = 4,
} VkyMouseState;

// taken from https://www.glfw.org/docs/3.3/group__keys.html
typedef enum
{
    VKY_KEY_UNKNOWN = -1,
    VKY_KEY_NONE = +0,
    VKY_KEY_SPACE = 32,
    VKY_KEY_APOSTROPHE = 39, /* ' */
    VKY_KEY_COMMA = 44,      /* , */
    VKY_KEY_MINUS = 45,      /* - */
    VKY_KEY_PERIOD = 46,     /* . */
    VKY_KEY_SLASH = 47,      /* / */
    VKY_KEY_0 = 48,
    VKY_KEY_1 = 49,
    VKY_KEY_2 = 50,
    VKY_KEY_3 = 51,
    VKY_KEY_4 = 52,
    VKY_KEY_5 = 53,
    VKY_KEY_6 = 54,
    VKY_KEY_7 = 55,
    VKY_KEY_8 = 56,
    VKY_KEY_9 = 57,
    VKY_KEY_SEMICOLON = 59, /* ; */
    VKY_KEY_EQUAL = 61,     /* = */
    VKY_KEY_A = 65,
    VKY_KEY_B = 66,
    VKY_KEY_C = 67,
    VKY_KEY_D = 68,
    VKY_KEY_E = 69,
    VKY_KEY_F = 70,
    VKY_KEY_G = 71,
    VKY_KEY_H = 72,
    VKY_KEY_I = 73,
    VKY_KEY_J = 74,
    VKY_KEY_K = 75,
    VKY_KEY_L = 76,
    VKY_KEY_M = 77,
    VKY_KEY_N = 78,
    VKY_KEY_O = 79,
    VKY_KEY_P = 80,
    VKY_KEY_Q = 81,
    VKY_KEY_R = 82,
    VKY_KEY_S = 83,
    VKY_KEY_T = 84,
    VKY_KEY_U = 85,
    VKY_KEY_V = 86,
    VKY_KEY_W = 87,
    VKY_KEY_X = 88,
    VKY_KEY_Y = 89,
    VKY_KEY_Z = 90,
    VKY_KEY_LEFT_BRACKET = 91,  /* [ */
    VKY_KEY_BACKSLASH = 92,     /* \ */
    VKY_KEY_RIGHT_BRACKET = 93, /* ] */
    VKY_KEY_GRAVE_ACCENT = 96,  /* ` */
    VKY_KEY_WORLD_1 = 161,      /* non-US #1 */
    VKY_KEY_WORLD_2 = 162,      /* non-US #2 */
    VKY_KEY_ESCAPE = 256,
    VKY_KEY_ENTER = 257,
    VKY_KEY_TAB = 258,
    VKY_KEY_BACKSPACE = 259,
    VKY_KEY_INSERT = 260,
    VKY_KEY_DELETE = 261,
    VKY_KEY_RIGHT = 262,
    VKY_KEY_LEFT = 263,
    VKY_KEY_DOWN = 264,
    VKY_KEY_UP = 265,
    VKY_KEY_PAGE_UP = 266,
    VKY_KEY_PAGE_DOWN = 267,
    VKY_KEY_HOME = 268,
    VKY_KEY_END = 269,
    VKY_KEY_CAPS_LOCK = 280,
    VKY_KEY_SCROLL_LOCK = 281,
    VKY_KEY_NUM_LOCK = 282,
    VKY_KEY_PRINT_SCREEN = 283,
    VKY_KEY_PAUSE = 284,
    VKY_KEY_F1 = 290,
    VKY_KEY_F2 = 291,
    VKY_KEY_F3 = 292,
    VKY_KEY_F4 = 293,
    VKY_KEY_F5 = 294,
    VKY_KEY_F6 = 295,
    VKY_KEY_F7 = 296,
    VKY_KEY_F8 = 297,
    VKY_KEY_F9 = 298,
    VKY_KEY_F10 = 299,
    VKY_KEY_F11 = 300,
    VKY_KEY_F12 = 301,
    VKY_KEY_F13 = 302,
    VKY_KEY_F14 = 303,
    VKY_KEY_F15 = 304,
    VKY_KEY_F16 = 305,
    VKY_KEY_F17 = 306,
    VKY_KEY_F18 = 307,
    VKY_KEY_F19 = 308,
    VKY_KEY_F20 = 309,
    VKY_KEY_F21 = 310,
    VKY_KEY_F22 = 311,
    VKY_KEY_F23 = 312,
    VKY_KEY_F24 = 313,
    VKY_KEY_F25 = 314,
    VKY_KEY_KP_0 = 320,
    VKY_KEY_KP_1 = 321,
    VKY_KEY_KP_2 = 322,
    VKY_KEY_KP_3 = 323,
    VKY_KEY_KP_4 = 324,
    VKY_KEY_KP_5 = 325,
    VKY_KEY_KP_6 = 326,
    VKY_KEY_KP_7 = 327,
    VKY_KEY_KP_8 = 328,
    VKY_KEY_KP_9 = 329,
    VKY_KEY_KP_DECIMAL = 330,
    VKY_KEY_KP_DIVIDE = 331,
    VKY_KEY_KP_MULTIPLY = 332,
    VKY_KEY_KP_SUBTRACT = 333,
    VKY_KEY_KP_ADD = 334,
    VKY_KEY_KP_ENTER = 335,
    VKY_KEY_KP_EQUAL = 336,
    VKY_KEY_LEFT_SHIFT = 340,
    VKY_KEY_LEFT_CONTROL = 341,
    VKY_KEY_LEFT_ALT = 342,
    VKY_KEY_LEFT_SUPER = 343,
    VKY_KEY_RIGHT_SHIFT = 344,
    VKY_KEY_RIGHT_CONTROL = 345,
    VKY_KEY_RIGHT_ALT = 346,
    VKY_KEY_RIGHT_SUPER = 347,
    VKY_KEY_MENU = 348,
    VKY_KEY_LAST = 348,
} VkyKey;

typedef enum
{
    VKY_BACKEND_NONE = 0,
    VKY_BACKEND_GLFW = 1,
    VKY_BACKEND_OFFSCREEN = 10,
    VKY_BACKEND_SCREENSHOT = 11,
    VKY_BACKEND_VIDEO = 12,
} VkyBackendType;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VkyVideo VkyVideo;
typedef struct VkyBackendVideoParams VkyBackendVideoParams;
typedef struct VkyScreenshot VkyScreenshot;
typedef struct VkyBackendScreenshotParams VkyBackendScreenshotParams;
typedef struct VkyMouse VkyMouse;
typedef struct VkyKeyboard VkyKeyboard;


// Return 1 if the canvas should be redrawn following the mouse event, 0 otherwise.
// typedef int (*VkyMouseCallback)(VkyCanvas*, VkyMouse*);

// Return 1 if the canvas should be redrawn following the keyboard event, 0 otherwise.
// typedef int (*VkyKeyboardCallback)(VkyCanvas*, VkyKeyboard*);

// Frame callback.
typedef struct VkyFrameCallbackStruct VkyFrameCallbackStruct;
typedef void (*VkyFrameCallback)(VkyCanvas*, void*);


/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VkyBackendVideoParams
{
    char* filename;
    int fps;
    int bitrate;
    double duration;
    VkyVideo* video;
};

struct VkyBackendScreenshotParams
{
    char* filename;
    uint32_t frame_index;
};



struct VkyApp
{
    VkyBackendType backend;
    void* backend_params;

    VkyGpu* gpu;

    uint32_t canvas_count;
    VkyCanvas** canvases;
    bool all_windows_closed;

    // links between canvases
    uint32_t link_count;
    void* links;

    VkyWaitCallback cb_wait;
};



struct VkyMouse
{
    VkyMouseButton button;
    vec2 press_pos;
    vec2 last_pos;
    vec2 cur_pos;
    vec2 wheel_delta;

    VkyMouseState prev_state;
    VkyMouseState cur_state;

    double press_time;
    double click_time;
};

struct VkyKeyboard
{
    VkyKey key;
    uint32_t modifiers;
    double press_time;
};

struct VkyFrameCallbackStruct
{
    VkyFrameCallback callback;
    void* data;
};

struct VkyEventController
{
    VkyCanvas* canvas;
    bool do_process_input;

    // TODO: no more pointers?
    VkyMouse* mouse;
    VkyKeyboard* keyboard;

    uint32_t mock_input_callback_count;
    VkyFrameCallbackStruct* mock_input_callbacks;

    uint32_t frame_callback_count;
    VkyFrameCallbackStruct* frame_callbacks;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

VKY_EXPORT VkyApp* vky_create_app(VkyBackendType, void*);
VKY_EXPORT VkyCanvas* vky_create_canvas(VkyApp*, uint32_t, uint32_t);
VKY_EXPORT void vky_run_app(VkyApp*);
VKY_EXPORT void vky_destroy_app(VkyApp*);
VKY_EXPORT void vky_close_canvas(VkyCanvas* canvas);
VKY_EXPORT bool vky_all_windows_closed(VkyApp*);

VKY_EXPORT void vky_create_event_controller(VkyCanvas*);
VKY_EXPORT void vky_reset_event_controller(VkyEventController* event_controller);
VKY_EXPORT void vky_destroy_event_controller(VkyEventController*);



/*************************************************************************************************/
/*  Mouse                                                                                        */
/*************************************************************************************************/

VKY_EXPORT VkyMouse* vky_event_mouse(VkyCanvas* canvas);

VKY_EXPORT void vky_update_mouse_state(VkyMouse*, vec2, VkyMouseButton);

void vky_mouse_normalize(vec2 size, vec2 center, vec2 pos);
void vky_mouse_normalize_window(VkyCanvas* canvas, vec2 pos);
void vky_mouse_normalize_viewport(VkyViewport viewport, vec2 pos);
void vky_mouse_move_delta(VkyMouse* mouse, VkyViewport viewport, vec2 delta);
void vky_mouse_press_delta(VkyMouse* mouse, VkyViewport viewport, vec2 delta);
void vky_mouse_cur_pos(VkyMouse* mouse, VkyViewport viewport, vec2 pos);
void vky_mouse_last_pos(VkyMouse* mouse, VkyViewport viewport, vec2 pos);
void vky_mouse_press_pos(VkyMouse* mouse, VkyViewport viewport, vec2 pos);



/*************************************************************************************************/
/*  Keyboard                                                                                     */
/*************************************************************************************************/

VKY_EXPORT VkyKeyboard* vky_event_keyboard(VkyCanvas* canvas);

VKY_EXPORT bool vky_is_key_modifier(VkyKey key);
VKY_EXPORT void
vky_update_keyboard_state(VkyKeyboard* keyboard, VkyKey key, VkyKeyModifiers modifiers);



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

VKY_EXPORT void vky_add_mock_input_callback(VkyCanvas* canvas, VkyFrameCallback cb, void* data);
VKY_EXPORT int vky_call_mock_input_callbacks(VkyCanvas* canvas);

VKY_EXPORT void vky_add_frame_callback(VkyCanvas* canvas, VkyFrameCallback cb, void* data);
VKY_EXPORT void vky_call_frame_callbacks(VkyCanvas* canvas); // TODO: return int?



/*************************************************************************************************/
/*  Event loop                                                                                   */
/*************************************************************************************************/

// This function updates the VkyMouse and VkyKeyboard structures:
//
// 1. If the canvas is handled by a pull-based backend, then Visky is in charge of the
//    (explicit) event loop. The vky_update_event_states() function is called at every
//    iteration. It calls canvas->mouse_callback and canvas->keyboard_callback (registered
//    by the backend during the creation of the canvas). These callback functions poll
//    the states of the mouse/keyboard via the backend-specific API, and update the VkyMouse
//    and VkyKeyboard structures directly. Then, the vky_next_frame() will process these
//    state updates.
//
// 2. If the canvas is handled by a push-based backend, then the backend is in charge
//    of the (implicit) event loop. The backend proposes to register callbacks that are
//    automatically called whenever a mouse/keyboard event occurs. In these callbacks,
//    Visky/the application calls vky_update_mouse() and vky_update_keyboard() manually
//    to update the VkyMouse and VkyKeyboard structures. Then, the vky_next_frame() will
//    process these state updates. The vky_update_event_states() does nothing in this case.
//
// This function may be blocking or non-blocking. If blocking, it waits until an event occurs.
//
VKY_EXPORT void vky_update_event_states(VkyEventController*);

VKY_EXPORT void vky_finish_event_states(VkyEventController* event_controller);

// This function is called at every event loop iteration. It calls:
//
// 1. vky_update_event_states(): update the VkyMouse VkyKeyboard structures via the backends
// 2. vky_call_mouse_callbacks() / vky_call_keyboard_callbacks(): call the user callbacks (and
// builtin interact callbacks)
//      loop over the callbacks, call them. Every callback returns 0, or 1 if the event require
//      a window update. This function takes the max of all outputs, if 0 ==> stop
// 3. submit the command buffers
VKY_EXPORT void vky_next_frame(VkyCanvas*);

void vky_upload_pending_data(VkyCanvas* canvas);


#ifdef __cplusplus
}
#endif

#endif
