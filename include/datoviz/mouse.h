/*************************************************************************************************/
/*  Mouse                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_MOUSE
#define DVZ_HEADER_MOUSE



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "common.h"
#include "keycode.h"
#include "list.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_MOUSE_CLICK_MAX_DELAY        0.25
#define DVZ_MOUSE_CLICK_MAX_SHIFT        5
#define DVZ_MOUSE_DOUBLE_CLICK_MAX_DELAY 0.2
#define DVZ_MOUSE_MOVE_MAX_PENDING       16
#define DVZ_MOUSE_MOVE_MIN_DELAY         0.01



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

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
} DvzMouseEventType;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzMouse DvzMouse;
typedef struct DvzMouseEvent DvzMouseEvent;
typedef struct DvzMousePayload DvzMousePayload;

typedef void (*DvzMouseCallback)(DvzMouse*, DvzMouseEvent, void*);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzMouseEvent
{
    DvzMouseEventType type;
    union
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
        } c;

    } content;
    int mods;
};



struct DvzMousePayload
{
    DvzMouseEventType type;
    DvzMouseCallback callback;
    void* user_data;
};



struct DvzMouse
{
    DvzList callbacks;
    bool is_active;

    DvzMouseState state;
    DvzMouseButton button;
    vec2 press_pos, last_pos, cur_pos, wheel_delta;
    double time, last_press, last_click;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Mouse functions                                                                              */
/*************************************************************************************************/

DVZ_EXPORT DvzMouse* dvz_mouse(void);



DVZ_EXPORT void dvz_mouse_move(DvzMouse* mouse, vec2 pos, int mods);



DVZ_EXPORT void dvz_mouse_press(DvzMouse* mouse, DvzMouseButton button, int mods);



DVZ_EXPORT void dvz_mouse_release(DvzMouse* mouse, DvzMouseButton button);



DVZ_EXPORT void dvz_mouse_wheel(DvzMouse* mouse, vec2 dir, int mods);



DVZ_EXPORT void dvz_mouse_tick(DvzMouse* mouse, double time);



DVZ_EXPORT void dvz_mouse_callback(
    DvzMouse* mouse, DvzMouseEventType type, DvzMouseCallback callback, void* user_data);



DVZ_EXPORT void dvz_mouse_destroy(DvzMouse* mouse);



EXTERN_C_OFF

#endif
