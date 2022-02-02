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

    DvzRenderer* rd = dvz_renderer(gpu, 0);
    DvzRequester rqr = dvz_requester();
    DvzRequest req = {0};

    // Create a boards.
    req = dvz_create_board(&rqr, WIDTH, HEIGHT, DVZ_DEFAULT_CLEAR_COLOR, 0);
    dvz_renderer_request(rd, req);
    DvzId board_id = req.id;

    // Board clear color.
    req = dvz_set_background(&rqr, board_id, (cvec4){32, 64, 128, 255});
    dvz_renderer_request(rd, req);

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

    // Binding #0: MVP.
    req = dvz_create_dat(&rqr, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzMVP), 0);
    dvz_renderer_request(rd, req);
    DvzId mvp_id = req.id;

    req = dvz_bind_dat(&rqr, graphics_id, 0, mvp_id);
    dvz_renderer_request(rd, req);

    DvzMVP mvp = dvz_mvp_default();
    // dvz_show_base64(sizeof(mvp), &mvp);
    req = dvz_upload_dat(&rqr, mvp_id, 0, sizeof(DvzMVP), &mvp);
    dvz_renderer_request(rd, req);

    // Binding #1: viewport.
    req = dvz_create_dat(&rqr, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzViewport), 0);
    dvz_renderer_request(rd, req);
    DvzId viewport_id = req.id;

    req = dvz_bind_dat(&rqr, graphics_id, 1, viewport_id);
    dvz_renderer_request(rd, req);

    DvzViewport viewport = dvz_viewport_default(WIDTH, HEIGHT);
    // dvz_show_base64(sizeof(viewport), &viewport);
    req = dvz_upload_dat(&rqr, viewport_id, 0, sizeof(DvzViewport), &viewport);
    dvz_renderer_request(rd, req);

    // Commands.
    dvz_requester_begin(&rqr);
    dvz_requester_add(&rqr, dvz_record_begin(&rqr, board_id));
    dvz_requester_add(
        &rqr, dvz_record_viewport(&rqr, board_id, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT));
    dvz_requester_add(&rqr, dvz_record_draw(&rqr, board_id, graphics_id, 0, 3));
    dvz_requester_add(&rqr, dvz_record_end(&rqr, board_id));
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
    uint8_t* rgb = dvz_renderer_image(rd, board_id, &size, NULL);

    // Save to a PNG.
    char imgpath[1024];
    snprintf(imgpath, sizeof(imgpath), "%s/renderer_1.png", ARTIFACTS_DIR);
    dvz_write_png(imgpath, WIDTH, HEIGHT, rgb);

    // Create a board deletion request.
    req = dvz_delete_board(&rqr, board_id);
    dvz_renderer_request(rd, req);

    // Destroy the renderer.
    dvz_renderer_destroy(rd);
    return 0;
}



int test_renderer_resize(TstSuite* suite)
{
    DvzGpu* gpu = get_gpu(suite);
    ASSERT(gpu != NULL);

    DvzRenderer* rd = dvz_renderer(gpu, 0);
    DvzRequester rqr = dvz_requester();
    DvzRequest req = {0};



    // Create a boards.
    req = dvz_create_board(&rqr, WIDTH / 2, HEIGHT / 2, DVZ_DEFAULT_CLEAR_COLOR, 0);
    dvz_renderer_request(rd, req);
    DvzId board_id = req.id;

    // Resize the board.
    req = dvz_resize_board(&rqr, board_id, WIDTH, HEIGHT);
    dvz_renderer_request(rd, req);

    // Check board resizing.
    DvzBoard* board = dvz_renderer_board(rd, board_id);
    AT(board->width == WIDTH);
    AT(board->height == HEIGHT);



    // Create a dat.
    req = dvz_create_dat(&rqr, DVZ_BUFFER_TYPE_VERTEX, 16, 0);
    dvz_renderer_request(rd, req);
    DvzId dat_id = req.id;

    // Resize the dat.
    req = dvz_resize_dat(&rqr, dat_id, 32);
    dvz_renderer_request(rd, req);

    // Check dat resizing.
    DvzDat* dat = dvz_renderer_dat(rd, dat_id);
    AT(dat->br.size == 32);



    // Create a tex.
    req =
        dvz_create_tex(&rqr, DVZ_TEX_3D, DVZ_FORMAT_R32_UINT, 2 * 3 * 4 * 4, (uvec3){2, 3, 4}, 0);
    dvz_renderer_request(rd, req);
    DvzId tex_id = req.id;

    // Resize the tex.
    req = dvz_resize_tex(&rqr, tex_id, 20 * 30 * 40 * 4, (uvec3){20, 30, 40});
    dvz_renderer_request(rd, req);

    // Check tex resizing.
    DvzTex* tex = dvz_renderer_tex(rd, tex_id);
    AT(tex->shape[0] == 20);
    AT(tex->shape[1] == 30);
    AT(tex->shape[2] == 40);



    // Destroy the renderer.
    dvz_renderer_destroy(rd);
    return 0;
}
