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
#include "vk_mem_alloc.h"
#include <volk.h>



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_MAX_IMAGES 4



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzDevice DvzDevice;
typedef struct DvzVma DvzVma;

typedef struct DvzImages DvzImages;
typedef struct DvzImageViews DvzImageViews;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/



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
 * Set the images layout.
 *
 * @param images the images
 * @param layout the image layout
 */
DVZ_EXPORT void dvz_images_layout(DvzImages* img, VkImageLayout layout);



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
 * Destroy image views.
 *
 * @param views the image views
 */
DVZ_EXPORT void dvz_image_views_destroy(DvzImageViews* views);



EXTERN_C_OFF
