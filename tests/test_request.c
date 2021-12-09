/*************************************************************************************************/
/*  Testing request                                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdio.h>

#include "map.h"
#include "request.h"
#include "test.h"
#include "test_request.h"
#include "testing.h"



/*************************************************************************************************/
/*  Request tests                                                                                */
/*************************************************************************************************/

int test_request_1(TstSuite* suite)
{
    DvzRequester rqr = dvz_requester();
    DvzRequest rq = {0};

    rq = dvz_create_board(&rqr, 800, 600, 0);
    dvz_request_print(&rq);

    rq = dvz_create_dat(&rqr, DVZ_BUFFER_TYPE_VERTEX, 16, 0);
    // rq = dvz_create_tex(&rqr, 2, (uvec3){2, 4, 3}, DVZ_FORMAT_R8G8B8A8_UNORM, 0);

    dvz_requester_destroy(&rqr);
    return 0;
}
