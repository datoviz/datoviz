/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_TEST_TRANSFERS
#define DVZ_HEADER_TEST_TRANSFERS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Transfers tests                                                                              */
/*************************************************************************************************/

int test_transfers_buffer_mappable(TstSuite* suite, TstItem* tstitem);

int test_transfers_buffer_large(TstSuite* suite, TstItem* tstitem);

int test_transfers_buffer_copy(TstSuite* suite, TstItem* tstitem);

int test_transfers_image_buffer(TstSuite* suite, TstItem* tstitem);

int test_transfers_direct_buffer(TstSuite* suite, TstItem* tstitem);

int test_transfers_direct_image(TstSuite* suite, TstItem* tstitem);

int test_transfers_dups_util(TstSuite* suite, TstItem* tstitem);

int test_transfers_dups_upload(TstSuite* suite, TstItem* tstitem);

int test_transfers_dups_copy(TstSuite* suite, TstItem* tstitem);



#endif
