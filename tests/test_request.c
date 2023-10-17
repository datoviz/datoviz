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

    DvzRequest req1 = dvz_create_board(batch, 800, 600, DVZ_DEFAULT_CLEAR_COLOR, 0);
    DvzRequest req2 = dvz_create_dat(batch, DVZ_BUFFER_TYPE_VERTEX, 16, 0);
    DvzRequest req3 =
        dvz_create_tex(batch, DVZ_TEX_2D, DVZ_FORMAT_R8G8B8A8_UNORM, (uvec3){2, 4, 1}, 0);

    DvzRequest* reqs = dvz_batch_requests(batch);
    uint32_t count = dvz_batch_size(batch);
    AT(count == 3);
    AT(reqs != NULL);
    dvz_batch_print(batch);

    AT(memcmp(&reqs[0], &req1, sizeof(DvzRequest)) == 0);
    AT(memcmp(&reqs[1], &req2, sizeof(DvzRequest)) == 0);
    AT(memcmp(&reqs[2], &req3, sizeof(DvzRequest)) == 0);

    dvz_batch_destroy(batch);
    return 0;
}
