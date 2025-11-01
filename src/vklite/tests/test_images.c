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

#include <float.h>
#include <inttypes.h>
#include <stdbool.h>

#include "../../vk/tests/test_vk.h"
#include "../../vk/types.h"
#include "../types.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "datoviz/common/macros.h"
#include "datoviz/vk/bootstrap.h"
#include "datoviz/vk/enums.h"
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
    DvzBootstrap bootstrap = {0};
    dvz_bootstrap(&bootstrap, 0);

    // Images.
    DvzImages images = {0};
    dvz_images(&bootstrap.device, &bootstrap.allocator, VK_IMAGE_TYPE_2D, 1, &images);
    dvz_images_format(&images, VK_FORMAT_R8G8B8A8_UNORM);
    dvz_images_size(&images, 256, 256, 1);
    dvz_images_mip(&images, 1);
    dvz_images_layers(&images, 2);
    dvz_images_samples(&images, VK_SAMPLE_COUNT_1_BIT);
    dvz_images_usage(&images, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    dvz_images_flags(&images, 0);
    dvz_images_create(&images);

    // Image views.
    DvzImageViews views = {0};
    dvz_image_views(&images, &views);
    dvz_image_views_create(&views);

    // Cleanup.
    dvz_image_views_destroy(&views);
    dvz_images_destroy(&images);
    dvz_bootstrap_destroy(&bootstrap);

    RETURN_VALIDATION
}
