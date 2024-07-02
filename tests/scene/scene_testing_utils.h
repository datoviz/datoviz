/*************************************************************************************************/
/*  Testing utils                                                                                */
/*************************************************************************************************/

#ifndef DVZ_HEADER_SCENE_TESTING_UTILS
#define DVZ_HEADER_SCENE_TESTING_UTILS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../testing_utils.h"
#include "_map.h"
#include "board.h"
#include "datoviz.h"
#include "datoviz_math.h"
#include "fileio.h"
#include "scene/visuals/image.h"
#include "scene/visuals/volume.h"
#include "testing.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define MOUSE_W 320
#define MOUSE_H 456
#define MOUSE_D 528



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
    unsigned long png_size = 0;
    unsigned char* png_bytes = dvz_resource_testdata("crate", &png_size);
    ASSERT(png_size > 0);
    uint32_t w = 512;
    ASSERT(png_size == w * w * 4);
    ANN(png_bytes);

    // dvz_write_png("crate.png", WIDTH, HEIGHT, rgb);

    // uint32_t png_width = 0, png_height = 0;
    // uint8_t* crate_data = dvz_load_png(png_size, png_bytes, &png_width, &png_height);
    // ASSERT(png_width > 0);
    // ASSERT(png_height > 0);
    out_shape[0] = w;
    out_shape[1] = w;

    DvzId tex =
        dvz_tex_image(batch, DVZ_FORMAT_R8G8B8A8_UNORM, out_shape[0], out_shape[1], png_bytes);

    // FREE(crate_data);
    return tex;
}



static DvzId load_brain_volume(DvzBatch* batch, uvec3 out_shape, bool use_rgb_volume)
{
    char path[1024];
    snprintf(
        path, sizeof(path), "%s/%s%s.npy", DATA_DIR, "allen_mouse_brain",
        use_rgb_volume ? "_rgba" : "");

    DvzSize size = 0;
    char* volume = dvz_read_npy(path, &size);

    if (!volume)
    {
        log_error("file not found: %s", path);
        return DVZ_ID_NONE;
    }

    log_info("load the Allen Mouse Brain volume (%s)", pretty_size(size));
    DvzFormat format = use_rgb_volume ? DVZ_FORMAT_R8G8B8A8_UNORM : DVZ_FORMAT_R16_UNORM;
    DvzId tex = dvz_tex_volume(batch, format, MOUSE_W, MOUSE_H, MOUSE_D, volume);
    FREE(volume);

    return tex;
}



#endif
