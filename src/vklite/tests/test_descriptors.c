/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing descriptors                                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stddef.h>

#include "test_vk.h"
#include "_assertions.h"
#include "datoviz/math/types.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/vklite/buffers.h"
#include "datoviz/vklite/descriptors.h"
#include "datoviz/vklite/images.h"
#include "datoviz/vklite/rendering.h"
#include "datoviz/vklite/slots.h"
#include "test_vklite.h"
#include "testing.h"
#include "vulkan_core.h"



/*************************************************************************************************/
/*  Descriptors tests                                                                            */
/*************************************************************************************************/

int test_vklite_descriptors_1(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Bootstrap.
    DvzGpuCtxConfig cfg = dvz_testing_gpu_ctx_config(suite);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&cfg);
    ANN(ctx);

    // Create slots.
    DvzSlots* slots = dvz_slots_create_wrapper();
    ANN(slots);
    dvz_slots(dvz_gpu_ctx_device(ctx), slots);

    // Bindings.
    dvz_slots_binding(slots, 0, 0, 1, VK_SHADER_STAGE_ALL, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    dvz_slots_binding(slots, 0, 1, 1, VK_SHADER_STAGE_ALL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    dvz_slots_binding(slots, 1, 0, 1, VK_SHADER_STAGE_ALL, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    // Create the slots.
    int res = dvz_slots_create(slots);
    AT(res == 0);

    // Buffers.
    DvzBuffer* ubuf = dvz_buffer_create_wrapper();
    DvzBuffer* sbuf = dvz_buffer_create_wrapper();
    ANN(ubuf);
    ANN(sbuf);
    DvzSize size = 256;

    dvz_buffer(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx), ubuf);
    dvz_buffer_size(ubuf, size);
    dvz_buffer_usage(ubuf, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    dvz_buffer_create(ubuf);

    dvz_buffer(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx), sbuf);
    dvz_buffer_size(sbuf, size);
    dvz_buffer_usage(sbuf, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    dvz_buffer_create(sbuf);

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
    dvz_images_usage(images, VK_IMAGE_USAGE_SAMPLED_BIT);
    dvz_images_create(images);

    // Image views.
    DvzImageViews* views = dvz_image_views_create_wrapper();
    ANN(views);
    dvz_image_views(images, views);
    dvz_image_views_create(views);


    // Descriptors.
    DvzDescriptors* desc = dvz_descriptors_create_wrapper();
    ANN(desc);
    dvz_descriptors(slots, desc);
    AT(dvz_descriptors_set_count(desc) == 2);
    AT(dvz_descriptors_handle(desc, 0) != VK_NULL_HANDLE);
    AT(dvz_descriptors_handle(desc, 1) != VK_NULL_HANDLE);
    VkDescriptorSet set0 = dvz_descriptors_handle(desc, 0);
    VkDescriptorSet set1 = dvz_descriptors_handle(desc, 1);
    tst_expect_error_begin(suite);
    dvz_descriptors(slots, desc);
    AT(tst_expect_error_end(suite) == 0);
    AT(dvz_descriptors_handle(desc, 0) == set0);
    AT(dvz_descriptors_handle(desc, 1) == set1);
    dvz_descriptors_buffer(desc, 0, 0, 0, dvz_buffer_handle(ubuf), 0, size);
    dvz_descriptors_buffer(desc, 1, 0, 0, dvz_buffer_handle(sbuf), 0, size);
    dvz_descriptors_image(
        desc, 0, 1, 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, dvz_image_views_handle(views, 0),
        NULL);


    // Cleanup.
    dvz_image_views_destroy(views);
    dvz_images_destroy(images);
    dvz_buffer_destroy(ubuf);
    dvz_buffer_destroy(sbuf);
    dvz_image_views_free(views);
    dvz_images_free(images);
    dvz_buffer_free(ubuf);
    dvz_buffer_free(sbuf);
    dvz_descriptors_free(desc);
    for (uint32_t i = 0; i < DVZ_MAX_DESCRIPTOR_SETS + 16; i++)
    {
        DvzDescriptors* recycled = dvz_descriptors_create_wrapper();
        ANN(recycled);
        dvz_descriptors(slots, recycled);
        AT(dvz_descriptors_handle(recycled, 0) != VK_NULL_HANDLE);
        AT(dvz_descriptors_handle(recycled, 1) != VK_NULL_HANDLE);
        dvz_descriptors_free(recycled);
    }
    dvz_slots_destroy(slots);
    dvz_slots_free(slots);
    uint32_t err_count = dvz_gpu_ctx_error_count(ctx);
    dvz_gpu_ctx_destroy(ctx);

    return err_count > 0;
}



int test_vklite_rendering_reset(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzRendering* rendering = dvz_rendering_create_wrapper();
    ANN(rendering);
    AT(dvz_rendering_layer_count(rendering) == 1);
    AT(dvz_rendering_color_count(rendering) == 0);
    AT(!dvz_rendering_has_depth(rendering));
    AT(!dvz_rendering_has_stencil(rendering));

    dvz_rendering_layers(rendering, 3);
    (void)dvz_rendering_color(rendering, 1);
    (void)dvz_rendering_depth(rendering);
    (void)dvz_rendering_stencil(rendering);

    AT(dvz_rendering_layer_count(rendering) == 3);
    AT(dvz_rendering_color_count(rendering) == 2);
    AT(dvz_rendering_has_depth(rendering));
    AT(dvz_rendering_has_stencil(rendering));

    dvz_rendering(rendering);
    AT(dvz_rendering_layer_count(rendering) == 1);
    AT(dvz_rendering_color_count(rendering) == 0);
    AT(!dvz_rendering_has_depth(rendering));
    AT(!dvz_rendering_has_stencil(rendering));
    dvz_rendering_free(rendering);

    return 0;
}
