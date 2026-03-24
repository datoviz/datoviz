/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing images                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_vk.h"
#include "_assertions.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/vklite/images.h"
#include "test_vklite.h"
#include "testing.h"
#include "vulkan_core.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define MAP_OFFSET 64
#define MAP_SIZE   1024



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_vklite_images_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Bootstrap.
    DvzGpuCtxConfig cfg = dvz_gpu_ctx_config();
    DvzGpuCtx* ctx = dvz_gpu_ctx(&cfg);
    ANN(ctx);

    // Images.
    DvzImages* images = dvz_images_create_wrapper();
    ANN(images);
    dvz_images(
        dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx), VK_IMAGE_TYPE_2D, 1, images);
    dvz_images_format(images, VK_FORMAT_R8G8B8A8_UNORM);
    dvz_images_size(images, 256, 256, 1);
    dvz_images_mip(images, 1);
    dvz_images_layers(images, 2);
    dvz_images_samples(images, VK_SAMPLE_COUNT_1_BIT);
    dvz_images_usage(images, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    AT(dvz_images_count(images) == 1);
    AT(dvz_images_format_value(images) == VK_FORMAT_R8G8B8A8_UNORM);
    dvz_images_create(images);
    AT(dvz_image_handle(images, 0) != VK_NULL_HANDLE);

    // Image views.
    DvzImageViews* views = dvz_image_views_create_wrapper();
    ANN(views);
    dvz_image_views(images, views);
    dvz_image_views_create(views);
    AT(dvz_image_views_count(views) == 1);
    AT(dvz_image_views_handle(views, 0) != VK_NULL_HANDLE);

    // Cleanup.
    dvz_image_views_destroy(views);
    dvz_image_views_destroy(views);
    dvz_images_destroy(images);
    dvz_images_destroy(images);
    AT(dvz_image_views_handle(views, 0) == VK_NULL_HANDLE);
    AT(dvz_image_handle(images, 0) == VK_NULL_HANDLE);

    AT(dvz_images_create(images) == 0);
    AT(dvz_image_handle(images, 0) != VK_NULL_HANDLE);
    dvz_image_views_create(views);
    AT(dvz_image_views_handle(views, 0) != VK_NULL_HANDLE);
    dvz_image_views_destroy(views);
    dvz_images_destroy(images);
    AT(dvz_image_views_handle(views, 0) == VK_NULL_HANDLE);
    AT(dvz_image_handle(images, 0) == VK_NULL_HANDLE);

    DvzImageCopy* copy = dvz_image_copy_create();
    DvzImageBlit* blit = dvz_image_blit_create();
    ANN(copy);
    ANN(blit);
    dvz_image_copy(copy);
    dvz_image_blit(blit);
    dvz_image_copy_free(copy);
    dvz_image_blit_free(blit);

    dvz_image_views_free(views);
    dvz_images_free(images);
    uint32_t err_count = dvz_gpu_ctx_error_count(ctx);
    dvz_gpu_ctx_destroy(ctx);

    return err_count > 0;
}
