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

int test_vklite_host(TstSuite*);
int test_vklite_commands(TstSuite*);
int test_vklite_buffer_1(TstSuite*);
int test_vklite_buffer_resize(TstSuite*);
int test_vklite_load_shader(TstSuite*);
int test_vklite_compute(TstSuite*);
int test_vklite_push(TstSuite*);
int test_vklite_images(TstSuite*);
int test_vklite_sampler(TstSuite*);
int test_vklite_barrier_buffer(TstSuite*);
int test_vklite_barrier_image(TstSuite*);
int test_vklite_submit(TstSuite*);
int test_vklite_offscreen(TstSuite*);
int test_vklite_shader(TstSuite*);
int test_vklite_surface(TstSuite*);
int test_vklite_swapchain(TstSuite*);
int test_vklite_graphics(TstSuite*);
int test_vklite_indirect(TstSuite*);
int test_vklite_indexed(TstSuite*);
int test_vklite_instanced(TstSuite*);
int test_vklite_vertex_bindings(TstSuite*);
int test_vklite_constattr(TstSuite*);
int test_vklite_specialization(TstSuite*);
int test_vklite_sync_full(TstSuite*);
int test_vklite_sync_fail(TstSuite*);



#endif
