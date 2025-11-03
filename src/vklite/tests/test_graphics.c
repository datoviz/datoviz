/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing graphics                                                                             */
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
#include "_log.h"
#include "datoviz/common/macros.h"
#include "datoviz/vk/bootstrap.h"
#include "datoviz/vk/device.h"
#include "datoviz/vklite/graphics.h"
#include "datoviz/vklite/images.h"
#include "datoviz/vklite/slots.h"
#include "test_vklite.h"
#include "testing.h"



/*************************************************************************************************/
/*  Graphics tests                                                                            */
/*************************************************************************************************/

int test_vklite_graphics_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Bootstrap.
    DvzBootstrap bootstrap = {0};
    dvz_bootstrap(&bootstrap, DVZ_BOOTSTRAP_MANUAL_CREATE_DEVICE);

    // Create a device with support for dynamic rendering.
    VkPhysicalDeviceVulkan13Features* features = dvz_device_request_features13(&bootstrap.device);
    features->dynamicRendering = true;
    dvz_device_create(&bootstrap.device);

    dvz_bootstrap_destroy(&bootstrap);

    RETURN_VALIDATION
}
