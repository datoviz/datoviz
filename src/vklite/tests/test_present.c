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
#include "wrap_surface_fixture.h"

#include <volk.h>

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

    uint32_t ext_count = dvz_window_host_required_extension_count(fixture->host, DVZ_BACKEND_GLFW);
    const char** extensions = NULL;
    bool extensions_allocated = false;
    if (ext_count > 0)
    {
        extensions = dvz_calloc(ext_count, sizeof(char*));
        if (extensions == NULL)
        {
            log_warn("vklite present tests skipped because extension-list allocation failed");
            return false;
        }
        extensions_allocated = true;
        int written =
            dvz_window_host_required_extensions(fixture->host, DVZ_BACKEND_GLFW, ext_count, extensions);
        if (written != (int)ext_count)
        {
            log_warn("vklite present tests skipped because required-extension query failed");
            dvz_free((void*)extensions);
            return false;
        }
    }
    if (ext_count == 0)
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
        if (extensions_allocated)
            dvz_free((void*)extensions);
        return false;
#endif
    }
    fixture->instance =
        _present_instance_create(extensions, ext_count, VK_API_VERSION_1_3, false);
    if (extensions_allocated)
        dvz_free((void*)extensions);
    if (fixture->instance == NULL)
    {
        log_warn("vklite present tests skipped because Vulkan instance creation failed");
        return false;
    }

    uint32_t gpu_count = dvz_instance_gpu_count(fixture->instance);
    if (gpu_count == 0)
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
        gpu_count = dvz_instance_gpu_count(fixture->instance);
        if (gpu_count == 0)
        {
            log_warn("vklite present tests skipped because no Vulkan GPU is available");
            return false;
        }
    }

    DvzGpuInfo gpu_info = {0};
    if (!dvz_instance_gpu_info(fixture->instance, 0, &gpu_info))
    {
        log_warn("vklite present tests skipped because GPU descriptor query failed");
        return false;
    }
    DvzQueues queues = {0};
    dvz_queues(&gpu_info.queue_caps, &queues);
    DvzDeviceConfig dcfg = dvz_device_default_config(fixture->instance);
    dvz_device_config_set_gpu_index(&dcfg, 0);
    for (uint32_t i = 0; i < queues.queue_count; i++)
    {
        DvzQueue* queue = &queues.queues[i];
        dvz_device_config_request_queue(&dcfg, dvz_queue_family(queue), 1);
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

    if (!dvz_swapchain_init_from_device(swapchain, fixture->device, surface))
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



#if DVZ_HAS_GLFW
typedef struct DvzVkliteWrapSurfaceFixture
{
    DvzWindow* wrap_window;
    GLFWwindow* external_handle;
    VkSurfaceKHR external_surface;
    DvzWindowExternalSurfaceInfo info;
    uvec2 size;
} DvzVkliteWrapSurfaceFixture;



/**
 * Initialize a wrap window and external GLFW surface for present-path tests.
 *
 * @param fixture parent Vulkan present fixture
 * @param cfg window configuration used for both wrap and external GLFW windows
 * @param wrap output wrap-surface fixture storage
 * @return true on success, false when setup should skip
 */
static bool _wrap_surface_fixture_create(
    DvzVklitePresentFixture* fixture, const DvzWindowConfig* cfg, DvzVkliteWrapSurfaceFixture* wrap)
{
    ANN(fixture);
    ANN(cfg);
    ANN(wrap);
    dvz_memset(wrap, sizeof(*wrap), 0, sizeof(*wrap));

    wrap->wrap_window = dvz_window_create(fixture->host, DVZ_BACKEND_WRAP, cfg);
    if (wrap->wrap_window == NULL || dvz_window_backend_type(wrap->wrap_window) != DVZ_BACKEND_WRAP)
    {
        log_warn("vklite wrap present test skipped because wrap window creation failed");
        return false;
    }

    wrap->external_handle = glfwCreateWindow((int)cfg->width, (int)cfg->height, cfg->title, NULL, NULL);
    if (wrap->external_handle == NULL)
    {
        log_warn("vklite wrap present test skipped because external GLFW window creation failed");
        return false;
    }

    VkInstance instance = dvz_instance_handle(fixture->instance);
    VkResult surface_res =
        glfwCreateWindowSurface(instance, wrap->external_handle, NULL, &wrap->external_surface);
    if (surface_res != VK_SUCCESS || wrap->external_surface == VK_NULL_HANDLE)
    {
        log_warn(
            "vklite wrap present test skipped because external GLFW surface creation failed (%d)",
            (int)surface_res);
        return false;
    }

    wrap->info = dvz_test_wrap_surface_info(
        instance, wrap->external_surface, cfg->width, cfg->height, 1.0f, 1.0f, false);
    if (dvz_window_wrap_attach_surface(wrap->wrap_window, &wrap->info) != 0)
    {
        log_warn("vklite wrap present test skipped because wrap attach_surface() failed");
        return false;
    }

    wrap->size[0] = cfg->width;
    wrap->size[1] = cfg->height;
    return true;
}



/**
 * Destroy wrap external-surface test resources.
 *
 * @param fixture parent Vulkan present fixture
 * @param wrap wrap-surface fixture storage
 */
static void
_wrap_surface_fixture_destroy(DvzVklitePresentFixture* fixture, DvzVkliteWrapSurfaceFixture* wrap)
{
    if (fixture == NULL || wrap == NULL)
        return;
    if (wrap->wrap_window != NULL)
    {
        dvz_window_wrap_detach_surface(wrap->wrap_window);
    }
    if (wrap->external_surface != VK_NULL_HANDLE && fixture->instance != NULL)
    {
        vkDestroySurfaceKHR(dvz_instance_handle(fixture->instance), wrap->external_surface, NULL);
        wrap->external_surface = VK_NULL_HANDLE;
    }
    if (wrap->external_handle != NULL)
    {
        glfwDestroyWindow(wrap->external_handle);
        wrap->external_handle = NULL;
    }
    if (wrap->wrap_window != NULL)
    {
        dvz_window_destroy(wrap->wrap_window);
        wrap->wrap_window = NULL;
    }
}
#endif



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

    AT(dvz_surface_init_from_device(&surface, fixture.device, fixture.queue_family));
    AT(dvz_surface_wrap_native(&surface, window_surface->surface, &window_surface->extent));

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

    AT(dvz_surface_init_from_device(&surface, fixture.device, fixture.queue_family));
    AT(dvz_surface_wrap_native(&surface, window_surface->surface, &window_surface->extent));
    AT(_swapchain_prepare(&swapchain, &fixture, &surface));

    // Device binding is required before recreate.
    uvec2 size = {window_surface->extent.width, window_surface->extent.height};
    DvzPresentStatus status = DVZ_PRESENT_STATUS_OK;
    tst_expect_error_begin(suite);
    status = dvz_swapchain_recreate(&swapchain, size);
    AT(status == DVZ_PRESENT_STATUS_ERROR);
    (void)tst_expect_error_end(suite);

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

    AT(dvz_surface_init_from_device(&surface, fixture.device, fixture.queue_family));
    AT(dvz_surface_wrap_native(&surface, window_surface->surface, &window_surface->extent));
    AT(dvz_swapchain_init_from_device(&swapchain, fixture.device, &surface));

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

    DvzPresentStatus status = DVZ_PRESENT_STATUS_OK;
    tst_expect_error_begin(suite);
    status = dvz_swapchain_present(&swapchain, (VkQueue)(uintptr_t)0x1, 1, VK_NULL_HANDLE);
    AT(status == DVZ_PRESENT_STATUS_ERROR);
    (void)tst_expect_error_end(suite);

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

    AT(dvz_surface_init_from_device(&surface, fixture.device, fixture.queue_family));
    AT(dvz_surface_wrap_native(&surface, window_surface->surface, &window_surface->extent));
    AT(dvz_swapchain_init_from_device(&swapchain, fixture.device, &surface));
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



/**
 * Verify wrap backend supports present-path setup, surface loss, and restore with an external
 * native surface.
 *
 * @param suite test suite
 * @param tstitem current test item
 * @return 0 on success
 */
#if DVZ_HAS_GLFW
int test_vklite_wrap_backend_external_surface_present(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzVklitePresentFixture fixture = {0};
    if (!_present_fixture_create(&fixture))
    {
        _present_fixture_destroy(&fixture);
        return 0;
    }

    uint32_t ext_count = dvz_window_host_required_extension_count(fixture.host, DVZ_BACKEND_GLFW);
    if (ext_count > 0)
    {
        const char** extensions = dvz_calloc(ext_count, sizeof(char*));
        ANN(extensions);
        AT(dvz_window_host_required_extensions(fixture.host, DVZ_BACKEND_GLFW, ext_count, extensions) == (int)ext_count);
        AT(dvz_window_wrap_set_required_extensions(fixture.host, ext_count, extensions) == 0);
        AT(dvz_window_host_required_extension_count(fixture.host, DVZ_BACKEND_WRAP) == ext_count);
        dvz_free((void*)extensions);
    }

    DvzWindowConfig cfg = dvz_test_wrap_window_config("vklite-wrap-external-surface", 320, 240);
    DvzVkliteWrapSurfaceFixture wrap = {0};
    if (!_wrap_surface_fixture_create(&fixture, &cfg, &wrap))
    {
        _wrap_surface_fixture_destroy(&fixture, &wrap);
        _present_fixture_destroy(&fixture);
        return 0;
    }

    DvzSurface surface = {0};
    DvzSwapchain swapchain = {0};
    AT(dvz_surface_init_from_device(&surface, fixture.device, fixture.queue_family));
    VkExtent2D wrap_extent = {.width = wrap.size[0], .height = wrap.size[1]};
    AT(dvz_surface_wrap_native(&surface, wrap.external_surface, &wrap_extent));
    AT(_swapchain_prepare(&swapchain, &fixture, &surface));
    AT(dvz_swapchain_device(&swapchain, dvz_device_handle(fixture.device)));

    DvzPresentStatus status = dvz_swapchain_recreate(&swapchain, wrap.size);
    if (status == DVZ_PRESENT_STATUS_SKIP_ZERO_EXTENT)
    {
        log_warn("vklite wrap present test skipped because wrap-window extent is zero");
        dvz_swapchain_destroy(&swapchain);
        dvz_surface_destroy(&surface);
        _wrap_surface_fixture_destroy(&fixture, &wrap);
        _present_fixture_destroy(&fixture);
        return 0;
    }

    AT(status == DVZ_PRESENT_STATUS_OK);
    AT(swapchain.ready);
    AT(swapchain.handle != VK_NULL_HANDLE);
    const DvzWindowSurface* window_surface = dvz_window_surface(wrap.wrap_window);
    ANN(window_surface);
    AT(window_surface->surface == wrap.external_surface);

    DvzWindowExternalSurfaceInfo loss =
        dvz_test_wrap_surface_info(VK_NULL_HANDLE, VK_NULL_HANDLE, cfg.width, cfg.height, 1.0f, 1.0f, false);
    AT(dvz_window_wrap_update_surface(wrap.wrap_window, &loss) == 0);
    window_surface = dvz_window_surface(wrap.wrap_window);
    ANN(window_surface);
    AT(window_surface->instance == VK_NULL_HANDLE);
    AT(window_surface->surface == VK_NULL_HANDLE);

    AT(dvz_window_wrap_update_surface(wrap.wrap_window, &wrap.info) == 0);
    window_surface = dvz_window_surface(wrap.wrap_window);
    ANN(window_surface);
    AT(window_surface->instance == wrap.info.instance);
    AT(window_surface->surface == wrap.external_surface);

    status = dvz_swapchain_recreate(&swapchain, wrap.size);
    if (status == DVZ_PRESENT_STATUS_SKIP_ZERO_EXTENT)
    {
        log_warn("vklite wrap present test skipped during restore because extent is zero");
        dvz_swapchain_destroy(&swapchain);
        dvz_surface_destroy(&surface);
        _wrap_surface_fixture_destroy(&fixture, &wrap);
        _present_fixture_destroy(&fixture);
        return 0;
    }
    AT(status == DVZ_PRESENT_STATUS_OK);

    dvz_swapchain_destroy(&swapchain);
    dvz_surface_destroy(&surface);
    dvz_window_wrap_detach_surface(wrap.wrap_window);
    window_surface = dvz_window_surface(wrap.wrap_window);
    ANN(window_surface);
    AT(window_surface->surface == VK_NULL_HANDLE);
    AT(window_surface->instance == VK_NULL_HANDLE);

    _wrap_surface_fixture_destroy(&fixture, &wrap);
    _present_fixture_destroy(&fixture);
    return 0;
}
#else
int test_vklite_wrap_backend_external_surface_present(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);
    log_warn("vklite wrap present test skipped because Datoviz was built without GLFW support");
    return 0;
}
#endif
