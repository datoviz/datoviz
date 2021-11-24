/*************************************************************************************************/
/*  Window API                                                                                   */
/*************************************************************************************************/

#include "window.h"
#include "_glfw.h"
#include "common.h"
#include "vklite.h"
#include "vkutils.h"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

void dvz_gpu_wait(DvzGpu* gpu);

void dvz_gpu_destroy(DvzGpu* gpu);



/*************************************************************************************************/
/*  Host                                                                                         */
/*************************************************************************************************/

DvzHost* dvz_host(DvzBackend backend)
{
    log_set_level_env();
    log_debug("create the host with backend %d", backend);

    DvzHost* host = calloc(1, sizeof(DvzHost));
    dvz_obj_init(&host->obj);
    host->obj.type = DVZ_OBJECT_TYPE_APP;

#if SWIFTSHADER
    if (backend != DVZ_BACKEND_OFFSCREEN)
    {
        log_warn("when the library is compiled for switshader, offscreen rendering is mandatory");
        backend = DVZ_BACKEND_OFFSCREEN;
    }
#endif

    // Fill the host.autorun struct with DVZ_RUN_* environment variables.
    // dvz_autorun_env(host);

    // // Take env variable "DVZ_RUN_OFFSCREEN" into account, forcing offscreen backend in this
    // case. if (host->autorun.enable && host->autorun.offscreen)
    // {
    //     log_info("forcing offscreen backend because DVZ_RUN_OFFSCREEN env variable is set");
    //     backend = DVZ_BACKEND_OFFSCREEN;
    // }

    // Backend-specific initialization code.
    host->backend = backend;
    backend_init(backend);

    // Initialize the global clock.
    host->clock = dvz_clock();

    host->gpus = dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzGpu), DVZ_OBJECT_TYPE_GPU);
    host->windows =
        dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzWindow), DVZ_OBJECT_TYPE_WINDOW);

    // Which extensions are required? Depends on the backend.
    uint32_t required_extension_count = 0;
    const char** required_extensions = backend_extensions(backend, &required_extension_count);

    // Create the instance.
    create_instance(
        required_extension_count, required_extensions, &host->instance, &host->debug_messenger,
        &host->n_errors);
    // debug_messenger != VK_NULL_HANDLE means validation enabled
    dvz_obj_created(&host->obj);

    // Count the number of devices.
    uint32_t gpu_count = 0;
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(host->instance, &gpu_count, NULL));
    log_trace("found %d GPU(s)", gpu_count);
    if (gpu_count == 0)
    {
        log_error("no compatible device found! aborting");
        exit(1);
    }

    // Discover the available GPUs.
    // ----------------------------
    {
        // Initialize the GPU(s).
        VkPhysicalDevice* physical_devices = calloc(gpu_count, sizeof(VkPhysicalDevice));
        VK_CHECK_RESULT(vkEnumeratePhysicalDevices(host->instance, &gpu_count, physical_devices));
        ASSERT(gpu_count <= DVZ_CONTAINER_DEFAULT_COUNT);
        DvzGpu* gpu = NULL;
        for (uint32_t i = 0; i < gpu_count; i++)
        {
            gpu = dvz_container_alloc(&host->gpus);
            dvz_obj_init(&gpu->obj);
            gpu->host = host;
            gpu->idx = i;
            discover_gpu(physical_devices[i], gpu);
            log_debug("found device #%d: %s", gpu->idx, gpu->name);
        }

        FREE(physical_devices);
    }

    return host;
}



void dvz_host_wait(DvzHost* host)
{
    ASSERT(host != NULL);
    log_trace("wait for all GPUs to be idle");
    DvzGpu* gpu = NULL;
    DvzContainerIterator iter = dvz_container_iterator(&host->gpus);
    while (iter.item != NULL)
    {
        gpu = iter.item;
        dvz_gpu_wait(gpu);
        dvz_container_iter(&iter);
    }
}



int dvz_host_destroy(DvzHost* host)
{
    ASSERT(host != NULL);

    log_debug("destroy the host with backend %d", host->backend);
    dvz_host_wait(host);

    // Destroy the canvases.
    // TODO
    // dvz_canvases_destroy(&host->canvases);

    // Destroy the GPUs.
    CONTAINER_DESTROY_ITEMS(DvzGpu, host->gpus, dvz_gpu_destroy)
    dvz_container_destroy(&host->gpus);

    // Destroy the windows.
    CONTAINER_DESTROY_ITEMS(DvzWindow, host->windows, dvz_window_destroy)
    dvz_container_destroy(&host->windows);

    // Destroy the debug messenger.
    if (host->debug_messenger)
    {
        destroy_debug_utils_messenger_EXT(host->instance, host->debug_messenger, NULL);
        host->debug_messenger = NULL;
    }

    // Destroy the instance.
    log_trace("destroy Vulkan instance");
    if (host->instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(host->instance, NULL);
        host->instance = 0;
    }

    // TODO
    // // Destroy the run.
    // if (host->run != NULL)
    // {
    //     dvz_run_destroy(host->run);
    // }

    // Backend-specific termination code.
    backend_terminate(host->backend);

    // Free the App memory.
    int res = (int)host->n_errors;
    FREE(host);
    log_trace("host destroyed");

    return res;
}



/*************************************************************************************************/
/*  Window                                                                                       */
/*************************************************************************************************/

DvzWindow* dvz_window(DvzHost* host, uint32_t width, uint32_t height)
{
    // NOTE: an offscreen canvas has NO DvzWindow, so this function should NEVER be called with an
    // offscreen backend, or for an offscreen canvas.

    ASSERT(host != NULL);

    DvzWindow* window = dvz_container_alloc(&host->windows);
    ASSERT(window != NULL);

    ASSERT(window->obj.type == DVZ_OBJECT_TYPE_WINDOW);
    ASSERT(window->obj.status == DVZ_OBJECT_STATUS_ALLOC);
    window->host = host;
    ASSERT(host->backend != DVZ_BACKEND_NONE && host->backend != DVZ_BACKEND_OFFSCREEN);

    window->width = width;
    window->height = height;
    window->close_on_esc = true;

    // Create the window, depending on the backend.
    window->backend_window =
        backend_window(host->instance, host->backend, width, height, window, &window->surface);

    if (window->surface == NULL)
    {
        log_error("could not create window surface");
        dvz_window_destroy(window);
        return NULL;
    }

    return window;
}



void dvz_window_get_size(
    DvzWindow* window, uint32_t* framebuffer_width, uint32_t* framebuffer_height)
{
    ASSERT(window != NULL);
    ASSERT(window->host != NULL);
    backend_window_get_size(
        window->host->backend, window->backend_window, //
        &window->width, &window->height,               //
        framebuffer_width, framebuffer_height);
}



void dvz_window_set_size(DvzWindow* window, uint32_t width, uint32_t height)
{
    ASSERT(window != NULL);
    ASSERT(window->host != NULL);
    backend_window_set_size(window->host->backend, window->backend_window, width, height);
}



void dvz_window_poll_events(DvzWindow* window)
{
    ASSERT(window != NULL);
    ASSERT(window->host != NULL);
    backend_poll_events(window->host->backend, window);
}



void dvz_window_destroy(DvzWindow* window)
{
    if (window == NULL || window->obj.status == DVZ_OBJECT_STATUS_DESTROYED)
    {
        log_trace("skip destruction of already-destroyed window");
        return;
    }
    ASSERT(window != NULL);
    ASSERT(window->host != NULL);
    backend_window_destroy(
        window->host->instance, window->host->backend, //
        window->backend_window, window->surface);
    dvz_obj_destroyed(&window->obj);
}
