/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing Dat alloc                                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdio.h>

#include "context.h"
#include "datalloc.h"
#include "test.h"
#include "test_datalloc.h"
#include "test_resources.h"
#include "testing.h"



/*************************************************************************************************/
/*  Alloc tests                                                                                  */
/*************************************************************************************************/

int test_datalloc_1(TstSuite* suite, TstItem* tstitem)
{
    GET_HOST_GPU

    // Create the resources object.
    DvzContext* ctx = dvz_context(gpu);

    DvzSize alignment = 0;
    DvzSize size = 128;

    // 2 allocations in the staging buffer.
    DvzDat* dat = dvz_dat(ctx, DVZ_BUFFER_TYPE_STAGING, size, 0);
    ANN(dat);
    log_trace("[128 | ---");
    AT(dat->br.offsets[0] == 0);
    AT(dat->br.size == size);

    // Get the buffer alignment.
    alignment = dat->br.buffer->vma.alignment;

    DvzDat* dat_1 = dvz_dat(ctx, DVZ_BUFFER_TYPE_STAGING, size, 0);
    ANN(dat_1);
    log_trace("[128 | 128 | ---");
    AT(dat_1->br.offsets[0] == _align(size, alignment));
    AT(dat_1->br.size == size);

    // Resize the second buffer.
    DvzSize new_size = 192;
    dvz_dat_resize(dat_1, new_size);
    log_trace("[128 | 192 | ---");
    // The offset should be the same, just the size should change.
    AT(dat_1->br.offsets[0] == _align(size, alignment));
    AT(dat_1->br.size == new_size);

    // 1 allocation in the vertex buffer.
    DvzDat* dat_2 = dvz_dat(ctx, DVZ_BUFFER_TYPE_VERTEX, size, 0);
    ANN(dat_2);
    AT(dat_2->br.offsets[0] == 0);
    AT(dat_2->br.size == size);


    // Delete the first staging allocation.
    log_trace("[--- | 192 | ---");
    dvz_dat_destroy(dat);

    // New allocation in the staging buffer.
    DvzDat* dat_3 = dvz_dat(ctx, DVZ_BUFFER_TYPE_STAGING, size, 0);
    ANN(dat_3);
    log_trace("[128 | 192 | ---");
    AT(dat_3->br.offsets[0] == 0);
    AT(dat_3->br.size == size);

    // Resize the lastly-created buffer, we should be in the first position.
    dvz_dat_resize(dat_3, size / 2);
    log_trace("[64 | 192 | ---");
    AT(dat_3->br.offsets[0] == 0);
    AT(dat_3->br.size == size / 2);

    // Resize the lastly-created buffer, we should now get a new position.
    new_size = 1024;
    dvz_dat_resize(dat_3, new_size);
    log_trace("[--- | 192 | 1024 | ---");
    AT(dat_3->br.offsets[0] >= 128 + 192);
    AT(dat_3->br.size == new_size);

    // dvz_datalloc_stats(&ctx->datalloc);

    // NOTE: resources destruction MUST occur before datalloc destruction.
    // dvz_resources_destroy(&res);
    // dvz_datalloc_destroy(&datalloc);
    dvz_context_destroy(ctx);
    return 0;
}



int test_datalloc_2(TstSuite* suite, TstItem* tstitem)
{
    GET_HOST_GPU

    // Create the resources object.
    DvzContext* ctx = dvz_context(gpu);

    DvzDat* dat_1 = dvz_dat(ctx, DVZ_BUFFER_TYPE_STAGING, 64, 0);
    ANN(dat_1);
    // log_error("%d", dat_1->br.offsets[0]);

    // Resize the buffer.
    dvz_dat_resize(dat_1, 128);
    // log_error("%d", dat_1->br.offsets[0]);

    // Resize the buffer.
    dvz_dat_resize(dat_1, 256);
    // log_error("%d", dat_1->br.offsets[0]);

    DvzDat* dat_2 = dvz_dat(ctx, DVZ_BUFFER_TYPE_STAGING, 64, 0);
    ANN(dat_2);

    dvz_dat_resize(dat_1, 64);
    dvz_dat_resize(dat_1, 128);
    dvz_dat_resize(dat_1, 256);
    dvz_dat_resize(dat_1, 512);

    log_info("destroy");

    // NOTE: resources destruction MUST occur before datalloc destruction.
    dvz_context_destroy(ctx);
    return 0;
}
