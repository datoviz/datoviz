/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Vulkan wrappers                                                                              */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PUBLIC_VULKAN
#define DVZ_HEADER_PUBLIC_VULKAN



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <vulkan/vulkan.h>

#include "datoviz_enums.h"
#include "datoviz_types.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

// Forward declarations.
typedef struct DvzHost DvzHost;
typedef struct DvzGpu DvzGpu;
typedef struct DvzRenderpass DvzRenderpass;
typedef struct DvzFramebuffers DvzFramebuffers;
typedef struct DvzCommands DvzCommands;
typedef struct DvzCanvas DvzCanvas;

// typedef struct VkInstance VkInstance;
// typedef struct VkDevice VkDevice;
// typedef struct VkRenderpass VkRenderpass;



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Placeholder.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT DvzHost* dvz_host_wrap(VkInstance instance);



/**
 * Placeholder.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT DvzGpu* dvz_gpu_wrap(DvzHost* host, VkDevice device);



/**
 * Placeholder.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT DvzRenderpass dvz_renderpass_wrap(DvzGpu* gpu, VkRenderPass vk_renderpass);



/**
 * Placeholder.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT DvzFramebuffers
dvz_framebuffers_wrap(DvzGpu* gpu, DvzRenderpass* renderpass, uint32_t img_count);



/**
 * Placeholder.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT void
dvz_framebuffers_set(DvzFramebuffers* framebuffers, uint32_t img_idx, VkFramebuffer framebuffer);



/**
 * Placeholder.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT DvzCommands dvz_commands_wrap(DvzGpu* gpu, uint32_t img_count);



/**
 * Placeholder.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT void dvz_commands_set(DvzCommands* cmds, uint32_t img_idx, VkCommandBuffer cmd);



/**
 * Placeholder.
 *
 * @param placeholder placeholder
 */
DVZ_EXPORT DvzCanvas*
dvz_canvas_wrap(DvzGpu* gpu, DvzRenderpass* renderpass, DvzFramebuffers* framebuffers);



EXTERN_C_OFF

#endif
