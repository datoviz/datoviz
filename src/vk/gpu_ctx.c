/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  GPU context                                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/vk/instance.h"
#include "datoviz/vk/memory.h"
#include "datoviz/vk/memory_interop.h"
#include "datoviz/vk/queues.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzGpuCtx
{
    DvzGpuCtxConfig cfg;
    uint32_t validation_error_count;
    DvzInstance* instance;
    DvzDevice* device;
    DvzVma* allocator;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Request device extensions needed when exporting memory to external APIs.
 *
 * @param cfg GPU context configuration
 * @param dcfg device configuration being built
 */
static void _gpu_ctx_request_export_extensions(
    const DvzGpuCtxConfig* cfg, DvzDeviceConfig* dcfg)
{
    ANN(cfg);
    ANN(dcfg);

    if (cfg->export_handle_type == 0)
    {
        return;
    }

    dvz_device_config_request_extension(dcfg, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
    dvz_device_config_request_extension(dcfg, VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
    dvz_device_config_request_extension(dcfg, VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);
#if OS_LINUX
    dvz_device_config_request_extension(dcfg, VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
    dvz_device_config_request_extension(dcfg, VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME);
#elif OS_WINDOWS
    dvz_device_config_request_extension(dcfg, VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME);
    dvz_device_config_request_extension(dcfg, VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME);
#endif
}



/**
 * Create the default queue requests for a selected GPU.
 *
 * @param instance the source instance
 * @param gpu_index the selected GPU index
 * @param[out] dcfg the destination device configuration
 * @return whether the queue requests were populated
 */
static bool _gpu_ctx_configure_queues(
    DvzInstance* instance, uint32_t gpu_index, DvzDeviceConfig* dcfg)
{
    ANN(instance);
    ANN(dcfg);

    DvzQueueCaps qc = {0};
    if (!dvz_instance_gpu_queue_caps(instance, gpu_index, &qc))
    {
        log_error("unable to query queue capabilities for GPU %" PRIu32, gpu_index);
        return false;
    }

    DvzQueues queues = {0};
    dvz_queues(&qc, &queues);
    for (uint32_t i = 0; i < queues.queue_count; i++)
    {
        DvzQueue* queue = &queues.queues[i];
        if (!dvz_device_config_request_queue(dcfg, dvz_queue_family(queue), 1))
        {
            log_error("unable to request queue family %" PRIu32, dvz_queue_family(queue));
            return false;
        }
    }

    return true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the default GPU-context configuration.
 *
 * @return the default configuration
 */
DvzGpuCtxConfig dvz_gpu_ctx_config(void)
{
    DvzGpuCtxConfig cfg = {
        .enable_validation = true,
        .gpu_index = 0,
        .export_handle_type = 0,
        .has_features10 = false,
        .has_features13 = false,
        .features10 = {0},
        .features13 = {0},
    };
    return cfg;
}



/**
 * Toggle instance validation for a GPU-context configuration.
 *
 * @param cfg the GPU-context configuration
 * @param enable_validation whether validation should be enabled
 */
void dvz_gpu_ctx_config_validation(DvzGpuCtxConfig* cfg, bool enable_validation)
{
    ANN(cfg);
    cfg->enable_validation = enable_validation;
}



/**
 * Select the GPU index to use for a GPU-context configuration.
 *
 * @param cfg the GPU-context configuration
 * @param gpu_index the selected GPU index
 */
void dvz_gpu_ctx_config_gpu(DvzGpuCtxConfig* cfg, uint32_t gpu_index)
{
    ANN(cfg);
    cfg->gpu_index = gpu_index;
}



/**
 * Select the allocator external-memory export policy for a GPU-context configuration.
 *
 * @param cfg the GPU-context configuration
 * @param export_handle_type external memory export flags
 */
void dvz_gpu_ctx_config_alloc(
    DvzGpuCtxConfig* cfg, VkExternalMemoryHandleTypeFlagsKHR export_handle_type)
{
    ANN(cfg);
    cfg->export_handle_type = export_handle_type;
}



/**
 * Copy Vulkan 1.0 features into a GPU-context configuration.
 *
 * @param cfg the GPU-context configuration
 * @param features the Vulkan 1.0 feature struct
 */
void dvz_gpu_ctx_config_features10(DvzGpuCtxConfig* cfg, const VkPhysicalDeviceFeatures* features)
{
    ANN(cfg);
    ANN(features);
    cfg->features10 = *features;
    cfg->has_features10 = true;
}


/**
 * Copy Vulkan 1.2 features into a GPU-context configuration.
 *
 * @param cfg the GPU-context configuration
 * @param features the Vulkan 1.2 feature struct
 */
void dvz_gpu_ctx_config_features12(
    DvzGpuCtxConfig* cfg, const VkPhysicalDeviceVulkan12Features* features)
{
    ANN(cfg);
    ANN(features);
    cfg->features12 = *features;
    cfg->has_features12 = true;
}



/**
 * Copy Vulkan 1.3 features into a GPU-context configuration.
 *
 * @param cfg the GPU-context configuration
 * @param features the Vulkan 1.3 feature struct
 */
void dvz_gpu_ctx_config_features13(
    DvzGpuCtxConfig* cfg, const VkPhysicalDeviceVulkan13Features* features)
{
    ANN(cfg);
    ANN(features);
    cfg->features13 = *features;
    cfg->has_features13 = true;
}



/**
 * Request an additional Vulkan instance extension.
 */
void dvz_gpu_ctx_config_add_instance_extension(DvzGpuCtxConfig* cfg, const char* extension)
{
    ANN(cfg);
    ANN(extension);
    if (cfg->instance_extension_count >= 16)
    {
        log_warn("too many instance extensions in DvzGpuCtxConfig");
        return;
    }
    cfg->instance_extensions[cfg->instance_extension_count++] = extension;
}



/**
 * Enable or disable canvas device extensions.
 */
void dvz_gpu_ctx_config_enable_canvas_extensions(DvzGpuCtxConfig* cfg, bool enable)
{
    ANN(cfg);
    cfg->enable_canvas_extensions = enable;
}



/**
 * Create an owned GPU context from a configuration.
 *
 * @param cfg the GPU-context configuration
 * @return allocated GPU context, or NULL on failure
 */
DvzGpuCtx* dvz_gpu_ctx(const DvzGpuCtxConfig* cfg)
{
    ANN(cfg);

    DvzGpuCtx* ctx = (DvzGpuCtx*)dvz_calloc(1, sizeof(DvzGpuCtx));
    ANN(ctx);
    ctx->cfg = *cfg;

    DvzInstanceConfig icfg = dvz_instance_default_config();
    icfg.flags = cfg->enable_validation ? DVZ_INSTANCE_VALIDATION_FLAGS : 0;
    for (uint32_t i = 0; i < cfg->instance_extension_count; i++)
        dvz_instance_config_request_extension(&icfg, cfg->instance_extensions[i]);
    ctx->instance = dvz_instance_create(&icfg);
    if (ctx->instance == NULL)
    {
        dvz_gpu_ctx_destroy(ctx);
        return NULL;
    }

    uint32_t gpu_count = dvz_instance_gpu_count(ctx->instance);
    if (gpu_count == 0 || cfg->gpu_index >= gpu_count)
    {
        log_error("invalid GPU index %" PRIu32 " for GPU context", cfg->gpu_index);
        dvz_gpu_ctx_destroy(ctx);
        return NULL;
    }

    DvzDeviceConfig dcfg = dvz_device_default_config(ctx->instance);
    if (!dvz_device_config_set_gpu_index(&dcfg, cfg->gpu_index))
    {
        dvz_gpu_ctx_destroy(ctx);
        return NULL;
    }
    if (!_gpu_ctx_configure_queues(ctx->instance, cfg->gpu_index, &dcfg))
    {
        dvz_gpu_ctx_destroy(ctx);
        return NULL;
    }
    if (cfg->has_features10)
    {
        dvz_device_config_set_features10(&dcfg, &cfg->features10);
    }
    if (cfg->has_features12)
    {
        dvz_device_config_set_features12(&dcfg, &cfg->features12);
    }
    if (cfg->has_features13)
    {
        dvz_device_config_set_features13(&dcfg, &cfg->features13);
    }
    if (cfg->enable_canvas_extensions)
    {
        dvz_device_config_enable_canvas_extensions(&dcfg, true);
    }
    _gpu_ctx_request_export_extensions(cfg, &dcfg);

    ctx->device = dvz_device_create(&dcfg);
    if (ctx->device == NULL)
    {
        dvz_gpu_ctx_destroy(ctx);
        return NULL;
    }

    ctx->allocator = dvz_allocator_create();
    if (ctx->allocator == NULL)
    {
        dvz_gpu_ctx_destroy(ctx);
        return NULL;
    }
    if (dvz_device_allocator(ctx->device, cfg->export_handle_type, ctx->allocator) != 0)
    {
        dvz_gpu_ctx_destroy(ctx);
        return NULL;
    }

    return ctx;
}


/**
 * Create an advanced GPU context for Vulkan-owned CUDA/CuPy interop buffers.
 *
 * @param gpu_index Vulkan physical-device index to use
 * @param memory_handle_type external memory handle type for exported allocations
 * @return owned GPU context, or NULL on failure
 */
DvzGpuCtx* dvz_interop_gpu_ctx_ex(
    uint32_t gpu_index, VkExternalMemoryHandleTypeFlagsKHR memory_handle_type,
    uint32_t instance_extension_count, const char* const* instance_extensions,
    bool enable_canvas_extensions)
{
    if (memory_handle_type == 0)
    {
        log_error("interop GPU context requires an external memory handle type");
        return NULL;
    }
    if (instance_extension_count > 0 && instance_extensions == NULL)
    {
        log_error("interop GPU context requires an extension-name array when count is nonzero");
        return NULL;
    }
    if (instance_extension_count > 16)
    {
        log_error("interop GPU context supports at most 16 Vulkan instance extensions");
        return NULL;
    }

    DvzGpuCtxConfig cfg = dvz_gpu_ctx_config();
    dvz_gpu_ctx_config_validation(&cfg, false);
    dvz_gpu_ctx_config_gpu(&cfg, gpu_index);
    dvz_gpu_ctx_config_alloc(&cfg, memory_handle_type);
    for (uint32_t i = 0; i < instance_extension_count; i++)
    {
        if (instance_extensions[i] == NULL)
        {
            log_error("interop GPU context received a NULL Vulkan instance extension name");
            return NULL;
        }
        dvz_gpu_ctx_config_add_instance_extension(&cfg, instance_extensions[i]);
    }
    dvz_gpu_ctx_config_add_instance_extension(
        &cfg, VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
    dvz_gpu_ctx_config_add_instance_extension(
        &cfg, VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);
    if (enable_canvas_extensions)
        dvz_gpu_ctx_config_enable_canvas_extensions(&cfg, true);

    VkPhysicalDeviceFeatures features10 = {0};
    features10.independentBlend = true;
    dvz_gpu_ctx_config_features10(&cfg, &features10);

    VkPhysicalDeviceVulkan12Features features12 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .timelineSemaphore = true,
    };
    dvz_gpu_ctx_config_features12(&cfg, &features12);

    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .dynamicRendering = true,
        .synchronization2 = true,
    };
    dvz_gpu_ctx_config_features13(&cfg, &features13);

    return dvz_gpu_ctx(&cfg);
}



/**
 * Create an advanced GPU context for Vulkan-owned CUDA/CuPy interop buffers.
 *
 * @param gpu_index Vulkan physical-device index to use
 * @param memory_handle_type external memory handle type for exported allocations
 * @return owned GPU context, or NULL on failure
 */
DvzGpuCtx* dvz_interop_gpu_ctx(
    uint32_t gpu_index, VkExternalMemoryHandleTypeFlagsKHR memory_handle_type)
{
    return dvz_interop_gpu_ctx_ex(gpu_index, memory_handle_type, 0, NULL, false);
}



/**
 * Return the instance owned by a GPU context.
 *
 * @param ctx the GPU context
 * @return borrowed instance
 */
DvzInstance* dvz_gpu_ctx_instance(DvzGpuCtx* ctx)
{
    ANN(ctx);
    return ctx->instance;
}



/**
 * Return the selected GPU index of a GPU context.
 *
 * @param ctx the GPU context
 * @return the selected GPU index, or UINT32_MAX when unavailable
 */
uint32_t dvz_gpu_ctx_gpu_index(DvzGpuCtx* ctx)
{
    ANN(ctx);
    return ctx->cfg.gpu_index;
}



/**
 * Return the selected GPU descriptor of a GPU context.
 *
 * @param ctx the GPU context
 * @param[out] out_info destination GPU descriptor
 * @return whether the descriptor could be retrieved
 */
bool dvz_gpu_ctx_gpu_info(DvzGpuCtx* ctx, DvzGpuInfo* out_info)
{
    ANN(ctx);
    ANN(out_info);
    if (ctx->instance == NULL)
    {
        return false;
    }
    return dvz_instance_gpu_info(ctx->instance, ctx->cfg.gpu_index, out_info);
}



/**
 * Return the device owned by a GPU context.
 *
 * @param ctx the GPU context
 * @return borrowed device
 */
DvzDevice* dvz_gpu_ctx_device(DvzGpuCtx* ctx)
{
    ANN(ctx);
    return ctx->device;
}



/**
 * Return the allocator owned by a GPU context.
 *
 * @param ctx the GPU context
 * @return borrowed allocator
 */
DvzVma* dvz_gpu_ctx_alloc(DvzGpuCtx* ctx)
{
    ANN(ctx);
    return ctx->allocator;
}



/**
 * Return a queue owned by the GPU-context device.
 *
 * @param ctx the GPU context
 * @param role the requested queue role
 * @return borrowed queue, or NULL when unavailable
 */
DvzQueue* dvz_gpu_ctx_queue(DvzGpuCtx* ctx, DvzQueueRole role)
{
    ANN(ctx);
    if (ctx->device == NULL)
    {
        return NULL;
    }
    return dvz_device_queue(ctx->device, role);
}



/**
 * Return the validation error count associated with a GPU context.
 *
 * @param ctx the GPU context
 * @return the validation error count
 */
uint32_t dvz_gpu_ctx_error_count(DvzGpuCtx* ctx)
{
    ANN(ctx);
    if (ctx->instance != NULL)
    {
        return dvz_instance_error_count(ctx->instance);
    }
    return ctx->validation_error_count;
}



/**
 * Destroy a GPU context and all owned runtime objects.
 *
 * @param ctx the GPU context
 */
void dvz_gpu_ctx_destroy(DvzGpuCtx* ctx)
{
    if (ctx == NULL)
    {
        return;
    }

    ctx->validation_error_count = dvz_gpu_ctx_error_count(ctx);
    if (ctx->allocator != NULL)
    {
        dvz_allocator_destroy(ctx->allocator);
        dvz_allocator_free(ctx->allocator);
        ctx->allocator = NULL;
    }
    if (ctx->device != NULL)
    {
        dvz_device_destroy(ctx->device);
        ctx->device = NULL;
    }
    if (ctx->instance != NULL)
    {
        dvz_instance_destroy(ctx->instance);
        ctx->instance = NULL;
    }
    dvz_free(ctx);
}
