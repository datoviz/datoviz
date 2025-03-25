/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Input                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_INPUT
#define DVZ_HEADER_INPUT



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_time_utils.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzInput DvzInput;

// Forward declarations.
typedef struct DvzKeyboard DvzKeyboard;
typedef struct DvzMouse DvzMouse;
typedef struct DvzWindow DvzWindow;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzInput
{
    DvzMouse* mouse;
    DvzKeyboard* keyboard;
    DvzClock clock;
    DvzWindow* window;
};



/*************************************************************************************************/
/*  Window input functions                                                                       */
/*************************************************************************************************/

DvzInput* dvz_input(DvzWindow* window);

DvzMouse* dvz_input_mouse(DvzInput* input);

DvzKeyboard* dvz_input_keyboard(DvzInput* input);

void dvz_input_destroy(DvzInput* input);



/*************************************************************************************************/
/*  Direct glfw input functions                                                                  */
/*************************************************************************************************/

/**
 * Return the last mouse position and pressed button.
 *
 * @param window a window
 * @param[out] x a pointer to the mouse x position
 * @param[out] y a pointer to the mouse y position
 * @param[out] button a pointer to the pressed button
 */
DVZ_EXPORT void dvz_window_mouse(DvzWindow* window, double* x, double* y, DvzMouseButton* button);



/**
 * Return the last keyboard key pressed.
 *
 * @param window a window
 * @param[out] key a pointer to the last pressed key
 */
DVZ_EXPORT void dvz_window_keyboard(DvzWindow* window, DvzKeyCode* key);



#endif
