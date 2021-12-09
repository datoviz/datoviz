/*************************************************************************************************/
/*  Testing renderer                                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_renderer.h"
#include "board.h"
#include "fileio.h"
#include "graphics.h"
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
    DvzRequest req = {0};

    // Create a boards.
    req = dvz_create_board(&rqr, WIDTH, HEIGHT, 0);
    dvz_renderer_request(rd, req);
    DvzId board_id = req.id;

    // Create a graphics.
    req = dvz_create_graphics(&rqr, board_id, DVZ_GRAPHICS_TRIANGLE, 0);
    dvz_renderer_request(rd, req);
    DvzId graphics_id = req.id;

    // Create the vertex buffer dat.
    req = dvz_create_dat(&rqr, DVZ_BUFFER_TYPE_VERTEX, 3 * sizeof(DvzVertex), 0);
    dvz_renderer_request(rd, req);
    DvzId dat_id = req.id;

    // Bind the vertex buffer dat to the graphics pipe.
    req = dvz_set_vertex(&rqr, graphics_id, dat_id);
    dvz_renderer_request(rd, req);

    // Upload the triangle data.
    DvzVertex data[] = {
        {{-1, -1, 0}, {255, 0, 0, 255}},
        {{+1, -1, 0}, {0, 255, 0, 255}},
        {{+0, +1, 0}, {0, 0, 255, 255}},
    };
    req = dvz_upload_dat(&rqr, dat_id, 0, sizeof(data), data);
    dvz_renderer_request(rd, req);

    // Commands.
    dvz_requester_begin(&rqr);
    dvz_requester_add(&rqr, dvz_set_begin(&rqr, board_id));
    dvz_requester_add(
        &rqr, dvz_set_viewport(&rqr, board_id, DVZ_VIEWPORT_DEFAULT, DVZ_VIEWPORT_DEFAULT));
    dvz_requester_add(&rqr, dvz_set_draw(&rqr, board_id, graphics_id, 0, 3));
    dvz_requester_add(&rqr, dvz_set_end(&rqr, board_id));
    uint32_t count = 0;
    DvzRequest* reqs = dvz_requester_end(&rqr, &count);
    AT(count > 0);
    dvz_renderer_requests(rd, count, reqs);

    // Render.
    req = dvz_update_board(&rqr, board_id);
    dvz_renderer_request(rd, req);

    // Retrieve the image.
    DvzSize size = 0;
    // This pointer will be freed automatically by the renderer.
    uint8_t* rgba = dvz_renderer_image(rd, board_id, &size, NULL);

    // Save to a PNG.
    char imgpath[1024];
    snprintf(imgpath, sizeof(imgpath), "%s/renderer_1.png", ARTIFACTS_DIR);
    dvz_write_png(imgpath, WIDTH, HEIGHT, rgba);

    // Create a board deletion request.
    req = dvz_delete_board(&rqr, board_id);
    dvz_renderer_request(rd, req);

    // Destroy the renderer.
    dvz_renderer_destroy(rd);
    return 0;
}
