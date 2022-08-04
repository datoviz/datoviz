/*************************************************************************************************/
/*  Keyboard                                                                                     */
/*************************************************************************************************/

#include "keyboard.h"
#include "common.h"



/*************************************************************************************************/
/*  Keyboard util functions                                                                      */
/*************************************************************************************************/

static bool _is_key_modifier(DvzKeyCode key)
{
    return (
        key == DVZ_KEY_LEFT_SHIFT || key == DVZ_KEY_RIGHT_SHIFT || key == DVZ_KEY_LEFT_CONTROL ||
        key == DVZ_KEY_RIGHT_CONTROL || key == DVZ_KEY_LEFT_ALT || key == DVZ_KEY_RIGHT_ALT ||
        key == DVZ_KEY_LEFT_SUPER || key == DVZ_KEY_RIGHT_SUPER);
}



static int _key_modifiers(int key_code)
{
    int mods = 0;
    if (key_code == DVZ_KEY_LEFT_CONTROL || key_code == DVZ_KEY_RIGHT_CONTROL)
        mods |= DVZ_KEY_MODIFIER_CONTROL;
    if (key_code == DVZ_KEY_LEFT_SHIFT || key_code == DVZ_KEY_RIGHT_SHIFT)
        mods |= DVZ_KEY_MODIFIER_SHIFT;
    if (key_code == DVZ_KEY_LEFT_ALT || key_code == DVZ_KEY_RIGHT_ALT)
        mods |= DVZ_KEY_MODIFIER_ALT;
    if (key_code == DVZ_KEY_LEFT_SUPER || key_code == DVZ_KEY_RIGHT_SUPER)
        mods |= DVZ_KEY_MODIFIER_SUPER;
    return mods;
}



/*************************************************************************************************/
/*  Keyboard functions                                                                           */
/*************************************************************************************************/

DvzKeyboard* dvz_keyboard()
{
    DvzKeyboard* keyboard = calloc(1, sizeof(DvzKeyboard));
    keyboard->keys = dvz_list();
    return keyboard;
}



void dvz_keyboard_press(DvzKeyboard* keyboard, DvzKeyCode key, DvzKeyboardModifiers mods)
{
    ASSERT(keyboard != NULL);
    dvz_list_append(&keyboard->keys, (int)key);
    keyboard->mods = mods;
}



void dvz_keyboard_release(DvzKeyboard* keyboard, DvzKeyCode key)
{
    ASSERT(keyboard != NULL);

    uint64_t idx = dvz_list_index(&keyboard->keys, (int)key);
    if (idx != UINT64_MAX)
        dvz_list_remove(&keyboard->keys, idx);
}



DvzKeyCode dvz_keyboard_get(DvzKeyboard* keyboard, uint32_t key_idx)
{
    ASSERT(keyboard != NULL);
    if (key_idx < keyboard->keys.count)
        return (DvzKeyCode)dvz_list_get(&keyboard->keys, key_idx);
    else
        return DVZ_KEY_NONE;
}



bool dvz_keyboard_is_pressed(DvzKeyboard* keyboard, DvzKeyCode key, DvzKeyboardModifiers mods)
{
    ASSERT(keyboard != NULL);
    return ((keyboard->mods & mods) == mods) && dvz_list_has(&keyboard->keys, key);
}



DvzKeyboardModifiers dvz_keyboard_mods(DvzKeyboard* keyboard)
{
    ASSERT(keyboard != NULL);
    return keyboard->mods;
}



void dvz_keyboard_destroy(DvzKeyboard* keyboard)
{
    ASSERT(keyboard != NULL);
    dvz_list_destroy(&keyboard->keys);
    FREE(keyboard);
}
