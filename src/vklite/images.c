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

#include "_alloc.h"
#include "_vk_utils.h"
#include "_assertions.h"
#include "_images.h"
#include "_log.h"
#include "obj.h"
#include "datoviz/vk/device.h"
#include "datoviz/math/types.h"
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

/**
 * Allocate an empty images wrapper.
 *
 * @return allocated images wrapper, or NULL on allocation failure
 */
DvzImages* dvz_images_create_wrapper(void)
{
    DvzImages* img = (DvzImages*)dvz_calloc(1, sizeof(DvzImages));
    ANN(img);
    return img;
}



/**
 * Free an images wrapper allocated by dvz_images_create_wrapper().
 *
 * @param img images wrapper to free
 */
void dvz_images_free(DvzImages* img)
{
    if (img == NULL)
    {
        return;
    }
    for (uint32_t i = 0; i < DVZ_MAX_IMAGES; i++)
    {
        if (img->allocs[i] != NULL)
        {
            dvz_allocation_free(img->allocs[i]);
            img->allocs[i] = NULL;
        }
    }
    dvz_free(img);
}



/**
 * Allocate an empty image-view wrapper.
 *
 * @return allocated image-view wrapper, or NULL on allocation failure
 */
DvzImageViews* dvz_image_views_create_wrapper(void)
{
    DvzImageViews* views = (DvzImageViews*)dvz_calloc(1, sizeof(DvzImageViews));
    ANN(views);
    return views;
}



/**
 * Free an image-view wrapper allocated by dvz_image_views_create_wrapper().
 *
 * @param views image-view wrapper to free
 */
void dvz_image_views_free(DvzImageViews* views)
{
    if (views == NULL)
    {
        return;
    }
    dvz_free(views);
}



void dvz_images(
    DvzDevice* device, DvzVma* allocator, VkImageType type, uint32_t count, DvzImages* img)
{
    ANN(device);
    ANN(allocator);
    ANN(img);
    ASSERT(count <= DVZ_MAX_IMAGES);
    dvz_memset(img, sizeof(*img), 0, sizeof(*img));
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



void dvz_images_alloc_flags(DvzImages* img, DvzAllocationFlags flags)
{
    ANN(img);
    img->req_alloc_flags = flags;
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
    if (dvz_obj_is_created(&img->obj))
    {
        log_error("cannot create images twice without destroying them first");
        return 1;
    }

    DvzDevice* device = img->device;
    ANN(device);

    DvzVma* allocator = img->allocator;
    ANN(allocator);
    DvzDevice* allocator_device = dvz_allocator_device(allocator);
    ANN(allocator_device);
    ASSERT(allocator_device == device);

    // Get GPU properties to check the dimensions of the image.
    VkPhysicalDeviceProperties props = {0};
    vkGetPhysicalDeviceProperties(dvz_device_physical_device(device), &props);
    uvec3 shape = {img->info.extent.width, img->info.extent.height, img->info.extent.depth};
    if (check_image_size(&props, img->info.imageType, shape) != 0)
    {
        log_error("abort image creation");
        return 1;
    }

    int out = 0;
    for (uint32_t i = 0; i < img->count; i++)
    {
        if (img->allocs[i] == NULL)
        {
            img->allocs[i] = dvz_allocation_create();
            ANN(img->allocs[i]);
        }
        dvz_allocation_set_flags(img->allocs[i], img->req_alloc_flags);
        out = dvz_allocator_image(
            allocator, &img->info, img->req_alloc_flags, img->allocs[i], &img->vk_images[i]);
        if (out != 0)
        {
            for (uint32_t j = 0; j <= i; j++)
            {
                if (img->vk_images[j] != VK_NULL_HANDLE)
                {
                    dvz_allocator_destroy_image(allocator, img->allocs[j], img->vk_images[j]);
                }
                img->vk_images[j] = VK_NULL_HANDLE;
                if (img->allocs[j] != NULL)
                {
                    dvz_allocation_free(img->allocs[j]);
                    img->allocs[j] = NULL;
                }
            }
            return out;
        }
    }

    dvz_obj_created(&img->obj);

    return out;
}



VkImage dvz_image_handle(DvzImages* img, uint32_t idx)
{
    ANN(img);
    return img->vk_images[idx];
}



/**
 * Return the number of images wrapped by an images object.
 *
 * @param img the images
 * @return the image count
 */
uint32_t dvz_images_count(DvzImages* img)
{
    ANN(img);
    return img->count;
}



/**
 * Return the configured image format for an images object.
 *
 * @param img the images
 * @return the Vulkan image format
 */
VkFormat dvz_images_format_value(DvzImages* img)
{
    ANN(img);
    return img->info.format;
}



void dvz_images_destroy(DvzImages* img)
{
    ANN(img);
    if (!dvz_obj_is_created(&img->obj))
    {
        log_trace("skip destruction of already-destroyed images");
        return;
    }
    ANN(img->device);

    DvzVma* allocator = img->allocator;
    ANN(allocator);


    log_trace("destroying images...");
    for (uint32_t i = 0; i < img->count; i++)
    {
        if (img->allocs[i] != NULL)
        {
            dvz_allocator_destroy_image(allocator, img->allocs[i], img->vk_images[i]);
            dvz_allocation_free(img->allocs[i]);
            img->allocs[i] = NULL;
        }
        img->vk_images[i] = VK_NULL_HANDLE;
    }
    dvz_obj_destroyed(&img->obj);
    log_trace("images destroyed");
}



void dvz_images_wrap(
    DvzDevice* device, DvzVma* allocator, VkImageType type, VkImage vk_image, DvzImages* img)
{
    ANN(device);
    ANN(allocator);
    ANN(img);
    dvz_memset(img, sizeof(*img), 0, sizeof(*img));
    img->device = device;
    img->allocator = allocator;
    img->count = 1;

    // Default values.
    img->info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img->info.imageType = type;
    img->info.mipLevels = 1;
    img->info.arrayLayers = 1;
    img->info.samples = VK_SAMPLE_COUNT_1_BIT;

    img->vk_images[0] = vk_image;

    dvz_obj_created(&img->obj);
}



/*************************************************************************************************/
/*  Image views                                                                                  */
/*************************************************************************************************/

void dvz_image_views(DvzImages* img, DvzImageViews* views)
{
    ANN(img);
    ANN(views);
    dvz_memset(views, sizeof(*views), 0, sizeof(*views));
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



int dvz_image_views_create(DvzImageViews* views)
{
    ANN(views);
    if (dvz_obj_is_created(&views->obj))
    {
        log_error("cannot create image views twice without destroying them first");
        return -1;
    }

    DvzImages* img = views->img;
    ANN(img);
    if (!dvz_obj_is_created(&img->obj))
    {
        log_error("cannot create image views from images that are not created");
        return -1;
    }

    DvzDevice* device = views->device;
    ANN(device);
    VkDevice vkd = dvz_device_handle(device);
    ANNVK(vkd);

    for (uint32_t i = 0; i < img->count; i++)
    {
        views->info.image = img->vk_images[i];
        views->info.format = img->info.format;
        VkResult res = vkCreateImageView(vkd, &views->info, NULL, &views->vk_views[i]);
        if (vk_result_check(res, __FILE__, __LINE__) != 0)
        {
            for (uint32_t j = 0; j <= i; j++)
            {
                if (views->vk_views[j] != VK_NULL_HANDLE)
                {
                    vkDestroyImageView(vkd, views->vk_views[j], NULL);
                    views->vk_views[j] = VK_NULL_HANDLE;
                }
            }
            return -1;
        }
    }

    dvz_obj_created(&views->obj);
    return 0;
}



VkImageView dvz_image_views_handle(DvzImageViews* views, uint32_t idx)
{
    ANN(views);
    ASSERT(idx < DVZ_MAX_IMAGES);
    return views->vk_views[idx];
}



/**
 * Return the number of image views owned by a views wrapper.
 *
 * @param views the image views
 * @return the image-view count
 */
uint32_t dvz_image_views_count(DvzImageViews* views)
{
    ANN(views);
    ANN(views->img);
    return views->img->count;
}



void dvz_image_views_destroy(DvzImageViews* views)
{
    ANN(views);
    if (!dvz_obj_is_created(&views->obj))
    {
        log_trace("skip destruction of already-destroyed image views");
        return;
    }

    DvzImages* img = views->img;
    ANN(img);

    DvzDevice* device = views->device;
    ANN(device);
    VkDevice vkd = dvz_device_handle(device);
    ANNVK(vkd);

    for (uint32_t i = 0; i < img->count; i++)
    {
        if (views->vk_views[i] != VK_NULL_HANDLE)
        {
            vkDestroyImageView(vkd, views->vk_views[i], NULL);
            views->vk_views[i] = VK_NULL_HANDLE;
        }
    }

    dvz_obj_destroyed(&views->obj);
}



/*************************************************************************************************/
/*  Command buffer                                                                               */
/*************************************************************************************************/

/**
 * Allocate an empty image-copy wrapper.
 *
 * @return allocated image-copy wrapper, or NULL on allocation failure
 */
DvzImageCopy* dvz_image_copy_create(void)
{
    DvzImageCopy* copy = (DvzImageCopy*)dvz_calloc(1, sizeof(DvzImageCopy));
    ANN(copy);
    dvz_image_copy(copy);
    return copy;
}



/**
 * Reset an image-copy wrapper to its default state.
 *
 * @param copy the image-copy wrapper
 */
void dvz_image_copy(DvzImageCopy* copy)
{
    ANN(copy);
    dvz_memset(copy, sizeof(DvzImageCopy), 0, sizeof(DvzImageCopy));
}



/**
 * Free an image-copy wrapper allocated by dvz_image_copy_create().
 *
 * @param copy image-copy wrapper to free
 */
void dvz_image_copy_free(DvzImageCopy* copy)
{
    if (copy == NULL)
    {
        return;
    }
    dvz_free(copy);
}



/**
 * Allocate an empty image-blit wrapper.
 *
 * @return allocated image-blit wrapper, or NULL on allocation failure
 */
DvzImageBlit* dvz_image_blit_create(void)
{
    DvzImageBlit* blit = (DvzImageBlit*)dvz_calloc(1, sizeof(DvzImageBlit));
    ANN(blit);
    dvz_image_blit(blit);
    return blit;
}



/**
 * Reset an image-blit wrapper to its default state.
 *
 * @param blit the image-blit wrapper
 */
void dvz_image_blit(DvzImageBlit* blit)
{
    ANN(blit);
    dvz_memset(blit, sizeof(DvzImageBlit), 0, sizeof(DvzImageBlit));
}



/**
 * Free an image-blit wrapper allocated by dvz_image_blit_create().
 *
 * @param blit image-blit wrapper to free
 */
void dvz_image_blit_free(DvzImageBlit* blit)
{
    if (blit == NULL)
    {
        return;
    }
    dvz_free(blit);
}



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
    DvzCommands* cmds, VkBuffer buffer, DvzSize offset, //
    VkImage img, VkImageLayout layout, DvzImageRegion* region)
{
    ANN(cmds);


    VkCommandBuffer cmd = dvz_commands_handle(cmds);
    ANNVK(cmd);

    region->bufferOffset = offset;

    VkCopyBufferToImageInfo2 info = {.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2};
    info.srcBuffer = buffer;
    info.dstImage = img;
    info.dstImageLayout = layout;
    info.regionCount = 1;
    info.pRegions = region;
    vkCmdCopyBufferToImage2(cmd, &info);
}



void dvz_cmd_copy_image_to_buffer(
    DvzCommands* cmds, VkImage img, VkImageLayout layout, DvzImageRegion* region, VkBuffer buffer,
    DvzSize offset)
{
    ANN(cmds);

    VkCommandBuffer cmd = dvz_commands_handle(cmds);
    ANNVK(cmd);

    region->bufferOffset = offset;

    VkCopyImageToBufferInfo2 info = {.sType = VK_STRUCTURE_TYPE_COPY_IMAGE_TO_BUFFER_INFO_2};
    info.srcImage = img;
    info.srcImageLayout = layout;
    info.dstBuffer = buffer;
    info.regionCount = 1;
    info.pRegions = region;
    vkCmdCopyImageToBuffer2(cmd, &info);
}



void dvz_cmd_copy_source(
    DvzImageCopy* copy, VkImage image, VkImageLayout layout, //
    int32_t x, int32_t y, int32_t z, uint32_t width, uint32_t height, uint32_t depth)
{
    ANN(copy);

    copy->info.srcImage = image;
    copy->info.srcImageLayout = layout;

    copy->copy.srcOffset.x = x;
    copy->copy.srcOffset.y = y;
    copy->copy.srcOffset.z = z;

    copy->copy.extent.width = width;
    copy->copy.extent.height = height;
    copy->copy.extent.depth = depth;

    copy->copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy->copy.srcSubresource.baseArrayLayer = 0;
    copy->copy.srcSubresource.layerCount = 1;
    copy->copy.srcSubresource.mipLevel = 0;
}



void dvz_cmd_copy_destination(
    DvzImageCopy* copy, VkImage image, VkImageLayout layout, int32_t x, int32_t y, int32_t z)
{
    ANN(copy);

    copy->info.dstImage = image;
    copy->info.dstImageLayout = layout;

    copy->copy.dstOffset.x = x;
    copy->copy.dstOffset.y = y;
    copy->copy.dstOffset.z = z;

    copy->copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy->copy.dstSubresource.baseArrayLayer = 0;
    copy->copy.dstSubresource.layerCount = 1;
    copy->copy.dstSubresource.mipLevel = 0;
}



void dvz_cmd_copy_image(DvzCommands* cmds, DvzImageCopy* copy)
{
    ANN(cmds);
    ANN(copy);

    VkCommandBuffer cmd = dvz_commands_handle(cmds);
    ANNVK(cmd);

    copy->info.sType = VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2;
    copy->info.regionCount = 1;
    copy->info.pRegions = &copy->copy;
    copy->copy.sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2;

    vkCmdCopyImage2(cmd, &copy->info);
}



void dvz_cmd_blit_source(
    DvzImageBlit* blit, VkImage image, VkImageLayout layout, //
    int32_t x0, int32_t y0, int32_t z0, int32_t x1, int32_t y1, int32_t z1)
{
    ANN(blit);

    blit->info.srcImage = image;
    blit->info.srcImageLayout = layout;

    blit->blit.srcOffsets[0].x = x0;
    blit->blit.srcOffsets[0].y = y0;
    blit->blit.srcOffsets[0].z = z0;
    blit->blit.srcOffsets[1].x = x1;
    blit->blit.srcOffsets[1].y = y1;
    blit->blit.srcOffsets[1].z = z1;

    blit->blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit->blit.srcSubresource.baseArrayLayer = 0;
    blit->blit.srcSubresource.layerCount = 1;
    blit->blit.srcSubresource.mipLevel = 0;
}



void dvz_cmd_blit_destination(
    DvzImageBlit* blit, VkImage image, VkImageLayout layout, //
    int32_t x0, int32_t y0, int32_t z0, int32_t x1, int32_t y1, int32_t z1)
{
    ANN(blit);

    blit->info.dstImage = image;
    blit->info.dstImageLayout = layout;

    blit->blit.dstOffsets[0].x = x0;
    blit->blit.dstOffsets[0].y = y0;
    blit->blit.dstOffsets[0].z = z0;
    blit->blit.dstOffsets[1].x = x1;
    blit->blit.dstOffsets[1].y = y1;
    blit->blit.dstOffsets[1].z = z1;

    blit->blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit->blit.dstSubresource.baseArrayLayer = 0;
    blit->blit.dstSubresource.layerCount = 1;
    blit->blit.dstSubresource.mipLevel = 0;
}



void dvz_cmd_blit_filter(DvzImageBlit* blit, VkFilter filter)
{
    ANN(blit);
    blit->info.filter = filter;
}



void dvz_cmd_blit_image(DvzCommands* cmds, DvzImageBlit* blit)
{
    ANN(cmds);
    ANN(blit);

    VkCommandBuffer cmd = dvz_commands_handle(cmds);
    ANNVK(cmd);

    blit->info.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
    blit->info.regionCount = 1;
    blit->info.pRegions = &blit->blit;
    blit->blit.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;

    vkCmdBlitImage2(cmd, &blit->info);
}
