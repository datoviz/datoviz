/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_TEST_RESOURCES
#define DVZ_HEADER_TEST_RESOURCES



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../src/resources_utils.h"
#include "test.h"
#include "testing.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Resources tests                                                                              */
/*************************************************************************************************/

int test_resources_1(TstSuite* suite, TstItem* tstitem);

int test_resources_dat_1(TstSuite* suite, TstItem* tstitem);

int test_resources_tex_1(TstSuite* suite, TstItem* tstitem);



/*************************************************************************************************/
/*  Resources data transfers tests                                                               */
/*************************************************************************************************/

int test_resources_dat_transfers(TstSuite* suite, TstItem* tstitem);

int test_resources_dat_resize(TstSuite* suite, TstItem* tstitem);

int test_resources_tex_transfers(TstSuite* suite, TstItem* tstitem);

int test_resources_tex_resize(TstSuite* suite, TstItem* tstitem);



#endif
