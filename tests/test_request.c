/*************************************************************************************************/
/*  Testing request                                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdio.h>

#include "_map.h"
#include "request.h"
#include "test.h"
#include "test_request.h"
#include "testing.h"



/*************************************************************************************************/
/*  Request tests                                                                                */
/*************************************************************************************************/

int test_request_1(TstSuite* suite)
{
    DvzBatch* batch = dvz_batch();
    DvzRequest req = {0};

    req = dvz_create_board(batch, 800, 600, DVZ_DEFAULT_CLEAR_COLOR, 0);
    dvz_request_print(&req);

    req = dvz_create_dat(batch, DVZ_BUFFER_TYPE_VERTEX, 16, 0);
    req = dvz_create_tex(batch, DVZ_TEX_2D, DVZ_FORMAT_R8G8B8A8_UNORM, (uvec3){2, 4, 1}, 0);

    dvz_batch_destroy(batch);
    return 0;
}



int test_request_2(TstSuite* suite)
{
    DvzBatch* batch = dvz_batch();

    dvz_create_board(batch, 800, 600, DVZ_DEFAULT_CLEAR_COLOR, 0);
    dvz_create_dat(batch, DVZ_BUFFER_TYPE_VERTEX, 16, 0);

    DvzRequest* reqs = dvz_batch_requests(batch);
    uint32_t count = dvz_batch_size(batch);
    AT(count == 2);
    AT(reqs != NULL);
    dvz_request_print(&reqs[0]);
    dvz_request_print(&reqs[1]);

    dvz_batch_destroy(batch);
    return 0;
}



int test_batch_1(TstSuite* suite)
{
    DvzBatch* batch = dvz_batch();
    DvzRequest req = dvz_create_board(batch, 800, 600, DVZ_DEFAULT_CLEAR_COLOR, 0);
    dvz_batch_add(batch, req);
    AT(dvz_batch_size(batch) == 2);

    DvzRequest* reqs = dvz_batch_requests(batch);
    uint32_t count = dvz_batch_size(batch);
    AT(count == 2);
    AT(reqs != NULL);
    AT(memcmp(reqs, &req, sizeof(DvzRequest)) == 0);
    AT(memcmp(&reqs[1], &req, sizeof(DvzRequest)) == 0);

    dvz_batch_destroy(batch);
    return 0;
}
