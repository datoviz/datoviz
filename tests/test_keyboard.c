/*************************************************************************************************/
/*  Testing keyboard                                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_keyboard.h"
#include "keyboard.h"
#include "test.h"
#include "testing.h"



/*************************************************************************************************/
/*  Keyboard tests                                                                               */
/*************************************************************************************************/

// static void _on_key_press(DvzInput* input, DvzEvent ev, void* user_data)
// {
//     ASSERT(input != NULL);
//     log_debug("key press %d, modifiers %d", ev.content.k.key_code, ev.mods);
//     DvzKeyCode* k = input->keyboard.keys;
//     log_debug("%d key(s) pressed: %d %d %d %d", input->keyboard.key_count, k[0], k[1], k[2],
//     k[3]); if (user_data != NULL)
//     {
//         *((int*)user_data) = ev.content.k.key_code;
//     }
// }

// static void _on_key_release(DvzInput* input, DvzEvent ev, void* user_data)
// {
//     ASSERT(input != NULL);
//     log_debug("key release %d", ev.content.k.key_code);
//     if (user_data != NULL)
//     {
//         *((int*)user_data) = DVZ_KEY_NONE;
//     }
// }

int test_keyboard_1(TstSuite* suite)
{
    // Create an input and window.
    DvzKeyboard* keyboard = dvz_keyboard();

    AT(dvz_keyboard_get(keyboard, 0) == DVZ_KEY_NONE);

    dvz_keyboard_press(keyboard, DVZ_KEY_A, DVZ_KEY_LEFT_CONTROL);
    AT(dvz_keyboard_get(keyboard, 0) == DVZ_KEY_A);
    AT(dvz_keyboard_get(keyboard, 1) == DVZ_KEY_NONE);

    dvz_keyboard_release(keyboard, DVZ_KEY_A);
    AT(dvz_keyboard_get(keyboard, 0) == DVZ_KEY_NONE);
    AT(dvz_keyboard_get(keyboard, 1) == DVZ_KEY_NONE);

    // Destroy the resources.
    dvz_keyboard_destroy(keyboard);
    return 0;
}
