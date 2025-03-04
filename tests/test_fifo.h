/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_TEST_FIFO
#define DVZ_HEADER_TEST_FIFO



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"



/*************************************************************************************************/
/*  FIFO tests                                                                                   */
/*************************************************************************************************/

int test_fifo_1(TstSuite* suite, TstItem* tstitem);

int test_fifo_2(TstSuite* suite, TstItem* tstitem);

int test_fifo_resize(TstSuite* suite, TstItem* tstitem);

int test_fifo_discard(TstSuite* suite, TstItem* tstitem);

int test_fifo_first(TstSuite* suite, TstItem* tstitem);



/*************************************************************************************************/
/*  Deq tests                                                                                    */
/*************************************************************************************************/

int test_deq_1(TstSuite* suite, TstItem* tstitem);

int test_deq_2(TstSuite* suite, TstItem* tstitem);

int test_deq_3(TstSuite* suite, TstItem* tstitem);



#endif
