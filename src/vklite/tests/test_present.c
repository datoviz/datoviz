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
#include "_env.h"
#include "_log.h"
#include "datoviz/vk/device.h"
#include "datoviz/vk/gpu.h"
#include "datoviz/vk/instance.h"
#include "datoviz/vk/queues.h"
#include "datoviz/vklite/surface.h"
#include "datoviz/vklite/swapchain.h"
#include "datoviz/vklite/sync.h"
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
 * Return whether automated vklite present tests should create visible windows.
 *
 * @return true when DVZ_TEST_VISIBLE is set to a non-zero value
 */
static bool _present_test_visible(void)
{
    return checkenv("DVZ_TEST_VISIBLE");
}



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

    log_debug("vklite present tests: VK_ICD_FILENAMES=%s", icd ? icd : "(null)");
    log_debug("vklite present tests: VK_DRIVER_FILES=%s", drv ? drv : "(null)");
    log_debug("vklite present tests: DYLD_LIBRARY_PATH=%s", dyld ? dyld : "(null)");

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
            log_debug(
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
    cfg.visible = _present_test_visible();

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
    VkSurfaceFormatKHR preferred_format = dvz_surface_preferred_format(surface);
    cfg.image_format = preferred_format.format;
    cfg.color_space = preferred_format.colorSpace;
    cfg.present_mode = dvz_surface_preferred_present_mode(surface);
    cfg.image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    cfg.composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    cfg.clipped = true;
    if (!dvz_swapchain_config(swapchain, cfg))
    {
        return false;
    }

    return true;
}



/**
 * Create and recreate a present-ready swapchain for the current fixture window.
 *
 * @param fixture initialized present fixture
 * @param[out] surface created surface wrapper
 * @param[out] swapchain created swapchain wrapper
 * @param[out] extent resolved target extent used for recreate
 * @return true when setup succeeds, false when the test should skip or fail early
 */
static bool _present_ready_swapchain(
    DvzVklitePresentFixture* fixture, DvzSurface** surface, DvzSwapchain** swapchain, uvec2 extent)
{
    ANN(fixture);
    ANN(surface);
    ANN(swapchain);

    *surface = dvz_surface_create_wrapper();
    *swapchain = dvz_swapchain_create_wrapper();
    ANN(*surface);
    ANN(*swapchain);

    const DvzWindowSurface* window_surface = dvz_window_surface(fixture->window);
    if (window_surface == NULL || window_surface->surface == VK_NULL_HANDLE)
    {
        log_warn("vklite present test skipped because native surface is unavailable");
        return false;
    }

    AT(dvz_surface_init_from_device(*surface, fixture->device, fixture->queue_family));
    AT(dvz_surface_wrap_native(*surface, window_surface->surface, &window_surface->extent));
    AT(_swapchain_prepare(*swapchain, fixture, *surface));

    DvzPresentStatus status = dvz_swapchain_recreate(*swapchain, extent);
    if (status == DVZ_PRESENT_STATUS_SKIP_ZERO_EXTENT)
    {
        log_warn("vklite present test skipped because window extent is zero");
        return false;
    }

    AT(status == DVZ_PRESENT_STATUS_OK);
    AT(dvz_swapchain_ready(*swapchain));
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

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, cfg->visible ? GLFW_TRUE : GLFW_FALSE);
    wrap->external_handle =
        glfwCreateWindow((int)cfg->width, (int)cfg->height, cfg->title, NULL, NULL);
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
int test_vklite_surface_query(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzVklitePresentFixture fixture = {0};
    if (!_present_fixture_create(&fixture))
    {
        tst_skip(suite, "vklite present fixture unavailable");
        _present_fixture_destroy(&fixture);
        return 0;
    }

    DvzSurface* surface = dvz_surface_create_wrapper();
    ANN(surface);

    const DvzWindowSurface* window_surface = dvz_window_surface(fixture.window);
    if (window_surface == NULL || window_surface->surface == VK_NULL_HANDLE)
    {
        log_warn("vklite surface query test skipped because native surface is unavailable");
        tst_skip(suite, "native surface unavailable");
        dvz_surface_free(surface);
        _present_fixture_destroy(&fixture);
        return 0;
    }

    AT(dvz_surface_init_from_device(surface, fixture.device, fixture.queue_family));
    AT(dvz_surface_wrap_native(surface, window_surface->surface, &window_surface->extent));

    AT(dvz_surface_ready(surface));
    AT(dvz_surface_handle(surface) != VK_NULL_HANDLE);
    AT(dvz_surface_handle(surface) == window_surface->surface);
    AT(dvz_surface_format_count(surface) > 0);
    AT(dvz_surface_present_mode_count(surface) > 0);
    VkSurfaceFormatKHR cached_format = {0};
    VkPresentModeKHR cached_mode = VK_PRESENT_MODE_FIFO_KHR;
    AT(dvz_surface_format(surface, 0, &cached_format));
    AT(dvz_surface_present_mode(surface, 0, &cached_mode));

    AT(dvz_surface_refresh(surface));
    AT(dvz_surface_ready(surface));

    dvz_surface_destroy(surface);
    dvz_surface_free(surface);
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
int test_vklite_swapchain_recreate(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzVklitePresentFixture fixture = {0};
    if (!_present_fixture_create(&fixture))
    {
        tst_skip(suite, "vklite present fixture unavailable");
        _present_fixture_destroy(&fixture);
        return 0;
    }

    DvzSurface* surface = dvz_surface_create_wrapper();
    DvzSwapchain* swapchain = dvz_swapchain_create_wrapper();
    ANN(surface);
    ANN(swapchain);

    const DvzWindowSurface* window_surface = dvz_window_surface(fixture.window);
    if (window_surface == NULL || window_surface->surface == VK_NULL_HANDLE)
    {
        log_warn("vklite swapchain recreate test skipped because native surface is unavailable");
        tst_skip(suite, "native surface unavailable");
        dvz_swapchain_free(swapchain);
        dvz_surface_free(surface);
        _present_fixture_destroy(&fixture);
        return 0;
    }

    AT(dvz_surface_init_from_device(surface, fixture.device, fixture.queue_family));
    AT(dvz_surface_wrap_native(surface, window_surface->surface, &window_surface->extent));
    AT(_swapchain_prepare(swapchain, &fixture, surface));

    uvec2 size = {window_surface->extent.width, window_surface->extent.height};
    DvzPresentStatus status = dvz_swapchain_recreate(swapchain, size);

    if (status == DVZ_PRESENT_STATUS_SKIP_ZERO_EXTENT)
    {
        log_warn("vklite swapchain recreate test skipped because window extent is zero");
        tst_skip(suite, "window extent is zero");
        dvz_swapchain_destroy(swapchain);
        dvz_surface_destroy(surface);
        dvz_swapchain_free(swapchain);
        dvz_surface_free(surface);
        _present_fixture_destroy(&fixture);
        return 0;
    }

    AT(status == DVZ_PRESENT_STATUS_OK);
    AT(dvz_swapchain_ready(swapchain));
    AT(dvz_swapchain_handle(swapchain) != VK_NULL_HANDLE);
    AT(dvz_swapchain_image_count(swapchain) > 0);
    VkImage image = VK_NULL_HANDLE;
    VkImageView image_view = VK_NULL_HANDLE;
    AT(dvz_swapchain_image(swapchain, 0, &image));
    AT(dvz_swapchain_image_view(swapchain, 0, &image_view));
    AT(image != VK_NULL_HANDLE);
    AT(image_view != VK_NULL_HANDLE);

    dvz_swapchain_destroy(swapchain);
    dvz_surface_destroy(surface);
    dvz_swapchain_free(swapchain);
    dvz_surface_free(surface);
    _present_fixture_destroy(&fixture);
    return 0;
}



/**
 * Verify present wrappers tolerate repeated destroy and reject live swapchain rebinding.
 *
 * @param suite test suite
 * @param tstitem current test item
 * @return 0 on success
 */
int test_vklite_surface_swapchain_destroy_idempotent(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzVklitePresentFixture fixture = {0};
    if (!_present_fixture_create(&fixture))
    {
        tst_skip(suite, "vklite present fixture unavailable");
        _present_fixture_destroy(&fixture);
        return 0;
    }

    DvzSurface* surface = dvz_surface_create_wrapper();
    DvzSwapchain* swapchain = dvz_swapchain_create_wrapper();
    ANN(surface);
    ANN(swapchain);

    const DvzWindowSurface* window_surface = dvz_window_surface(fixture.window);
    if (window_surface == NULL || window_surface->surface == VK_NULL_HANDLE)
    {
        log_warn("vklite destroy-idempotent test skipped because native surface is unavailable");
        tst_skip(suite, "native surface unavailable");
        dvz_swapchain_free(swapchain);
        dvz_surface_free(surface);
        _present_fixture_destroy(&fixture);
        return 0;
    }

    AT(dvz_surface_init_from_device(surface, fixture.device, fixture.queue_family));
    AT(dvz_surface_wrap_native(surface, window_surface->surface, &window_surface->extent));
    AT(_swapchain_prepare(swapchain, &fixture, surface));

    uvec2 size = {window_surface->extent.width, window_surface->extent.height};
    DvzPresentStatus status = dvz_swapchain_recreate(swapchain, size);
    if (status == DVZ_PRESENT_STATUS_SKIP_ZERO_EXTENT)
    {
        log_warn("vklite destroy-idempotent test skipped because window extent is zero");
        tst_skip(suite, "window extent is zero");
        dvz_swapchain_destroy(swapchain);
        dvz_surface_destroy(surface);
        dvz_swapchain_free(swapchain);
        dvz_surface_free(surface);
        _present_fixture_destroy(&fixture);
        return 0;
    }

    AT(status == DVZ_PRESENT_STATUS_OK);
    AT(dvz_swapchain_ready(swapchain));

    tst_expect_error_begin(suite);
    AT(!dvz_swapchain_device(swapchain, (VkDevice)(uintptr_t)0x1));
    AT(tst_expect_error_end(suite) == 0);

    dvz_swapchain_destroy(swapchain);
    dvz_swapchain_destroy(swapchain);
    dvz_surface_destroy(surface);
    dvz_surface_destroy(surface);

    AT(!dvz_swapchain_ready(swapchain));
    AT(dvz_swapchain_handle(swapchain) == VK_NULL_HANDLE);
    AT(dvz_swapchain_image_count(swapchain) == 0);
    AT(dvz_surface_handle(surface) == VK_NULL_HANDLE);
    AT(!dvz_surface_ready(surface));

    dvz_swapchain_free(swapchain);
    dvz_surface_free(surface);
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
int test_vklite_swapchain_config_present_mode_immediate(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzVklitePresentFixture fixture = {0};
    if (!_present_fixture_create(&fixture))
    {
        tst_skip(suite, "vklite present fixture unavailable");
        _present_fixture_destroy(&fixture);
        return 0;
    }

    DvzSurface* surface = dvz_surface_create_wrapper();
    DvzSwapchain* swapchain = dvz_swapchain_create_wrapper();
    ANN(surface);
    ANN(swapchain);

    const DvzWindowSurface* window_surface = dvz_window_surface(fixture.window);
    if (window_surface == NULL || window_surface->surface == VK_NULL_HANDLE)
    {
        log_warn("vklite swapchain config test skipped because native surface is unavailable");
        tst_skip(suite, "native surface unavailable");
        dvz_swapchain_free(swapchain);
        dvz_surface_free(surface);
        _present_fixture_destroy(&fixture);
        return 0;
    }

    AT(dvz_surface_init_from_device(surface, fixture.device, fixture.queue_family));
    AT(dvz_surface_wrap_native(surface, window_surface->surface, &window_surface->extent));
    AT(dvz_swapchain_init_from_device(swapchain, fixture.device, surface));

    DvzSwapchainConfig cfg = {0};
    VkSurfaceFormatKHR preferred_format = dvz_surface_preferred_format(surface);
    cfg.image_format = preferred_format.format;
    cfg.color_space = preferred_format.colorSpace;
    cfg.present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    cfg.image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    cfg.composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    cfg.clipped = true;
    AT(dvz_swapchain_config(swapchain, cfg));
    DvzSwapchainConfig resolved_config = dvz_swapchain_get_config(swapchain);
    AT(resolved_config.present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR);

    dvz_swapchain_destroy(swapchain);
    dvz_surface_destroy(surface);
    dvz_swapchain_free(swapchain);
    dvz_surface_free(surface);
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
int test_vklite_swapchain_config_defaults_partial(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzSwapchain* swapchain = dvz_swapchain_create_wrapper();
    ANN(swapchain);
    DvzSwapchainConfig cfg = {0};
    cfg.image_format = VK_FORMAT_B8G8R8A8_UNORM;
    cfg.clipped = true;

    AT(dvz_swapchain_config(swapchain, cfg));
    DvzSwapchainConfig resolved_config = dvz_swapchain_get_config(swapchain);
    AT(resolved_config.image_format == VK_FORMAT_B8G8R8A8_UNORM);
    AT(resolved_config.color_space == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);
    // VK_PRESENT_MODE_IMMEDIATE_KHR has enum value 0; only fully-zeroed configs default to FIFO.
    AT(resolved_config.present_mode == 0);
    AT(
        resolved_config.image_usage ==
        (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT));
    AT(resolved_config.composite_alpha == VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR);

    dvz_swapchain_free(swapchain);
    return 0;
}



/**
 * Verify present returns an error when image index is out of range.
 *
 * @param suite test suite
 * @param tstitem current test item
 * @return 0 on success
 */
int test_vklite_swapchain_present_invalid_index(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzSwapchain* swapchain = dvz_swapchain_create_wrapper();
    ANN(swapchain);

    DvzPresentStatus status = DVZ_PRESENT_STATUS_OK;
    tst_expect_error_begin(suite);
    status = dvz_swapchain_present(swapchain, (VkQueue)(uintptr_t)0x1, 1, VK_NULL_HANDLE);
    AT(status == DVZ_PRESENT_STATUS_ERROR);
    (void)tst_expect_error_end(suite);

    dvz_swapchain_free(swapchain);
    return 0;
}



/**
 * Verify recreate persists the resolved swapchain state selected against surface capabilities.
 *
 * @param suite test suite
 * @param tstitem current test item
 * @return 0 on success
 */
int test_vklite_swapchain_recreate_resolved_state(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzVklitePresentFixture fixture = {0};
    if (!_present_fixture_create(&fixture))
    {
        tst_skip(suite, "vklite present fixture unavailable");
        _present_fixture_destroy(&fixture);
        return 0;
    }

    DvzSurface* surface = dvz_surface_create_wrapper();
    DvzSwapchain* swapchain = dvz_swapchain_create_wrapper();
    ANN(surface);
    ANN(swapchain);

    const DvzWindowSurface* window_surface = dvz_window_surface(fixture.window);
    if (window_surface == NULL || window_surface->surface == VK_NULL_HANDLE)
    {
        log_warn("vklite resolved state test skipped because native surface is unavailable");
        tst_skip(suite, "native surface unavailable");
        dvz_swapchain_free(swapchain);
        dvz_surface_free(surface);
        _present_fixture_destroy(&fixture);
        return 0;
    }

    AT(dvz_surface_init_from_device(surface, fixture.device, fixture.queue_family));
    AT(dvz_surface_wrap_native(surface, window_surface->surface, &window_surface->extent));
    AT(dvz_swapchain_init_from_device(swapchain, fixture.device, surface));

    DvzSwapchainConfig cfg = {0};
    cfg.image_format = VK_FORMAT_B8G8R8A8_UNORM;
    cfg.color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    cfg.present_mode = (VkPresentModeKHR)999;
    cfg.image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    cfg.composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    cfg.clipped = true;
    AT(dvz_swapchain_config(swapchain, cfg));

    uvec2 size = {window_surface->extent.width, window_surface->extent.height};
    DvzPresentStatus status = dvz_swapchain_recreate(swapchain, size);
    if (status == DVZ_PRESENT_STATUS_SKIP_ZERO_EXTENT)
    {
        log_warn("vklite resolved state test skipped because window extent is zero");
        tst_skip(suite, "window extent is zero");
        dvz_swapchain_destroy(swapchain);
        dvz_surface_destroy(surface);
        dvz_swapchain_free(swapchain);
        dvz_surface_free(surface);
        _present_fixture_destroy(&fixture);
        return 0;
    }

    AT(status == DVZ_PRESENT_STATUS_OK);
    VkSurfaceFormatKHR preferred_format = dvz_surface_preferred_format(surface);
    AT(dvz_swapchain_image_format(swapchain) != VK_FORMAT_UNDEFINED);
    AT(dvz_swapchain_image_format(swapchain) == preferred_format.format);
    AT(dvz_swapchain_color_space(swapchain) == preferred_format.colorSpace);
    AT(dvz_swapchain_present_mode(swapchain) == dvz_surface_preferred_present_mode(surface));

    dvz_swapchain_destroy(swapchain);
    dvz_surface_destroy(surface);
    dvz_swapchain_free(swapchain);
    dvz_surface_free(surface);
    _present_fixture_destroy(&fixture);
    return 0;
}



/**
 * Verify repeated recreate keeps the swapchain usable and refreshes the resolved state cleanly.
 *
 * @param suite test suite
 * @param tstitem current test item
 * @return 0 on success
 */
int test_vklite_swapchain_recreate_repeat_state(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzVklitePresentFixture fixture = {0};
    if (!_present_fixture_create(&fixture))
    {
        tst_skip(suite, "vklite present fixture unavailable");
        _present_fixture_destroy(&fixture);
        return 0;
    }

    const DvzWindowSurface* window_surface = dvz_window_surface(fixture.window);
    if (window_surface == NULL)
    {
        tst_skip(suite, "native surface unavailable");
        _present_fixture_destroy(&fixture);
        return 0;
    }

    uvec2 size = {window_surface->extent.width, window_surface->extent.height};
    DvzSurface* surface = NULL;
    DvzSwapchain* swapchain = NULL;
    if (!_present_ready_swapchain(&fixture, &surface, &swapchain, size))
    {
        tst_skip(suite, "vklite present swapchain unavailable");
        dvz_swapchain_free(swapchain);
        dvz_surface_free(surface);
        _present_fixture_destroy(&fixture);
        return 0;
    }

    uint32_t first_image_count = dvz_swapchain_image_count(swapchain);
    VkFormat first_format = dvz_swapchain_image_format(swapchain);
    VkColorSpaceKHR first_color_space = dvz_swapchain_color_space(swapchain);
    VkPresentModeKHR first_present_mode = dvz_swapchain_present_mode(swapchain);
    VkExtent2D first_extent = dvz_swapchain_extent(swapchain);
    VkImage first_image = VK_NULL_HANDLE;
    VkImageView first_view = VK_NULL_HANDLE;
    AT(dvz_swapchain_image(swapchain, 0, &first_image));
    AT(dvz_swapchain_image_view(swapchain, 0, &first_view));
    AT(first_image != VK_NULL_HANDLE);
    AT(first_view != VK_NULL_HANDLE);

    DvzPresentStatus status = dvz_swapchain_recreate(swapchain, size);
    if (status == DVZ_PRESENT_STATUS_SKIP_ZERO_EXTENT)
    {
        log_warn("vklite repeat-recreate test skipped because extent became zero");
        tst_skip(suite, "window extent is zero");
        dvz_swapchain_destroy(swapchain);
        dvz_surface_destroy(surface);
        dvz_swapchain_free(swapchain);
        dvz_surface_free(surface);
        _present_fixture_destroy(&fixture);
        return 0;
    }

    AT(status == DVZ_PRESENT_STATUS_OK);
    AT(dvz_swapchain_ready(swapchain));
    AT(dvz_swapchain_handle(swapchain) != VK_NULL_HANDLE);
    AT(dvz_swapchain_image_count(swapchain) > 0);
    AT(dvz_swapchain_image(swapchain, 0, &first_image));
    AT(dvz_swapchain_image_view(swapchain, 0, &first_view));
    AT(first_image != VK_NULL_HANDLE);
    AT(first_view != VK_NULL_HANDLE);
    AT(dvz_swapchain_image_format(swapchain) == first_format);
    AT(dvz_swapchain_color_space(swapchain) == first_color_space);
    AT(dvz_swapchain_present_mode(swapchain) == first_present_mode);
    AT(dvz_swapchain_extent(swapchain).width == first_extent.width);
    AT(dvz_swapchain_extent(swapchain).height == first_extent.height);
    AT(first_image_count > 0);

    dvz_swapchain_destroy(swapchain);
    dvz_surface_destroy(surface);
    dvz_swapchain_free(swapchain);
    dvz_surface_free(surface);
    _present_fixture_destroy(&fixture);
    return 0;
}



/**
 * Verify destroy clears cached swapchain image/view access and resolved state.
 *
 * @param suite test suite
 * @param tstitem current test item
 * @return 0 on success
 */
int test_vklite_swapchain_destroy_clears_cached_state(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzVklitePresentFixture fixture = {0};
    if (!_present_fixture_create(&fixture))
    {
        tst_skip(suite, "vklite present fixture unavailable");
        _present_fixture_destroy(&fixture);
        return 0;
    }

    const DvzWindowSurface* window_surface = dvz_window_surface(fixture.window);
    if (window_surface == NULL)
    {
        tst_skip(suite, "native surface unavailable");
        _present_fixture_destroy(&fixture);
        return 0;
    }

    uvec2 size = {window_surface->extent.width, window_surface->extent.height};
    DvzSurface* surface = NULL;
    DvzSwapchain* swapchain = NULL;
    if (!_present_ready_swapchain(&fixture, &surface, &swapchain, size))
    {
        tst_skip(suite, "vklite present swapchain unavailable");
        dvz_swapchain_free(swapchain);
        dvz_surface_free(surface);
        _present_fixture_destroy(&fixture);
        return 0;
    }

    dvz_swapchain_destroy(swapchain);

    VkImage image = VK_NULL_HANDLE;
    VkImageView image_view = VK_NULL_HANDLE;
    AT(!dvz_swapchain_ready(swapchain));
    AT(dvz_swapchain_handle(swapchain) == VK_NULL_HANDLE);
    AT(dvz_swapchain_image_count(swapchain) == 0);
    AT(dvz_swapchain_image_format(swapchain) == VK_FORMAT_UNDEFINED);
    AT(dvz_swapchain_present_mode(swapchain) == VK_PRESENT_MODE_FIFO_KHR);
    AT(dvz_swapchain_extent(swapchain).width == 0);
    AT(dvz_swapchain_extent(swapchain).height == 0);
    AT(!dvz_swapchain_image(swapchain, 0, &image));
    AT(!dvz_swapchain_image_view(swapchain, 0, &image_view));

    dvz_surface_destroy(surface);
    dvz_swapchain_free(swapchain);
    dvz_surface_free(surface);
    _present_fixture_destroy(&fixture);
    return 0;
}



/**
 * Verify acquire/present transitions reject destroyed state and accept one live cycle.
 *
 * @param suite test suite
 * @param tstitem current test item
 * @return 0 on success
 */
int test_vklite_swapchain_acquire_present_cycle(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzVklitePresentFixture fixture = {0};
    if (!_present_fixture_create(&fixture))
    {
        tst_skip(suite, "vklite present fixture unavailable");
        _present_fixture_destroy(&fixture);
        return 0;
    }

    const DvzWindowSurface* window_surface = dvz_window_surface(fixture.window);
    if (window_surface == NULL)
    {
        tst_skip(suite, "native surface unavailable");
        _present_fixture_destroy(&fixture);
        return 0;
    }

    uvec2 size = {window_surface->extent.width, window_surface->extent.height};
    DvzSurface* surface = NULL;
    DvzSwapchain* swapchain = NULL;
    if (!_present_ready_swapchain(&fixture, &surface, &swapchain, size))
    {
        tst_skip(suite, "vklite present swapchain unavailable");
        dvz_swapchain_free(swapchain);
        dvz_surface_free(surface);
        _present_fixture_destroy(&fixture);
        return 0;
    }

    DvzSemaphore* image_available = dvz_semaphore_create_wrapper();
    ANN(image_available);
    dvz_semaphore(fixture.device, image_available);

    uint32_t image_idx = UINT32_MAX;
    DvzPresentStatus status =
        dvz_swapchain_acquire(swapchain, dvz_semaphore_handle(image_available), UINT64_MAX, &image_idx);
    if (status == DVZ_PRESENT_STATUS_RECREATE || status == DVZ_PRESENT_STATUS_SKIP_ZERO_EXTENT)
    {
        log_warn("vklite acquire/present cycle test skipped because acquire requested recreate");
        tst_skip(suite, "acquire requested recreate");
        dvz_semaphore_destroy(image_available);
        dvz_semaphore_free(image_available);
        dvz_swapchain_destroy(swapchain);
        dvz_surface_destroy(surface);
        dvz_swapchain_free(swapchain);
        dvz_surface_free(surface);
        _present_fixture_destroy(&fixture);
        return 0;
    }

    AT(status == DVZ_PRESENT_STATUS_OK);
    AT(image_idx < dvz_swapchain_image_count(swapchain));
    status = dvz_swapchain_present(swapchain, fixture.queue, image_idx, VK_NULL_HANDLE);
    AT(status == DVZ_PRESENT_STATUS_OK || status == DVZ_PRESENT_STATUS_RECREATE);

    dvz_swapchain_destroy(swapchain);

    image_idx = UINT32_MAX;
    tst_expect_error_begin(suite);
    status =
        dvz_swapchain_acquire(swapchain, dvz_semaphore_handle(image_available), UINT64_MAX, &image_idx);
    AT(status == DVZ_PRESENT_STATUS_ERROR);
    AT(image_idx == UINT32_MAX);
    AT(tst_expect_error_end(suite) == 0);

    tst_expect_error_begin(suite);
    status = dvz_swapchain_present(swapchain, fixture.queue, 0, VK_NULL_HANDLE);
    AT(status == DVZ_PRESENT_STATUS_ERROR);
    AT(tst_expect_error_end(suite) == 0);

    dvz_semaphore_destroy(image_available);
    dvz_semaphore_free(image_available);
    dvz_surface_destroy(surface);
    dvz_swapchain_free(swapchain);
    dvz_surface_free(surface);
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
int test_vklite_wrap_backend_external_surface_present(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    DvzVklitePresentFixture fixture = {0};
    if (!_present_fixture_create(&fixture))
    {
        tst_skip(suite, "vklite present fixture unavailable");
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
        tst_skip(suite, "vklite wrap surface unavailable");
        _wrap_surface_fixture_destroy(&fixture, &wrap);
        _present_fixture_destroy(&fixture);
        return 0;
    }

    DvzSurface* surface = dvz_surface_create_wrapper();
    DvzSwapchain* swapchain = dvz_swapchain_create_wrapper();
    ANN(surface);
    ANN(swapchain);
    AT(dvz_surface_init_from_device(surface, fixture.device, fixture.queue_family));
    VkExtent2D wrap_extent = {.width = wrap.size[0], .height = wrap.size[1]};
    AT(dvz_surface_wrap_native(surface, wrap.external_surface, &wrap_extent));
    AT(_swapchain_prepare(swapchain, &fixture, surface));

    DvzPresentStatus status = dvz_swapchain_recreate(swapchain, wrap.size);
    if (status == DVZ_PRESENT_STATUS_SKIP_ZERO_EXTENT)
    {
        log_warn("vklite wrap present test skipped because wrap-window extent is zero");
        tst_skip(suite, "window extent is zero");
        dvz_swapchain_destroy(swapchain);
        dvz_surface_destroy(surface);
        dvz_swapchain_free(swapchain);
        dvz_surface_free(surface);
        _wrap_surface_fixture_destroy(&fixture, &wrap);
        _present_fixture_destroy(&fixture);
        return 0;
    }

    AT(status == DVZ_PRESENT_STATUS_OK);
    AT(dvz_swapchain_ready(swapchain));
    AT(dvz_swapchain_handle(swapchain) != VK_NULL_HANDLE);
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

    status = dvz_swapchain_recreate(swapchain, wrap.size);
    if (status == DVZ_PRESENT_STATUS_SKIP_ZERO_EXTENT)
    {
        log_warn("vklite wrap present test skipped during restore because extent is zero");
        tst_skip(suite, "window extent is zero");
        dvz_swapchain_destroy(swapchain);
        dvz_surface_destroy(surface);
        dvz_swapchain_free(swapchain);
        dvz_surface_free(surface);
        _wrap_surface_fixture_destroy(&fixture, &wrap);
        _present_fixture_destroy(&fixture);
        return 0;
    }
    AT(status == DVZ_PRESENT_STATUS_OK);

    dvz_swapchain_destroy(swapchain);
    dvz_surface_destroy(surface);
    dvz_swapchain_free(swapchain);
    dvz_surface_free(surface);
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
int test_vklite_wrap_backend_external_surface_present(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);
    ANN(tstitem);
    log_warn("vklite wrap present test skipped because Datoviz was built without GLFW support");
    tst_skip(suite, "GLFW support unavailable");
    return 0;
}
#endif
