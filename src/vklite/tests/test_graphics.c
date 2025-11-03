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
#include "datoviz/vklite/shader.h"
#include "datoviz/vklite/slots.h"
#include "test_vklite.h"
#include "testing.h"
#include "vulkan_core.h"



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

    DvzDevice* device = &bootstrap.device;

    // Create a device with support for dynamic rendering.
    VkPhysicalDeviceVulkan13Features* features = dvz_device_request_features13(device);
    features->dynamicRendering = true;
    dvz_device_create(device);

    // Graphics setup.
    DvzGraphics graphics = {0};
    dvz_graphics(device, &graphics);

    DvzShader vs = {0};
    DvzShader fs = {0};
    // dvz_shader(device, vs_size, vs_spirv, &vs);
    // dvz_shader(device, fs_size, fs_spirv, &fs);

    dvz_graphics_shader(&graphics, VK_SHADER_STAGE_VERTEX_BIT, vs.vk_shader);
    dvz_graphics_shader(&graphics, VK_SHADER_STAGE_FRAGMENT_BIT, fs.vk_shader);
    dvz_graphics_attachment_color(&graphics, 0, VK_FORMAT_R8G8B8A8_UNORM);
    dvz_graphics_primitive(
        &graphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, DVZ_GRAPHICS_FLAGS_FIXED);
    dvz_graphics_viewport(&graphics, 0, 0, 800, 600, 0, 1, DVZ_GRAPHICS_FLAGS_DYNAMIC);

    // Graphics creation.
    dvz_graphics_create(&graphics);

    // Cleanup.
    dvz_graphics_destroy(&graphics);
    dvz_bootstrap_destroy(&bootstrap);

    RETURN_VALIDATION
}
