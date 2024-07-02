/*************************************************************************************************/
/*  Keyboard                                                                                     */
/*************************************************************************************************/

#ifndef DVZ_HEADER_INPUT_COMMON
#define DVZ_HEADER_INPUT_COMMON



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_keycode.h"
#include "_math.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Keyboard mods
// NOTE: must match GLFW values! no mapping is done as of now
typedef enum
{
    DVZ_KEY_MODIFIER_NONE = 0x00000000,
    DVZ_KEY_MODIFIER_SHIFT = 0x00000001,
    DVZ_KEY_MODIFIER_CONTROL = 0x00000002,
    DVZ_KEY_MODIFIER_ALT = 0x00000004,
    DVZ_KEY_MODIFIER_SUPER = 0x00000008,
} DvzKeyboardModifiers;



// Keyboard event type (press or release)
typedef enum
{
    DVZ_KEYBOARD_EVENT_NONE,
    DVZ_KEYBOARD_EVENT_PRESS,
    DVZ_KEYBOARD_EVENT_RELEASE,
} DvzKeyboardEventType;



// Mouse buttons
typedef enum
{
    DVZ_MOUSE_BUTTON_NONE = 0,
    DVZ_MOUSE_BUTTON_LEFT = 1,
    DVZ_MOUSE_BUTTON_MIDDLE = 2,
    DVZ_MOUSE_BUTTON_RIGHT = 3,
} DvzMouseButton;



// Mouse states.
typedef enum
{
    DVZ_MOUSE_STATE_RELEASE = 0,
    DVZ_MOUSE_STATE_PRESS = 1,
    DVZ_MOUSE_STATE_CLICK = 3,
    DVZ_MOUSE_STATE_CLICK_PRESS = 4,
    DVZ_MOUSE_STATE_DOUBLE_CLICK = 5,
    DVZ_MOUSE_STATE_DRAGGING = 11,
} DvzMouseState;



// Mouse events.
typedef enum
{
    DVZ_MOUSE_EVENT_RELEASE = 0,
    DVZ_MOUSE_EVENT_PRESS = 1,
    DVZ_MOUSE_EVENT_MOVE = 2,
    DVZ_MOUSE_EVENT_CLICK = 3,
    DVZ_MOUSE_EVENT_DOUBLE_CLICK = 5,
    DVZ_MOUSE_EVENT_DRAG_START = 10,
    DVZ_MOUSE_EVENT_DRAG = 11,
    DVZ_MOUSE_EVENT_DRAG_STOP = 12,
    DVZ_MOUSE_EVENT_WHEEL = 20,
    DVZ_MOUSE_EVENT_ALL = 255,
} DvzMouseEventType;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzKeyboardEvent DvzKeyboardEvent;
typedef struct DvzMouseEvent DvzMouseEvent;
typedef union DvzMouseEventUnion DvzMouseEventUnion;
typedef struct DvzMouseButtonEvent DvzMouseButtonEvent;
typedef struct DvzMouseWheelEvent DvzMouseWheelEvent;
typedef struct DvzMouseDragEvent DvzMouseDragEvent;
typedef struct DvzMouseClickEvent DvzMouseClickEvent;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzKeyboardEvent
{
    DvzKeyboardEventType type;
    DvzKeyCode key;
    int mods;
    void* user_data;
};



struct DvzMouseButtonEvent
{
    DvzMouseButton button;
};

struct DvzMouseWheelEvent
{
    vec2 dir;
};

struct DvzMouseDragEvent
{
    DvzMouseButton button;
    vec2 cur_pos; // press_pos is in the mouse event itself
    vec2 shift;
};

struct DvzMouseClickEvent
{
    DvzMouseButton button;
};

union DvzMouseEventUnion
{
    DvzMouseButtonEvent b;
    DvzMouseWheelEvent w;
    DvzMouseDragEvent d;
    DvzMouseClickEvent c;
};

struct DvzMouseEvent
{
    DvzMouseEventType type;
    DvzMouseEventUnion content;
    vec2 pos;
    int mods;
    float content_scale;
    void* user_data;
};



#endif
