/*************************************************************************************************/
/*  Testing pixel                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/test_pixel.h"
#include "renderer.h"
#include "request.h"
#include "scene/visuals/pixel.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Pixel tests                                                                                  */
/*************************************************************************************************/

int test_pixel_1(TstSuite* suite)
{
    DvzRequester* rqr = dvz_requester();
    dvz_requester_begin(rqr);

    DvzPixel* pixel = dvz_pixel(rqr, 0);

    // Upload the data.
    const uint32_t n = 5000;

    // Position.
    vec3* pos = (vec3*)calloc(n, sizeof(vec3));
    for (uint32_t i = 0; i < n; i++)
    {
        pos[i][0] = .25 * dvz_rand_normal();
        pos[i][1] = .25 * dvz_rand_normal();
    }
    dvz_pixel_position(pixel, 0, n, pos, 0);

    // Color.
    cvec4* color = (cvec4*)calloc(n, sizeof(cvec4));
    for (uint32_t i = 0; i < n; i++)
    {
        dvz_colormap(DVZ_CMAP_HSV, i % n, color[i]);
        color[i][3] = 128;
    }
    dvz_pixel_color(pixel, 0, n, color, 0);



    DvzGpu* gpu = get_gpu(suite);
    ANN(gpu);

    DvzRenderer* rd = dvz_renderer(gpu, 0);

    // Create a boards.
    DvzRequest req = dvz_create_board(rqr, WIDTH, HEIGHT, DVZ_DEFAULT_CLEAR_COLOR, 0);
    DvzId board_id = req.id;
    // dvz_renderer_request(rd, req);

    // Board clear color.
    req = dvz_set_background(rqr, board_id, (cvec4){32, 64, 128, 255});
    // dvz_renderer_request(rd, req);

    // Record commands.
    dvz_record_begin(rqr, board_id);
    dvz_record_viewport(rqr, board_id, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
    dvz_pixel_draw(pixel, board_id, 0, n, 0);
    dvz_record_end(rqr, board_id);

    // Update the board.
    dvz_update_board(rqr, board_id);

    // Execute the requests.
    uint32_t count = 0;
    DvzRequest* reqs = dvz_requester_end(rqr, &count);
    dvz_renderer_requests(rd, count, reqs);

    // Retrieve the image.
    DvzSize size = 0;
    // This pointer will be freed automatically by the renderer.
    uint8_t* rgb = dvz_renderer_image(rd, board_id, &size, NULL);

    // Save to a PNG.
    char imgpath[1024];
    snprintf(imgpath, sizeof(imgpath), "%s/visual_pixel.png", ARTIFACTS_DIR);
    dvz_write_png(imgpath, WIDTH, HEIGHT, rgb);
    AT(!dvz_is_empty(WIDTH * HEIGHT * 3, rgb));

    // Destroy the requester and renderer.
    dvz_renderer_destroy(rd);



    // Cleanup
    dvz_pixel_destroy(pixel);
    dvz_requester_destroy(rqr);
    FREE(pos);
    FREE(color);
    return 0;
}
