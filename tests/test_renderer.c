/*************************************************************************************************/
/*  Testing renderer                                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_renderer.h"
#include "renderer.h"
#include "test.h"
#include "test_resources.h"
#include "test_vklite.h"
#include "testing.h"



/*************************************************************************************************/
/*  Renderer tests                                                                               */
/*************************************************************************************************/

int test_renderer_1(TstSuite* suite)
{
    DvzGpu* gpu = get_gpu(suite);
    ASSERT(gpu != NULL);

    DvzRenderer* rd = dvz_renderer_offscreen(gpu);
    DvzRequester rqr = dvz_requester();
    DvzRequest rq = {0};

    // Create a board creation request.
    rq = dvz_create_board(&rqr, WIDTH, HEIGHT, 0);
    DvzId id = rq.id;

    // Submit the request to the renderer.
    dvz_renderer_request(rd, rq);

    // Create a board deletion request.
    dvz_delete_board(&rqr, id);

    dvz_renderer_destroy(rd);
    return 0;
}
