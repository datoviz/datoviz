/*************************************************************************************************/
/*  Mouse                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_MOUSE
#define DVZ_HEADER_MOUSE



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_input.h"
#include "_keycode.h"
#include "_list.h"
#include "common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_MOUSE_CLICK_MAX_DELAY        0.25
#define DVZ_MOUSE_CLICK_MAX_SHIFT        5
#define DVZ_MOUSE_DOUBLE_CLICK_MAX_DELAY 0.2
#define DVZ_MOUSE_MOVE_MAX_PENDING       16
#define DVZ_MOUSE_MOVE_MIN_DELAY         0.01



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzMouse DvzMouse;
typedef struct DvzMousePayload DvzMousePayload;

typedef void (*DvzMouseCallback)(DvzMouse*, DvzMouseEvent, void*);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

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



DVZ_EXPORT void dvz_mouse_release(DvzMouse* mouse, DvzMouseButton button, int mods);



DVZ_EXPORT void dvz_mouse_wheel(DvzMouse* mouse, vec2 dir, int mods);



DVZ_EXPORT void dvz_mouse_tick(DvzMouse* mouse, double time);



DVZ_EXPORT void dvz_mouse_callback(
    DvzMouse* mouse, DvzMouseEventType type, DvzMouseCallback callback, void* user_data);



DVZ_EXPORT void dvz_mouse_destroy(DvzMouse* mouse);



EXTERN_C_OFF

#endif
