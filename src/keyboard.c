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



static void _callbacks(DvzKeyboard* keyboard, DvzKeyboardEvent event)
{
    DvzKeyboardPayload* payload = NULL;
    uint32_t n = dvz_list_count(&keyboard->callbacks);
    for (uint32_t i = 0; i < n; i++)
    {
        payload = (DvzKeyboardPayload*)dvz_list_get(&keyboard->callbacks, i).p;
        if (payload->type == event.type)
        {
            payload->callback(keyboard, event, payload->user_data);
        }
    }
}



/*************************************************************************************************/
/*  Keyboard functions                                                                           */
/*************************************************************************************************/

DvzKeyboard* dvz_keyboard()
{
    DvzKeyboard* keyboard = calloc(1, sizeof(DvzKeyboard));
    keyboard->keys = dvz_list();
    keyboard->callbacks = dvz_list();
    keyboard->mods = 0;
    return keyboard;
}



void dvz_keyboard_press(DvzKeyboard* keyboard, DvzKeyCode key)
{
    ASSERT(keyboard != NULL);
    if (_is_key_modifier(key))
    {
        keyboard->mods |= _key_modifiers(key);
    }
    else
    {
        dvz_list_append(&keyboard->keys, (DvzListItem){.i = (int)key});
    }

    // Create the PRESS event struct.
    DvzKeyboardEvent ev = {.type = DVZ_KEYBOARD_EVENT_PRESS, .mods = keyboard->mods, .key = key};
    // Call the registered callbacks.
    _callbacks(keyboard, ev);
}



void dvz_keyboard_release(DvzKeyboard* keyboard, DvzKeyCode key)
{
    ASSERT(keyboard != NULL);

    if (_is_key_modifier(key))
    {
        keyboard->mods &= ~_key_modifiers(key);
    }
    else
    {
        uint64_t idx = dvz_list_index(&keyboard->keys, (int)key);
        if (idx != UINT64_MAX)
            dvz_list_remove(&keyboard->keys, idx);
    }

    // Create the PRESS event struct.
    DvzKeyboardEvent ev = {.type = DVZ_KEYBOARD_EVENT_RELEASE, .mods = keyboard->mods, .key = key};
    // Call the registered callbacks.
    _callbacks(keyboard, ev);
}



DvzKeyCode dvz_keyboard_get(DvzKeyboard* keyboard, uint32_t key_idx)
{
    ASSERT(keyboard != NULL);
    if (key_idx < keyboard->keys.count)
        return (DvzKeyCode)dvz_list_get(&keyboard->keys, key_idx).i;
    else
        return DVZ_KEY_NONE;
}



bool dvz_keyboard_is_pressed(DvzKeyboard* keyboard, DvzKeyCode key, int mods)
{
    ASSERT(keyboard != NULL);
    return ((keyboard->mods & mods) == mods) && dvz_list_has(&keyboard->keys, key);
}



int dvz_keyboard_mods(DvzKeyboard* keyboard)
{
    ASSERT(keyboard != NULL);
    return keyboard->mods;
}



void dvz_keyboard_callback(
    DvzKeyboard* keyboard, DvzKeyboardEventType type, DvzKeyboardCallback callback,
    void* user_data)
{
    ASSERT(keyboard != NULL);
    DvzKeyboardPayload* payload = (DvzKeyboardPayload*)calloc(1, sizeof(DvzKeyboardPayload));
    payload->type = type;
    payload->callback = callback;
    payload->user_data = user_data;
    dvz_list_append(&keyboard->callbacks, (DvzListItem){.p = (void*)payload});
}



void dvz_keyboard_destroy(DvzKeyboard* keyboard)
{
    ASSERT(keyboard != NULL);

    // Free the callback payloads.
    DvzKeyboardPayload* payload = NULL;
    for (uint32_t i = 0; i < keyboard->callbacks.count; i++)
    {
        payload = (DvzKeyboardPayload*)(dvz_list_get(&keyboard->callbacks, i).p);
        ASSERT(payload != NULL);
        FREE(payload);
    }
    dvz_list_destroy(&keyboard->callbacks);

    dvz_list_destroy(&keyboard->keys);
    FREE(keyboard);
}
