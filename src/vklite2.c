#include "../include/visky/vklite2.h"
#include "vklite2_utils.h"
#include <stdlib.h>

BEGIN_INCL_NO_WARN
#include <stb_image.h>
END_INCL_NO_WARN



/*************************************************************************************************/
/*  App                                                                                          */
/*************************************************************************************************/

VklApp* vkl_app(VklBackend backend)
{
    VklApp* app = calloc(1, sizeof(VklApp));
    obj_init(&app->obj, VKL_OBJECT_TYPE_APP);
    app->backend = backend;

    // Which extensions are required? Depends on the backend.
    uint32_t required_extension_count = 0;
    const char** required_extensions = backend_extensions(backend, &required_extension_count);

    // Create the instance.
    create_instance(
        required_extension_count, required_extensions, &app->instance, &app->debug_messenger);
    // debug_messenger != 0 means validation enabled
    obj_created(&app->obj);

    // Count the number of devices.
    vkEnumeratePhysicalDevices(app->instance, &app->gpu_count, NULL);
    log_trace("found %d GPU(s)", app->gpu_count);
    if (app->gpu_count == 0)
    {
        log_error("no compatible device found! aborting");
        exit(1);
    }

    // Discover the available GPUs.
    // ----------------------------
    {
        // Initialize the GPU(s).
        VkPhysicalDevice* physical_devices = calloc(app->gpu_count, sizeof(VkPhysicalDevice));
        vkEnumeratePhysicalDevices(app->instance, &app->gpu_count, physical_devices);
        ASSERT(app->gpu_count <= VKL_MAX_GPUS);
        app->gpus = calloc(app->gpu_count, sizeof(VklGpu));
        for (uint32_t i = 0; i < app->gpu_count; i++)
        {
            obj_init(&app->gpus[i].obj, VKL_OBJECT_TYPE_GPU);
            app->gpus[i].app = app;
            app->gpus[i].idx = i;
            discover_gpu(physical_devices[i], &app->gpus[i]);
            log_debug("found device #%d: %s", app->gpus[i].idx, app->gpus[i].name);
        }

        FREE(physical_devices);
    }

    INSTANCES_INIT(VklWindow, app, windows, VKL_MAX_WINDOWS, VKL_OBJECT_TYPE_WINDOW)

    return app;
}



void vkl_app_destroy(VklApp* app)
{
    log_trace("starting destruction of app...");


    // Destroy the GPUs.
    ASSERT(app->gpus != NULL);
    for (uint32_t i = 0; i < app->gpu_count; i++)
    {
        vkl_gpu_destroy(&app->gpus[i]);
    }
    INSTANCES_DESTROY(app->gpus);


    // Destroy the windows.
    ASSERT(app->windows != NULL);
    for (uint32_t i = 0; i < app->window_count; i++)
    {
        vkl_window_destroy(&app->windows[i]);
    }
    INSTANCES_DESTROY(app->windows)


    // Destroy the windows.
    if (app->canvases != NULL)
    {
        vkl_canvases_destroy(app->canvas_count, app->canvases);
        INSTANCES_DESTROY(app->canvases)
    }


    // Destroy the debug messenger.
    if (app->debug_messenger)
    {
        destroy_debug_utils_messenger_EXT(app->instance, app->debug_messenger, NULL);
        app->debug_messenger = NULL;
    }


    // Destroy the instance.
    log_trace("destroy Vulkan instance");
    if (app->instance != 0)
    {
        vkDestroyInstance(app->instance, NULL);
        app->instance = 0;
    }


    // Free the App memory.
    FREE(app);
    log_trace("app destroyed");
}



/*************************************************************************************************/
/*  GPU                                                                                          */
/*************************************************************************************************/

VklGpu* vkl_gpu(VklApp* app, uint32_t idx)
{
    if (idx >= app->gpu_count)
    {
        log_error("GPU index %d higher than number of GPUs %d", idx, app->gpu_count);
        idx = 0;
    }
    VklGpu* gpu = &app->gpus[idx];
    return gpu;
}



void vkl_gpu_request_features(VklGpu* gpu, VkPhysicalDeviceFeatures requested_features)
{
    gpu->requested_features = requested_features;
}



void vkl_gpu_create(VklGpu* gpu, VkSurfaceKHR surface)
{
    log_trace(
        "starting creation of GPU #%d WITH%s surface...", gpu->idx, surface != 0 ? "" : "OUT");
    create_device(gpu, surface);

    // Create command pools
    create_command_pool(
        gpu->device, gpu->queues.indices[VKL_QUEUE_GRAPHICS], &gpu->cmd_pools[VKL_QUEUE_GRAPHICS]);

    create_command_pool(
        gpu->device, gpu->queues.indices[VKL_QUEUE_COMPUTE], &gpu->cmd_pools[VKL_QUEUE_COMPUTE]);

    // Create descriptor pool
    // TODO

    obj_created(&gpu->obj);
    log_trace("GPU #%d created", gpu->idx);
}



void vkl_gpu_destroy(VklGpu* gpu)
{
    log_trace("starting destruction of GPU #%d...", gpu->idx);
    ASSERT(gpu != NULL);
    if (gpu->obj.status < VKL_OBJECT_STATUS_CREATED)
    {

        log_trace("skip destruction of GPU as it was not properly created");
        ASSERT(gpu->device == 0);
        return;
    }
    VkDevice device = gpu->device;
    ASSERT(device != 0);

    // Destroy the command pool.
    log_trace("destroy command pools");
    for (uint32_t i = 0; i < gpu->cmd_pool_count; i++)
    {
        if (gpu->cmd_pools[i] != 0)
        {
            vkDestroyCommandPool(device, gpu->cmd_pools[i], NULL);
            gpu->cmd_pools[i] = 0;
        }
    }

    // log_trace("destroy descriptor pool");
    // if (gpu->descriptor_pool)
    //     vkDestroyDescriptorPool(gpu->device, gpu->descriptor_pool, NULL);

    // Destroy the device.
    log_trace("destroy device");
    vkDestroyDevice(gpu->device, NULL);
    gpu->device = 0;

    obj_destroyed(&gpu->obj);
    log_trace("GPU #%d destroyed", gpu->idx);
}



/*************************************************************************************************/
/*  Window                                                                                       */
/*************************************************************************************************/

VklWindow* vkl_window(VklApp* app, uint32_t width, uint32_t height)
{
    INSTANCE_NEW(VklWindow, window, app->windows, app->window_count)

    ASSERT(window->obj.type == VKL_OBJECT_TYPE_WINDOW);
    ASSERT(window->obj.status == VKL_OBJECT_STATUS_INIT);
    window->app = app;

    window->width = width;
    window->height = height;

    // Create the window, depending on the backend.
    window->backend_window =
        backend_window(app->instance, app->backend, width, height, &window->surface);

    return window;
}



void vkl_window_destroy(VklWindow* window)
{
    if (window == NULL || window->obj.status == VKL_OBJECT_STATUS_DESTROYED)
    {
        log_trace("skip destruction of already-destroyed window");
        return;
    }
    backend_window_destroy(
        window->app->instance, window->app->backend, window->backend_window, window->surface);
    obj_destroyed(&window->obj);
}



/*************************************************************************************************/
/*  Swapchain                                                                                    */
/*************************************************************************************************/

VklSwapchain* vkl_swapchain(VklGpu* gpu, VklWindow* window, uint32_t min_img_count)
{
    VklSwapchain* swapchain = calloc(1, sizeof(VklSwapchain));

    swapchain->gpu = gpu;
    swapchain->window = window;
    swapchain->img_count = min_img_count;
    return swapchain;
}



void vkl_swapchain_create(VklSwapchain* swapchain, VkFormat format, VkPresentModeKHR present_mode)
{
    log_trace("starting creation of swapchain...");

    // Create swapchain
    create_swapchain(
        swapchain->gpu->device, swapchain->gpu->physical_device, swapchain->window->surface,
        swapchain->img_count, format, present_mode, swapchain->gpu->queues,
        &swapchain->window->caps, &swapchain->swapchain);

    obj_created(&swapchain->obj);
    log_trace("swapchain created");
}



void vkl_swapchain_destroy(VklSwapchain* swapchain)
{
    log_trace("starting destruction of swapchain...");

    if (swapchain->swapchain != 0)
        vkDestroySwapchainKHR(swapchain->gpu->device, swapchain->swapchain, NULL);

    FREE(swapchain);
    log_trace("swapchain destroyed");
}



/*************************************************************************************************/
/*  Commands */
/*************************************************************************************************/

VklCommands* vkl_commands(VklGpu* gpu, VklQueueType queue, uint32_t count) { return NULL; }



void vkl_cmd_begin(VklCommands* cmds) {}



void vkl_cmd_end(VklCommands* cmds) {}



void vkl_cmd_reset(VklCommands* cmds) {}



void vkl_cmd_free(VklCommands* cmds) { FREE(cmds); }
