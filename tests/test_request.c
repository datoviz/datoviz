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
    DvzRequester* rqr = dvz_requester();
    DvzRequest req = {0};

    req = dvz_create_board(rqr, 800, 600, DVZ_DEFAULT_CLEAR_COLOR, 0);
    dvz_request_print(&req);

    req = dvz_create_dat(rqr, DVZ_BUFFER_TYPE_VERTEX, 16, 0);
    req = dvz_create_tex(rqr, DVZ_TEX_2D, DVZ_FORMAT_R8G8B8A8_UNORM, (uvec3){2, 4, 1}, 0);

    dvz_requester_destroy(rqr);
    return 0;
}



int test_request_2(TstSuite* suite)
{
    DvzRequester* rqr = dvz_requester();

    dvz_requester_begin(rqr);
    dvz_create_board(rqr, 800, 600, DVZ_DEFAULT_CLEAR_COLOR, 0);
    dvz_create_dat(rqr, DVZ_BUFFER_TYPE_VERTEX, 16, 0);

    uint32_t count = 0;
    DvzRequest* reqs = dvz_requester_end(rqr, &count);
    AT(count == 2);
    AT(reqs != NULL);
    dvz_request_print(&reqs[0]);
    dvz_request_print(&reqs[1]);

    dvz_requester_destroy(rqr);
    return 0;
}
