/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Images                                                                                       */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/common/macros.h"
#include "datoviz/common/obj.h"
#include "datoviz/math/types.h"
#include "datoviz/vk/memory.h"
#include "vk_mem_alloc.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_MAX_IMAGES 4



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzDevice DvzDevice;
typedef struct DvzCommands DvzCommands;
typedef struct DvzVma DvzVma;

typedef struct DvzBuffer DvzBuffer;
typedef struct DvzImages DvzImages;
typedef struct DvzImageViews DvzImageViews;
typedef struct VkBufferImageCopy2 DvzImageRegion;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzImages
{
    DvzObject obj;
    DvzDevice* device;
    DvzVma* allocator;

    uint32_t count;
    // uvec3 shape;
    bool is_swapchain;

    // This wraps:
    // VkImageUsageFlags usage;
    // VkFormat format;
    // VkImageTiling tiling;
    // VkImageType image_type;
    // uint32_t mip;    // number of mip levels
    // uint32_t layers; // number of array layers
    // VkSampleCountFlags samples; // for multisample antialiasing
    VkImageCreateInfo info;

    VkImage vk_images[DVZ_MAX_IMAGES];
    DvzAllocation allocs[DVZ_MAX_IMAGES];
};



struct DvzImageViews
{
    DvzObject obj;
    DvzDevice* device;
    DvzImages* img;

    // This wraps:
    // VkImageViewType type;
    // VkImageAspectFlags aspect;
    // uint32_t mip_base;
    // uint32_t mip_count;
    // uint32_t layers_base;
    // uint32_t layers_count;
    VkImageViewCreateInfo info;

    VkImageView vk_views[DVZ_MAX_IMAGES];
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Images                                                                                       */
/*************************************************************************************************/

/**
 * Initialize a set of GPU images.
 *
 * @param device the device
 * @param allocator the VMA allocator
 * @param type the image type (1D, 2D, or 3D)
 * @param count the number of images
 * @param[out] images the initialized images
 */
DVZ_EXPORT void dvz_images(
    DvzDevice* device, DvzVma* allocator, VkImageType type, uint32_t count, DvzImages* images);



/**
 * Set the images format.
 *
 * @param images the images
 * @param format the image format
 */
DVZ_EXPORT void dvz_images_format(DvzImages* img, VkFormat format);



/**
 * Set the images shape.
 *
 * @param images the images
 * @param width the image width, in pixels
 * @param height the image height, in pixels
 * @param depth the image depth, in pixels
 */
DVZ_EXPORT void dvz_images_size(DvzImages* img, uint32_t width, uint32_t height, uint32_t depth);



/**
 * Set the images tiling.
 *
 * @param images the images
 * @param tiling the image tiling
 */
DVZ_EXPORT void dvz_images_tiling(DvzImages* img, VkImageTiling tiling);



/**
 * Set the images usage.
 *
 * @param images the images
 * @param usage the image usage
 */
DVZ_EXPORT void dvz_images_usage(DvzImages* img, VkImageUsageFlags usage);



/**
 * Set the VMA creation flags.
 *
 * @param image the image
 * @param flags the flags
 */
DVZ_EXPORT void dvz_images_flags(DvzImages* img, VmaAllocationCreateFlags flags);



/**
 * Set the number of mip levels.
 *
 * @param image the image
 * @param mip the number of mip levels
 */
DVZ_EXPORT void dvz_images_mip(DvzImages* img, uint32_t mip);



/**
 * Set the number of MSAA samples.
 *
 * @param image the image
 * @param samples the Vulkan samples flags
 */
DVZ_EXPORT void dvz_images_samples(DvzImages* img, VkSampleCountFlags samples);



/**
 * Set the number of array layers.
 *
 * @param image the image
 * @param layers the number of array layers
 */
DVZ_EXPORT void dvz_images_layers(DvzImages* img, uint32_t layers);



/**
 * Create the images after they have been set up.
 *
 * @param images the images
 * @returns the Vulkan creation result code
 */
DVZ_EXPORT int dvz_images_create(DvzImages* img);



/**
 * Return the Vulkan handle of an image.
 *
 * @param img the images
 * @param idx the image index
 * @returns the Vulkan image handle
 */
DVZ_EXPORT VkImage dvz_image_handle(DvzImages* img, uint32_t idx);



/**
 * Destroy images.
 *
 * @param images the images
 */
DVZ_EXPORT void dvz_images_destroy(DvzImages* img);



/*************************************************************************************************/
/*  Image views                                                                                  */
/*************************************************************************************************/

/**
 * Create image views.
 *
 * @param img the images
 * @param[out] views the created image views
 */
DVZ_EXPORT void dvz_image_views(DvzImages* img, DvzImageViews* views);



/**
 * Set the image views type.
 *
 * @param views the image views
 * @param type the view type
 */
DVZ_EXPORT void dvz_image_views_type(DvzImageViews* views, VkImageViewType type);



/**
 * Set the image views aspect.
 *
 * @param views the image views
 * @param aspect the aspect
 */
DVZ_EXPORT void dvz_image_views_aspect(DvzImageViews* views, VkImageAspectFlags aspect);



/**
 * Set the MIP levels for the views.
 *
 * @param views the image views
 * @param base the mip level base
 * @param count the mip level count
 */
DVZ_EXPORT void dvz_image_views_mip(DvzImageViews* views, uint32_t base, uint32_t count);



/**
 * Set the array layers for the views.
 *
 * @param views the image views
 * @param base the array layer base
 * @param count the array layer count
 */
DVZ_EXPORT void dvz_image_views_layers(DvzImageViews* views, uint32_t base, uint32_t count);



/**
 * Create image views.
 *
 * @param views the image views
 */
DVZ_EXPORT void dvz_image_views_create(DvzImageViews* views);



/**
 * Return the Vulkan handle of an image view.
 *
 * @param views the image views
 * @param idx the image view index
 * @returns the Vulkan image view handle
 */
DVZ_EXPORT VkImageView dvz_image_views_handle(DvzImageViews* views, uint32_t idx);



/**
 * Destroy image views.
 *
 * @param views the image views
 */
DVZ_EXPORT void dvz_image_views_destroy(DvzImageViews* views);



/*************************************************************************************************/
/*  Image region                                                                                 */
/*************************************************************************************************/

/**
 * Initialize an image region.
 *
 * @param region the image region
 */
DVZ_EXPORT void dvz_image_region(DvzImageRegion* region);



/**
 * Set the image region offset.
 *
 * @param region the image region
 * @param x offset x
 * @param y offset y
 * @param z offset z
 */
DVZ_EXPORT void dvz_image_region_offset(DvzImageRegion* region, int32_t x, int32_t y, int32_t z);



/**
 * Set the image region extent.
 *
 * @param region the image region
 * @param w the width
 * @param h the height
 * @param d the depth
 */
DVZ_EXPORT void
dvz_image_region_extent(DvzImageRegion* region, uint32_t w, uint32_t h, uint32_t d);



/**
 * Set the image region aspect.
 *
 * @param region the image region
 * @param aspect the aspect mask
 */
DVZ_EXPORT void dvz_image_region_aspect(DvzImageRegion* region, VkImageAspectFlags aspect);



/**
 * Set the MIP level of the image region.
 *
 * @param region the image region
 * @param mip the MIP level
 */
DVZ_EXPORT void dvz_image_region_mip(DvzImageRegion* region, uint32_t mip);



/**
 * Set the array layers of the image region.
 *
 * @param region the image region
 * @param base_layer the base layer
 * @param layer_count the number of layers
 */
DVZ_EXPORT void
dvz_image_region_layers(DvzImageRegion* region, uint32_t base_layer, uint32_t layer_count);



/*************************************************************************************************/
/*  Image commands                                                                               */
/*************************************************************************************************/

/**
 * Copy a GPU buffer to a GPU image.
 *
 * @param cmds the command buffers
 * @param idx the command buffer index
 * @param buffer the source buffer
 * @param offset the offset in the source buffer
 * @param img the target image
 * @param layout the image layout
 * @param region the image region
 */
DVZ_EXPORT void dvz_cmd_copy_buffer_to_image(
    DvzCommands* cmds, uint32_t idx, VkBuffer buffer, DvzSize offset, //
    VkImage img, VkImageLayout layout, DvzImageRegion* region);



/**
 * Copy a GPU image to a GPU buffer.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param tex_offset the texture offset
 * @param shape the texture shape
 * @param images the image
 * @param buffer the buffer
 * @param buf_offset the buffer offset
 */
DVZ_EXPORT void dvz_cmd_copy_image_to_buffer(
    DvzCommands* cmds, uint32_t idx, VkImage img, VkImageLayout layout, DvzImageRegion* region,
    VkBuffer buffer, DvzSize offset);



EXTERN_C_OFF
