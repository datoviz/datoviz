/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing request                                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdio.h>

#include "_map.h"
#include "datoviz_defaults.h"
#include "datoviz_protocol.h"
#include "test.h"
#include "test_request.h"
#include "testing.h"



/*************************************************************************************************/
/*  Request tests                                                                                */
/*************************************************************************************************/

int test_request_1(TstSuite* suite, TstItem* tstitem)
{
    DvzBatch* batch = dvz_batch();

    DvzRequest req1 =
        dvz_create_canvas(batch, 800, 600, DVZ_DEFAULT_CLEAR_COLOR, DVZ_APP_FLAGS_OFFSCREEN);
    DvzRequest req2 = dvz_create_dat(batch, DVZ_BUFFER_TYPE_VERTEX, 16, 0);
    DvzRequest req3 =
        dvz_create_tex(batch, DVZ_TEX_2D, DVZ_FORMAT_R8G8B8A8_UNORM, (uvec3){2, 4, 1}, 0);

    DvzRequest* reqs = dvz_batch_requests(batch);
    uint32_t count = dvz_batch_size(batch);
    AT(count == 3);
    AT(reqs != NULL);
    dvz_batch_print(batch, 0);

    AT(memcmp(&reqs[0], &req1, sizeof(DvzRequest)) == 0);
    AT(memcmp(&reqs[1], &req2, sizeof(DvzRequest)) == 0);
    AT(memcmp(&reqs[2], &req3, sizeof(DvzRequest)) == 0);

    DvzBatch* cpy = dvz_batch_copy(batch);
    AT(cpy != batch);
    AT(cpy->requests != batch->requests);
    AT(memcmp(cpy->requests, batch->requests, batch->count * sizeof(DvzRequest)) == 0);

    dvz_batch_destroy(batch);
    return 0;
}



int test_requester_1(TstSuite* suite, TstItem* tstitem)
{
    // Create a requester.
    DvzRequester* rqr = dvz_requester();

    // Create a batch of requests.
    DvzBatch* batch = dvz_batch();
    DvzRequest req1 =
        dvz_create_canvas(batch, 800, 600, DVZ_DEFAULT_CLEAR_COLOR, DVZ_APP_FLAGS_OFFSCREEN);
    DvzRequest req2 = dvz_create_dat(batch, DVZ_BUFFER_TYPE_VERTEX, 16, 0);
    AT(dvz_batch_size(batch) == 2);

    // Commit the batch to the requester.
    dvz_requester_commit(rqr, batch);

    uint32_t count = 0;
    DvzBatch* batches = dvz_requester_flush(rqr, &count);
    AT(count == 1);
    AT(batches != NULL);
    AT(memcmp(&batches[0].requests[0], &req1, sizeof(DvzRequest)) == 0);
    AT(memcmp(&batches[0].requests[1], &req2, sizeof(DvzRequest)) == 0);

    dvz_batch_destroy(batch);
    return 0;
}
