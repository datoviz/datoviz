#include "../include/visky/vklite.h"
#include "spirv.h"
#include "vklite_utils.h"
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
    obj_init(&app->obj);
    app->obj.type = VKL_OBJECT_TYPE_APP;
    app->backend = backend;

    // Initialize the global clock.
    _clock_init(&app->clock);

    INSTANCES_INIT(VklGpu, app, gpus, max_gpus, VKL_MAX_GPUS, VKL_OBJECT_TYPE_GPU)
    INSTANCES_INIT(VklWindow, app, windows, max_windows, VKL_MAX_WINDOWS, VKL_OBJECT_TYPE_WINDOW)

    // Which extensions are required? Depends on the backend.
    uint32_t required_extension_count = 0;
    const char** required_extensions = backend_extensions(backend, &required_extension_count);

    // Create the instance.
    create_instance(
        required_extension_count, required_extensions, &app->instance, &app->debug_messenger,
        &app->n_errors);
    // debug_messenger != VK_NULL_HANDLE means validation enabled
    obj_created(&app->obj);

    // Count the number of devices.
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(app->instance, &app->gpu_count, NULL));
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
        VK_CHECK_RESULT(
            vkEnumeratePhysicalDevices(app->instance, &app->gpu_count, physical_devices));
        ASSERT(app->gpu_count <= VKL_MAX_GPUS);
        // app->gpus = calloc(app->gpu_count, sizeof(VklGpu));
        for (uint32_t i = 0; i < app->gpu_count; i++)
        {
            obj_init(&app->gpus[i].obj);
            app->gpus[i].app = app;
            app->gpus[i].idx = i;
            discover_gpu(physical_devices[i], &app->gpus[i]);
            log_debug("found device #%d: %s", app->gpus[i].idx, app->gpus[i].name);
        }

        FREE(physical_devices);
    }

    // INSTANCES_INIT(VklWindow, app, windows, max_windows, VKL_MAX_WINDOWS,
    // VKL_OBJECT_TYPE_WINDOW) NOTE: init canvas in canvas.c instead, as the struct is defined
    // there and not here

    return app;
}



int vkl_app_destroy(VklApp* app)
{
    log_trace("starting destruction of app...");
    vkl_app_wait(app);

    // Destroy the windows.
    if (app->canvases != NULL)
    {
        vkl_canvases_destroy(app->max_canvases, app->canvases);
        INSTANCES_DESTROY(app->canvases)
    }


    // Destroy the GPUs.
    ASSERT(app->gpus != NULL);
    for (uint32_t i = 0; i < app->gpu_count; i++)
    {
        vkl_gpu_destroy(&app->gpus[i]);
    }
    INSTANCES_DESTROY(app->gpus);


    // Destroy the windows.
    ASSERT(app->windows != NULL);
    for (uint32_t i = 0; i < app->max_windows; i++)
    {
        if (app->windows[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_window_destroy(&app->windows[i]);
    }
    INSTANCES_DESTROY(app->windows)


    // Destroy the debug messenger.
    if (app->debug_messenger)
    {
        destroy_debug_utils_messenger_EXT(app->instance, app->debug_messenger, NULL);
        app->debug_messenger = NULL;
    }


    // Destroy the instance.
    log_trace("destroy Vulkan instance");
    if (app->instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(app->instance, NULL);
        app->instance = 0;
    }


    // Free the App memory.
    int res = (int)app->n_errors;
    FREE(app);
    log_trace("app destroyed");

    return res;
}



/*************************************************************************************************/
/*  Thread                                                                                       */
/*************************************************************************************************/

VklThread vkl_thread(VklThreadCallback callback, void* user_data)
{
    VklThread thread = {0};
    if (pthread_create(&thread.thread, NULL, callback, user_data) != 0)
        log_error("thread creation failed");
    if (pthread_mutex_init(&thread.lock, NULL) != 0)
        log_error("mutex creation failed");
    obj_created(&thread.obj);
    return thread;
}



void vkl_thread_join(VklThread* thread)
{
    ASSERT(thread != NULL);
    pthread_join(thread->thread, NULL);
    pthread_mutex_destroy(&thread->lock);
    obj_destroyed(&thread->obj);
}



void vkl_thread_lock(VklThread* thread)
{
    ASSERT(thread != NULL);
    if (is_obj_created(&thread->obj))
        pthread_mutex_lock(&thread->lock);
}



void vkl_thread_unlock(VklThread* thread)
{
    ASSERT(thread != NULL);
    if (is_obj_created(&thread->obj))
        pthread_mutex_unlock(&thread->lock);
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
        "starting creation of GPU #%d WITH%s surface...", gpu->idx,
        surface != VK_NULL_HANDLE ? "" : "OUT");
    create_device(gpu, surface);

    VklQueues* q = &gpu->queues;

    // Create queues.
    uint32_t qf = 0;
    for (uint32_t i = 0; i < q->queue_count; i++)
    {
        qf = q->queue_families[i];
        vkGetDeviceQueue(gpu->device, qf, q->queue_indices[i], &q->queues[i]);

        // Create command pool only for the queue families that appear in the requested queues.
        if (q->cmd_pools[qf] == VK_NULL_HANDLE)
            create_command_pool(gpu->device, qf, &q->cmd_pools[qf]);
    }

    // Create descriptor pool.
    create_descriptor_pool(gpu->device, &gpu->dset_pool);

    obj_created(&gpu->obj);
    log_trace("GPU #%d created", gpu->idx);
}



void vkl_queue_wait(VklGpu* gpu, uint32_t queue_idx)
{
    ASSERT(gpu != NULL);
    ASSERT(queue_idx < gpu->queues.queue_count);
    // log_trace("waiting for queue #%d", queue_idx);
    vkQueueWaitIdle(gpu->queues.queues[queue_idx]);
}



void vkl_gpu_wait(VklGpu* gpu)
{
    ASSERT(gpu != NULL);
    log_trace("waiting for device");
    if (gpu->device != VK_NULL_HANDLE)
        vkDeviceWaitIdle(gpu->device);
}



void vkl_app_wait(VklApp* app)
{
    ASSERT(app != NULL);
    log_trace("wait for all GPUs to be idle");
    for (uint32_t i = 0; i < app->max_gpus; i++)
    {
        if (app->gpus[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_gpu_wait(&app->gpus[i]);
    }
}



void vkl_gpu_destroy(VklGpu* gpu)
{
    log_trace("starting destruction of GPU #%d...", gpu->idx);
    ASSERT(gpu != NULL);
    if (!is_obj_created(&gpu->obj))
    {

        log_trace("skip destruction of GPU as it was not properly created");
        ASSERT(gpu->device == 0);
        return;
    }
    VkDevice device = gpu->device;
    ASSERT(device != VK_NULL_HANDLE);

    if (gpu->context != NULL)
    {
        vkl_context_destroy(gpu->context);
        FREE(gpu->context);
        gpu->context = NULL;
    }

    log_trace("GPU destroy %d command pool(s)", gpu->queues.queue_family_count);
    for (uint32_t i = 0; i < gpu->queues.queue_family_count; i++)
    {
        if (gpu->queues.cmd_pools[i] != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(device, gpu->queues.cmd_pools[i], NULL);
            gpu->queues.cmd_pools[i] = VK_NULL_HANDLE;
        }
    }


    if (gpu->dset_pool != VK_NULL_HANDLE)
    {
        log_trace("destroy descriptor pool");
        vkDestroyDescriptorPool(gpu->device, gpu->dset_pool, NULL);
        gpu->dset_pool = VK_NULL_HANDLE;
    }


    // Destroy the device.
    log_trace("destroy device");
    if (gpu->device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(gpu->device, NULL);
        gpu->device = VK_NULL_HANDLE;
    }


    obj_destroyed(&gpu->obj);
    log_trace("GPU #%d destroyed", gpu->idx);
}



/*************************************************************************************************/
/*  Window                                                                                       */
/*************************************************************************************************/

VklWindow* vkl_window(VklApp* app, uint32_t width, uint32_t height)
{
    INSTANCE_NEW(VklWindow, window, app->windows, app->max_windows)

    ASSERT(window->obj.type == VKL_OBJECT_TYPE_WINDOW);
    ASSERT(window->obj.status == VKL_OBJECT_STATUS_INIT);
    window->app = app;

    window->width = width;
    window->height = height;
    window->close_on_esc = true;

    // Create the window, depending on the backend.
    window->backend_window =
        backend_window(app->instance, app->backend, width, height, window, &window->surface);

    return window;
}



void vkl_window_get_size(
    VklWindow* window, uint32_t* framebuffer_width, uint32_t* framebuffer_height)
{
    ASSERT(window != NULL);
    backend_window_get_size(
        window->app->backend, window->backend_window, //
        &window->width, &window->height,              //
        framebuffer_width, framebuffer_height);
}



void vkl_window_poll_events(VklWindow* window)
{
    ASSERT(window != NULL);
    ASSERT(window->app != NULL);
    backend_poll_events(window->app->backend, window);
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

static void _swapchain_create(VklSwapchain* swapchain)
{
    uint32_t width, height;
    create_swapchain(
        swapchain->gpu->device, swapchain->gpu->physical_device, swapchain->window->surface,
        swapchain->img_count, swapchain->format, swapchain->present_mode, &swapchain->gpu->queues,
        swapchain->requested_width, swapchain->requested_height, //
        &swapchain->window->caps, &swapchain->swapchain, &width, &height);

    swapchain->support_transfer =
        (swapchain->window->caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) != 0;

    // Actual framebuffer size in pixels, as determined by the swapchain creation process.
    ASSERT(width > 0);
    ASSERT(height > 0);
    vkl_images_size(swapchain->images, width, height, 1);

    // Get the number of swapchain images.
    vkGetSwapchainImagesKHR(
        swapchain->gpu->device, swapchain->swapchain, &swapchain->img_count, NULL);
    log_trace("get %d swapchain images", swapchain->img_count);
    vkGetSwapchainImagesKHR(
        swapchain->gpu->device, swapchain->swapchain, &swapchain->img_count,
        swapchain->images->images);

    vkl_images_layout(swapchain->images, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    // Create the swap chain image views (will skip the image creation as they are given by the
    // swapchain directly).
    vkl_images_create(swapchain->images);
}



static void _swapchain_destroy(VklSwapchain* swapchain)
{
    if (swapchain->images != NULL)
        vkl_images_destroy(swapchain->images);
    if (swapchain->swapchain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(swapchain->gpu->device, swapchain->swapchain, NULL);
}



VklSwapchain vkl_swapchain(VklGpu* gpu, VklWindow* window, uint32_t min_img_count)
{
    ASSERT(gpu != NULL);
    ASSERT(is_obj_created(&gpu->obj));

    VklSwapchain swapchain = {0};

    swapchain.gpu = gpu;
    swapchain.window = window;
    swapchain.img_count = min_img_count;
    return swapchain;
}



void vkl_swapchain_format(VklSwapchain* swapchain, VkFormat format)
{
    ASSERT(swapchain != NULL);
    swapchain->format = format;
}



void vkl_swapchain_present_mode(VklSwapchain* swapchain, VkPresentModeKHR present_mode)
{
    ASSERT(swapchain != NULL);
    ASSERT(swapchain->gpu != NULL);
    ASSERT(is_obj_created(&swapchain->gpu->obj));
    for (uint32_t i = 0; i < swapchain->gpu->present_mode_count; i++)
    {
        if (swapchain->gpu->present_modes[i] == present_mode)
        {
            swapchain->present_mode = present_mode;
            return;
        }
    }
    log_error("unsupported swapchain present mode VkPresentModeKHR #%02d", present_mode);
}



void vkl_swapchain_requested_size(VklSwapchain* swapchain, uint32_t width, uint32_t height)
{
    ASSERT(swapchain != NULL);
    swapchain->requested_width = width;
    swapchain->requested_height = height;
}



void vkl_swapchain_create(VklSwapchain* swapchain)
{
    ASSERT(swapchain != NULL);
    ASSERT(swapchain->gpu != NULL);

    log_trace("starting creation of swapchain...");

    // Create the VklImages struct.
    {
        swapchain->images = calloc(1, sizeof(VklImages));
        *swapchain->images = vkl_images(swapchain->gpu, VK_IMAGE_TYPE_2D, swapchain->img_count);
        swapchain->images->is_swapchain = true;
        vkl_images_format(swapchain->images, swapchain->format);
    }

    // Create swapchain
    _swapchain_create(swapchain);

    obj_created(&swapchain->images->obj);
    obj_created(&swapchain->obj);
    log_trace("swapchain created");
}



void vkl_swapchain_recreate(VklSwapchain* swapchain)
{
    ASSERT(swapchain != NULL);
    _swapchain_destroy(swapchain);
    _swapchain_create(swapchain);

    obj_created(&swapchain->images->obj);
    obj_created(&swapchain->obj);
}



void vkl_swapchain_acquire(
    VklSwapchain* swapchain, VklSemaphores* semaphores, uint32_t semaphore_idx, VklFences* fences,
    uint32_t fence_idx)
{
    ASSERT(swapchain != NULL);
    // log_trace(
    //     "acquiring swapchain image with semaphore %d...",
    //     semaphores->semaphores[semaphore_idx]);

    VkSemaphore semaphore = {0};
    if (semaphores != NULL)
        semaphore = semaphores->semaphores[semaphore_idx];

    VkFence fence = {0};
    if (fences != NULL)
        fence = fences->fences[fence_idx];

    VkResult res = vkAcquireNextImageKHR(
        swapchain->gpu->device, swapchain->swapchain, 1000000000, //
        semaphore, fence, &swapchain->img_idx);
    // log_trace("acquired swapchain image #%d", swapchain->img_idx);

    switch (res)
    {
    case VK_SUCCESS:
        // do nothing
        break;
    case VK_ERROR_OUT_OF_DATE_KHR:
        log_trace("out of date swapchain, need to recreate it");
        swapchain->obj.status = VKL_OBJECT_STATUS_NEED_RECREATE;
        break;
    case VK_SUBOPTIMAL_KHR:
        log_warn("suboptimal frame, but do nothing");
        break;
    default:
        log_error("failed acquiring the swapchain image");
        // TODO: is that correct? destroying the object if we failed acquiring the swapchain image?
        swapchain->obj.status = VKL_OBJECT_STATUS_NEED_DESTROY;
        break;
    }
}



void vkl_swapchain_present(
    VklSwapchain* swapchain, uint32_t queue_idx, VklSemaphores* semaphores, uint32_t semaphore_idx)
{
    ASSERT(swapchain != NULL);
    ASSERT(swapchain->swapchain != VK_NULL_HANDLE);
    // log_trace(
    //     "present swapchain image #%d and wait for semaphore %d", swapchain->img_idx,
    //     semaphores->semaphores[semaphore_idx]);
    ASSERT(queue_idx < swapchain->gpu->queues.queue_count);

    // Present the buffer to the surface.
    VkPresentInfoKHR info = {0};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    if (semaphores != NULL)
    {
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &semaphores->semaphores[semaphore_idx];
    }

    info.swapchainCount = 1;
    info.pSwapchains = &swapchain->swapchain;
    info.pImageIndices = &swapchain->img_idx;

    VkResult res = vkQueuePresentKHR(swapchain->gpu->queues.queues[queue_idx], &info);

    switch (res)
    {
    case VK_SUCCESS:
        // do nothing
        break;
    case VK_ERROR_OUT_OF_DATE_KHR:
    case VK_SUBOPTIMAL_KHR:
        log_trace("out of date swapchain, need to recreate it");
        swapchain->obj.status = VKL_OBJECT_STATUS_NEED_RECREATE;
        break;
    default:
        log_error("failed presenting the swapchain image");
        break;
    }
}



void vkl_swapchain_destroy(VklSwapchain* swapchain)
{
    ASSERT(swapchain != NULL);
    if (!is_obj_created(&swapchain->obj))
    {
        log_trace("skip destruction of already-destroyed swapchain");
        return;
    }

    log_trace("starting destruction of swapchain...");

    _swapchain_destroy(swapchain);

    if (swapchain->images != NULL)
    {
        FREE(swapchain->images);
        swapchain->images = VK_NULL_HANDLE;
    }

    if (swapchain->swapchain != VK_NULL_HANDLE)
        swapchain->swapchain = VK_NULL_HANDLE;

    obj_destroyed(&swapchain->obj);
    log_trace("swapchain destroyed");
}



/*************************************************************************************************/
/*  Commands                                                                                     */
/*************************************************************************************************/

VklCommands vkl_commands(VklGpu* gpu, uint32_t queue, uint32_t count)
{
    ASSERT(gpu != NULL);
    ASSERT(is_obj_created(&gpu->obj));

    ASSERT(count <= VKL_MAX_COMMAND_BUFFERS_PER_SET);
    ASSERT(queue < gpu->queues.queue_count);
    ASSERT(count > 0);
    uint32_t qf = gpu->queues.queue_families[queue];
    ASSERT(qf < gpu->queues.queue_family_count);
    ASSERT(gpu->queues.cmd_pools[qf] != VK_NULL_HANDLE);
    log_trace("creating commands on queue #%d, queue family #%d", queue, qf);

    VklCommands commands = {0};
    commands.gpu = gpu;
    commands.queue_idx = queue;
    commands.count = count;
    allocate_command_buffers(gpu->device, gpu->queues.cmd_pools[qf], count, commands.cmds);

    obj_init(&commands.obj);

    return commands;
}



void vkl_cmd_begin(VklCommands* cmds, uint32_t idx)
{
    ASSERT(cmds != NULL);
    ASSERT(cmds->count > 0);
    log_trace("begin command buffer");
    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CHECK_RESULT(vkBeginCommandBuffer(cmds->cmds[idx], &begin_info));
}



void vkl_cmd_end(VklCommands* cmds, uint32_t idx)
{
    ASSERT(cmds != NULL);
    ASSERT(cmds->count > 0);

    log_trace("end command buffer");
    VK_CHECK_RESULT(vkEndCommandBuffer(cmds->cmds[idx]));

    obj_created(&cmds->obj);
}



void vkl_cmd_reset(VklCommands* cmds, uint32_t idx)
{
    ASSERT(cmds != NULL);
    ASSERT(cmds->count > 0);

    log_trace("reset %d command buffer(s)", cmds->count);
    ASSERT(cmds->cmds[idx] != VK_NULL_HANDLE);
    VK_CHECK_RESULT(vkResetCommandBuffer(cmds->cmds[idx], 0));
}



void vkl_cmd_free(VklCommands* cmds)
{
    ASSERT(cmds != NULL);
    ASSERT(cmds->count > 0);
    ASSERT(cmds->gpu != NULL);
    ASSERT(cmds->gpu->device != VK_NULL_HANDLE);

    log_trace("free %d command buffer(s)", cmds->count);
    vkFreeCommandBuffers(
        cmds->gpu->device, cmds->gpu->queues.cmd_pools[cmds->queue_idx], //
        cmds->count, cmds->cmds);

    obj_init(&cmds->obj);
}



void vkl_cmd_submit_sync(VklCommands* cmds, uint32_t idx)
{
    log_debug("[SLOW] submit %d command buffer(s)", cmds->count);

    VklQueues* q = &cmds->gpu->queues;
    VkQueue queue = q->queues[cmds->queue_idx];

    vkQueueWaitIdle(queue);
    VkSubmitInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.commandBufferCount = cmds->count;
    info.pCommandBuffers = cmds->cmds;
    vkQueueSubmit(queue, 1, &info, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
}



void vkl_commands_destroy(VklCommands* cmds)
{
    ASSERT(cmds != NULL);
    if (!is_obj_created(&cmds->obj))
    {
        log_trace("skip destruction of already-destroyed commands");
        return;
    }
    log_trace("destroy commands");
    obj_destroyed(&cmds->obj);
}



/*************************************************************************************************/
/*  Buffers                                                                                      */
/*************************************************************************************************/

VklBuffer vkl_buffer(VklGpu* gpu)
{
    ASSERT(gpu != NULL);
    ASSERT(is_obj_created(&gpu->obj));

    VklBuffer buffer = {0};
    buffer.obj.status = VKL_OBJECT_STATUS_INIT;
    buffer.gpu = gpu;

    // Default values.
    buffer.memory = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    return buffer;
}



void vkl_buffer_size(VklBuffer* buffer, VkDeviceSize size)
{
    ASSERT(buffer != NULL);
    buffer->size = size;
}



void vkl_buffer_usage(VklBuffer* buffer, VkBufferUsageFlags usage)
{
    ASSERT(buffer != NULL);
    buffer->usage = usage;
}



void vkl_buffer_memory(VklBuffer* buffer, VkMemoryPropertyFlags memory)
{
    ASSERT(buffer != NULL);
    buffer->memory = memory;
}



void vkl_buffer_queue_access(VklBuffer* buffer, uint32_t queue)
{
    ASSERT(buffer != NULL);
    ASSERT(queue < buffer->gpu->queues.queue_count);
    buffer->queues[buffer->queue_count++] = queue;
}



static void _buffer_create(VklBuffer* buffer)
{
    create_buffer2(
        buffer->gpu->device,                                           //
        &buffer->gpu->queues, buffer->queue_count, buffer->queues,     //
        buffer->usage, buffer->memory, buffer->gpu->memory_properties, //
        buffer->size, &buffer->buffer, &buffer->device_memory);
}



static void _buffer_destroy(VklBuffer* buffer)
{
    if (buffer->buffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(buffer->gpu->device, buffer->buffer, NULL);
        buffer->buffer = VK_NULL_HANDLE;
    }
    if (buffer->device_memory != VK_NULL_HANDLE)
    {
        vkFreeMemory(buffer->gpu->device, buffer->device_memory, NULL);
        buffer->device_memory = VK_NULL_HANDLE;
    }

    buffer->buffer = VK_NULL_HANDLE;
    buffer->device_memory = VK_NULL_HANDLE;
}



static void _buffer_copy(VklBuffer* buffer0, VklBuffer* buffer1)
{
    // Copy the parameters of a buffer.
    buffer1->gpu = buffer0->gpu;
    buffer1->obj = buffer0->obj;
    buffer1->queue_count = buffer0->queue_count;
    memcpy(buffer1->queues, buffer0->queues, sizeof(buffer0->queues));
    buffer1->size = buffer0->size;
    buffer1->usage = buffer0->usage;
    buffer1->memory = buffer0->memory;
}



void vkl_buffer_create(VklBuffer* buffer)
{
    ASSERT(buffer != NULL);
    ASSERT(buffer->gpu != NULL);
    ASSERT(buffer->gpu->device != VK_NULL_HANDLE);
    ASSERT(buffer->size > 0);
    ASSERT(buffer->usage != VK_NULL_HANDLE);
    ASSERT(buffer->memory != VK_NULL_HANDLE);

    log_trace("starting creation of buffer...");
    _buffer_create(buffer);

    obj_created(&buffer->obj);
    log_trace("buffer created");
}



void vkl_buffer_resize(VklBuffer* buffer, VkDeviceSize size, uint32_t queue_idx, VklCommands* cmds)
{
    ASSERT(buffer != NULL);
    log_debug("[SLOW] resize buffer to size %d", size);
    VklGpu* gpu = buffer->gpu;

    // Create the new buffer with the new size.
    VklBuffer new_buffer = vkl_buffer(gpu);
    _buffer_copy(buffer, &new_buffer);
    // Make sure we can copy to the new buffer.
    if ((new_buffer.usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) == 0)
    {
        log_warn("buffer was not created with VK_BUFFER_USAGE_TRANSFER_DST_BIT and therefore the "
                 "data cannot be kept while resizing it");
        cmds = NULL;
    }
    new_buffer.size = size;
    _buffer_create(&new_buffer);
    // At this point, the new buffer is empty.

    // If a VklCommands object was passed for the data transfer, transfer the data from the
    // old buffer to the new, by flushing the corresponding queue and waiting for completion.
    if (cmds != NULL)
    {
        log_debug("copying data from the old buffer to the new one before destroying the old one");
        ASSERT(queue_idx < gpu->queues.queue_count);
        ASSERT(size >= buffer->size);

        vkl_cmd_reset(cmds, 0);
        vkl_cmd_begin(cmds, 0);
        vkl_cmd_copy_buffer(cmds, 0, buffer, 0, &new_buffer, 0, buffer->size);
        vkl_cmd_end(cmds, 0);

        VkQueue queue = gpu->queues.queues[queue_idx];
        vkl_cmd_submit_sync(cmds, 0);
        vkQueueWaitIdle(queue);
    }

    // Delete the old buffer after the transfer has finished.
    _buffer_destroy(buffer);

    // Update the existing buffer's size.
    buffer->size = new_buffer.size;
    ASSERT(buffer->size == size);

    // Update the existing VklBuffer struct with the newly-created Vulkan objects.
    buffer->buffer = new_buffer.buffer;
    buffer->device_memory = new_buffer.device_memory;
    ASSERT(buffer->buffer != VK_NULL_HANDLE);
    ASSERT(buffer->device_memory != VK_NULL_HANDLE);
}



void* vkl_buffer_map(VklBuffer* buffer, VkDeviceSize offset, VkDeviceSize size)
{
    ASSERT(buffer != NULL);
    ASSERT(buffer->gpu != NULL);
    ASSERT(buffer->gpu->device != VK_NULL_HANDLE);
    ASSERT(is_obj_created(&buffer->obj));
    ASSERT(
        (buffer->memory & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) && //
        (buffer->memory & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

    // log_trace("map buffer");
    void* cdata = NULL;
    VK_CHECK_RESULT(
        vkMapMemory(buffer->gpu->device, buffer->device_memory, offset, size, 0, &cdata));
    return cdata;
}



void vkl_buffer_unmap(VklBuffer* buffer)
{
    ASSERT(buffer != NULL);
    ASSERT(buffer->gpu != NULL);
    ASSERT(buffer->gpu->device != VK_NULL_HANDLE);
    ASSERT(is_obj_created(&buffer->obj));

    ASSERT(
        (buffer->memory & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) && //
        (buffer->memory & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

    // log_trace("unmap buffer");
    vkUnmapMemory(buffer->gpu->device, buffer->device_memory);
}



void vkl_buffer_upload(VklBuffer* buffer, VkDeviceSize offset, VkDeviceSize size, const void* data)
{
    ASSERT(buffer != NULL);
    ASSERT(size != 0);
    ASSERT(data != NULL);

    log_trace("uploading %d bytes to GPU buffer", size);
    void* mapped = vkl_buffer_map(buffer, offset, size);
    ASSERT(mapped != NULL);
    memcpy(mapped, data, size);
    vkl_buffer_unmap(buffer);
}



void vkl_buffer_download(VklBuffer* buffer, VkDeviceSize offset, VkDeviceSize size, void* data)
{
    log_trace("downloading %d bytes from GPU buffer", size);
    VklBufferRegions br = {0};
    br.buffer = buffer;
    br.count = 1;
    br.offsets[0] = offset;
    br.size = size;
    void* mapped = vkl_buffer_regions_map(&br, 0);
    memcpy(data, mapped, size);
    vkl_buffer_regions_unmap(&br);
}



void vkl_buffer_destroy(VklBuffer* buffer)
{
    ASSERT(buffer != NULL);
    if (!is_obj_created(&buffer->obj))
    {
        log_trace("skip destruction of already-destroyed buffer");
        return;
    }
    log_trace("destroy buffer");
    _buffer_destroy(buffer);
    obj_destroyed(&buffer->obj);
}



/*************************************************************************************************/
/*  Buffer regions                                                                               */
/*************************************************************************************************/

VklBufferRegions vkl_buffer_regions(
    VklBuffer* buffer, uint32_t count, //
    VkDeviceSize offset, VkDeviceSize size, VkDeviceSize alignment)
{
    ASSERT(buffer != NULL);
    ASSERT(buffer->gpu != NULL);
    ASSERT(buffer->gpu->device != VK_NULL_HANDLE);
    ASSERT(is_obj_created(&buffer->obj));
    ASSERT(count <= VKL_MAX_BUFFER_REGIONS_PER_SET);

    VklBufferRegions regions = {0};
    regions.buffer = buffer;
    regions.count = count;
    regions.size = size;
    regions.alignment = alignment;

    VkDeviceSize offset_req = offset;
    if (alignment > 0)
    {
        // Aligned size for uniform buffers.
        regions.aligned_size = aligned_size(size, alignment);
        // Align the offset.
        offset = aligned_size(offset, alignment);
        ASSERT(offset >= offset_req);
        ASSERT(regions.aligned_size >= regions.size);
        // Align the size.
        size = regions.aligned_size;
    }

    // Compute the offsets.
    for (uint32_t i = 0; i < count; i++)
    {
        regions.offsets[i] = offset + i * size;
        if (alignment > 0)
        {
            ASSERT(regions.offsets[i] % alignment == 0);
        }
    }

    return regions;
}



void* vkl_buffer_regions_map(VklBufferRegions* buffer_regions, uint32_t idx)
{
    ASSERT(buffer_regions != NULL);
    VklBuffer* buffer = buffer_regions->buffer;
    return vkl_buffer_map(buffer, buffer_regions->offsets[idx], buffer_regions->size);
}



void vkl_buffer_regions_unmap(VklBufferRegions* buffer_regions)
{
    ASSERT(buffer_regions != NULL);
    VklBuffer* buffer = buffer_regions->buffer;
    ASSERT(buffer != NULL);
    vkl_buffer_unmap(buffer);
}



void vkl_buffer_regions_upload(VklBufferRegions* br, uint32_t idx, const void* data)
{
    ASSERT(br != NULL);
    VklBuffer* buffer = br->buffer;
    VkDeviceSize size = br->size;

    ASSERT(buffer != NULL);
    ASSERT(size != 0);
    ASSERT(data != NULL);

    log_trace("uploading %d bytes to GPU buffer", size);
    void* mapped = vkl_buffer_regions_map(br, idx);
    ASSERT(mapped != NULL);
    memcpy(mapped, data, size);
    vkl_buffer_regions_unmap(br);
}



/*************************************************************************************************/
/*  Images                                                                                       */
/*************************************************************************************************/

VklImages vkl_images(VklGpu* gpu, VkImageType type, uint32_t count)
{
    ASSERT(gpu != NULL);
    ASSERT(is_obj_created(&gpu->obj));

    VklImages images = {0};
    images.obj.status = VKL_OBJECT_STATUS_INIT;

    images.gpu = gpu;
    images.image_type = type;
    ASSERT(type <= VK_IMAGE_TYPE_3D);
    // HACK: find the matching view type.
    images.view_type = (VkImageViewType)type;
    images.count = count;

    // Default options.
    images.tiling = VK_IMAGE_TILING_OPTIMAL;
    images.memory = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    images.aspect = VK_IMAGE_ASPECT_COLOR_BIT;

    return images;
}



void vkl_images_format(VklImages* images, VkFormat format)
{
    ASSERT(images != NULL);
    images->format = format;
}



void vkl_images_layout(VklImages* images, VkImageLayout layout)
{
    ASSERT(images != NULL);
    images->layout = layout;
}



void vkl_images_size(VklImages* images, uint32_t width, uint32_t height, uint32_t depth)
{
    ASSERT(images != NULL);

    log_trace("set image size %dx%d", width, height);
    check_dims(images->image_type, width, height, depth);

    images->width = width;
    images->height = height;
    images->depth = depth;
}



void vkl_images_tiling(VklImages* images, VkImageTiling tiling)
{
    ASSERT(images != NULL);
    images->tiling = tiling;
}



void vkl_images_usage(VklImages* images, VkImageUsageFlags usage)
{
    ASSERT(images != NULL);
    images->usage = usage;
}



void vkl_images_memory(VklImages* images, VkMemoryPropertyFlags memory)
{
    ASSERT(images != NULL);
    images->memory = memory;
}



void vkl_images_aspect(VklImages* images, VkImageAspectFlags aspect)
{
    ASSERT(images != NULL);
    images->aspect = aspect;
}



void vkl_images_queue_access(VklImages* images, uint32_t queue)
{
    ASSERT(images != NULL);
    ASSERT(queue < images->gpu->queues.queue_count);
    images->queues[images->queue_count++] = queue;
}



static void _images_create(VklImages* images)
{
    VklGpu* gpu = images->gpu;
    for (uint32_t i = 0; i < images->count; i++)
    {
        if (!images->is_swapchain)
            create_image2(
                gpu->device, &gpu->queues, images->queue_count, images->queues, images->image_type,
                images->width, images->height, images->depth, images->format, images->tiling,
                images->usage, images->memory, gpu->memory_properties, &images->images[i],
                &images->memories[i]);

        // HACK: staging images do not require an image view
        if (images->tiling != VK_IMAGE_TILING_LINEAR)
            create_image_view2(
                gpu->device, images->images[i], images->view_type, images->format, images->aspect,
                &images->image_views[i]);
    }
}



static void _images_destroy(VklImages* images)
{
    for (uint32_t i = 0; i < images->count; i++)
    {
        if (images->image_views[i] != VK_NULL_HANDLE)
        {
            vkDestroyImageView(images->gpu->device, images->image_views[i], NULL);
            images->image_views[i] = VK_NULL_HANDLE;
        }
        if (!images->is_swapchain && images->images[i] != VK_NULL_HANDLE)
        {
            vkDestroyImage(images->gpu->device, images->images[i], NULL);
            images->images[i] = VK_NULL_HANDLE;
        }
        if (images->memories[i] != VK_NULL_HANDLE)
        {
            vkFreeMemory(images->gpu->device, images->memories[i], NULL);
            images->memories[i] = VK_NULL_HANDLE;
        }
    }
}



void vkl_images_create(VklImages* images)
{
    ASSERT(images != NULL);
    ASSERT(images->gpu != NULL);
    ASSERT(images->gpu->device != VK_NULL_HANDLE);

    check_dims(images->image_type, images->width, images->height, images->depth);

    log_trace("starting creation of %d images...", images->count);
    _images_create(images);
    obj_created(&images->obj);
    log_trace("%d images created", images->count);
}



void vkl_images_transition(VklImages* images)
{
    ASSERT(images != NULL);
    VklGpu* gpu = images->gpu;
    ASSERT(gpu != NULL);

    // Start the image transition command buffer.
    VklCommands cmds = vkl_commands(gpu, 0, 1);
    VklBarrier barrier = vkl_barrier(gpu);

    vkl_cmd_begin(&cmds, 0);
    vkl_barrier_stages(&barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    vkl_barrier_images(&barrier, images);
    vkl_barrier_images_layout(&barrier, VK_IMAGE_LAYOUT_UNDEFINED, images->layout);
    // vkl_barrier_images_access(&barrier, 0, VK_ACCESS_TRANSFER_WRITE_BIT);
    vkl_cmd_barrier(&cmds, 0, &barrier);
    vkl_cmd_end(&cmds, 0);

    vkl_gpu_wait(gpu);
    vkl_cmd_submit_sync(&cmds, 0);
}



void vkl_images_resize(VklImages* images, uint32_t width, uint32_t height, uint32_t depth)
{
    ASSERT(images != NULL);
    log_debug(
        "[SLOW] resize images to size %dx%dx%d, losing the data in it", width, height, depth);
    _images_destroy(images);
    vkl_images_size(images, width, height, depth);
    _images_create(images);
}



void vkl_images_download(VklImages* staging, uint32_t idx, bool swizzle, uint8_t* rgb)
{
    VkImageSubresource subResource = {0};
    subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    VkSubresourceLayout subResourceLayout = {0};
    vkGetImageSubresourceLayout(
        staging->gpu->device, staging->images[idx], &subResource, &subResourceLayout);

    // Map image memory so we can start copying from it
    void* data = NULL;
    vkMapMemory(staging->gpu->device, staging->memories[idx], 0, VK_WHOLE_SIZE, 0, &data);
    ASSERT(data != NULL);
    VkDeviceSize offset = subResourceLayout.offset;
    VkDeviceSize row_pitch = subResourceLayout.rowPitch;
    ASSERT(row_pitch > 0);

    uint32_t w = staging->width;
    uint32_t h = staging->height;
    ASSERT(w > 0);
    ASSERT(h > 0);

    // First, memcopy from the GPU to the CPU.
    uint8_t* image = calloc(w * h, 4);
    uint8_t* image_orig = image;
    memcpy(image, data, w * h * 4);
    vkUnmapMemory(staging->gpu->device, staging->memories[idx]);

    // Then, swizzle.
    image += offset;
    uint32_t src_offset = 0;
    uint32_t dst_offset = 0;
    for (uint32_t y = 0; y < h; y++)
    {
        src_offset = 0;
        for (uint32_t x = 0; x < w; x++)
        {
            if (swizzle)
            {
                rgb[dst_offset + 0] = image[src_offset + 2];
                rgb[dst_offset + 1] = image[src_offset + 1];
                rgb[dst_offset + 2] = image[src_offset + 0];
                // rgba[dst_offset + 3] = image[src_offset + 3];
            }
            else
            {
                rgb[dst_offset + 0] = image[src_offset + 0];
                rgb[dst_offset + 1] = image[src_offset + 1];
                rgb[dst_offset + 2] = image[src_offset + 2];
                // rgba[dst_offset + 3] = image[src_offset + 3];
            }
            src_offset += 4;
            dst_offset += 3;
        }
        image += row_pitch;
    }
    ASSERT(dst_offset == w * h * 3);
    FREE(image_orig);
}



void vkl_images_destroy(VklImages* images)
{
    ASSERT(images != NULL);
    if (!is_obj_created(&images->obj))
    {
        log_trace("skip destruction of already-destroyed images");
        return;
    }
    log_trace("destroy %d image(s) and image view(s)", images->count);
    _images_destroy(images);
    obj_destroyed(&images->obj);
}



/*************************************************************************************************/
/*  Sampler                                                                                      */
/*************************************************************************************************/

VklSampler vkl_sampler(VklGpu* gpu)
{
    ASSERT(gpu != NULL);
    ASSERT(is_obj_created(&gpu->obj));

    VklSampler sampler = {0};
    sampler.obj.status = VKL_OBJECT_STATUS_INIT;
    sampler.gpu = gpu;

    return sampler;
}



void vkl_sampler_min_filter(VklSampler* sampler, VkFilter filter)
{
    ASSERT(sampler != NULL);
    sampler->min_filter = filter;
}



void vkl_sampler_mag_filter(VklSampler* sampler, VkFilter filter)
{
    ASSERT(sampler != NULL);
    sampler->mag_filter = filter;
}



void vkl_sampler_address_mode(
    VklSampler* sampler, VklTextureAxis axis, VkSamplerAddressMode address_mode)
{
    ASSERT(sampler != NULL);
    ASSERT(axis <= 2);
    sampler->address_modes[axis] = address_mode;
}



void vkl_sampler_create(VklSampler* sampler)
{
    ASSERT(sampler != NULL);
    ASSERT(sampler->gpu != NULL);
    ASSERT(sampler->gpu->device != VK_NULL_HANDLE);

    log_trace("starting creation of sampler...");

    create_texture_sampler2(
        sampler->gpu->device, sampler->mag_filter, sampler->min_filter, //
        sampler->address_modes, false, &sampler->sampler);

    obj_created(&sampler->obj);
    log_trace("sampler created");
}



void vkl_sampler_destroy(VklSampler* sampler)
{
    ASSERT(sampler != NULL);
    if (!is_obj_created(&sampler->obj))
    {
        log_trace("skip destruction of already-destroyed sampler");
        return;
    }
    log_trace("destroy sampler");
    if (sampler->sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(sampler->gpu->device, sampler->sampler, NULL);
        sampler->sampler = VK_NULL_HANDLE;
    }
    obj_destroyed(&sampler->obj);
}



/*************************************************************************************************/
/*  Slots                                                                                        */
/*************************************************************************************************/

VklSlots vkl_slots(VklGpu* gpu)
{
    ASSERT(gpu != NULL);
    ASSERT(is_obj_created(&gpu->obj));

    VklSlots slots = {0};
    slots.gpu = gpu;
    obj_init(&slots.obj);

    return slots;
}



void vkl_slots_binding(VklSlots* slots, uint32_t idx, VkDescriptorType type)
{
    ASSERT(slots != NULL);
    ASSERT(idx == slots->slot_count);
    ASSERT(idx < VKL_MAX_BINDINGS_SIZE);
    slots->types[slots->slot_count++] = type;
}



void vkl_slots_push(
    VklSlots* slots, VkDeviceSize offset, VkDeviceSize size, VkShaderStageFlags shaders)
{
    ASSERT(slots != NULL);
    uint32_t idx = slots->push_count;
    ASSERT(idx < VKL_MAX_PUSH_CONSTANTS);

    slots->push_offsets[idx] = offset;
    slots->push_sizes[idx] = size;
    slots->push_shaders[idx] = shaders;

    slots->push_count++;
}



void vkl_slots_create(VklSlots* slots)
{
    ASSERT(slots != NULL);
    ASSERT(slots->gpu != NULL);
    ASSERT(slots->gpu->device != VK_NULL_HANDLE);

    log_trace("starting creation of slots...");

    create_descriptor_set_layout(
        slots->gpu->device, slots->slot_count, slots->types, &slots->dset_layout);

    // Push constants.
    VkPushConstantRange push_constants[VKL_MAX_PUSH_CONSTANTS] = {0};
    for (uint32_t i = 0; i < slots->push_count; i++)
    {
        push_constants[i].offset = slots->push_offsets[i];
        push_constants[i].size = slots->push_sizes[i];
        push_constants[i].stageFlags = slots->push_shaders[i];
    }

    // Create the pipeline layout.
    create_pipeline_layout(
        slots->gpu->device, slots->push_count, push_constants, //
        &slots->dset_layout, &slots->pipeline_layout);

    obj_created(&slots->obj);
    log_trace("slots created");
}



void vkl_slots_destroy(VklSlots* slots)
{
    ASSERT(slots != NULL);
    ASSERT(slots->gpu != NULL);
    if (!is_obj_created(&slots->obj))
    {
        log_trace("skip destruction of already-destroyed slots");
        return;
    }
    log_trace("destroy slots");
    VkDevice device = slots->gpu->device;
    if (slots->pipeline_layout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(device, slots->pipeline_layout, NULL);
        slots->pipeline_layout = VK_NULL_HANDLE;
    }
    if (slots->dset_layout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(device, slots->dset_layout, NULL);
        slots->dset_layout = VK_NULL_HANDLE;
    }
    obj_destroyed(&slots->obj);
}



/*************************************************************************************************/
/*  Bindings                                                                                     */
/*************************************************************************************************/

VklBindings vkl_bindings(VklSlots* slots, uint32_t dset_count)
{
    ASSERT(slots != NULL);
    VklGpu* gpu = slots->gpu;
    ASSERT(gpu != NULL);
    ASSERT(is_obj_created(&gpu->obj));

    VklBindings bindings = {0};
    bindings.slots = slots;
    bindings.gpu = gpu;

    obj_init(&bindings.obj);

    if (!is_obj_created(&slots->obj))
        vkl_slots_create(slots);
    ASSERT(dset_count > 0);
    ASSERT(slots->dset_layout != VK_NULL_HANDLE);

    log_trace("starting creation of bindings with %d descriptor sets...", dset_count);
    bindings.dset_count = dset_count;

    allocate_descriptor_sets(
        gpu->device, gpu->dset_pool, slots->dset_layout, bindings.dset_count, bindings.dsets);

    obj_created(&bindings.obj);
    log_trace("bindings created");

    return bindings;
}



void vkl_bindings_buffer(VklBindings* bindings, uint32_t idx, VklBufferRegions buffer_regions)
{
    ASSERT(bindings != NULL);
    ASSERT(buffer_regions.buffer != VK_NULL_HANDLE);
    ASSERT(buffer_regions.count > 0);
    ASSERT(bindings->dset_count > 0);
    ASSERT(buffer_regions.count == 1 || buffer_regions.count == bindings->dset_count);
    log_trace("set bindings with buffer for binding #%d", idx);

    bindings->buffer_regions[idx] = buffer_regions;

    if (bindings->obj.status == VKL_OBJECT_STATUS_CREATED)
        bindings->obj.status = VKL_OBJECT_STATUS_NEED_UPDATE;
}



void vkl_bindings_texture(VklBindings* bindings, uint32_t idx, VklTexture* texture)
{
    ASSERT(bindings != NULL);
    ASSERT(texture != NULL);

    VklImages* images = texture->image;
    VklSampler* sampler = texture->sampler;

    ASSERT(images != NULL);
    ASSERT(sampler != NULL);
    ASSERT(images->count == 1 || images->count == bindings->dset_count);

    log_trace("set bindings with texture for binding #%d", idx);
    bindings->images[idx] = images;
    bindings->samplers[idx] = sampler;

    if (bindings->obj.status == VKL_OBJECT_STATUS_CREATED)
        bindings->obj.status = VKL_OBJECT_STATUS_NEED_UPDATE;
}



void vkl_bindings_update(VklBindings* bindings)
{
    log_trace("update bindings");
    ASSERT(bindings->slots != NULL);
    ASSERT(is_obj_created(&bindings->slots->obj));
    ASSERT(bindings->slots->dset_layout != VK_NULL_HANDLE);
    ASSERT(bindings->dset_count > 0);
    ASSERT(bindings->dset_count <= VKL_MAX_SWAPCHAIN_IMAGES);

    for (uint32_t i = 0; i < bindings->dset_count; i++)
    {
        update_descriptor_set(
            bindings->gpu->device, bindings->slots->slot_count, bindings->slots->types,
            bindings->buffer_regions, bindings->images, bindings->samplers, //
            i, bindings->dsets[i]);
    }

    if (bindings->obj.status == VKL_OBJECT_STATUS_NEED_UPDATE)
        bindings->obj.status = VKL_OBJECT_STATUS_CREATED;
}



void vkl_bindings_destroy(VklBindings* bindings)
{
    ASSERT(bindings != NULL);
    ASSERT(bindings->gpu != NULL);
    if (!is_obj_created(&bindings->obj))
    {
        log_trace("skip destruction of already-destroyed bindings");
        return;
    }
    log_trace("destroy bindings");
    obj_destroyed(&bindings->obj);
}



/*************************************************************************************************/
/*  Compute                                                                                      */
/*************************************************************************************************/

VklCompute vkl_compute(VklGpu* gpu, const char* shader_path)
{
    ASSERT(gpu != NULL);
    ASSERT(is_obj_created(&gpu->obj));

    VklCompute compute = {0};
    compute.obj.status = VKL_OBJECT_STATUS_INIT;

    compute.gpu = gpu;
    strcpy(compute.shader_path, shader_path);

    compute.slots = vkl_slots(gpu);

    return compute;
}



void vkl_compute_code(VklCompute* compute, const char* code)
{
    ASSERT(compute != NULL);
    compute->shader_code = code;
}



void vkl_compute_slot(VklCompute* compute, uint32_t idx, VkDescriptorType type)
{
    ASSERT(compute != NULL);
    vkl_slots_binding(&compute->slots, idx, type);
}



void vkl_compute_push(
    VklCompute* compute, VkDeviceSize offset, VkDeviceSize size, VkShaderStageFlags shaders)
{
    ASSERT(compute != NULL);
    vkl_slots_push(&compute->slots, offset, size, shaders);
}



void vkl_compute_bindings(VklCompute* compute, VklBindings* bindings)
{
    ASSERT(compute != NULL);
    ASSERT(bindings != NULL);
    compute->bindings = bindings;
}



void vkl_compute_create(VklCompute* compute)
{
    ASSERT(compute != NULL);
    ASSERT(compute->gpu != NULL);
    ASSERT(compute->gpu->device != VK_NULL_HANDLE);
    ASSERT(compute->shader_path != NULL);
    if (!is_obj_created(&compute->slots.obj))
        vkl_slots_create(&compute->slots);

    if (compute->bindings == NULL)
    {
        log_error("vkl_compute_bindings() must be called before creating the compute");
        return;
    }

    log_trace("starting creation of compute...");

    if (compute->shader_code != NULL)
    {
        compute->shader_module =
            vkl_shader_compile(compute->gpu, compute->shader_code, VK_SHADER_STAGE_COMPUTE_BIT);
    }
    else
    {
        compute->shader_module =
            create_shader_module_from_file(compute->gpu->device, compute->shader_path);
    }

    create_compute_pipeline(
        compute->gpu->device, compute->shader_module, //
        compute->slots.pipeline_layout, &compute->pipeline);

    obj_created(&compute->obj);
    log_trace("compute created");
}



void vkl_compute_destroy(VklCompute* compute)
{
    ASSERT(compute != NULL);
    ASSERT(compute->gpu != NULL);
    if (!is_obj_created(&compute->obj))
    {
        log_trace("skip destruction of already-destroyed compute");
        return;
    }
    log_trace("destroy compute");

    // Destroy the compute slots.
    if (is_obj_created(&compute->slots.obj))
        vkl_slots_destroy(&compute->slots);

    VkDevice device = compute->gpu->device;
    if (compute->shader_module != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(device, compute->shader_module, NULL);
        compute->shader_module = VK_NULL_HANDLE;
    }
    if (compute->pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(device, compute->pipeline, NULL);
        compute->pipeline = VK_NULL_HANDLE;
    }

    obj_destroyed(&compute->obj);
}



/*************************************************************************************************/
/*  Graphics                                                                                     */
/*************************************************************************************************/

VklGraphics vkl_graphics(VklGpu* gpu)
{
    ASSERT(gpu != NULL);
    ASSERT(is_obj_created(&gpu->obj));

    VklGraphics graphics = {0};
    graphics.gpu = gpu;
    obj_init(&graphics.obj);

    graphics.slots = vkl_slots(gpu);

    return graphics;
}



void vkl_graphics_renderpass(VklGraphics* graphics, VklRenderpass* renderpass, uint32_t subpass)
{
    ASSERT(graphics != NULL);
    graphics->renderpass = renderpass;
    graphics->subpass = subpass;
}



void vkl_graphics_topology(VklGraphics* graphics, VkPrimitiveTopology topology)
{
    ASSERT(graphics != NULL);
    graphics->topology = topology;
}



void vkl_graphics_shader_glsl(VklGraphics* graphics, VkShaderStageFlagBits stage, const char* code)
{
    ASSERT(graphics != NULL);
    ASSERT(graphics->gpu != NULL);
    ASSERT(graphics->gpu->device != VK_NULL_HANDLE);

    graphics->shader_stages[graphics->shader_count] = stage;
    graphics->shader_modules[graphics->shader_count] =
        vkl_shader_compile(graphics->gpu, code, stage);
    graphics->shader_count++;
}



void vkl_graphics_shader(
    VklGraphics* graphics, VkShaderStageFlagBits stage, const char* shader_path)
{
    ASSERT(graphics != NULL);
    ASSERT(graphics->gpu != NULL);
    ASSERT(graphics->gpu->device != VK_NULL_HANDLE);

    graphics->shader_stages[graphics->shader_count] = stage;
    graphics->shader_modules[graphics->shader_count++] =
        create_shader_module_from_file(graphics->gpu->device, shader_path);
}



void vkl_graphics_shader_spirv(
    VklGraphics* graphics, VkShaderStageFlagBits stage, //
    VkDeviceSize size, const uint32_t* buffer)
{
    ASSERT(graphics != NULL);
    ASSERT(graphics->gpu != NULL);
    ASSERT(graphics->gpu->device != VK_NULL_HANDLE);

    graphics->shader_stages[graphics->shader_count] = stage;
    graphics->shader_modules[graphics->shader_count++] =
        create_shader_module(graphics->gpu->device, size, buffer);
}



void vkl_graphics_vertex_binding(VklGraphics* graphics, uint32_t binding, VkDeviceSize stride)
{
    ASSERT(graphics != NULL);
    VklVertexBinding* vb = &graphics->vertex_bindings[graphics->vertex_binding_count++];
    vb->binding = binding;
    vb->stride = stride;
}



void vkl_graphics_vertex_attr(
    VklGraphics* graphics, uint32_t binding, uint32_t location, VkFormat format,
    VkDeviceSize offset)
{
    ASSERT(graphics != NULL);
    VklVertexAttr* va = &graphics->vertex_attrs[graphics->vertex_attr_count++];
    va->binding = binding;
    va->location = location;
    va->format = format;
    va->offset = offset;
}



void vkl_graphics_blend(VklGraphics* graphics, VklBlendType blend_type)
{
    ASSERT(graphics != NULL);
    graphics->blend_type = blend_type;
}



void vkl_graphics_depth_test(VklGraphics* graphics, VklDepthTest depth_test)
{
    ASSERT(graphics != NULL);
    graphics->depth_test = depth_test;
}



void vkl_graphics_polygon_mode(VklGraphics* graphics, VkPolygonMode polygon_mode)
{
    ASSERT(graphics != NULL);
    graphics->polygon_mode = polygon_mode;
}



void vkl_graphics_cull_mode(VklGraphics* graphics, VkCullModeFlags cull_mode)
{
    ASSERT(graphics != NULL);
    graphics->cull_mode = cull_mode;
}



void vkl_graphics_front_face(VklGraphics* graphics, VkFrontFace front_face)
{
    ASSERT(graphics != NULL);
    graphics->front_face = front_face;
}



void vkl_graphics_slot(VklGraphics* graphics, uint32_t idx, VkDescriptorType type)
{
    ASSERT(graphics != NULL);
    vkl_slots_binding(&graphics->slots, idx, type);
}



void vkl_graphics_push(
    VklGraphics* graphics, VkDeviceSize offset, VkDeviceSize size, VkShaderStageFlags shaders)
{
    ASSERT(graphics != NULL);
    vkl_slots_push(&graphics->slots, offset, size, shaders);
}



void vkl_graphics_create(VklGraphics* graphics)
{
    ASSERT(graphics != NULL);
    ASSERT(graphics->gpu != NULL);
    ASSERT(graphics->gpu->device != VK_NULL_HANDLE);
    ASSERT(graphics->renderpass != NULL);
    if (!is_obj_created(&graphics->slots.obj))
        vkl_slots_create(&graphics->slots);

    log_trace("starting creation of graphics pipeline...");

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {0};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    // Vertex bindings.
    VkVertexInputBindingDescription bindings_info[VKL_MAX_VERTEX_BINDINGS] = {0};
    for (uint32_t i = 0; i < graphics->vertex_binding_count; i++)
    {
        bindings_info[i].binding = graphics->vertex_bindings[i].binding;
        bindings_info[i].stride = graphics->vertex_bindings[i].stride;
        bindings_info[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    }
    vertex_input_info.vertexBindingDescriptionCount = graphics->vertex_binding_count;
    vertex_input_info.pVertexBindingDescriptions = bindings_info;

    // Vertex attributes.
    VkVertexInputAttributeDescription attrs_info[VKL_MAX_VERTEX_ATTRS] = {0};
    for (uint32_t i = 0; i < graphics->vertex_attr_count; i++)
    {
        attrs_info[i].binding = graphics->vertex_attrs[i].binding;
        attrs_info[i].location = graphics->vertex_attrs[i].location;
        attrs_info[i].format = graphics->vertex_attrs[i].format;
        attrs_info[i].offset = graphics->vertex_attrs[i].offset;
    }
    vertex_input_info.vertexAttributeDescriptionCount = graphics->vertex_attr_count;
    vertex_input_info.pVertexAttributeDescriptions = attrs_info;

    // Shaders.
    VkPipelineShaderStageCreateInfo shader_stages[VKL_MAX_SHADERS_PER_GRAPHICS] = {0};
    for (uint32_t i = 0; i < graphics->shader_count; i++)
    {
        shader_stages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stages[i].stage = graphics->shader_stages[i];
        shader_stages[i].module = graphics->shader_modules[i];
        ASSERT(graphics->shader_stages[i] != VK_NULL_HANDLE);
        ASSERT(graphics->shader_modules[i] != NULL);
        shader_stages[i].pName = "main";
    }

    // Pipeline.
    VkPipelineInputAssemblyStateCreateInfo input_assembly =
        create_input_assembly(graphics->topology);
    VkPipelineRasterizationStateCreateInfo rasterizer = create_rasterizer();
    VkPipelineMultisampleStateCreateInfo multisampling = create_multisampling();
    VkPipelineColorBlendAttachmentState color_blend_attachment = create_color_blend_attachment();
    VkPipelineColorBlendStateCreateInfo color_blending =
        create_color_blending(&color_blend_attachment);
    VkPipelineDepthStencilStateCreateInfo depth_stencil =
        create_depth_stencil((bool)graphics->depth_test);
    VkPipelineViewportStateCreateInfo viewport_state = create_viewport_state();
    VkPipelineDynamicStateCreateInfo dynamic_state = create_dynamic_states(
        2, (VkDynamicState[]){VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR});


    // Finally create the pipeline.
    VkGraphicsPipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = graphics->shader_count;
    pipelineInfo.pStages = shader_stages;
    pipelineInfo.pVertexInputState = &vertex_input_info;
    pipelineInfo.pInputAssemblyState = &input_assembly;
    pipelineInfo.pViewportState = &viewport_state;
    pipelineInfo.pDynamicState = &dynamic_state;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &color_blending;
    pipelineInfo.pDepthStencilState = &depth_stencil;
    ASSERT(graphics->slots.pipeline_layout != VK_NULL_HANDLE);
    pipelineInfo.layout = graphics->slots.pipeline_layout;
    pipelineInfo.renderPass = graphics->renderpass->renderpass;
    pipelineInfo.subpass = graphics->subpass;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(
        graphics->gpu->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphics->pipeline));
    log_trace("graphics pipeline created");
    obj_created(&graphics->obj);
}



void vkl_graphics_destroy(VklGraphics* graphics)
{
    ASSERT(graphics != NULL);
    if (!is_obj_created(&graphics->obj))
    {
        log_trace("skip destruction of already-destroyed graphics");
        return;
    }
    ASSERT(graphics->gpu != NULL);
    log_trace("destroy graphics");

    VkDevice device = graphics->gpu->device;
    for (uint32_t i = 0; i < graphics->shader_count; i++)
    {
        if (graphics->shader_modules[i] != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(device, graphics->shader_modules[i], NULL);
            graphics->shader_modules[i] = VK_NULL_HANDLE;
        }
    }
    if (graphics->pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(device, graphics->pipeline, NULL);
        graphics->pipeline = VK_NULL_HANDLE;
    }

    // Destroy slots.
    if (is_obj_created(&graphics->slots.obj))
        vkl_slots_destroy(&graphics->slots);

    obj_destroyed(&graphics->obj);
}



/*************************************************************************************************/
/*  Barrier                                                                                      */
/*************************************************************************************************/

VklBarrier vkl_barrier(VklGpu* gpu)
{
    ASSERT(gpu != NULL);
    ASSERT(is_obj_created(&gpu->obj));

    VklBarrier barrier = {0};
    barrier.gpu = gpu;
    return barrier;
}



void vkl_barrier_stages(
    VklBarrier* barrier, VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage)
{
    ASSERT(barrier != NULL);
    barrier->src_stage = src_stage;
    barrier->dst_stage = dst_stage;
}



void vkl_barrier_buffer(VklBarrier* barrier, VklBufferRegions* buffer_regions)
{
    ASSERT(barrier != NULL);

    VklBarrierBuffer* b = &barrier->buffer_barriers[barrier->buffer_barrier_count++];

    b->buffer_regions = *buffer_regions;
}



void vkl_barrier_buffer_queue(VklBarrier* barrier, uint32_t src_queue, uint32_t dst_queue)
{
    ASSERT(barrier != NULL);

    VklBarrierBuffer* b = &barrier->buffer_barriers[barrier->buffer_barrier_count - 1];
    ASSERT(b->buffer_regions.buffer != NULL);

    b->queue_transfer = true;
    b->src_queue = src_queue;
    b->dst_queue = dst_queue;
}



void vkl_barrier_buffer_access(
    VklBarrier* barrier, VkAccessFlags src_access, VkAccessFlags dst_access)
{
    ASSERT(barrier != NULL);

    VklBarrierBuffer* b = &barrier->buffer_barriers[barrier->buffer_barrier_count - 1];
    ASSERT(b->buffer_regions.buffer != NULL);

    b->src_access = src_access;
    b->dst_access = dst_access;
}



void vkl_barrier_images(VklBarrier* barrier, VklImages* images)
{
    ASSERT(barrier != NULL);

    VklBarrierImage* b = &barrier->image_barriers[barrier->image_barrier_count++];

    b->images = images;
}



void vkl_barrier_images_layout(
    VklBarrier* barrier, VkImageLayout src_layout, VkImageLayout dst_layout)
{
    ASSERT(barrier != NULL);

    VklBarrierImage* b = &barrier->image_barriers[barrier->image_barrier_count - 1];
    ASSERT(b->images != NULL);

    b->src_layout = src_layout;
    b->dst_layout = dst_layout;
}



void vkl_barrier_images_access(
    VklBarrier* barrier, VkAccessFlags src_access, VkAccessFlags dst_access)
{
    ASSERT(barrier != NULL);

    VklBarrierImage* b = &barrier->image_barriers[barrier->image_barrier_count - 1];
    ASSERT(b->images != NULL);

    b->src_access = src_access;
    b->dst_access = dst_access;
}



void vkl_barrier_images_queue(VklBarrier* barrier, uint32_t src_queue, uint32_t dst_queue)
{
    ASSERT(barrier != NULL);

    VklBarrierImage* b = &barrier->image_barriers[barrier->image_barrier_count - 1];
    ASSERT(b->images != NULL);

    b->queue_transfer = true;
    b->src_queue = src_queue;
    b->dst_queue = dst_queue;
}



/*************************************************************************************************/
/*  Semaphores                                                                                   */
/*************************************************************************************************/

VklSemaphores vkl_semaphores(VklGpu* gpu, uint32_t count)
{
    ASSERT(gpu != NULL);
    ASSERT(is_obj_created(&gpu->obj));

    ASSERT(count > 0);
    log_trace("create set of %d semaphore(s)", count);

    VklSemaphores semaphores = {0};
    semaphores.gpu = gpu;
    semaphores.count = count;

    VkSemaphoreCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for (uint32_t i = 0; i < count; i++)
        VK_CHECK_RESULT(vkCreateSemaphore(gpu->device, &info, NULL, &semaphores.semaphores[i]));

    obj_created(&semaphores.obj);

    return semaphores;
}



void vkl_semaphores_destroy(VklSemaphores* semaphores)
{
    ASSERT(semaphores != NULL);
    if (!is_obj_created(&semaphores->obj))
    {
        log_trace("skip destruction of already-destroyed semaphores");
        return;
    }

    ASSERT(semaphores->count > 0);
    log_trace("destroy set of %d semaphore(s)", semaphores->count);

    for (uint32_t i = 0; i < semaphores->count; i++)
    {
        if (semaphores->semaphores[i] != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(semaphores->gpu->device, semaphores->semaphores[i], NULL);
            semaphores->semaphores[i] = VK_NULL_HANDLE;
        }
    }
    obj_destroyed(&semaphores->obj);
}



/*************************************************************************************************/
/*  Fences                                                                                       */
/*************************************************************************************************/

VklFences vkl_fences(VklGpu* gpu, uint32_t count)
{
    ASSERT(gpu != NULL);
    ASSERT(is_obj_created(&gpu->obj));

    VklFences fences = {0};

    ASSERT(count > 0);
    log_trace("create set of %d fences(s)", count);

    fences.gpu = gpu;
    fences.count = count;

    VkFenceCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < fences.count; i++)
        VK_CHECK_RESULT(vkCreateFence(fences.gpu->device, &info, NULL, &fences.fences[i]));

    obj_created(&fences.obj);
    return fences;
}



void vkl_fences_copy(
    VklFences* src_fences, uint32_t src_idx, VklFences* dst_fences, uint32_t dst_idx)
{
    ASSERT(src_fences != NULL);
    ASSERT(dst_fences != NULL);

    ASSERT(src_idx < src_fences->count);
    ASSERT(dst_idx < dst_fences->count);

    // Wait for the destination fence first (if it is not null).
    // vkl_fences_wait(dst_fences, dst_idx);

    // log_trace("copy fence %d to %d", src_fences->fences[src_idx], dst_fences->fences[dst_idx]);
    dst_fences->fences[dst_idx] = src_fences->fences[src_idx];
}



void vkl_fences_wait(VklFences* fences, uint32_t idx)
{
    ASSERT(fences != NULL);
    ASSERT(idx < fences->count);
    if (fences->fences[idx] != VK_NULL_HANDLE)
    {
        // log_trace("wait for fence %d", fences->fences[idx]);
        vkWaitForFences(fences->gpu->device, 1, &fences->fences[idx], VK_TRUE, 1000000000);
    }
    else
    {
        log_trace("skip wait for fence");
    }
}



bool vkl_fences_ready(VklFences* fences, uint32_t idx)
{
    ASSERT(fences != NULL);
    ASSERT(idx < fences->count);
    ASSERT(fences->fences[idx] != VK_NULL_HANDLE);
    VkResult res = vkGetFenceStatus(fences->gpu->device, fences->fences[idx]);
    if (res == VK_SUCCESS)
        return true;
    return false;
}



void vkl_fences_reset(VklFences* fences, uint32_t idx)
{
    ASSERT(fences != NULL);
    if (fences->fences[idx] != NULL)
    {
        // log_trace("reset fence %d", fences->fences[idx]);
        vkResetFences(fences->gpu->device, 1, &fences->fences[idx]);
    }
}



void vkl_fences_destroy(VklFences* fences)
{
    ASSERT(fences != NULL);
    if (!is_obj_created(&fences->obj))
    {
        log_trace("skip destruction of already-destroyed fences");
        return;
    }

    ASSERT(fences->count > 0);
    log_trace("destroy set of %d fences(s)", fences->count);

    for (uint32_t i = 0; i < fences->count; i++)
    {
        if (fences->fences[i] != VK_NULL_HANDLE)
        {
            vkDestroyFence(fences->gpu->device, fences->fences[i], NULL);
            fences->fences[i] = VK_NULL_HANDLE;
        }
    }
    obj_destroyed(&fences->obj);
}



/*************************************************************************************************/
/*  Renderpass                                                                                   */
/*************************************************************************************************/

VklRenderpass vkl_renderpass(VklGpu* gpu)
{
    ASSERT(gpu != NULL);
    ASSERT(is_obj_created(&gpu->obj));

    VklRenderpass renderpass = {0};
    renderpass.gpu = gpu;

    return renderpass;
}



void vkl_renderpass_clear(VklRenderpass* renderpass, VkClearValue value)
{
    ASSERT(renderpass != NULL);
    renderpass->clear_values[renderpass->clear_count++] = value;
}



void vkl_renderpass_attachment(
    VklRenderpass* renderpass, uint32_t idx, VklRenderpassAttachmentType type, VkFormat format,
    VkImageLayout ref_layout)
{
    ASSERT(renderpass != NULL);
    renderpass->attachments[idx].ref_layout = ref_layout;
    renderpass->attachments[idx].type = type;
    renderpass->attachments[idx].format = format;
    renderpass->attachment_count = MAX(renderpass->attachment_count, idx + 1);
}



void vkl_renderpass_attachment_layout(
    VklRenderpass* renderpass, uint32_t idx, VkImageLayout src_layout, VkImageLayout dst_layout)
{
    ASSERT(renderpass != NULL);
    renderpass->attachments[idx].src_layout = src_layout;
    renderpass->attachments[idx].dst_layout = dst_layout;
    renderpass->attachment_count = MAX(renderpass->attachment_count, idx + 1);
}



void vkl_renderpass_attachment_ops(
    VklRenderpass* renderpass, uint32_t idx, //
    VkAttachmentLoadOp load_op, VkAttachmentStoreOp store_op)
{
    ASSERT(renderpass != NULL);
    renderpass->attachments[idx].load_op = load_op;
    renderpass->attachments[idx].store_op = store_op;
    renderpass->attachment_count = MAX(renderpass->attachment_count, idx + 1);
}



void vkl_renderpass_subpass_attachment(
    VklRenderpass* renderpass, uint32_t subpass_idx, uint32_t attachment_idx)
{
    ASSERT(renderpass != NULL);
    renderpass->subpasses[subpass_idx]
        .attachments[renderpass->subpasses[subpass_idx].attachment_count++] = attachment_idx;
    renderpass->subpass_count = MAX(renderpass->subpass_count, subpass_idx + 1);
}



void vkl_renderpass_subpass_dependency(
    VklRenderpass* renderpass, uint32_t dependency_idx, //
    uint32_t src_subpass, uint32_t dst_subpass)
{
    ASSERT(renderpass != NULL);
    renderpass->dependencies[dependency_idx].src_subpass = src_subpass;
    renderpass->dependencies[dependency_idx].dst_subpass = dst_subpass;
    renderpass->dependency_count = MAX(renderpass->dependency_count, dependency_idx + 1);
}



void vkl_renderpass_subpass_dependency_access(
    VklRenderpass* renderpass, uint32_t dependency_idx, //
    VkAccessFlags src_access, VkAccessFlags dst_access)
{
    ASSERT(renderpass != NULL);
    renderpass->dependencies[dependency_idx].src_access = src_access;
    renderpass->dependencies[dependency_idx].dst_access = dst_access;
}



void vkl_renderpass_subpass_dependency_stage(
    VklRenderpass* renderpass, uint32_t dependency_idx, //
    VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage)
{
    ASSERT(renderpass != NULL);
    renderpass->dependencies[dependency_idx].src_stage = src_stage;
    renderpass->dependencies[dependency_idx].dst_stage = dst_stage;
}



void vkl_renderpass_create(VklRenderpass* renderpass)
{
    ASSERT(renderpass != NULL);

    ASSERT(renderpass->gpu != NULL);
    ASSERT(renderpass->gpu->device != VK_NULL_HANDLE);
    log_trace("starting creation of renderpass...");

    // Attachments.
    VkAttachmentDescription attachments[VKL_MAX_ATTACHMENTS_PER_RENDERPASS] = {0};
    VkAttachmentReference attachment_refs[VKL_MAX_ATTACHMENTS_PER_RENDERPASS] = {0};
    for (uint32_t i = 0; i < renderpass->attachment_count; i++)
    {
        attachments[i] = create_attachment(
            renderpass->attachments[i].format,                                           //
            renderpass->attachments[i].load_op, renderpass->attachments[i].store_op,     //
            renderpass->attachments[i].src_layout, renderpass->attachments[i].dst_layout //
        );
        attachment_refs[i] = create_attachment_ref(i, renderpass->attachments[i].ref_layout);
    }

    // Subpasses.
    VkSubpassDescription subpasses[VKL_MAX_SUBPASSES_PER_RENDERPASS] = {0};
    VkAttachmentReference attachment_refs_matrix[VKL_MAX_ATTACHMENTS_PER_RENDERPASS]
                                                [VKL_MAX_ATTACHMENTS_PER_RENDERPASS] = {0};
    uint32_t attachment = 0;
    uint32_t k = 0;
    // bool has_depth_attachment = false;
    for (uint32_t i = 0; i < renderpass->subpass_count; i++)
    {
        k = 0;
        subpasses[i].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        for (uint32_t j = 0; j < renderpass->subpasses[i].attachment_count; j++)
        {
            attachment = renderpass->subpasses[i].attachments[j];
            ASSERT(attachment < renderpass->attachment_count);
            if (renderpass->attachments[attachment].type == VKL_RENDERPASS_ATTACHMENT_DEPTH)
            {
                subpasses[i].pDepthStencilAttachment = &attachment_refs[j];
            }
            else
            {
                attachment_refs_matrix[i][k++] =
                    create_attachment_ref(i, renderpass->attachments[i].ref_layout);
            }
        }
        subpasses[i].colorAttachmentCount = k;
        subpasses[i].pColorAttachments = attachment_refs_matrix[i];
    }

    // Dependencies.
    VkSubpassDependency dependencies[VKL_MAX_DEPENDENCIES_PER_RENDERPASS] = {0};
    for (uint32_t i = 0; i < renderpass->dependency_count; i++)
    {
        dependencies[i].srcSubpass = renderpass->dependencies[i].src_subpass;
        dependencies[i].srcAccessMask = renderpass->dependencies[i].src_access;
        dependencies[i].srcStageMask = renderpass->dependencies[i].src_stage;

        dependencies[i].dstSubpass = renderpass->dependencies[i].dst_subpass;
        dependencies[i].dstAccessMask = renderpass->dependencies[i].dst_access;
        dependencies[i].dstStageMask = renderpass->dependencies[i].dst_stage;
    }

    // Create renderpass.
    VkRenderPassCreateInfo render_pass_info = {0};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

    render_pass_info.attachmentCount = renderpass->attachment_count;
    render_pass_info.pAttachments = attachments;

    render_pass_info.subpassCount = renderpass->subpass_count;
    render_pass_info.pSubpasses = subpasses;

    render_pass_info.dependencyCount = renderpass->dependency_count;
    render_pass_info.pDependencies = dependencies;

    VK_CHECK_RESULT(vkCreateRenderPass(
        renderpass->gpu->device, &render_pass_info, NULL, &renderpass->renderpass));

    log_trace("renderpass created");
    obj_created(&renderpass->obj);
}



void vkl_renderpass_destroy(VklRenderpass* renderpass)
{
    ASSERT(renderpass != NULL);
    if (!is_obj_created(&renderpass->obj))
    {
        log_trace("skip destruction of already-destroyed renderpass");
        return;
    }

    log_trace("destroy renderpass");
    if (renderpass->renderpass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(renderpass->gpu->device, renderpass->renderpass, NULL);
        renderpass->renderpass = VK_NULL_HANDLE;
    }

    obj_destroyed(&renderpass->obj);
}



/*************************************************************************************************/
/*  Framebuffers                                                                                 */
/*************************************************************************************************/

VklFramebuffers vkl_framebuffers(VklGpu* gpu)
{
    ASSERT(gpu != NULL);
    ASSERT(is_obj_created(&gpu->obj));

    VklFramebuffers framebuffers = {0};
    framebuffers.gpu = gpu;

    return framebuffers;
}



void vkl_framebuffers_attachment(
    VklFramebuffers* framebuffers, uint32_t attachment_idx, VklImages* images)
{
    ASSERT(framebuffers != NULL);

    ASSERT(images != NULL);
    ASSERT(images->count > 0);
    ASSERT(images->width > 0);
    ASSERT(images->height > 0);

    ASSERT(attachment_idx < VKL_MAX_ATTACHMENTS_PER_RENDERPASS);
    framebuffers->attachment_count = MAX(framebuffers->attachment_count, attachment_idx + 1);
    framebuffers->attachments[attachment_idx] = images;

    framebuffers->framebuffer_count = MAX(framebuffers->framebuffer_count, images->count);
}



static void _framebuffers_create(VklFramebuffers* framebuffers)
{
    VklRenderpass* renderpass = framebuffers->renderpass;
    ASSERT(renderpass != NULL);

    // The actual framebuffer size in pixels is determined by the first attachment (color images)
    // as these images are created by the swapchain.
    ASSERT(framebuffers->attachment_count > 0);
    uint32_t width = framebuffers->attachments[0]->width;
    uint32_t height = framebuffers->attachments[0]->height;
    log_trace(
        "create %d framebuffer(s) with size %dx%d", framebuffers->framebuffer_count, width,
        height);

    // Loop first over the framebuffers (swapchain images).
    for (uint32_t i = 0; i < framebuffers->framebuffer_count; i++)
    {
        VklImages* images = NULL;
        VkImageView attachments[VKL_MAX_ATTACHMENTS_PER_RENDERPASS] = {0};

        // Loop over the attachments.
        for (uint32_t j = 0; j < framebuffers->attachment_count; j++)
        {
            images = framebuffers->attachments[j];
            attachments[j] = images->image_views[MIN(i, images->count - 1)];
        }
        ASSERT(images != NULL);

        VkFramebufferCreateInfo info = {0};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = renderpass->renderpass;
        info.attachmentCount = renderpass->attachment_count;
        info.pAttachments = attachments;
        info.width = width;
        info.height = height;
        info.layers = 1;

        VK_CHECK_RESULT(vkCreateFramebuffer(
            framebuffers->gpu->device, &info, NULL, &framebuffers->framebuffers[i]));
    }
}



static void _framebuffers_destroy(VklFramebuffers* framebuffers)
{
    for (uint32_t i = 0; i < framebuffers->framebuffer_count; i++)
    {
        if (framebuffers->framebuffers[i] != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(framebuffers->gpu->device, framebuffers->framebuffers[i], NULL);
            framebuffers->framebuffers[i] = VK_NULL_HANDLE;
        }
    }
}



void vkl_framebuffers_create(VklFramebuffers* framebuffers, VklRenderpass* renderpass)
{
    ASSERT(framebuffers != NULL);

    ASSERT(framebuffers->gpu != NULL);
    ASSERT(framebuffers->gpu->device != VK_NULL_HANDLE);

    ASSERT(renderpass != NULL);
    ASSERT(is_obj_created(&renderpass->obj));

    framebuffers->renderpass = renderpass;

    ASSERT(framebuffers->attachment_count > 0);
    ASSERT(framebuffers->framebuffer_count > 0);

    ASSERT(renderpass->attachment_count > 0);
    ASSERT(renderpass->attachment_count == framebuffers->attachment_count);

    // Create the framebuffers.
    log_trace("starting creation of %d framebuffer(s)", framebuffers->framebuffer_count);
    _framebuffers_create(framebuffers);
    log_trace("framebuffers created");
    obj_created(&framebuffers->obj);
}



void vkl_framebuffers_destroy(VklFramebuffers* framebuffers)
{
    ASSERT(framebuffers != NULL);
    if (!is_obj_created(&framebuffers->obj))
    {
        log_trace("skip destruction of already-destroyed framebuffers");
        return;
    }
    log_trace("destroying %d framebuffers", framebuffers->framebuffer_count);
    _framebuffers_destroy(framebuffers);
    obj_destroyed(&framebuffers->obj);
}



/*************************************************************************************************/
/*  Submit                                                                                       */
/*************************************************************************************************/

VklSubmit vkl_submit(VklGpu* gpu)
{
    ASSERT(gpu != NULL);
    ASSERT(is_obj_created(&gpu->obj));

    // INSTANCE_NEW(VklSubmit, submit, gpu->submits, gpu->submit_count)

    VklSubmit submit = {0};
    submit.gpu = gpu;

    return submit;
}



void vkl_submit_commands(VklSubmit* submit, VklCommands* commands)
{
    ASSERT(submit != NULL);
    ASSERT(commands != NULL);

    uint32_t n = submit->commands_count;
    ASSERT(n < VKL_MAX_COMMANDS_PER_SUBMIT);
    // log_trace("adding commands #%d to submit", n);
    submit->commands[n] = commands;
    submit->commands_count++;
}



void vkl_submit_wait_semaphores(
    VklSubmit* submit, VkPipelineStageFlags stage, VklSemaphores* semaphores, uint32_t idx)
{
    ASSERT(submit != NULL);
    ASSERT(semaphores != NULL);

    ASSERT(idx < semaphores->count);
    ASSERT(idx < VKL_MAX_SEMAPHORES_PER_SET);
    uint32_t n = submit->wait_semaphores_count;
    ASSERT(n < VKL_MAX_SEMAPHORES_PER_SUBMIT);

    ASSERT(semaphores->semaphores[idx] != VK_NULL_HANDLE);

    submit->wait_semaphores[n] = semaphores;
    submit->wait_stages[n] = stage;
    submit->wait_semaphores_idx[n] = idx;

    submit->wait_semaphores_count++;
}



void vkl_submit_signal_semaphores(VklSubmit* submit, VklSemaphores* semaphores, uint32_t idx)
{
    ASSERT(submit != NULL);

    ASSERT(idx < VKL_MAX_SEMAPHORES_PER_SET);
    uint32_t n = submit->signal_semaphores_count;
    ASSERT(n < VKL_MAX_SEMAPHORES_PER_SUBMIT);

    submit->signal_semaphores[n] = semaphores;
    submit->signal_semaphores_idx[n] = idx;

    submit->signal_semaphores_count++;
}



void vkl_submit_send(VklSubmit* submit, uint32_t cmd_idx, VklFences* fence, uint32_t fence_idx)
{
    ASSERT(submit != NULL);
    // log_trace("starting command buffer submission...");

    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[VKL_MAX_SEMAPHORES_PER_SUBMIT] = {0};
    for (uint32_t i = 0; i < submit->wait_semaphores_count; i++)
    {
        wait_semaphores[i] =
            submit->wait_semaphores[i]->semaphores[submit->wait_semaphores_idx[i]];
        // log_trace("wait for semaphore %d", wait_semaphores[i]);
        ASSERT(submit->wait_stages[i] != VK_NULL_HANDLE);
    }

    VkSemaphore signal_semaphores[VKL_MAX_SEMAPHORES_PER_SUBMIT] = {0};
    for (uint32_t i = 0; i < submit->signal_semaphores_count; i++)
    {
        signal_semaphores[i] =
            submit->signal_semaphores[i]->semaphores[submit->signal_semaphores_idx[i]];
        // log_trace("signal semaphore %d", signal_semaphores[i]);
    }

    VkCommandBuffer cmd_bufs[VKL_MAX_COMMANDS_PER_SUBMIT] = {0};

    // Find the queue to submit to.
    ASSERT(submit->commands_count > 0);
    uint32_t queue_idx = submit->commands[0]->queue_idx;
    for (uint32_t i = 0; i < submit->commands_count; i++)
    {
        // All commands should belong to the same queue.
        if (submit->commands[i]->queue_idx == queue_idx)
            cmd_bufs[i] = submit->commands[i]->cmds[cmd_idx];
        else
            log_error("all submitted commands should belong to the same queue #%d", queue_idx);
    }

    submit_info.commandBufferCount = submit->commands_count;
    submit_info.pCommandBuffers = cmd_bufs;

    submit_info.waitSemaphoreCount = submit->wait_semaphores_count;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = submit->wait_stages;

    submit_info.signalSemaphoreCount = submit->signal_semaphores_count;
    submit_info.pSignalSemaphores = signal_semaphores;

    VkFence vfence = fence == NULL ? 0 : fence->fences[fence_idx];

    if (vfence != VK_NULL_HANDLE)
    {
        vkl_fences_wait(fence, fence_idx);
        vkl_fences_reset(fence, fence_idx);
    }
    // log_trace("submit queue and signal fence %d", vfence);
    VK_CHECK_RESULT(vkQueueSubmit(submit->gpu->queues.queues[queue_idx], 1, &submit_info, vfence));

    // log_trace("submit done");
}



void vkl_submit_reset(VklSubmit* submit)
{
    ASSERT(submit != NULL);
    // log_trace("reset Submit instance");
    submit->commands_count = 0;
    submit->wait_semaphores_count = 0;
    submit->signal_semaphores_count = 0;
}



/*************************************************************************************************/
/*  Command buffer filling                                                                       */
/*************************************************************************************************/

void vkl_cmd_begin_renderpass(
    VklCommands* cmds, uint32_t idx, VklRenderpass* renderpass, VklFramebuffers* framebuffers)
{
    ASSERT(renderpass != NULL);
    ASSERT(framebuffers != NULL);

    ASSERT(is_obj_created(&renderpass->obj));
    ASSERT(is_obj_created(&framebuffers->obj));
    ASSERT(renderpass->renderpass != VK_NULL_HANDLE);

    // Find the framebuffer size.
    ASSERT(framebuffers->attachment_count > 0);
    uint32_t width = framebuffers->attachments[0]->width;
    uint32_t height = framebuffers->attachments[0]->height;
    log_trace("begin renderpass with size %dx%d", width, height);

    CMD_START_CLIP(cmds->count)
    ASSERT(framebuffers->framebuffers[iclip] != VK_NULL_HANDLE);
    begin_render_pass(
        renderpass->renderpass, cb, framebuffers->framebuffers[iclip], //
        width, height, renderpass->clear_count, renderpass->clear_values);
    CMD_END
}



void vkl_cmd_end_renderpass(VklCommands* cmds, uint32_t idx)
{
    CMD_START
    vkCmdEndRenderPass(cb);
    CMD_END
}



void vkl_cmd_compute(VklCommands* cmds, uint32_t idx, VklCompute* compute, uvec3 size)
{
    ASSERT(compute->bindings != NULL);
    ASSERT(compute->bindings->dsets != NULL);
    ASSERT(compute->pipeline != VK_NULL_HANDLE);
    ASSERT(compute->slots.pipeline_layout != VK_NULL_HANDLE);

    CMD_START
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, compute->pipeline);
    vkCmdBindDescriptorSets(
        cb, VK_PIPELINE_BIND_POINT_COMPUTE, compute->slots.pipeline_layout, 0, 1,
        compute->bindings->dsets, 0, 0);
    vkCmdDispatch(cb, size[0], size[1], size[2]);
    CMD_END
}



void vkl_cmd_barrier(VklCommands* cmds, uint32_t idx, VklBarrier* barrier)
{
    ASSERT(barrier != NULL);
    VklQueues* q = &cmds->gpu->queues;
    CMD_START

    // Buffer barriers
    VkBufferMemoryBarrier buffer_barriers[VKL_MAX_BARRIERS_PER_SET] = {0};
    VkBufferMemoryBarrier* buffer_barrier = NULL;
    VklBarrierBuffer* buffer_info = NULL;

    for (uint32_t j = 0; j < barrier->buffer_barrier_count; j++)
    {
        buffer_barrier = &buffer_barriers[j];
        buffer_info = &barrier->buffer_barriers[j];

        buffer_barrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        buffer_barrier->buffer = buffer_info->buffer_regions.buffer->buffer;
        buffer_barrier->size = buffer_info->buffer_regions.size;
        ASSERT(i < buffer_info->buffer_regions.count);
        buffer_barrier->offset = buffer_info->buffer_regions.offsets[i];

        buffer_barrier->srcAccessMask = buffer_info->src_access;
        buffer_barrier->dstAccessMask = buffer_info->dst_access;

        if (buffer_info->queue_transfer)
        {
            buffer_barrier->srcQueueFamilyIndex = q->queue_families[buffer_info->src_queue];
            buffer_barrier->dstQueueFamilyIndex = q->queue_families[buffer_info->dst_queue];
        }
        else
        {
            buffer_barrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            buffer_barrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        }
    }

    // Image barriers
    VkImageMemoryBarrier image_barriers[VKL_MAX_BARRIERS_PER_SET] = {0};
    VkImageMemoryBarrier* image_barrier = NULL;
    VklBarrierImage* image_info = NULL;

    for (uint32_t j = 0; j < barrier->image_barrier_count; j++)
    {
        image_barrier = &image_barriers[j];
        image_info = &barrier->image_barriers[j];

        image_barrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        ASSERT(i < image_info->images->count);
        image_barrier->image = image_info->images->images[i];
        image_barrier->oldLayout = image_info->src_layout;
        image_barrier->newLayout = image_info->dst_layout;

        image_barrier->srcAccessMask = image_info->src_access;
        image_barrier->dstAccessMask = image_info->dst_access;

        if (image_info->queue_transfer)
        {
            image_barrier->srcQueueFamilyIndex = q->queue_families[image_info->src_queue];
            image_barrier->dstQueueFamilyIndex = q->queue_families[image_info->dst_queue];
        }
        else
        {
            image_barrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_barrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        }
        image_barrier->subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_barrier->subresourceRange.baseMipLevel = 0;
        image_barrier->subresourceRange.levelCount = 1;
        image_barrier->subresourceRange.baseArrayLayer = 0;
        image_barrier->subresourceRange.layerCount = 1;
    }

    vkCmdPipelineBarrier(
        cb, barrier->src_stage, barrier->dst_stage, 0, 0, NULL, //
        barrier->buffer_barrier_count, buffer_barriers,         //
        barrier->image_barrier_count, image_barriers);          //

    CMD_END
}



void vkl_cmd_copy_buffer_to_image(
    VklCommands* cmds, uint32_t idx, VklBuffer* buffer, VklImages* images)
{
    CMD_START_CLIP(images->count)

    VkBufferImageCopy region = {0};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset.x = 0;
    region.imageOffset.y = 0;
    region.imageOffset.z = 0;

    region.imageExtent.width = images->width;
    region.imageExtent.height = images->height;
    region.imageExtent.depth = images->depth;

    vkCmdCopyBufferToImage(
        cb, buffer->buffer, images->images[iclip], //
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    CMD_END
}



void vkl_cmd_copy_image_to_buffer(
    VklCommands* cmds, uint32_t idx, VklImages* images, VklBuffer* buffer)
{
    CMD_START_CLIP(images->count)

    VkBufferImageCopy region = {0};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset.x = 0;
    region.imageOffset.y = 0;
    region.imageOffset.z = 0;

    region.imageExtent.width = images->width;
    region.imageExtent.height = images->height;
    region.imageExtent.depth = images->depth;

    vkCmdCopyImageToBuffer(
        cb, images->images[iclip], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, //
        buffer->buffer, 1, &region);

    CMD_END
}



void vkl_cmd_copy_image(VklCommands* cmds, uint32_t idx, VklImages* src_img, VklImages* dst_img)
{
    ASSERT(src_img != NULL);
    ASSERT(dst_img != NULL);

    ASSERT(src_img->width = dst_img->width);
    ASSERT(src_img->height = dst_img->height);

    CMD_START_CLIP(src_img->count)

    uint32_t i0 = 0;
    uint32_t i1 = 0;

    i0 = CLIP(i, 0, src_img->count - 1);
    i1 = CLIP(i, 0, dst_img->count - 1);

    ASSERT(src_img->images[i0] != VK_NULL_HANDLE);
    ASSERT(dst_img->images[i1] != VK_NULL_HANDLE);

    VkImageCopy imageCopyRegion = {0};
    imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.srcSubresource.layerCount = 1;
    imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.dstSubresource.layerCount = 1;
    imageCopyRegion.extent.width = src_img->width;
    imageCopyRegion.extent.height = src_img->height;
    imageCopyRegion.extent.depth = 1;
    vkCmdCopyImage(
        cb,                                                        //
        src_img->images[i0], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, //
        dst_img->images[i1], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, //
        1, &imageCopyRegion);
    CMD_END
}



void vkl_cmd_viewport(VklCommands* cmds, uint32_t idx, VkViewport viewport)
{
    CMD_START
    vkCmdSetViewport(cb, 0, 1, &viewport);
    VkRect2D scissor = {
        {viewport.x, viewport.y}, {(uint32_t)viewport.width, (uint32_t)viewport.height}};
    vkCmdSetScissor(cb, 0, 1, &scissor);
    CMD_END
}



void vkl_cmd_bind_graphics(
    VklCommands* cmds, uint32_t idx, VklGraphics* graphics, //
    VklBindings* bindings, uint32_t dynamic_idx)
{
    ASSERT(graphics != NULL);
    VklSlots* slots = &graphics->slots;
    ASSERT(slots != NULL);
    ASSERT(bindings != NULL);

    // Count the number of dynamic uniforms.
    uint32_t dyn_count = 0;
    uint32_t dyn_offsets[VKL_MAX_BINDINGS_SIZE] = {0};
    ASSERT(slots->slot_count <= VKL_MAX_BINDINGS_SIZE);
    for (uint32_t i = 0; i < slots->slot_count; i++)
    {
        if (slots->types[i] == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
        {
            ASSERT(bindings->buffer_regions[i].aligned_size > 0);
            dyn_offsets[dyn_count++] = dynamic_idx * bindings->buffer_regions[i].aligned_size;
        }
    }

    CMD_START_CLIP(bindings->dset_count)
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics->pipeline);
    vkCmdBindDescriptorSets(
        cb, VK_PIPELINE_BIND_POINT_GRAPHICS, slots->pipeline_layout, //
        0, 1, &bindings->dsets[iclip], dyn_count, dyn_offsets);
    CMD_END
}



void vkl_cmd_bind_vertex_buffer(
    VklCommands* cmds, uint32_t idx, VklBufferRegions* buffer_regions, VkDeviceSize offset)
{
    CMD_START_CLIP(buffer_regions->count)
    VkDeviceSize offsets[] = {buffer_regions->offsets[iclip] + offset};
    vkCmdBindVertexBuffers(cb, 0, 1, &buffer_regions->buffer->buffer, offsets);
    CMD_END
}



void vkl_cmd_bind_index_buffer(
    VklCommands* cmds, uint32_t idx, VklBufferRegions* buffer_regions, VkDeviceSize offset)
{
    CMD_START_CLIP(buffer_regions->count)
    vkCmdBindIndexBuffer(
        cb, buffer_regions->buffer->buffer, buffer_regions->offsets[iclip] + offset,
        VK_INDEX_TYPE_UINT32);
    CMD_END
}



void vkl_cmd_draw(VklCommands* cmds, uint32_t idx, uint32_t first_vertex, uint32_t vertex_count)
{
    CMD_START
    vkCmdDraw(cb, vertex_count, 1, first_vertex, 0);
    CMD_END
}



void vkl_cmd_draw_indexed(
    VklCommands* cmds, uint32_t idx, uint32_t first_index, uint32_t vertex_offset,
    uint32_t index_count)
{
    CMD_START
    vkCmdDrawIndexed(cb, index_count, 1, first_index, (int32_t)vertex_offset, 0);
    CMD_END
}



void vkl_cmd_draw_indirect(VklCommands* cmds, uint32_t idx, VklBufferRegions* indirect)
{
    CMD_START_CLIP(indirect->count)
    vkCmdDrawIndirect(cb, indirect->buffer->buffer, indirect->offsets[iclip], 1, 0);
    CMD_END
}



void vkl_cmd_draw_indexed_indirect(VklCommands* cmds, uint32_t idx, VklBufferRegions* indirect)
{
    CMD_START_CLIP(indirect->count)
    vkCmdDrawIndexedIndirect(cb, indirect->buffer->buffer, indirect->offsets[iclip], 1, 0);
    CMD_END
}



void vkl_cmd_copy_buffer(
    VklCommands* cmds, uint32_t idx,             //
    VklBuffer* src_buf, VkDeviceSize src_offset, //
    VklBuffer* dst_buf, VkDeviceSize dst_offset, //
    VkDeviceSize size)
{
    ASSERT(cmds != NULL);
    ASSERT(src_buf != NULL);
    ASSERT(dst_buf != NULL);
    ASSERT(size > 0);
    ASSERT(src_offset + size <= src_buf->size);
    ASSERT(dst_offset + size <= dst_buf->size);

    VkBufferCopy copy_region = {0};
    copy_region.size = size;

    VkCommandBuffer cb = cmds->cmds[idx];
    copy_region.srcOffset = src_offset;
    copy_region.dstOffset = dst_offset;
    vkCmdCopyBuffer(cb, src_buf->buffer, dst_buf->buffer, 1, &copy_region);
}



void vkl_cmd_push(
    VklCommands* cmds, uint32_t idx, VklSlots* slots, VkShaderStageFlagBits shaders, //
    VkDeviceSize offset, VkDeviceSize size, const void* data)
{
    CMD_START
    vkCmdPushConstants(cb, slots->pipeline_layout, shaders, offset, size, data);
    CMD_END
}
