/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Sampler                                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "../src/vk/macros.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/common/macros.h"
#include "datoviz/common/obj.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/queues.h"
#include "datoviz/vklite/sampler.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void dvz_sampler(DvzDevice* device, DvzSampler* sampler)
{
    ANN(device);
    ANN(sampler);

    dvz_obj_init(&sampler->obj);
    sampler->device = device;
}



void dvz_sampler_min_filter(DvzSampler* sampler, VkFilter filter)
{
    ANN(sampler);
    sampler->min_filter = filter;
}



void dvz_sampler_mag_filter(DvzSampler* sampler, VkFilter filter)
{
    ANN(sampler);
    sampler->mag_filter = filter;
}



void dvz_sampler_address_mode(
    DvzSampler* sampler, DvzSamplerAxis axis, VkSamplerAddressMode address_mode)
{
    ANN(sampler);
    ASSERT(axis <= 2);
    sampler->address_modes[axis] = address_mode;
}



void dvz_sampler_anisotropy(DvzSampler* sampler, float anisotropy)
{
    ANN(sampler);
    ANN(sampler->device);

    VkPhysicalDeviceFeatures* features = dvz_device_request_features10(sampler->device);
    if (anisotropy != 0 && !features->samplerAnisotropy)
    {
        log_warn("unable to set sampler anisotropy because the device was not created with "
                 "samplerAnisotropy Vulkan 1.0 feature");
        return;
    }

    sampler->anisotropy = anisotropy;
}



int dvz_sampler_create(DvzSampler* sampler)
{
    ANN(sampler);

    DvzDevice* device = sampler->device;
    ANN(device);

    log_trace("starting creation of sampler...");

    VkSamplerCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    info.magFilter = sampler->mag_filter;
    info.minFilter = sampler->min_filter;

    info.addressModeU = sampler->address_modes[0];
    info.addressModeV = sampler->address_modes[1];
    info.addressModeW = sampler->address_modes[2];

    info.anisotropyEnable = sampler->anisotropy != 0;
    info.maxAnisotropy = sampler->anisotropy;
    info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    info.unnormalizedCoordinates = VK_FALSE;
    info.compareEnable = VK_FALSE;
    info.compareOp = VK_COMPARE_OP_ALWAYS;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.mipLodBias = 0.0f;
    info.minLod = 0.0f;
    info.maxLod = 0.0f;

    VK_RETURN_RESULT(
        vkCreateSampler(dvz_device_handle(device), &info, NULL, &sampler->vk_sampler));
    if (out == 0)
    {
        dvz_obj_created(&sampler->obj);
        log_trace("sampler created");
    }

    return out;
}



void dvz_sampler_destroy(DvzSampler* sampler)
{
    ANN(sampler);

    DvzDevice* device = sampler->device;
    ANN(device);

    if (!dvz_obj_is_created(&sampler->obj))
    {
        log_trace("skip destruction of already-destroyed sampler");
        return;
    }
    log_trace("destroy sampler");
    if (sampler->vk_sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(dvz_device_handle(device), sampler->vk_sampler, NULL);
        sampler->vk_sampler = VK_NULL_HANDLE;
    }
    dvz_obj_destroyed(&sampler->obj);
}
