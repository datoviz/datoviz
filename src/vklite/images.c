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

#include <stddef.h>
#include <stdint.h>
#include <volk.h>

#include "../src/vk/macros.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/common/obj.h"
#include "datoviz/math/types.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/instance.h"
#include "datoviz/vk/memory.h"
#include "datoviz/vklite/commands.h"
#include "datoviz/vklite/images.h"



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
/*  Images                                                                                       */
/*************************************************************************************************/

void dvz_images(
    DvzDevice* device, DvzVma* allocator, VkImageType type, uint32_t count, DvzImages* img)
{
    ANN(device);
    ANN(allocator);
    ANN(img);
    ASSERT(count <= DVZ_MAX_IMAGES);

    img->device = device;
    img->allocator = allocator;
    img->count = count;

    // Default values.
    img->info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img->info.imageType = type;
    img->info.mipLevels = 1;
    img->info.arrayLayers = 1;
    img->info.samples = VK_SAMPLE_COUNT_1_BIT;

    dvz_obj_init(&img->obj);
}



void dvz_images_format(DvzImages* img, VkFormat format)
{
    ANN(img);
    img->info.format = format;
}



void dvz_images_size(DvzImages* img, uint32_t width, uint32_t height, uint32_t depth)
{
    ANN(img);
    img->info.extent.width = width;
    img->info.extent.height = height;
    img->info.extent.depth = depth;
}



void dvz_images_tiling(DvzImages* img, VkImageTiling tiling)
{
    ANN(img);
    img->info.tiling = tiling;
}



void dvz_images_usage(DvzImages* img, VkImageUsageFlags usage)
{
    ANN(img);
    img->info.usage = usage;
}



void dvz_images_vma_flags(DvzImages* img, VmaAllocationCreateFlags flags)
{
    ANN(img);
    for (uint32_t i = 0; i < img->count; i++)
    {
        img->allocs[i].flags = flags;
    }
}



void dvz_images_flags(DvzImages* img, VkImageCreateFlags flags)
{
    ANN(img);
    img->info.flags |= flags;
}



void dvz_images_mip(DvzImages* img, uint32_t mip)
{
    ANN(img);
    img->info.mipLevels = mip;
}



void dvz_images_layers(DvzImages* img, uint32_t layers)
{
    ANN(img);
    img->info.arrayLayers = layers;
}



void dvz_images_samples(DvzImages* img, VkSampleCountFlags samples)
{
    ANN(img);
    img->info.samples = samples;
}



int dvz_images_create(DvzImages* img)
{
    ANN(img);
    ANN(img->device);

    DvzDevice* device = img->device;
    ANN(device);

    DvzVma* allocator = img->allocator;
    ANN(allocator);
    ANN(allocator->device);
    ASSERT(allocator->device == device);

    // Get GPU properties to check the dimensions of the image.
    VkPhysicalDeviceProperties* props = dvz_gpu_properties10(device->gpu);
    uvec3 shape = {img->info.extent.width, img->info.extent.height, img->info.extent.depth};
    if (check_image_size(props, img->info.imageType, shape) != 0)
    {
        log_error("abort image creation");
        return 1;
    }

    int out = 0;
    for (uint32_t i = 0; i < img->count; i++)
    {
        out += dvz_allocator_image(
            allocator, &img->info, img->allocs[i].flags, &img->allocs[i], &img->vk_images[i]);
    }

    dvz_obj_created(&img->obj);

    return out;
}



VkImage dvz_image_handle(DvzImages* img, uint32_t idx)
{
    ANN(img);
    return img->vk_images[idx];
}



void dvz_images_destroy(DvzImages* img)
{
    ANN(img);
    ANN(img->device);

    DvzVma* allocator = img->allocator;
    ANN(allocator);


    log_trace("destroying images...");
    for (uint32_t i = 0; i < img->count; i++)
    {
        dvz_allocator_destroy_image(allocator, &img->allocs[i], img->vk_images[i]);
    }
    dvz_obj_destroyed(&img->obj);
    log_trace("images destroyed");
}



/*************************************************************************************************/
/*  Image views                                                                                  */
/*************************************************************************************************/

void dvz_image_views(DvzImages* img, DvzImageViews* views)
{
    ANN(img);
    ANN(views);

    views->device = img->device;
    views->img = img;

    // Default values.
    views->info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    views->info.subresourceRange.layerCount = 1;
    views->info.subresourceRange.levelCount = 1;
    views->info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    if (img->info.imageType == VK_IMAGE_TYPE_1D)
        views->info.viewType = VK_IMAGE_VIEW_TYPE_1D;
    if (img->info.imageType == VK_IMAGE_TYPE_2D)
        views->info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    if (img->info.imageType == VK_IMAGE_TYPE_3D)
        views->info.viewType = VK_IMAGE_VIEW_TYPE_3D;

    dvz_obj_init(&views->obj);
}



void dvz_image_views_type(DvzImageViews* views, VkImageViewType type)
{
    ANN(views);
    views->info.viewType = type;
}



void dvz_image_views_aspect(DvzImageViews* views, VkImageAspectFlags aspect)
{
    ANN(views);
    views->info.subresourceRange.aspectMask = aspect;
}



void dvz_image_views_mip(DvzImageViews* views, uint32_t base, uint32_t count)
{
    ANN(views);
    views->info.subresourceRange.baseMipLevel = base;
    views->info.subresourceRange.levelCount = count;
}



void dvz_image_views_layers(DvzImageViews* views, uint32_t base, uint32_t count)
{
    ANN(views);
    views->info.subresourceRange.baseArrayLayer = base;
    views->info.subresourceRange.layerCount = count;
}



void dvz_image_views_create(DvzImageViews* views)
{
    ANN(views);

    DvzImages* img = views->img;
    ANN(img);

    DvzDevice* device = views->device;
    ANN(device);

    for (uint32_t i = 0; i < img->count; i++)
    {
        views->info.image = img->vk_images[i];
        views->info.format = img->info.format;
        VK_CHECK_RESULT(
            vkCreateImageView(device->vk_device, &views->info, NULL, &views->vk_views[i]));
    }

    dvz_obj_created(&views->obj);
}



VkImageView dvz_image_views_handle(DvzImageViews* views, uint32_t idx)
{
    ANN(views);
    ASSERT(idx < DVZ_MAX_IMAGES);
    return views->vk_views[idx];
}



void dvz_image_views_destroy(DvzImageViews* views)
{
    ANN(views);

    DvzImages* img = views->img;
    ANN(img);

    DvzDevice* device = views->device;
    ANN(device);

    for (uint32_t i = 0; i < img->count; i++)
    {
        if (views->vk_views[i] != VK_NULL_HANDLE)
        {
            vkDestroyImageView(device->vk_device, views->vk_views[i], NULL);
            views->vk_views[i] = VK_NULL_HANDLE;
        }
    }

    dvz_obj_destroyed(&views->obj);
}



/*************************************************************************************************/
/*  Command buffer                                                                               */
/*************************************************************************************************/

void dvz_image_region(DvzImageRegion* region)
{
    ANN(region);
    region->sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;

    // Default values.
    region->imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region->imageSubresource.layerCount = 1;
}



void dvz_image_region_offset(DvzImageRegion* region, int32_t x, int32_t y, int32_t z)
{
    ANN(region);
    region->imageOffset.x = x;
    region->imageOffset.y = y;
    region->imageOffset.z = z;
}



void dvz_image_region_extent(DvzImageRegion* region, uint32_t w, uint32_t h, uint32_t d)
{
    ANN(region);
    region->imageExtent.width = w;
    region->imageExtent.height = h;
    region->imageExtent.depth = d;
}



void dvz_image_region_aspect(DvzImageRegion* region, VkImageAspectFlags aspect)
{
    ANN(region);
    region->imageSubresource.aspectMask = aspect;
}



void dvz_image_region_mip(DvzImageRegion* region, uint32_t mip)
{
    ANN(region);
    region->imageSubresource.mipLevel = mip;
}



void dvz_image_region_layers(DvzImageRegion* region, uint32_t base_layer, uint32_t layer_count)
{
    ANN(region);
    region->imageSubresource.baseArrayLayer = base_layer;
    region->imageSubresource.layerCount = layer_count;
}



void dvz_cmd_copy_buffer_to_image(
    DvzCommands* cmds, uint32_t idx, VkBuffer buffer, DvzSize offset, //
    VkImage img, VkImageLayout layout, DvzImageRegion* region)
{
    ANN(cmds);

    region->bufferOffset = offset;

    VkCopyBufferToImageInfo2 info = {.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2};
    info.srcBuffer = buffer;
    info.dstImage = img;
    info.dstImageLayout = layout;
    info.regionCount = 1;
    info.pRegions = region;
    vkCmdCopyBufferToImage2(cmds->cmds[idx], &info);
}



void dvz_cmd_copy_image_to_buffer(
    DvzCommands* cmds, uint32_t idx, VkImage img, VkImageLayout layout, DvzImageRegion* region,
    VkBuffer buffer, DvzSize offset)
{
    ANN(cmds);

    region->bufferOffset = offset;

    VkCopyImageToBufferInfo2 info = {.sType = VK_STRUCTURE_TYPE_COPY_IMAGE_TO_BUFFER_INFO_2};
    info.srcImage = img;
    info.srcImageLayout = layout;
    info.dstBuffer = buffer;
    info.regionCount = 1;
    info.pRegions = region;
    vkCmdCopyImageToBuffer2(cmds->cmds[idx], &info);
}
