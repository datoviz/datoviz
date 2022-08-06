/*************************************************************************************************/
/*  Keyboard                                                                                     */
/*************************************************************************************************/

#ifndef DVZ_HEADER_KEYBOARD
#define DVZ_HEADER_KEYBOARD



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_input.h"
#include "_list.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

// Maximum number of simultaneously pressed keys.
#define DVZ_KEYBOARD_MAX_KEYS 8



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzKeyboard DvzKeyboard;
typedef struct DvzKeyboardPayload DvzKeyboardPayload;

typedef void (*DvzKeyboardCallback)(DvzKeyboard* keyboard, DvzKeyboardEvent ev);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzKeyboardPayload
{
    DvzKeyboardEventType type;
    DvzKeyboardCallback callback;
    void* user_data;
};



struct DvzKeyboard
{
    DvzList* keys;
    int mods;

    DvzList* callbacks;

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
