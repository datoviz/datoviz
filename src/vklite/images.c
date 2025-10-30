/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Images                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "../src/vk/macros.h"
#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "datoviz/common/macros.h"
#include "datoviz/common/obj.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/memory.h"
#include "datoviz/vk/queues.h"
#include "datoviz/vklite/images.h"
#include "types.h"
#include "vulkan/vulkan_core.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static int check_image_size(VkPhysicalDeviceProperties* props, VkImageType image_type, uvec3 shape)
{
    ANN(props);


    if (image_type == VK_IMAGE_TYPE_1D && shape[0] > props->limits.maxImageDimension1D)
    {
        log_error(
            "image width %d larger than maximal image dimension supported on the device (%d)",
            shape[0], props->limits.maxImageDimension1D);
        return 1;
    }



    if (image_type == VK_IMAGE_TYPE_2D && shape[0] > props->limits.maxImageDimension2D)
    {
        log_error(
            "image width %d larger than maximal image dimension supported on the device (%d)",
            shape[0], props->limits.maxImageDimension2D);
        return 1;
    }
    if (image_type == VK_IMAGE_TYPE_2D && shape[1] > props->limits.maxImageDimension2D)
    {
        log_error(
            "image height %d larger than maximal image dimension supported on the device (%d)",
            shape[1], props->limits.maxImageDimension2D);
        return 1;
    }



    if (image_type == VK_IMAGE_TYPE_3D && shape[0] > props->limits.maxImageDimension3D)
    {
        log_error(
            "image width %d larger than maximal image dimension supported on the device (%d)",
            shape[0], props->limits.maxImageDimension3D);
        return 1;
    }
    if (image_type == VK_IMAGE_TYPE_3D && shape[1] > props->limits.maxImageDimension3D)
    {
        log_error(
            "image height %d larger than maximal image dimension supported on the device (%d)",
            shape[1], props->limits.maxImageDimension3D);
        return 1;
    }
    if (image_type == VK_IMAGE_TYPE_3D && shape[2] > props->limits.maxImageDimension3D)
    {
        log_error(
            "image height %d larger than maximal image dimension supported on the device (%d)",
            shape[2], props->limits.maxImageDimension3D);
        return 1;
    }
    return 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void dvz_images(
    DvzDevice* device, DvzVma* allocator, VkImageType type, uint32_t count, DvzImages* images)
{
    ANN(device);
    ANN(allocator);
    ANN(images);
    ASSERT(count <= DVZ_MAX_IMAGES);

    images->device = device;
    images->allocator = allocator;
    images->image_type = type;
    images->count = count;
    dvz_obj_init(&images->obj);
}



void dvz_images_format(DvzImages* img, VkFormat format)
{
    ANN(img);
    img->format = format;
}



void dvz_images_layout(DvzImages* img, VkImageLayout layout)
{
    ANN(img);
    img->layout = layout;
}



void dvz_images_size(DvzImages* images, uint32_t width, uint32_t height, uint32_t depth)
{
    ANN(images);
    images->shape[0] = width;
    images->shape[1] = height;
    images->shape[2] = depth;
}



void dvz_images_tiling(DvzImages* img, VkImageTiling tiling)
{
    ANN(img);
    img->tiling = tiling;
}



void dvz_images_usage(DvzImages* images, VkImageUsageFlags usage)
{
    ANN(images);
    // images->req_usage = usage;
    // TODO: delete?
}



void dvz_images_flags(DvzImages* images, VmaAllocationCreateFlags flags)
{
    ANN(images);
    for (uint32_t i = 0; i < images->count; i++)
    {
        images->allocs[i].flags = flags;
    }
}



int dvz_images_create(DvzImages* images)
{
    ANN(images);
    ANN(images->device);

    DvzDevice* device = images->device;
    ANN(device);

    DvzVma* allocator = images->allocator;
    ANN(allocator);

    // Get GPU properties to check the dimensions of the image.
    VkPhysicalDeviceProperties* props = dvz_gpu_properties10(device->gpu);
    if (check_image_size(props, images->image_type, images->shape) != 0)
    {
        log_error("abort image creation");
        return 1;
    }

    VkImageCreateInfo info = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    int out = 0;
    for (uint32_t i = 0; i < images->count; i++)
    {
        out += dvz_allocator_image(
            allocator, &info, images->allocs[i].flags, &images->allocs[i], &images->vk_images[i]);
    }

    dvz_obj_created(&images->obj);

    return out;
}



void dvz_images_destroy(DvzImages* images)
{
    ANN(images);
    ANN(images->device);

    DvzVma* allocator = images->allocator;
    ANN(allocator);


    log_trace("destroying images...");
    for (uint32_t i = 0; i < images->count; i++)
    {
        dvz_allocator_destroy_image(allocator, &images->allocs[i], images->vk_images[i]);
    }
    dvz_obj_destroyed(&images->obj);
    log_trace("images destroyed");
}
