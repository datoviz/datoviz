/*************************************************************************************************/
/*  Keyboard                                                                                     */
/*************************************************************************************************/

#ifndef DVZ_HEADER_KEYBOARD
#define DVZ_HEADER_KEYBOARD



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_prng.h"
#include "common.h"
#include "fifo.h"
#include "keycode.h"
#include "list.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_KEYBOARD_MAX_CALLBACKS 64
#define DVZ_KEYBOARD_PRESS_DELAY   .05
#define DVZ_KEYBOARD_MAX_KEYS      8



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



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzKeyboard DvzKeyboard;

// typedef void (*DvzKeyboardCallback)(DvzKeyboard*, DvzEvent, void*);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzKeyboard
{
    DvzList keys;
    int mods;

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



DVZ_EXPORT void dvz_keyboard_destroy(DvzKeyboard* keyboard);

// void dvz_keyboard_callback(
//     DvzKeyboard* keyboard, DvzKeyboardEventType type, DvzKeyboardCallback callback,
//     void* user_data);



EXTERN_C_OFF

#endif
