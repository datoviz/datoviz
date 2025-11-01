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

#include <float.h>
#include <inttypes.h>
#include <stdbool.h>

#include "../../vk/types.h"
#include "../types.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/common/macros.h"
#include "datoviz/vk/bootstrap.h"
#include "datoviz/vklite/descriptors.h"
#include "datoviz/vklite/images.h"
#include "datoviz/vklite/slots.h"
#include "test_vklite.h"
#include "testing.h"
#include "vulkan_core.h"



/*************************************************************************************************/
/*  Descriptors tests                                                                            */
/*************************************************************************************************/

int test_vklite_descriptors_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Bootstrap.
    DvzBootstrap bootstrap = {0};
    dvz_bootstrap(&bootstrap, 0);

    // Create slots.
    DvzSlots slots = {0};
    dvz_slots(&bootstrap.device, &slots);

    // Bindings.
    dvz_slots_binding(&slots, 0, 0, VK_SHADER_STAGE_ALL, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1);
    dvz_slots_binding(&slots, 0, 1, VK_SHADER_STAGE_ALL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1);
    dvz_slots_binding(&slots, 1, 0, VK_SHADER_STAGE_ALL, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1);

    // Create the slots.
    int res = dvz_slots_create(&slots);
    AT(res == 0);

    // Buffers.
    DvzBuffer ubuf = {0};
    DvzBuffer sbuf = {0};
    DvzSize size = 256;

    dvz_buffer(&bootstrap.device, &bootstrap.allocator, &ubuf);
    dvz_buffer_size(&ubuf, size);
    dvz_buffer_usage(&ubuf, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    dvz_buffer_create(&ubuf);

    dvz_buffer(&bootstrap.device, &bootstrap.allocator, &sbuf);
    dvz_buffer_size(&sbuf, size);
    dvz_buffer_usage(&sbuf, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    dvz_buffer_create(&sbuf);

    // Images.
    DvzImages images = {0};
    dvz_images(&bootstrap.device, &bootstrap.allocator, VK_IMAGE_TYPE_2D, 1, &images);
    dvz_images_format(&images, VK_FORMAT_R8G8B8A8_UNORM);
    dvz_images_size(&images, 256, 256, 1);
    dvz_images_tiling(&images, VK_IMAGE_TILING_OPTIMAL);
    dvz_images_layout(&images, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    dvz_images_mip(&images, 1);
    dvz_images_layers(&images, 2);
    dvz_images_samples(&images, VK_SAMPLE_COUNT_1_BIT);
    dvz_images_usage(&images, VK_IMAGE_USAGE_SAMPLED_BIT);
    dvz_images_flags(&images, 0);
    dvz_images_create(&images);

    // Image views.
    DvzImageViews views = {0};
    dvz_image_views(&images, &views);
    dvz_image_views_create(&views);


    // Descriptors.
    DvzDescriptors desc = {0};
    dvz_descriptors(&slots, &desc);
    dvz_descriptors_buffer(&desc, 0, 0, 0, ubuf.vk_buffer, 0, size);
    dvz_descriptors_buffer(&desc, 1, 0, 0, sbuf.vk_buffer, 0, size);
    dvz_descriptors_image(&desc, 0, 1, 0, views.images->layout, views.vk_views[0], NULL);


    // Cleanup.
    dvz_image_views_destroy(&views);
    dvz_images_destroy(&images);
    dvz_buffer_destroy(&ubuf);
    dvz_buffer_destroy(&sbuf);
    dvz_slots_destroy(&slots);
    dvz_bootstrap_destroy(&bootstrap);
    return 0;
}
