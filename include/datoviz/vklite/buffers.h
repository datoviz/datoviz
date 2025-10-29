/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Buffers                                                                                      */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/common/macros.h"
#include "datoviz/math/types.h"
#include "datoviz/vk/enums.h"
#include "vulkan/vulkan_core.h"
#include <stdint.h>
#include <volk.h>



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzDevice DvzDevice;
typedef struct DvzVma DvzVma;

typedef struct DvzBuffer DvzBuffer;
typedef struct DvzBufferViews DvzBufferViews;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON



/**
 * Initialize a GPU buffer.
 *
 * @param device the device
 * @param allocator the VMA allocator
 * @param[out] buffer the create buffer
 */
DVZ_EXPORT void dvz_buffer(DvzDevice* device, DvzVma* allocator, DvzBuffer* buffer);



/**
 * Set the buffer size.
 *
 * @param buffer the buffer
 * @param size the buffer size, in bytes
 */
DVZ_EXPORT void dvz_buffer_size(DvzBuffer* buffer, DvzSize size);



/**
 * Set the buffer usage.
 *
 * @param buffer the buffer
 * @param usage the buffer usage
 */
DVZ_EXPORT void dvz_buffer_usage(DvzBuffer* buffer, VkBufferUsageFlags usage);



/**
 * Create the buffer after it has been set.
 *
 * @param buffer the buffer
 * @returns the Vulkan creation result code
 */
DVZ_EXPORT int dvz_buffer_create(DvzBuffer* buffer);



/**
 * Resize a buffer.
 *
 * @param buffer the buffer
 * @param size the new buffer size, in bytes
 */
DVZ_EXPORT void dvz_buffer_resize(DvzBuffer* buffer, DvzSize size);



/**
 * Destroy a buffer
 *
 * @param buffer the buffer
 */
DVZ_EXPORT void dvz_buffer_destroy(DvzBuffer* buffer);



EXTERN_C_OFF
