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

int test_prng_1(TstContext* suite, const TstCase* tstitem);



/*************************************************************************************************/
/*  Box tests                                                                                    */
/*************************************************************************************************/

int test_box_1(TstContext* suite, const TstCase* tstitem);

int test_box_2(TstContext* suite, const TstCase* tstitem);

int test_box_3(TstContext* suite, const TstCase* tstitem);

int test_box_4(TstContext* suite, const TstCase* tstitem);

int test_box_5(TstContext* suite, const TstCase* tstitem);

int test_box_6(TstContext* suite, const TstCase* tstitem);



/*************************************************************************************************/
/*  Stat tests                                                                                   */
/*************************************************************************************************/

int test_stats_parallel(TstContext* suite, const TstCase* tstitem);



/*************************************************************************************************/
/*  Anim tests                                                                                   */
/*************************************************************************************************/

int test_anim_1(TstContext* suite, const TstCase* tstitem);



/*************************************************************************************************/
/*  Array tests                                                                                  */
/*************************************************************************************************/

int test_array_1(TstContext* suite, const TstCase* tstitem);

int test_array_2(TstContext* suite, const TstCase* tstitem);

int test_array_3(TstContext* suite, const TstCase* tstitem);

int test_array_4(TstContext* suite, const TstCase* tstitem);

int test_array_5(TstContext* suite, const TstCase* tstitem);

int test_array_6(TstContext* suite, const TstCase* tstitem);

int test_array_7(TstContext* suite, const TstCase* tstitem);

int test_array_cast(TstContext* suite, const TstCase* tstitem);

int test_array_mvp(TstContext* suite, const TstCase* tstitem);

int test_array_3D(TstContext* suite, const TstCase* tstitem);



/*************************************************************************************************/
/*  Math test entry-point                                                                        */
/*************************************************************************************************/

int test_math(TstSuite* suite);
