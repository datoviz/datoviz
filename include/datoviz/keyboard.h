/*************************************************************************************************/
/*  Keyboard                                                                                     */
/*************************************************************************************************/

#ifndef DVZ_HEADER_KEYBOARD
#define DVZ_HEADER_KEYBOARD



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_keycode.h"
#include "common.h"
#include "list.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

// Maximum number of simultaneously pressed keys.
#define DVZ_KEYBOARD_MAX_KEYS 8



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



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzKeyboard DvzKeyboard;
typedef struct DvzKeyboardEvent DvzKeyboardEvent;
typedef struct DvzKeyboardPayload DvzKeyboardPayload;

typedef void (*DvzKeyboardCallback)(DvzKeyboard*, DvzKeyboardEvent, void*);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzKeyboardEvent
{
    DvzKeyboardEventType type;
    DvzKeyCode key;
    int mods;
};



struct DvzKeyboardPayload
{
    DvzKeyboardEventType type;
    DvzKeyboardCallback callback;
    void* user_data;
};



struct DvzKeyboard
{
    DvzList keys;
    int mods;

    DvzList callbacks;

    // double press_time;
    bool is_active;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Keyboard functions                                                                           */
/*************************************************************************************************/

DVZ_EXPORT DvzKeyboard* dvz_keyboard(void);



DVZ_EXPORT void dvz_keyboard_press(DvzKeyboard* keyboard, DvzKeyCode key);



DVZ_EXPORT void dvz_keyboard_release(DvzKeyboard* keyboard, DvzKeyCode key);



DVZ_EXPORT DvzKeyCode dvz_keyboard_get(DvzKeyboard* keyboard, uint32_t key_idx);



DVZ_EXPORT bool dvz_keyboard_is_pressed(DvzKeyboard* keyboard, DvzKeyCode key, int mods);



DVZ_EXPORT int dvz_keyboard_mods(DvzKeyboard* keyboard);



DVZ_EXPORT void dvz_keyboard_callback(
    DvzKeyboard* keyboard, DvzKeyboardEventType type, DvzKeyboardCallback callback,
    void* user_data);



DVZ_EXPORT void dvz_keyboard_destroy(DvzKeyboard* keyboard);



EXTERN_C_OFF

#endif
