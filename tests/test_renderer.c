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
    DvzRequest req = dvz_request();

    // Create a board creation request.
    dvz_create_board(&req, WIDTH, HEIGHT, 0);

    // Submit the request to the renderer.
    DvzId id = dvz_renderer_request(rd, req);

    // Create a board deletion request.
    dvz_delete_board(&req, id);

    dvz_renderer_destroy(rd);
    return 0;
}
