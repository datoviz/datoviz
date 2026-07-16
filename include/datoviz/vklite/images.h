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
#include "datoviz/math/types.h"
#include "datoviz/vk/memory.h"



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
typedef struct DvzImageBlit DvzImageBlit;
typedef struct DvzImageCopy DvzImageCopy;



EXTERN_C_ON

/*************************************************************************************************/
/*  Images                                                                                       */
/*************************************************************************************************/

/**
 * Allocate an empty images wrapper.
 *
 * Heap-allocated wrappers follow the same lifecycle as stack-owned wrappers:
 * initialize with dvz_images(), configure, call dvz_images_create() once, then
 * destroy before any recreate and free only if this wrapper came from
 * dvz_images_create_wrapper().
 *
 * @return allocated images wrapper, or NULL on allocation failure
 */
DVZ_EXPORT DvzImages* dvz_images_create_wrapper(void);



/**
 * Free an images wrapper allocated by dvz_images_create_wrapper().
 *
 * @param img images wrapper to free
 */
DVZ_EXPORT void dvz_images_free(DvzImages* img);



/**
 * Allocate an empty image-view wrapper.
 *
 * Heap-allocated wrappers follow the same lifecycle as stack-owned wrappers:
 * initialize with dvz_image_views(), configure, call dvz_image_views_create()
 * once, then destroy before any recreate and free only if this wrapper came
 * from dvz_image_views_create_wrapper().
 *
 * @return allocated image-view wrapper, or NULL on allocation failure
 */
DVZ_EXPORT DvzImageViews* dvz_image_views_create_wrapper(void);



/**
 * Free an image-view wrapper allocated by dvz_image_views_create_wrapper().
 *
 * @param views image-view wrapper to free
 */
DVZ_EXPORT void dvz_image_views_free(DvzImageViews* views);

/**
 * Initialize a set of GPU images.
 *
 * This prepares the wrapper for configuration. Call dvz_images_create() once
 * after setting the desired format, size, usage, and allocation policy.
 * Recreating a live image set requires dvz_images_destroy() first.
 *
 * @param device logical device that owns created images; must outlive `images`
 * @param allocator allocator used for image memory; must outlive created images
 * @param type the image type (1D, 2D, or 3D)
 * @param count the number of images
 * @param[out] images the initialized images
 */
DVZ_EXPORT void dvz_images(
    DvzDevice* device, DvzVma* allocator, VkImageType type, uint32_t count, DvzImages* images);



/**
 * Set the images format.
 *
 * @param img initialized image wrapper
 * @param format the image format
 */
DVZ_EXPORT void dvz_images_format(DvzImages* img, VkFormat format);



/**
 * Set the images shape.
 *
 * @param img initialized image wrapper
 * @param width the image width, in pixels
 * @param height the image height, in pixels
 * @param depth the image depth, in pixels
 */
DVZ_EXPORT void dvz_images_size(DvzImages* img, uint32_t width, uint32_t height, uint32_t depth);



/**
 * Set the images tiling.
 *
 * @param img initialized image wrapper
 * @param tiling the image tiling
 */
DVZ_EXPORT void dvz_images_tiling(DvzImages* img, VkImageTiling tiling);



/**
 * Set the images usage.
 *
 * @param img initialized image wrapper
 * @param usage the image usage
 */
DVZ_EXPORT void dvz_images_usage(DvzImages* img, VkImageUsageFlags usage);



/**
 * Set the allocator policy flags used when the images create their memory.
 *
 * @param img initialized image wrapper
 * @param flags Datoviz allocation policy flags
 */
DVZ_EXPORT void dvz_images_alloc_flags(DvzImages* img, DvzAllocationFlags flags);



/**
 * Set the image creation flags.
 *
 * @param img initialized image wrapper
 * @param flags Vulkan image-creation flags
 */
DVZ_EXPORT void dvz_images_flags(DvzImages* img, VkImageCreateFlags flags);



/**
 * Set the number of mip levels.
 *
 * @param img initialized image wrapper
 * @param mip the number of mip levels
 */
DVZ_EXPORT void dvz_images_mip(DvzImages* img, uint32_t mip);



/**
 * Set the number of MSAA samples.
 *
 * @param img initialized image wrapper
 * @param samples the Vulkan samples flags
 */
DVZ_EXPORT void dvz_images_samples(DvzImages* img, VkSampleCountFlags samples);



/**
 * Set the number of array layers.
 *
 * @param img initialized image wrapper
 * @param layers the number of array layers
 */
DVZ_EXPORT void dvz_images_layers(DvzImages* img, uint32_t layers);



/**
 * Create the images after they have been set up.
 *
 * This function creates the wrapped Vulkan images exactly once per live
 * wrapper. Call dvz_images_destroy() before attempting to create them again.
 *
 * @param img configured image wrapper
 * @return 0 on success, non-zero on Vulkan or Datoviz state failure
 */
DVZ_EXPORT int dvz_images_create(DvzImages* img);



/**
 * Return the Vulkan handle of an image.
 *
 * @param img the images
 * @param idx image index in `[0, dvz_images_count(img))`
 * @return borrowed Vulkan image handle, possibly `VK_NULL_HANDLE` when not created
 */
DVZ_EXPORT VkImage dvz_image_handle(DvzImages* img, uint32_t idx);



/**
 * Return the number of images wrapped by an images object.
 *
 * @param img the images
 * @return the image count
 */
DVZ_EXPORT uint32_t dvz_images_count(DvzImages* img);



/**
 * Return the configured image format for an images object.
 *
 * @param img the images
 * @return the Vulkan image format
 */
DVZ_EXPORT VkFormat dvz_images_format_value(DvzImages* img);



/**
 * Destroy images.
 *
 * This releases Datoviz-owned Vulkan images and returns the wrapper to a reusable initialized
 * state. Images installed with `dvz_images_wrap()` are borrowed and are not destroyed.
 *
 * @param img live image wrapper; borrowed wrapped handles are retained
 */
DVZ_EXPORT void dvz_images_destroy(DvzImages* img);



/**
 * Wrap an existing Vulkan image into a DvzImages struct.
 *
 * The Vulkan image remains externally owned. Datoviz records the handle for view creation,
 * transitions, and descriptor binding according to later calls, but does not destroy the image.
 *
 * @param device logical device associated with `vk_image`; must outlive `img`
 * @param allocator allocator associated with the image, or NULL when not needed
 * @param type the image type (1D, 2D, or 3D)
 * @param vk_image externally owned live Vulkan image handle
 * @param[out] img initialized wrapper that borrows `vk_image`
 */
DVZ_EXPORT void dvz_images_wrap(
    DvzDevice* device, DvzVma* allocator, VkImageType type, VkImage vk_image, DvzImages* img);



/*************************************************************************************************/
/*  Image views                                                                                  */
/*************************************************************************************************/

/**
 * Initialize image views for an existing images wrapper.
 *
 * This prepares the wrapper for configuration. Call dvz_image_views_create()
 * once after setting the desired view type, aspect, and subresource range.
 * Recreating live image views requires dvz_image_views_destroy() first.
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
 * This function creates the wrapped Vulkan image views exactly once per live
 * wrapper. Call dvz_image_views_destroy() before attempting to create them
 * again.
 *
 * @param views the image views
 * @return 0 on success, non-zero on Vulkan or Datoviz state failure
 */
DVZ_EXPORT int dvz_image_views_create(DvzImageViews* views);



/**
 * Return the Vulkan handle of an image view.
 *
 * @param views the image views
 * @param idx the image view index
 * @return borrowed Vulkan image-view handle, possibly `VK_NULL_HANDLE` when not created
 */
DVZ_EXPORT VkImageView dvz_image_views_handle(DvzImageViews* views, uint32_t idx);



/**
 * Return the number of image views owned by a views wrapper.
 *
 * @param views the image views
 * @return the image-view count
 */
DVZ_EXPORT uint32_t dvz_image_views_count(DvzImageViews* views);



/**
 * Destroy image views.
 *
 * This releases the wrapped Vulkan image views and returns the wrapper to a
 * reusable initialized state.
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
 * Allocate an empty image-copy wrapper.
 *
 * @return allocated image-copy wrapper, or NULL on allocation failure
 */
DVZ_EXPORT DvzImageCopy* dvz_image_copy_create(void);



/**
 * Reset an image-copy wrapper to its default state.
 *
 * @param copy the image-copy wrapper
 */
DVZ_EXPORT void dvz_image_copy(DvzImageCopy* copy);



/**
 * Free an image-copy wrapper allocated by dvz_image_copy_create().
 *
 * @param copy image-copy wrapper to free
 */
DVZ_EXPORT void dvz_image_copy_free(DvzImageCopy* copy);



/**
 * Allocate an empty image-blit wrapper.
 *
 * @return allocated image-blit wrapper, or NULL on allocation failure
 */
DVZ_EXPORT DvzImageBlit* dvz_image_blit_create(void);



/**
 * Reset an image-blit wrapper to its default state.
 *
 * @param blit the image-blit wrapper
 */
DVZ_EXPORT void dvz_image_blit(DvzImageBlit* blit);



/**
 * Free an image-blit wrapper allocated by dvz_image_blit_create().
 *
 * @param blit image-blit wrapper to free
 */
DVZ_EXPORT void dvz_image_blit_free(DvzImageBlit* blit);



/**
 * Copy a GPU buffer to a GPU image.
 *
 * @param cmds the command buffers
 * @param buffer the source buffer
 * @param offset source byte offset within `buffer`
 * @param img the target image
 * @param layout current layout of `img`, valid for transfer-destination access
 * @param region destination subresource, offset, and extent
 */
DVZ_EXPORT void dvz_cmd_copy_buffer_to_image(
    DvzCommands* cmds, VkBuffer buffer, DvzSize offset, //
    VkImage img, VkImageLayout layout, DvzImageRegion* region);



/**
 * Copy a GPU image to a GPU buffer.
 *
 * @param cmds the set of command buffers to record
 * @param img source image handle
 * @param layout current layout of `img`, valid for transfer-source access
 * @param region source subresource, offset, and extent
 * @param buffer destination buffer handle
 * @param offset destination byte offset within `buffer`
 */
DVZ_EXPORT void dvz_cmd_copy_image_to_buffer(
    DvzCommands* cmds, VkImage img, VkImageLayout layout, DvzImageRegion* region, VkBuffer buffer,
    DvzSize offset);



/**
 * Define the source image and region of an image copy operation.
 *
 * @param copy the copy structure
 * @param image source image handle
 * @param layout current source image layout, valid for transfer-source access
 * @param x the source offset x
 * @param y the source offset y
 * @param z the source offset z
 * @param width the width
 * @param height the height
 * @param depth the depth
 */
DVZ_EXPORT void dvz_cmd_copy_source(
    DvzImageCopy* copy, VkImage image, VkImageLayout layout, //
    int32_t x, int32_t y, int32_t z, uint32_t width, uint32_t height, uint32_t depth);



/**
 * Define the destination image and offset of an image copy operation.
 *
 * @param copy the copy structure
 * @param image destination image handle
 * @param layout current destination image layout, valid for transfer-destination access
 * @param x the destination offset x
 * @param y the destination offset y
 * @param z the destination offset z
 */
DVZ_EXPORT void dvz_cmd_copy_destination(
    DvzImageCopy* copy, VkImage image, VkImageLayout layout, int32_t x, int32_t y, int32_t z);



/**
 * Record the configured image copy operation.
 *
 * @param cmds recording command-buffer wrapper
 * @param copy the copy structure
 */
DVZ_EXPORT void dvz_cmd_copy_image(DvzCommands* cmds, DvzImageCopy* copy);



/**
 * Define the source of a blit operation.
 *
 * @param blit the blit structure
 * @param image source image handle
 * @param layout current source image layout, valid for transfer-source access
 * @param x0 first source x coordinate
 * @param y0 first source y coordinate
 * @param z0 first source z coordinate
 * @param x1 the source offset x
 * @param y1 the source offset y
 * @param z1 the source offset z
 */
DVZ_EXPORT void dvz_cmd_blit_source(
    DvzImageBlit* blit, VkImage image, VkImageLayout layout, //
    int32_t x0, int32_t y0, int32_t z0, int32_t x1, int32_t y1, int32_t z1);



/**
 * Define the destination of a blit operation.
 *
 * @param blit the blit structure
 * @param image destination image handle
 * @param layout current destination image layout, valid for transfer-destination access
 * @param x0 first destination x coordinate
 * @param y0 first destination y coordinate
 * @param z0 first destination z coordinate
 * @param x1 the destination offset x
 * @param y1 the destination offset y
 * @param z1 the destination offset z
 */
DVZ_EXPORT void dvz_cmd_blit_destination(
    DvzImageBlit* blit, VkImage image, VkImageLayout layout, //
    int32_t x0, int32_t y0, int32_t z0, int32_t x1, int32_t y1, int32_t z1);



/**
 * Set the filter of a blit operation.
 *
 * @param blit the blit structure
 * @param filter the filter
 */
DVZ_EXPORT void dvz_cmd_blit_filter(DvzImageBlit* blit, VkFilter filter);



/**
 * Record the configured image blit operation.
 *
 * @param cmds recording command-buffer wrapper
 * @param blit the blit structure
 */
DVZ_EXPORT void dvz_cmd_blit_image(DvzCommands* cmds, DvzImageBlit* blit);



EXTERN_C_OFF
