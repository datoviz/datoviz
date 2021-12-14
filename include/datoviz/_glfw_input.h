/*************************************************************************************************/
/*  Vklite utils                                                                                 */
/*************************************************************************************************/

#ifndef DVZ_HEADER_GLFW_INPUT
#define DVZ_HEADER_GLFW_INPUT



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "common.h"
#include "input.h"
#include "window.h"



/*************************************************************************************************/
/*  Input-window binding                                                                         */
/*************************************************************************************************/

static void backend_attach_mouse(DvzMouse* mouse, DvzWindow* window)
{
    ASSERT(mouse != NULL);
    ASSERT(window != NULL);
}



static void backend_attach_keyboard(DvzKeyboard* keyboard, DvzWindow* window)
{
    ASSERT(keyboard != NULL);
    ASSERT(window != NULL);
}



#endif
