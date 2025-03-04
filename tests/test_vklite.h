/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_TEST_VKLITE
#define DVZ_HEADER_TEST_VKLITE



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_vklite_commands(TstSuite* suite, TstItem* tstitem);
int test_vklite_buffer_1(TstSuite* suite, TstItem* tstitem);
int test_vklite_buffer_resize(TstSuite* suite, TstItem* tstitem);
int test_vklite_load_shader(TstSuite* suite, TstItem* tstitem);
int test_vklite_compute(TstSuite* suite, TstItem* tstitem);
int test_vklite_push(TstSuite* suite, TstItem* tstitem);
int test_vklite_images(TstSuite* suite, TstItem* tstitem);
int test_vklite_sampler(TstSuite* suite, TstItem* tstitem);
int test_vklite_barrier_buffer(TstSuite* suite, TstItem* tstitem);
int test_vklite_barrier_image(TstSuite* suite, TstItem* tstitem);
int test_vklite_submit(TstSuite* suite, TstItem* tstitem);
int test_vklite_offscreen(TstSuite* suite, TstItem* tstitem);
int test_vklite_shader(TstSuite* suite, TstItem* tstitem);
int test_vklite_surface(TstSuite* suite, TstItem* tstitem);
int test_vklite_swapchain(TstSuite* suite, TstItem* tstitem);
int test_vklite_graphics(TstSuite* suite, TstItem* tstitem);
int test_vklite_indirect(TstSuite* suite, TstItem* tstitem);
int test_vklite_indexed(TstSuite* suite, TstItem* tstitem);
int test_vklite_instanced(TstSuite* suite, TstItem* tstitem);
int test_vklite_vertex_bindings(TstSuite* suite, TstItem* tstitem);
int test_vklite_constattr(TstSuite* suite, TstItem* tstitem);
int test_vklite_specialization(TstSuite* suite, TstItem* tstitem);
int test_vklite_sync_full(TstSuite* suite, TstItem* tstitem);
int test_vklite_sync_fail(TstSuite* suite, TstItem* tstitem);



#endif
