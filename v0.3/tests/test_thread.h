/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_TEST_THREAD
#define DVZ_HEADER_TEST_THREAD



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_thread_1(TstSuite* suite, TstItem* tstitem);

int test_mutex_1(TstSuite* suite, TstItem* tstitem);

int test_cond_1(TstSuite* suite, TstItem* tstitem);

int test_atomic_1(TstSuite* suite, TstItem* tstitem);



#endif
