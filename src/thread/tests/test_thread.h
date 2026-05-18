/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing thread                                                                               */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_thread_1(TstContext* suite, const TstCase* tstitem);

int test_mutex_1(TstContext* suite, const TstCase* tstitem);

int test_cond_1(TstContext* suite, const TstCase* tstitem);

int test_atomic_1(TstContext* suite, const TstCase* tstitem);



int test_fifo_1(TstContext* suite, const TstCase* tstitem);

int test_fifo_2(TstContext* suite, const TstCase* tstitem);

int test_fifo_resize(TstContext* suite, const TstCase* tstitem);

int test_fifo_discard(TstContext* suite, const TstCase* tstitem);

int test_fifo_first(TstContext* suite, const TstCase* tstitem);

int test_deq_1(TstContext* suite, const TstCase* tstitem);

int test_deq_2(TstContext* suite, const TstCase* tstitem);

int test_deq_3(TstContext* suite, const TstCase* tstitem);



int test_thread(TstSuite* suite);
