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
    // NOTE: init canvas in canvas.c instead, as the struct is defined there and not here

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

    INSTANCES_INIT(VklCommands, gpu, commands, VKL_MAX_COMMANDS, VKL_OBJECT_TYPE_COMMANDS)
    INSTANCES_INIT(VklBuffers, gpu, buffers, VKL_MAX_BUFFERS, VKL_OBJECT_TYPE_BUFFER)

    return gpu;
}



void vkl_gpu_request_features(VklGpu* gpu, VkPhysicalDeviceFeatures requested_features)
{
    gpu->requested_features = requested_features;
}



void vkl_gpu_queue(VklGpu* gpu, VklQueueType type, uint32_t idx)
{
    ASSERT(gpu != NULL);
    VklQueues* q = &gpu->queues;
    ASSERT(q != NULL);
    ASSERT(idx < VKL_MAX_QUEUES);
    q->queue_types[idx] = type;
    ASSERT(idx == q->queue_count);
    q->queue_count++;
}



void vkl_gpu_create(VklGpu* gpu, VkSurfaceKHR surface)
{
    if (gpu->queues.queue_count == 0)
    {
        log_error(
            "you must request at least one queue with vkl_gpu_queue() before creating the GPU");
        exit(1);
    }
    log_trace(
        "starting creation of GPU #%d WITH%s surface...", gpu->idx, surface != 0 ? "" : "OUT");
    create_device(gpu, surface);

    // Create queues and command pools.
    VklQueues* q = &gpu->queues;
    for (uint32_t i = 0; i < q->queue_count; i++)
    {
        vkGetDeviceQueue(gpu->device, q->queue_families[i], q->queue_indices[i], &q->queues[i]);
        create_command_pool(gpu->device, q->queue_families[i], &q->cmd_pools[i]);
    }

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
    log_trace("destroy %d command pool(s)", gpu->queues.queue_family_count);
    for (uint32_t i = 0; i < gpu->queues.queue_family_count; i++)
    {
        if (gpu->queues.cmd_pools[i] != 0)
        {
            vkDestroyCommandPool(device, gpu->queues.cmd_pools[i], NULL);
            gpu->queues.cmd_pools[i] = 0;
        }
    }

    // log_trace("destroy descriptor pool");
    // if (gpu->descriptor_pool)
    //     vkDestroyDescriptorPool(gpu->device, gpu->descriptor_pool, NULL);

    log_trace("destroy %d buffers sets", gpu->buffers_count);
    for (uint32_t i = 0; i < gpu->buffers_count; i++)
    {
        vkl_buffers_destroy(&gpu->buffers[i]);
    }

    // Destroy the device.
    log_trace("destroy device");
    vkDestroyDevice(gpu->device, NULL);
    gpu->device = 0;

    INSTANCES_DESTROY(gpu->commands)
    INSTANCES_DESTROY(gpu->buffers)

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
    ASSERT(gpu != NULL);
    ASSERT(window != NULL);

    VklSwapchain* swapchain = calloc(1, sizeof(VklSwapchain));

    swapchain->gpu = gpu;
    swapchain->window = window;
    swapchain->img_count = min_img_count;
    return swapchain;
}



void vkl_swapchain_create(VklSwapchain* swapchain, VkFormat format, VkPresentModeKHR present_mode)
{
    ASSERT(swapchain != NULL);
    ASSERT(swapchain->gpu != NULL);

    log_trace("starting creation of swapchain...");

    // Create swapchain
    create_swapchain(
        swapchain->gpu->device, swapchain->gpu->physical_device, swapchain->window->surface,
        swapchain->img_count, format, present_mode, &swapchain->gpu->queues,
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

VklCommands* vkl_commands(VklGpu* gpu, uint32_t queue, uint32_t count)
{
    ASSERT(gpu != NULL);
    ASSERT(gpu->obj.status >= VKL_OBJECT_STATUS_CREATED);

    INSTANCE_NEW(VklCommands, commands, gpu->commands, gpu->commands_count)

    ASSERT(count <= VKL_MAX_COMMAND_BUFFERS_PER_SET);
    ASSERT(queue <= gpu->queues.queue_count);
    ASSERT(count > 0);
    ASSERT(gpu->queues.cmd_pools[queue] != 0);

    commands->gpu = gpu;
    commands->queue_idx = queue;
    commands->count = count;
    allocate_command_buffers(gpu->device, gpu->queues.cmd_pools[queue], count, commands->cmds);

    obj_created(&commands->obj);

    return commands;
}



void vkl_cmd_begin(VklCommands* cmds)
{
    ASSERT(cmds != NULL);
    ASSERT(cmds->count > 0);

    log_trace("begin %d command buffer(s)", cmds->count);
    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    for (uint32_t i = 0; i < cmds->count; i++)
        VK_CHECK_RESULT(vkBeginCommandBuffer(cmds->cmds[i], &begin_info));
}



void vkl_cmd_end(VklCommands* cmds)
{
    ASSERT(cmds != NULL);
    ASSERT(cmds->count > 0);

    log_trace("end %d command buffer(s)", cmds->count);
    for (uint32_t i = 0; i < cmds->count; i++)
        VK_CHECK_RESULT(vkEndCommandBuffer(cmds->cmds[i]));
}



void vkl_cmd_reset(VklCommands* cmds)
{
    ASSERT(cmds != NULL);
    ASSERT(cmds->count > 0);

    log_trace("reset %d command buffer(s)", cmds->count);
    for (uint32_t i = 0; i < cmds->count; i++)
    {
        ASSERT(cmds->cmds[i] != 0);
        vkResetCommandBuffer(cmds->cmds[i], 0);
    }
}



void vkl_cmd_free(VklCommands* cmds)
{
    ASSERT(cmds != NULL);
    ASSERT(cmds->count > 0);
    ASSERT(cmds->gpu != NULL);
    ASSERT(cmds->gpu->device != 0);

    log_trace("free %d command buffer(s)", cmds->count);
    vkFreeCommandBuffers(
        cmds->gpu->device, cmds->gpu->queues.cmd_pools[cmds->queue_idx], //
        cmds->count, cmds->cmds);
}



/*************************************************************************************************/
/*  Buffers                                                                                      */
/*************************************************************************************************/

VklBuffers* vkl_buffers(VklGpu* gpu, uint32_t count)
{
    ASSERT(gpu != NULL);
    ASSERT(gpu->obj.status >= VKL_OBJECT_STATUS_CREATED);

    INSTANCE_NEW(VklBuffers, buffers, gpu->buffers, gpu->buffers_count)

    ASSERT(count <= VKL_MAX_BUFFERS);

    buffers->gpu = gpu;
    buffers->count = count;

    // Default values.
    buffers->memory = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    return buffers;
}



void vkl_buffers_size(VklBuffers* buffers, VkDeviceSize size, VkDeviceSize item_size)
{
    ASSERT(buffers != NULL);
    buffers->size = size;
    buffers->item_size = item_size;
}



void vkl_buffers_usage(VklBuffers* buffers, VkBufferUsageFlags usage)
{
    ASSERT(buffers != NULL);
    buffers->usage = usage;
}



void vkl_buffers_memory(VklBuffers* buffers, VkMemoryPropertyFlags memory)
{
    ASSERT(buffers != NULL);
    buffers->memory = memory;
}



void vkl_buffers_create(VklBuffers* buffers)
{
    ASSERT(buffers != NULL);
    ASSERT(buffers->gpu != NULL);
    ASSERT(buffers->gpu->device != 0);
    ASSERT(buffers->count > 0);
    ASSERT(buffers->size > 0);
    ASSERT(buffers->usage != 0);
    ASSERT(buffers->memory != 0);

    log_trace("starting creation of %d buffer(s)...", buffers->count);
    for (uint32_t i = 0; i < buffers->count; i++)
    {
        create_buffer2(
            buffers->gpu->device, buffers->usage, buffers->memory, buffers->gpu->memory_properties,
            buffers->size, &buffers->buffers[i], &buffers->memories[i]);
    }

    obj_created(&buffers->obj);
    log_trace("buffers created");
}



void* vkl_buffers_map(VklBuffers* buffers, uint32_t idx, VkDeviceSize offset, VkDeviceSize size)
{
    ASSERT(buffers != NULL);
    ASSERT(buffers->gpu != NULL);
    ASSERT(buffers->gpu->device != 0);
    ASSERT(buffers->obj.status >= VKL_OBJECT_STATUS_CREATED);

    ASSERT(
        (buffers->memory & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) && //
        (buffers->memory & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

    log_trace("map buffer #%d", idx);
    void* cdata = NULL;
    vkMapMemory(buffers->gpu->device, buffers->memories[idx], offset, size, 0, &cdata);
    return cdata;
}



void vkl_buffers_unmap(VklBuffers* buffers, uint32_t idx)
{
    ASSERT(buffers != NULL);
    ASSERT(buffers->gpu != NULL);
    ASSERT(buffers->gpu->device != 0);
    ASSERT(buffers->obj.status >= VKL_OBJECT_STATUS_CREATED);

    ASSERT(
        (buffers->memory & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) && //
        (buffers->memory & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

    log_trace("unmap buffer #%d", idx);
    vkUnmapMemory(buffers->gpu->device, buffers->memories[idx]);
}



VklBufferRegion
vkl_buffers_region(VklBuffers* buffers, uint32_t idx, VkDeviceSize offset, VkDeviceSize size)
{
    ASSERT(buffers != NULL);
    ASSERT(buffers->gpu != NULL);
    ASSERT(buffers->gpu->device != 0);
    ASSERT(buffers->obj.status >= VKL_OBJECT_STATUS_CREATED);

    VklBufferRegion region = {0};
    // TODO
    return region;
}



void vkl_buffers_destroy(VklBuffers* buffers)
{
    ASSERT(buffers != NULL);
    if (buffers->obj.status < VKL_OBJECT_STATUS_CREATED)
    {
        log_trace("skip destruction of already-destroyed buffers");
        return;
    }
    log_trace("destroy %d buffer(s)", buffers->count);
    for (uint32_t i = 0; i < buffers->count; i++)
    {
        vkDestroyBuffer(buffers->gpu->device, buffers->buffers[i], NULL);
        vkFreeMemory(buffers->gpu->device, buffers->memories[i], NULL);
    }
    obj_destroyed(&buffers->obj);
}
