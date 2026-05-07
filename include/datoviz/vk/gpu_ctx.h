/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  GPU context                                                                                  */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <vulkan/vulkan_core.h>

#include "datoviz/common/macros.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/instance.h"
#include "datoviz/vk/memory.h"
#include "datoviz/vk/queues.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzGpuCtx DvzGpuCtx;
typedef struct DvzGpuCtxConfig DvzGpuCtxConfig;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzGpuCtxConfig
{
    bool enable_validation;
    uint32_t gpu_index;
    VkExternalMemoryHandleTypeFlagsKHR export_handle_type;
    bool has_features10;
    bool has_features12;
    bool has_features13;
    VkPhysicalDeviceFeatures features10;
    VkPhysicalDeviceVulkan12Features features12;
    VkPhysicalDeviceVulkan13Features features13;
    uint32_t instance_extension_count;
    const char* instance_extensions[16];
    bool enable_canvas_extensions;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON

/**
 * Return the default GPU-context configuration.
 *
 * @return the default configuration
 */
DVZ_EXPORT DvzGpuCtxConfig dvz_gpu_ctx_config(void);



/**
 * Toggle instance validation for a GPU-context configuration.
 *
 * @param cfg the GPU-context configuration
 * @param enable_validation whether validation should be enabled
 */
DVZ_EXPORT void
dvz_gpu_ctx_config_validation(DvzGpuCtxConfig* cfg, bool enable_validation);



/**
 * Select the GPU index to use for a GPU-context configuration.
 *
 * @param cfg the GPU-context configuration
 * @param gpu_index the selected GPU index
 */
DVZ_EXPORT void dvz_gpu_ctx_config_gpu(DvzGpuCtxConfig* cfg, uint32_t gpu_index);



/**
 * Select the allocator external-memory export policy for a GPU-context configuration.
 *
 * @param cfg the GPU-context configuration
 * @param export_handle_type external memory export flags
 */
DVZ_EXPORT void
dvz_gpu_ctx_config_alloc(DvzGpuCtxConfig* cfg, VkExternalMemoryHandleTypeFlagsKHR export_handle_type);



/**
 * Copy Vulkan 1.0 features into a GPU-context configuration.
 *
 * @param cfg the GPU-context configuration
 * @param features the Vulkan 1.0 feature struct
 */
DVZ_EXPORT void
dvz_gpu_ctx_config_features10(DvzGpuCtxConfig* cfg, const VkPhysicalDeviceFeatures* features);


/**
 * Copy Vulkan 1.2 features into a GPU-context configuration.
 *
 * @param cfg the GPU-context configuration
 * @param features the Vulkan 1.2 feature struct
 */
DVZ_EXPORT void dvz_gpu_ctx_config_features12(
    DvzGpuCtxConfig* cfg, const VkPhysicalDeviceVulkan12Features* features);



/**
 * Copy Vulkan 1.3 features into a GPU-context configuration.
 *
 * @param cfg the GPU-context configuration
 * @param features the Vulkan 1.3 feature struct
 */
DVZ_EXPORT void dvz_gpu_ctx_config_features13(
    DvzGpuCtxConfig* cfg, const VkPhysicalDeviceVulkan13Features* features);



/**
 * Request an additional Vulkan instance extension for a GPU-context configuration.
 *
 * @param cfg the GPU-context configuration
 * @param extension null-terminated extension name (must outlive the config)
 */
DVZ_EXPORT void
dvz_gpu_ctx_config_add_instance_extension(DvzGpuCtxConfig* cfg, const char* extension);



/**
 * Enable or disable canvas (swapchain/surface) device extensions for a GPU-context configuration.
 *
 * Must be set before dvz_gpu_ctx() is called.  Required when creating present canvases.
 *
 * @param cfg the GPU-context configuration
 * @param enable whether canvas extensions should be enabled on the device
 */
DVZ_EXPORT void dvz_gpu_ctx_config_enable_canvas_extensions(DvzGpuCtxConfig* cfg, bool enable);



/**
 * Create an owned GPU context from a configuration.
 *
 * @param cfg the GPU-context configuration
 * @return allocated GPU context, or NULL on failure
 */
DVZ_EXPORT DvzGpuCtx* dvz_gpu_ctx(const DvzGpuCtxConfig* cfg);



/**
 * Return the instance owned by a GPU context.
 *
 * @param ctx the GPU context
 * @return borrowed instance
 */
DVZ_EXPORT DvzInstance* dvz_gpu_ctx_instance(DvzGpuCtx* ctx);



/**
 * Return the selected GPU index of a GPU context.
 *
 * @param ctx the GPU context
 * @return the selected GPU index, or UINT32_MAX when unavailable
 */
DVZ_EXPORT uint32_t dvz_gpu_ctx_gpu_index(DvzGpuCtx* ctx);



/**
 * Return the selected GPU descriptor of a GPU context.
 *
 * @param ctx the GPU context
 * @param[out] out_info destination GPU descriptor
 * @return whether the descriptor could be retrieved
 */
DVZ_EXPORT bool dvz_gpu_ctx_gpu_info(DvzGpuCtx* ctx, DvzGpuInfo* out_info);



/**
 * Return the device owned by a GPU context.
 *
 * @param ctx the GPU context
 * @return borrowed device
 */
DVZ_EXPORT DvzDevice* dvz_gpu_ctx_device(DvzGpuCtx* ctx);



/**
 * Return the allocator owned by a GPU context.
 *
 * @param ctx the GPU context
 * @return borrowed allocator
 */
DVZ_EXPORT DvzVma* dvz_gpu_ctx_alloc(DvzGpuCtx* ctx);



/**
 * Return a queue owned by the GPU-context device.
 *
 * @param ctx the GPU context
 * @param role the requested queue role
 * @return borrowed queue, or NULL when unavailable
 */
DVZ_EXPORT DvzQueue* dvz_gpu_ctx_queue(DvzGpuCtx* ctx, DvzQueueRole role);



/**
 * Return the validation error count associated with a GPU context.
 *
 * @param ctx the GPU context
 * @return the validation error count
 */
DVZ_EXPORT uint32_t dvz_gpu_ctx_error_count(DvzGpuCtx* ctx);



/**
 * Destroy a GPU context and all owned runtime objects.
 *
 * @param ctx the GPU context
 */
DVZ_EXPORT void dvz_gpu_ctx_destroy(DvzGpuCtx* ctx);



/*************************************************************************************************/
/*  GLSL compilation utility                                                                     */
/*************************************************************************************************/

/**
 * Compile a GLSL source string to SPIR-V using shaderc (lazy-loaded).
 *
 * The returned buffer is heap-allocated and must be freed with dvz_free().
 * Returns NULL if shaderc is unavailable or compilation fails.
 *
 * @param stage   shader stage: "vertex", "fragment", or "compute"
 * @param glsl    null-terminated GLSL source string
 * @param out_size receives the byte size of the returned SPIR-V buffer
 * @return heap-allocated SPIR-V words, or NULL on failure
 */
DVZ_EXPORT uint32_t* dvz_compile_glsl(const char* stage, const char* glsl, uint64_t* out_size);

EXTERN_C_OFF
