/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_TEST_MOUSE
#define DVZ_HEADER_TEST_MOUSE



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test.h"
#include "testing.h"



/*************************************************************************************************/
/*  Mouse tests                                                                                  */
/*************************************************************************************************/

int test_mouse_move(TstSuite* suite, TstItem* tstitem);

int test_mouse_press(TstSuite* suite, TstItem* tstitem);

int test_mouse_wheel(TstSuite* suite, TstItem* tstitem);

int test_mouse_drag(TstSuite* suite, TstItem* tstitem);



#endif
