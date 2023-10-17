/*************************************************************************************************/
/*  Testing utils                                                                                */
/*************************************************************************************************/

#ifndef DVZ_HEADER_SCENE_TESTING_UTILS
#define DVZ_HEADER_SCENE_TESTING_UTILS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../testing_utils.h"
#include "_math.h"
#include "board.h"
#include "fileio.h"
#include "testing.h"



/*************************************************************************************************/
/*  Visual tests                                                                                 */
/*************************************************************************************************/

static int render_requests(DvzBatch* batch, DvzGpu* gpu, DvzId board, const char* name)
{
    ANN(batch);
    ANN(gpu);

    DvzRenderer* rd = dvz_renderer(gpu, 0);

    // Update the board.
    dvz_update_board(batch, board);

    // Execute the requests.
    // uint32_t count = 0;
    // DvzRequest* reqs = dvz_requester_end(batch, &count);
    // dvz_renderer_requests(rd, count, reqs);

    // Retrieve the image.
    DvzSize size = 0;
    // This pointer will be freed automatically by the renderer.
    uint8_t* rgb = dvz_renderer_image(rd, board, &size, NULL);

    DvzBoard* b = dvz_renderer_board(rd, board);

    // Save to a PNG.
    char imgpath[1024];
    snprintf(imgpath, sizeof(imgpath), "%s/%s.png", ARTIFACTS_DIR, name);
    dvz_write_png(imgpath, b->width, b->height, rgb);
    AT(!dvz_is_empty(b->width * b->height * 3, rgb));

    // Destroy the requester and renderer.
    dvz_renderer_destroy(rd);

    return 0;
}



#endif
