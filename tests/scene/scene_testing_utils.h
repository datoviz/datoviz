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
#include "scene/visuals/image.h"
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
    dvz_renderer_requests(rd, dvz_batch_size(batch), dvz_batch_requests(batch));

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



static DvzId load_crate_texture(DvzBatch* batch, uvec3 out_shape)
{
    unsigned long jpg_size = 0;
    unsigned char* jpg_bytes = dvz_resource_texture("crate", &jpg_size);
    ASSERT(jpg_size > 0);
    ANN(jpg_bytes);

    uint32_t jpg_width = 0, jpg_height = 0;
    uint8_t* crate_data = dvz_read_jpg(jpg_size, jpg_bytes, &jpg_width, &jpg_height);
    ASSERT(jpg_width > 0);
    ASSERT(jpg_height > 0);
    out_shape[0] = jpg_width;
    out_shape[1] = jpg_height;
    out_shape[2] = 1;

    DvzId tex = dvz_tex_image(batch, DVZ_FORMAT_R8G8B8A8_UNORM, jpg_width, jpg_height, crate_data);

    FREE(crate_data);

    return tex;
}



#endif
