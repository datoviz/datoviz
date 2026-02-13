/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing presentation                                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if OS_UNIX
#include <unistd.h>
#endif

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/instance.h"
#include "datoviz/vk/queues.h"
#include "datoviz/vklite/surface.h"
#include "datoviz/vklite/swapchain.h"
#include "datoviz/window.h"
#include "test_vklite.h"
#include "testing.h"

#ifndef DVZ_HAS_GLFW
#define DVZ_HAS_GLFW 0
#endif

#if DVZ_HAS_GLFW
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzVklitePresentFixture
{
    DvzWindowHost* host;
    DvzWindow* window;

    DvzInstance* instance;
    DvzGpu* gpu;
    DvzDevice* device;

    VkQueue queue;
    uint32_t queue_family;
} DvzVklitePresentFixture;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Destroy all resources owned by a presentation fixture.
 *
 * @param fixture fixture to cleanup
 */
static void _present_fixture_destroy(DvzVklitePresentFixture* fixture)
{
    if (fixture == NULL)
    {
        return;
    }

    if (fixture->host != NULL)
    {
        dvz_window_host_destroy(fixture->host);
        fixture->host = NULL;
    }

    if (fixture->device != NULL)
    {
        dvz_device_destroy(fixture->device);
        fixture->device = NULL;
    }

    if (fixture->instance != NULL)
    {
        dvz_instance_destroy(fixture->instance);
        fixture->instance = NULL;
    }
}



/**
 * Create a Vulkan instance configured for desktop surface presentation tests.
 *
 * @param instance instance structure to initialize
 * @param extensions GLFW-required extension names
 * @param ext_count number of extension names
 * @return true when Vulkan instance creation succeeds
 */
static DvzInstance* _present_instance_create(
    const char** extensions, uint32_t ext_count, uint32_t vk_version, bool force_portability)
{
    DvzInstanceConfig cfg = dvz_instance_default_config();
    cfg.flags = 0;
    cfg.vk_version = vk_version;
    dvz_instance_config_request_extension(&cfg, VK_KHR_SURFACE_EXTENSION_NAME);
    for (uint32_t i = 0; i < ext_count; i++)
    {
        dvz_instance_config_request_extension(&cfg, extensions[i]);
    }

    if (force_portability)
    {
        log_warn("vklite present tests: forcing portability enumeration extension request");
        cfg.portability = true;
    }
    else
    {
        cfg.portability = true;
    }

    return dvz_instance_create(&cfg);
}



/**
 * Print Vulkan loader-related environment diagnostics for the current process.
 */
static void _present_log_loader_env(void)
{
    const char* icd = getenv("VK_ICD_FILENAMES");
    const char* drv = getenv("VK_DRIVER_FILES");
    const char* dyld = getenv("DYLD_LIBRARY_PATH");

    log_warn("vklite present tests: VK_ICD_FILENAMES=%s", icd ? icd : "(null)");
    log_warn("vklite present tests: VK_DRIVER_FILES=%s", drv ? drv : "(null)");
    log_warn("vklite present tests: DYLD_LIBRARY_PATH=%s", dyld ? dyld : "(null)");

#if OS_UNIX
    if (icd != NULL && icd[0] != '\0')
    {
        // Validate the first ICD path in case multiple JSON files are provided with ':' separators.
        const char* sep = strchr(icd, ':');
        size_t len = (sep == NULL) ? strlen(icd) : (size_t)(sep - icd);
        char* first = (char*)dvz_calloc(len + 1, sizeof(char));
        if (first != NULL)
        {
            if (len > 0)
            {
                dvz_memcpy(first, len + 1, icd, len);
            }
            first[len] = '\0';
            int exists = access(first, R_OK);
            log_warn(
                "vklite present tests: first ICD path '%s' readable=%d", first,
                exists == 0 ? 1 : 0);
            dvz_free(first);
        }
    }
#endif
}



/**
 * Create a GLFW-backed Vulkan fixture for vklite presentation tests.
 *
 * @param fixture fixture to initialize
 * @return true when the fixture is ready, false when test should skip
 */
static bool _present_fixture_create(DvzVklitePresentFixture* fixture)
{
    ANN(fixture);
    dvz_memset(fixture, sizeof(*fixture), 0, sizeof(*fixture));

#if !DVZ_HAS_GLFW
    log_warn("vklite present tests skipped because Datoviz was built without GLFW support");
    return false;
#else
    _present_log_loader_env();

    fixture->host = dvz_window_host();
    if (fixture->host == NULL)
    {
        log_warn("vklite present tests skipped because window host creation failed");
        return false;
    }

    if (!dvz_window_glfw_init())
    {
        log_warn("vklite present tests skipped because GLFW could not initialize");
        return false;
    }

    uint32_t ext_count = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&ext_count);
    if (extensions == NULL || ext_count == 0)
    {
#if OS_MACOS
        static const char* macos_fallback_extensions[] = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            "VK_EXT_metal_surface",
        };
        log_warn(
            "vklite present tests: GLFW returned no Vulkan extensions, using macOS fallback "
            "extension list");
        extensions = macos_fallback_extensions;
        ext_count = 2;
#else
        log_warn("vklite present tests skipped because GLFW returned no Vulkan extensions");
        return false;
#endif
    }
    fixture->instance =
        _present_instance_create(extensions, ext_count, VK_API_VERSION_1_3, false);
    if (fixture->instance == NULL)
    {
        log_warn("vklite present tests skipped because Vulkan instance creation failed");
        return false;
    }

    uint32_t gpu_count = 0;
    DvzGpu* gpus = dvz_instance_gpus(fixture->instance, &gpu_count);
    if (gpus == NULL || gpu_count == 0)
    {
        log_warn(
            "vklite present tests: no Vulkan GPU found on first attempt, retrying with explicit "
            "portability setup");
        dvz_instance_destroy(fixture->instance);
        fixture->instance =
            _present_instance_create(extensions, ext_count, VK_API_VERSION_1_3, true);
        if (fixture->instance == NULL)
        {
            log_warn(
                "vklite present tests skipped because Vulkan instance recreation failed "
                "during portability retry");
            return false;
        }
        gpus = dvz_instance_gpus(fixture->instance, &gpu_count);
        if (gpus == NULL || gpu_count == 0)
        {
            log_warn("vklite present tests skipped because no Vulkan GPU is available");
            return false;
        }
    }
    fixture->gpu = &gpus[0];

    DvzQueues queues = {0};
    dvz_queues(dvz_gpu_queue_caps(fixture->gpu), &queues);
    DvzDeviceConfig dcfg = dvz_device_default_config(fixture->gpu);
    for (uint32_t i = 0; i < queues.queue_count; i++)
    {
        DvzQueue* queue = &queues.queues[i];
        dvz_device_config_request_queue(&dcfg, queue->family_idx, 1);
    }
    dvz_device_config_enable_canvas_extensions(&dcfg, true);
    fixture->device = dvz_device_create(&dcfg);
    if (fixture->device == NULL)
    {
        log_warn("vklite present tests skipped because Vulkan device creation failed");
        return false;
    }

    DvzWindowConfig cfg = dvz_window_default_config();
    cfg.title = "vklite-present-test";
    cfg.width = 320;
    cfg.height = 240;

    fixture->window = dvz_window_create(fixture->host, DVZ_BACKEND_GLFW, &cfg);
    if (fixture->window == NULL || dvz_window_backend_type(fixture->window) != DVZ_BACKEND_GLFW)
    {
        log_warn("vklite present tests skipped because GLFW window creation failed");
        return false;
    }

    DvzQueue* queue_ref = dvz_device_queue(fixture->device, DVZ_QUEUE_MAIN);
    if (queue_ref == NULL)
    {
        log_warn("vklite present tests skipped because main queue is unavailable");
        return false;
    }
    fixture->queue = dvz_queue_handle(queue_ref);
    fixture->queue_family = dvz_queue_family(queue_ref);
    return true;
#endif
}



/**
 * Configure a swapchain from a ready surface wrapper.
 *
 * @param swapchain swapchain wrapper to initialize
 * @param fixture initialized fixture
 * @param surface initialized surface wrapper
 * @return true on success
 */
static bool _swapchain_prepare(
    DvzSwapchain* swapchain, DvzVklitePresentFixture* fixture, DvzSurface* surface)
{
    ANN(swapchain);
    ANN(fixture);
    ANN(surface);

    if (!dvz_swapchain_init(swapchain, fixture->gpu, surface))
    {
        return false;
    }

    DvzSwapchainConfig cfg = {0};
    cfg.image_format = surface->preferred_format.format;
    cfg.color_space = surface->preferred_format.colorSpace;
    cfg.present_mode = surface->preferred_present_mode;
    cfg.image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    cfg.composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    cfg.clipped = true;
    if (!dvz_swapchain_config(swapchain, cfg))
    {
        return false;
    }

    return true;
}



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

/**
 * Verify vklite surface wrapper queries and caches capabilities.
 *
 * @param suite test suite
 * @param tstitem current test item
 * @return 0 on success
 */
int test_vklite_surface_query(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzVklitePresentFixture fixture = {0};
    if (!_present_fixture_create(&fixture))
    {
        _present_fixture_destroy(&fixture);
        return 0;
    }

    DvzSurface surface = {0};

    const DvzWindowSurface* window_surface = dvz_window_surface(fixture.window);
    if (window_surface == NULL || window_surface->surface == VK_NULL_HANDLE)
    {
        log_warn("vklite surface query test skipped because native surface is unavailable");
        _present_fixture_destroy(&fixture);
        return 0;
    }

    AT(dvz_surface_init(&surface, fixture.gpu, fixture.queue_family));
    AT(dvz_surface_wrap_native(&surface, window_surface->surface, fixture.window));

    AT(surface.ready);
    AT(surface.handle != VK_NULL_HANDLE);
    AT(surface.handle == window_surface->surface);
    AT(surface.format_count > 0);
    AT(surface.present_mode_count > 0);
    AT(surface.formats != NULL);
    AT(surface.present_modes != NULL);

    AT(dvz_surface_refresh(&surface));
    AT(surface.ready);

    dvz_surface_destroy(&surface);
    _present_fixture_destroy(&fixture);
    return 0;
}



/**
 * Verify swapchain recreate allocates images/views and reports explicit status codes.
 *
 * @param suite test suite
 * @param tstitem current test item
 * @return 0 on success
 */
int test_vklite_swapchain_recreate(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzVklitePresentFixture fixture = {0};
    if (!_present_fixture_create(&fixture))
    {
        _present_fixture_destroy(&fixture);
        return 0;
    }

    DvzSurface surface = {0};
    DvzSwapchain swapchain = {0};

    const DvzWindowSurface* window_surface = dvz_window_surface(fixture.window);
    if (window_surface == NULL || window_surface->surface == VK_NULL_HANDLE)
    {
        log_warn("vklite swapchain recreate test skipped because native surface is unavailable");
        _present_fixture_destroy(&fixture);
        return 0;
    }

    AT(dvz_surface_init(&surface, fixture.gpu, fixture.queue_family));
    AT(dvz_surface_wrap_native(&surface, window_surface->surface, fixture.window));
    AT(_swapchain_prepare(&swapchain, &fixture, &surface));

    // Device binding is required before recreate.
    uvec2 size = {window_surface->extent.width, window_surface->extent.height};
    DvzPresentStatus status = dvz_swapchain_recreate(&swapchain, size);
    AT(status == DVZ_PRESENT_STATUS_ERROR);

    AT(dvz_swapchain_device(&swapchain, dvz_device_handle(fixture.device)));
    status = dvz_swapchain_recreate(&swapchain, size);

    if (status == DVZ_PRESENT_STATUS_SKIP_ZERO_EXTENT)
    {
        log_warn("vklite swapchain recreate test skipped because window extent is zero");
        dvz_swapchain_destroy(&swapchain);
        dvz_surface_destroy(&surface);
        _present_fixture_destroy(&fixture);
        return 0;
    }

    AT(status == DVZ_PRESENT_STATUS_OK);
    AT(swapchain.ready);
    AT(swapchain.handle != VK_NULL_HANDLE);
    AT(swapchain.image_count > 0);
    AT(swapchain.images != NULL);
    AT(swapchain.image_views != NULL);

    dvz_swapchain_destroy(&swapchain);
    dvz_surface_destroy(&surface);
    _present_fixture_destroy(&fixture);
    return 0;
}



/**
 * Verify IMMEDIATE present mode is preserved when explicitly requested by config.
 *
 * @param suite test suite
 * @param tstitem current test item
 * @return 0 on success
 */
int test_vklite_swapchain_config_present_mode_immediate(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzVklitePresentFixture fixture = {0};
    if (!_present_fixture_create(&fixture))
    {
        _present_fixture_destroy(&fixture);
        return 0;
    }

    DvzSurface surface = {0};
    DvzSwapchain swapchain = {0};

    const DvzWindowSurface* window_surface = dvz_window_surface(fixture.window);
    if (window_surface == NULL || window_surface->surface == VK_NULL_HANDLE)
    {
        log_warn("vklite swapchain config test skipped because native surface is unavailable");
        _present_fixture_destroy(&fixture);
        return 0;
    }

    AT(dvz_surface_init(&surface, fixture.gpu, fixture.queue_family));
    AT(dvz_surface_wrap_native(&surface, window_surface->surface, fixture.window));
    AT(dvz_swapchain_init(&swapchain, fixture.gpu, &surface));

    DvzSwapchainConfig cfg = {0};
    cfg.image_format = surface.preferred_format.format;
    cfg.color_space = surface.preferred_format.colorSpace;
    cfg.present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    cfg.image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    cfg.composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    cfg.clipped = true;
    AT(dvz_swapchain_config(&swapchain, cfg));
    AT(swapchain.config.present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR);

    dvz_swapchain_destroy(&swapchain);
    dvz_surface_destroy(&surface);
    _present_fixture_destroy(&fixture);
    return 0;
}



/**
 * Verify partial swapchain config keeps deterministic defaults for unspecified fields.
 *
 * @param suite test suite
 * @param tstitem current test item
 * @return 0 on success
 */
int test_vklite_swapchain_config_defaults_partial(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzSwapchain swapchain = {0};
    DvzSwapchainConfig cfg = {0};
    cfg.image_format = VK_FORMAT_B8G8R8A8_UNORM;
    cfg.clipped = true;

    AT(dvz_swapchain_config(&swapchain, cfg));
    AT(swapchain.config.image_format == VK_FORMAT_B8G8R8A8_UNORM);
    AT(swapchain.config.color_space == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);
    // VK_PRESENT_MODE_IMMEDIATE_KHR has enum value 0; only fully-zeroed configs default to FIFO.
    AT(swapchain.config.present_mode == 0);
    AT(
        swapchain.config.image_usage ==
        (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT));
    AT(swapchain.config.composite_alpha == VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR);

    return 0;
}



/**
 * Verify present returns an error when image index is out of range.
 *
 * @param suite test suite
 * @param tstitem current test item
 * @return 0 on success
 */
int test_vklite_swapchain_present_invalid_index(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzSwapchain swapchain = {0};
    swapchain.ready = true;
    swapchain.image_count = 1;
    swapchain.handle = (VkSwapchainKHR)(uintptr_t)0x1;

    DvzPresentStatus status =
        dvz_swapchain_present(&swapchain, (VkQueue)(uintptr_t)0x1, 1, VK_NULL_HANDLE);
    AT(status == DVZ_PRESENT_STATUS_ERROR);

    return 0;
}



/**
 * Verify recreate persists the resolved swapchain state selected against surface capabilities.
 *
 * @param suite test suite
 * @param tstitem current test item
 * @return 0 on success
 */
int test_vklite_swapchain_recreate_resolved_state(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzVklitePresentFixture fixture = {0};
    if (!_present_fixture_create(&fixture))
    {
        _present_fixture_destroy(&fixture);
        return 0;
    }

    DvzSurface surface = {0};
    DvzSwapchain swapchain = {0};

    const DvzWindowSurface* window_surface = dvz_window_surface(fixture.window);
    if (window_surface == NULL || window_surface->surface == VK_NULL_HANDLE)
    {
        log_warn("vklite resolved state test skipped because native surface is unavailable");
        _present_fixture_destroy(&fixture);
        return 0;
    }

    AT(dvz_surface_init(&surface, fixture.gpu, fixture.queue_family));
    AT(dvz_surface_wrap_native(&surface, window_surface->surface, fixture.window));
    AT(dvz_swapchain_init(&swapchain, fixture.gpu, &surface));
    AT(dvz_swapchain_device(&swapchain, dvz_device_handle(fixture.device)));

    DvzSwapchainConfig cfg = {0};
    cfg.image_format = VK_FORMAT_B8G8R8A8_UNORM;
    cfg.color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    cfg.present_mode = (VkPresentModeKHR)999;
    cfg.image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    cfg.composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    cfg.clipped = true;
    AT(dvz_swapchain_config(&swapchain, cfg));

    uvec2 size = {window_surface->extent.width, window_surface->extent.height};
    DvzPresentStatus status = dvz_swapchain_recreate(&swapchain, size);
    if (status == DVZ_PRESENT_STATUS_SKIP_ZERO_EXTENT)
    {
        log_warn("vklite resolved state test skipped because window extent is zero");
        dvz_swapchain_destroy(&swapchain);
        dvz_surface_destroy(&surface);
        _present_fixture_destroy(&fixture);
        return 0;
    }

    AT(status == DVZ_PRESENT_STATUS_OK);
    AT(swapchain.image_format != VK_FORMAT_UNDEFINED);
    AT(swapchain.image_format == swapchain.surface->preferred_format.format);
    AT(swapchain.color_space == swapchain.surface->preferred_format.colorSpace);
    AT(swapchain.present_mode == swapchain.surface->preferred_present_mode);

    dvz_swapchain_destroy(&swapchain);
    dvz_surface_destroy(&surface);
    _present_fixture_destroy(&fixture);
    return 0;
}
