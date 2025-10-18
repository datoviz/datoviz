/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

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
#include "canvas.h"
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

static int render_requests(DvzBatch* batch, DvzGpu* gpu, DvzId canvas_id, const char* name)
{
    ANN(batch);
    ANN(gpu);

    DvzRenderer* rd = dvz_renderer(gpu, 0);

    // Update the canvas.
    dvz_update_canvas(batch, canvas_id);

    // Execute the requests.
    dvz_renderer_requests(rd, dvz_batch_size(batch), dvz_batch_requests(batch));

    // Retrieve the image.
    DvzSize size = 0;
    // This pointer will be freed automatically by the renderer.
    uint8_t* rgb = dvz_renderer_image(rd, canvas_id, &size, NULL);

    DvzCanvas* canvas = dvz_renderer_canvas(rd, canvas_id);

    // Save to a PNG.
    char imgpath[1024] = {0};
    snprintf(imgpath, sizeof(imgpath), "%s/%s.png", ARTIFACTS_DIR, name);
    dvz_write_png(imgpath, canvas->width, canvas->height, rgb);
    AT(!dvz_is_empty(canvas->width * canvas->height * 3, rgb));

    // Destroy the requester and renderer.
    dvz_renderer_destroy(rd);

    return 0;
}



static DvzTexture* load_crate_texture(DvzBatch* batch, uvec3 out_shape)
{
    ANN(batch);
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

    DvzTexture* texture = dvz_texture_2D(
        batch, DVZ_FORMAT_R8G8B8A8_UNORM, DVZ_FILTER_LINEAR,
        DVZ_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, out_shape[0], out_shape[1], png_bytes, 0);

    // FREE(crate_data);
    return texture;
}



static DvzTexture* load_brain_volume(DvzBatch* batch, uvec3 out_shape, bool use_rgb_volume)
{
    // Path to the .npy.gz file.
    char path[1024] = {0};
    snprintf(
        path, sizeof(path), "%s/volumes/%s%s.npy.gz", DATA_DIR, "allen_mouse_brain",
        use_rgb_volume ? "_rgba" : "");

    // Decompress the .gz file.
    DvzSize size = 0;
    char* npy_bytes = dvz_read_gz(path, &size);
    if (size == 0 || npy_bytes == NULL)
    {
        log_error("file not found: %s", path);
        return NULL;
    }
    ASSERT(size > 0);

    // Parse the NPY file.
    char* volume = dvz_parse_npy(size, npy_bytes);
    if (volume == NULL)
    {
        log_error("unable to load the volume file: %s", path);
        return NULL;
    }

    log_info("loaded the Allen Mouse Brain volume (%s)", pretty_size(size));
    DvzFormat format = use_rgb_volume ? DVZ_FORMAT_R8G8B8A8_UNORM : DVZ_FORMAT_R16_UNORM;
    DvzTexture* texture = dvz_texture_3D(
        batch, format, DVZ_FILTER_LINEAR, DVZ_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, MOUSE_W,
        MOUSE_H, MOUSE_D, volume, 0);
    FREE(volume);

    return texture;
}



#endif
