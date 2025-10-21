/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing math                                                                                 */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"



/*************************************************************************************************/
/*  Random tests                                                                                 */
/*************************************************************************************************/

int test_prng_1(TstSuite* suite, TstItem* tstitem);



/*************************************************************************************************/
/*  Box tests                                                                                    */
/*************************************************************************************************/

int test_box_1(TstSuite* suite, TstItem* tstitem);

int test_box_2(TstSuite* suite, TstItem* tstitem);

int test_box_3(TstSuite* suite, TstItem* tstitem);

int test_box_4(TstSuite* suite, TstItem* tstitem);

int test_box_5(TstSuite* suite, TstItem* tstitem);

int test_box_6(TstSuite* suite, TstItem* tstitem);



/*************************************************************************************************/
/*  Stat tests                                                                                   */
/*************************************************************************************************/

int test_stats_parallel(TstSuite* suite, TstItem* tstitem);



/*************************************************************************************************/
/*  Anim tests                                                                                   */
/*************************************************************************************************/

int test_anim_1(TstSuite* suite, TstItem* tstitem);



/*************************************************************************************************/
/*  Array tests                                                                                  */
/*************************************************************************************************/

int test_array_1(TstSuite* suite, TstItem* tstitem);

int test_array_2(TstSuite* suite, TstItem* tstitem);

int test_array_3(TstSuite* suite, TstItem* tstitem);

int test_array_4(TstSuite* suite, TstItem* tstitem);

int test_array_5(TstSuite* suite, TstItem* tstitem);

int test_array_6(TstSuite* suite, TstItem* tstitem);

int test_array_7(TstSuite* suite, TstItem* tstitem);

int test_array_cast(TstSuite* suite, TstItem* tstitem);

int test_array_mvp(TstSuite* suite, TstItem* tstitem);

int test_array_3D(TstSuite* suite, TstItem* tstitem);



/*************************************************************************************************/
/*  Math test entry-point                                                                        */
/*************************************************************************************************/

int test_math(TstSuite* suite);
