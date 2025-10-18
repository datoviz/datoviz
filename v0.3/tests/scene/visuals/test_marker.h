/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_TEST_MARKER
#define DVZ_HEADER_TEST_MARKER



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"



/*************************************************************************************************/
/*  Pixel tests                                                                                  */
/*************************************************************************************************/

int test_marker_code(TstSuite* suite, TstItem* tstitem);

int test_marker_bitmap(TstSuite* suite, TstItem* tstitem);

int test_marker_sdf(TstSuite* suite, TstItem* tstitem);

int test_marker_msdf(TstSuite* suite, TstItem* tstitem);

int test_marker_rotation(TstSuite* suite, TstItem* tstitem);



#endif
