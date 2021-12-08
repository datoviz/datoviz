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
    DvzMap* map = dvz_map();

    DvzRequest rq = dvz_request();
    dvz_create_canvas(&rq, dvz_map_id(map), 800, 600, 0);
    dvz_request_print(&rq);

    dvz_create_dat(&rq, 1, DVZ_BUFFER_TYPE_VERTEX, 16, 0);
    dvz_create_tex(&rq, 2, 2, (uvec3){2, 4, 3}, DVZ_FORMAT_R8G8B8A8_UNORM, 0);

    dvz_map_destroy(map);
    return 0;
}
