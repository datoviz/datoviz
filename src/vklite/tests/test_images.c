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
#include "_alloc.h"
#include "../_images.h"
#include "_assertions.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/vklite/images.h"
#include "test_vklite.h"
#include "testing.h"
#include "vulkan_core.h"

#include <volk.h>



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define MAP_OFFSET 64
#define MAP_SIZE   1024



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_vklite_images_1(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Bootstrap.
    DvzGpuCtxConfig cfg = dvz_testing_gpu_ctx_config(suite);
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



int test_vklite_images_create_requires_destroy(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzGpuCtxConfig cfg = dvz_testing_gpu_ctx_config(suite);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&cfg);
    ANN(ctx);

    DvzImages* images = dvz_images_create_wrapper();
    ANN(images);
    dvz_images(
        dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx), VK_IMAGE_TYPE_2D, 1, images);
    dvz_images_format(images, VK_FORMAT_R8G8B8A8_UNORM);
    dvz_images_mip(images, 1);
    dvz_images_layers(images, 1);
    dvz_images_samples(images, VK_SAMPLE_COUNT_1_BIT);
    dvz_images_usage(images, VK_IMAGE_USAGE_SAMPLED_BIT);

    VkPhysicalDeviceProperties props = {0};
    vkGetPhysicalDeviceProperties(
        dvz_device_physical_device(dvz_gpu_ctx_device(ctx)), &props);
    dvz_images_size(images, props.limits.maxImageDimension2D + 1, 16, 1);

    AT_EXPECTED_ERROR_STRICT(suite, dvz_images_create(images) != 0);
    AT(dvz_image_handle(images, 0) == VK_NULL_HANDLE);

    DvzImageViews* views = dvz_image_views_create_wrapper();
    ANN(views);
    dvz_image_views(images, views);
    tst_expect_error_begin(suite);
    dvz_image_views_create(views);
    AT(tst_expect_error_end(suite) == 0);
    AT(dvz_image_views_handle(views, 0) == VK_NULL_HANDLE);

    dvz_image_views_destroy(views);
    dvz_images_destroy(images);
    AT(dvz_image_handle(images, 0) == VK_NULL_HANDLE);

    dvz_images_size(images, 64, 64, 1);
    AT(dvz_images_create(images) == 0);
    AT(dvz_image_handle(images, 0) != VK_NULL_HANDLE);

    AT_EXPECTED_ERROR_STRICT(suite, dvz_images_create(images) != 0);
    AT(dvz_image_handle(images, 0) != VK_NULL_HANDLE);

    dvz_image_views_create(views);
    AT(dvz_image_views_handle(views, 0) != VK_NULL_HANDLE);
    tst_expect_error_begin(suite);
    dvz_image_views_create(views);
    AT(tst_expect_error_end(suite) == 0);
    AT(dvz_image_views_handle(views, 0) != VK_NULL_HANDLE);

    dvz_image_views_destroy(views);
    dvz_images_destroy(images);
    dvz_image_views_free(views);
    dvz_images_free(images);
    uint32_t err_count = dvz_gpu_ctx_error_count(ctx);
    dvz_gpu_ctx_destroy(ctx);

    return err_count > 0;
}



/**
 * Reject host allocation while exercising image creation unwind.
 *
 * @param count allocation element count
 * @param size allocation element size
 * @return NULL
 */
static void* _images_reject_calloc(DvzSize count, DvzSize size)
{
    (void)count;
    (void)size;
    return NULL;
}



/**
 * Unwind image creation after either the first or a later allocation-wrapper failure.
 *
 * @param suite test context
 * @param item test case
 * @return zero on success
 */
int test_vklite_images_allocation_failure(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzGpuCtxConfig cfg = dvz_testing_gpu_ctx_config(suite);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&cfg);
    ANN(ctx);
    DvzImages* images = dvz_images_create_wrapper();
    ANN(images);
    dvz_images(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx), VK_IMAGE_TYPE_2D, 2, images);
    dvz_images_format(images, VK_FORMAT_R8G8B8A8_UNORM);
    dvz_images_size(images, 16, 16, 1);
    dvz_images_usage(images, VK_IMAGE_USAGE_SAMPLED_BIT);

    const DvzAllocator* previous_allocator = dvz_get_allocator();
    DvzAllocator failing_allocator = *previous_allocator;
    failing_allocator.calloc_fn = _images_reject_calloc;
    for (uint32_t preallocated = 0; preallocated < 2; preallocated++)
    {
        // Reuse an allocation wrapper to let the first image succeed before the second fails.
        if (preallocated != 0)
        {
            images->allocs[0] = dvz_allocation_create();
            ANN(images->allocs[0]);
        }
        dvz_set_allocator(&failing_allocator);
        int result = dvz_images_create(images);
        dvz_set_allocator(previous_allocator);
        AT(result != 0);
        for (uint32_t i = 0; i < 2; i++)
        {
            AT(dvz_image_handle(images, i) == VK_NULL_HANDLE);
            AT(images->allocs[i] == NULL);
        }
        // The same configured images remain reusable after the failed transaction.
        AT(dvz_images_create(images) == 0);
        AT(dvz_image_handle(images, 0) != VK_NULL_HANDLE);
        AT(dvz_image_handle(images, 1) != VK_NULL_HANDLE);
        dvz_images_destroy(images);
        dvz_images_destroy(images);
    }
    dvz_images_free(images);
    uint32_t errors = dvz_gpu_ctx_error_count(ctx);
    dvz_gpu_ctx_destroy(ctx);
    return errors > 0;
}
