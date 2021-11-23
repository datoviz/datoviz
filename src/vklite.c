/*************************************************************************************************/
/*  Vklite                                                                                       */
/*************************************************************************************************/

#include "vklite.h"
#include "common.h"
#include "vklite_utils.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define CMD_START                                                                                 \
    ASSERT(cmds != NULL);                                                                         \
    VkCommandBuffer cb = {0};                                                                     \
    uint32_t i = idx;                                                                             \
    cb = cmds->cmds[i];


#define CMD_START_CLIP(cnt)                                                                       \
    ASSERT(cmds != NULL);                                                                         \
    ASSERT(cnt > 0);                                                                              \
    if (!((cnt) == 1 || (cnt) == cmds->count))                                                    \
        log_debug("mismatch between image count and cmd buf count");                              \
    VkCommandBuffer cb = {0};                                                                     \
    uint32_t iclip = 0;                                                                           \
    uint32_t i = idx;                                                                             \
    iclip = (cnt) == 1 ? 0 : (MIN(i, (cnt)-1));                                                   \
    ASSERT(iclip < (cnt));                                                                        \
    cb = cmds->cmds[i];


#define CMD_END //


#define COPY_STR(env, dst)                                                                        \
    s = getenv(env);                                                                              \
    if (s != NULL && strlen(s) > 0)                                                               \
    {                                                                                             \
        ASSERT(strlen(s) < DVZ_PATH_MAX_LEN);                                                     \
        strncpy(dst, s, DVZ_PATH_MAX_LEN);                                                        \
    }



/*************************************************************************************************/
/*  Backend-specific initialization                                                              */
/*************************************************************************************************/

static void _glfw_error(int error_code, const char* description)
{
    log_error("glfw error code #%d: %s", error_code, description);
}



static void backend_init(DvzBackend backend)
{
    switch (backend)
    {
    case DVZ_BACKEND_GLFW:

        log_debug("initialize glfw");
        glfwSetErrorCallback(_glfw_error);
        if (!glfwInit())
        {
            exit(1);
        }

        break;
    default:
        break;
    }
}



static void backend_terminate(DvzBackend backend)
{
    switch (backend)
    {
    case DVZ_BACKEND_GLFW:
        log_debug("terminate glfw");
        glfwTerminate();
        break;
    default:
        break;
    }
}



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
/*  GPU                                                                                          */
/*************************************************************************************************/

DvzGpu* dvz_gpu(DvzHost* host, uint32_t idx)
{
    if (idx >= host->gpus.count)
    {
        log_error("GPU index %d higher than number of GPUs %d", idx, host->gpus.count);
        idx = 0;
    }
    DvzGpu* gpu = host->gpus.items[idx];
    return gpu;
}



DvzGpu* dvz_gpu_best(DvzHost* host)
{
    ASSERT(host != NULL);
    log_trace("start looking for the best GPU on the system among %d GPU(s)", host->gpus.count);

    DvzGpu* gpu = NULL;
    DvzGpu* best_gpu = NULL;
    DvzGpu* best_gpu_discrete = NULL;
    VkDeviceSize best_vram = 0;
    VkDeviceSize best_vram_discrete = 0;

    ASSERT(host->gpus.count > 0);
    for (uint32_t i = 0; i < host->gpus.count; i++)
    {
        gpu = dvz_gpu(host, i);
        ASSERT(gpu != NULL);
        ASSERT(gpu->vram > 0);

        // Find the discrete GPU with the most VRAM.
        if (gpu->device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            if (gpu->vram > best_vram_discrete)
            {
                log_trace(
                    "best discrete GPU so far: %s with %s VRAM", gpu->name,
                    pretty_size(gpu->vram));
                best_vram_discrete = gpu->vram;
                best_gpu_discrete = gpu;
            }
        }

        // Find the GPU with the most VRAM.
        if (gpu->vram > best_vram)
        {
            log_trace("best GPU so far: %s with %s VRAM", gpu->name, pretty_size(gpu->vram));
            best_vram = gpu->vram;
            best_gpu = gpu;
        }
    }

    best_gpu = best_gpu_discrete != NULL ? best_gpu_discrete : best_gpu;
    ASSERT(best_gpu != NULL);
    log_trace("best GPU: %s with %s VRAM", best_gpu->name, pretty_size(best_gpu->vram));
    return best_gpu;
}



void dvz_gpu_request_features(DvzGpu* gpu, VkPhysicalDeviceFeatures requested_features)
{
    ASSERT(gpu != NULL);
    gpu->requested_features = requested_features;
}



void dvz_gpu_queue(DvzGpu* gpu, uint32_t idx, DvzQueueType type)
{
    ASSERT(gpu != NULL);
    DvzQueues* q = &gpu->queues;
    ASSERT(q != NULL);
    ASSERT(idx < DVZ_MAX_QUEUES);
    q->queue_types[idx] = type;
    ASSERT(idx == q->queue_count);
    q->queue_count++;
}



void dvz_gpu_create(DvzGpu* gpu, VkSurfaceKHR surface)
{
    ASSERT(gpu != NULL);
    ASSERT(gpu->host != NULL);

    if (gpu->queues.queue_count == 0)
    {
        log_error(
            "you must request at least one queue with dvz_gpu_queue() before creating the GPU");
        exit(1);
    }
    log_trace(
        "starting creation of GPU #%d WITH%s surface...", gpu->idx,
        surface != VK_NULL_HANDLE ? "" : "OUT");
    create_device(gpu, surface);

    DvzQueues* q = &gpu->queues;

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

    // Create allocator.
    VmaAllocatorCreateInfo alloc_info = {0};
    alloc_info.vulkanApiVersion = DVZ_VULKAN_API;
    alloc_info.physicalDevice = gpu->physical_device;
    alloc_info.device = gpu->device;
    alloc_info.instance = gpu->host->instance;
    vmaCreateAllocator(&alloc_info, &gpu->allocator);
    ASSERT(gpu->allocator != VK_NULL_HANDLE);

    dvz_obj_created(&gpu->obj);
    log_trace("GPU #%d created", gpu->idx);
}



void dvz_queue_wait(DvzGpu* gpu, uint32_t queue_idx)
{
    ASSERT(gpu != NULL);
    ASSERT(queue_idx < gpu->queues.queue_count);
    // log_trace("waiting for queue #%d", queue_idx);
    vkQueueWaitIdle(gpu->queues.queues[queue_idx]);
}



void dvz_gpu_wait(DvzGpu* gpu)
{
    ASSERT(gpu != NULL);
    log_trace("waiting for device");
    if (gpu->device != VK_NULL_HANDLE)
        vkDeviceWaitIdle(gpu->device);
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



void dvz_gpu_destroy(DvzGpu* gpu)
{
    ASSERT(gpu != NULL);
    log_trace("starting destruction of GPU #%d...", gpu->idx);
    if (!dvz_obj_is_created(&gpu->obj))
    {

        log_trace("skip destruction of GPU as it was not properly created");
        ASSERT(gpu->device == 0);
        return;
    }
    VkDevice device = gpu->device;
    ASSERT(device != VK_NULL_HANDLE);

    // Destroy the context.
    if (gpu->context != NULL)
    {
        // TODO
        // dvz_context_destroy(gpu->context);
        FREE(gpu->context);
        gpu->context = NULL;
    }

    // Destroy the command pools.
    log_trace("GPU destroy %d command pool(s)", gpu->queues.queue_family_count);
    for (uint32_t i = 0; i < gpu->queues.queue_family_count; i++)
    {
        if (gpu->queues.cmd_pools[i] != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(device, gpu->queues.cmd_pools[i], NULL);
            gpu->queues.cmd_pools[i] = VK_NULL_HANDLE;
        }
    }

    // Destroy the descriptor pools.
    if (gpu->dset_pool != VK_NULL_HANDLE)
    {
        log_trace("destroy descriptor pool");
        vkDestroyDescriptorPool(gpu->device, gpu->dset_pool, NULL);
        gpu->dset_pool = VK_NULL_HANDLE;
    }

    // Destroy the allocator.
    ASSERT(gpu->allocator != VK_NULL_HANDLE);
    vmaDestroyAllocator(gpu->allocator);

    // Destroy the device.
    log_trace("destroy device");
    if (gpu->device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(gpu->device, NULL);
        gpu->device = VK_NULL_HANDLE;
    }

    // dvz_obj_destroyed(&gpu->obj);
    dvz_obj_init(&gpu->obj);
    gpu->queues.queue_count = 0;
    log_trace("GPU #%d destroyed", gpu->idx);
}



/*************************************************************************************************/
/*  Window                                                                                       */
/*************************************************************************************************/

DvzWindow* dvz_window(DvzGpu* gpu, uint32_t width, uint32_t height)
{
    // NOTE: an offscreen canvas has NO DvzWindow, so this function should NEVER be called with an
    // offscreen backend, or for an offscreen canvas.

    ASSERT(gpu != NULL);

    DvzHost* host = gpu->host;
    ASSERT(host != NULL);

    DvzWindow* window = dvz_container_alloc(&host->windows);
    ASSERT(window != NULL);

    ASSERT(window->obj.type == DVZ_OBJECT_TYPE_WINDOW);
    ASSERT(window->obj.status == DVZ_OBJECT_STATUS_ALLOC);
    window->gpu = gpu;
    ASSERT(host->backend != DVZ_BACKEND_NONE && host->backend != DVZ_BACKEND_OFFSCREEN);

    window->width = width;
    window->height = height;
    window->close_on_esc = true;

    // Create the window, depending on the backend.
    window->backend_window =
        backend_window(host->instance, host->backend, width, height, window, &window->surface);

    if (window->surface == VK_NULL_HANDLE)
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
    ASSERT(window->gpu != NULL);
    ASSERT(window->gpu->host != NULL);
    backend_window_get_size(
        window->gpu->host->backend, window->backend_window, //
        &window->width, &window->height,                    //
        framebuffer_width, framebuffer_height);
}



void dvz_window_set_size(DvzWindow* window, uint32_t width, uint32_t height)
{
    ASSERT(window != NULL);
    ASSERT(window->gpu != NULL);
    ASSERT(window->gpu->host != NULL);
    backend_window_set_size(window->gpu->host->backend, window->backend_window, width, height);
}



void dvz_window_poll_events(DvzWindow* window)
{
    ASSERT(window != NULL);
    ASSERT(window->gpu != NULL);
    ASSERT(window->gpu->host != NULL);
    backend_poll_events(window->gpu->host->backend, window);
}



void dvz_window_destroy(DvzWindow* window)
{
    if (window == NULL || window->obj.status == DVZ_OBJECT_STATUS_DESTROYED)
    {
        log_trace("skip destruction of already-destroyed window");
        return;
    }
    ASSERT(window != NULL);
    ASSERT(window->gpu != NULL);
    ASSERT(window->gpu->host != NULL);
    backend_window_destroy(
        window->gpu->host->instance, window->gpu->host->backend, //
        window->backend_window, window->surface);
    dvz_obj_destroyed(&window->obj);
}



/*************************************************************************************************/
/*  Swapchain                                                                                    */
/*************************************************************************************************/

static void _swapchain_create(DvzSwapchain* swapchain)
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
    dvz_images_size(swapchain->images, (uvec3){width, height, 1});

    // Get the number of swapchain images.
    vkGetSwapchainImagesKHR(
        swapchain->gpu->device, swapchain->swapchain, &swapchain->img_count, NULL);
    log_trace("get %d swapchain images", swapchain->img_count);
    vkGetSwapchainImagesKHR(
        swapchain->gpu->device, swapchain->swapchain, &swapchain->img_count,
        swapchain->images->images);

    dvz_images_layout(swapchain->images, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    // Create the swap chain image views (will skip the image creation as they are given by the
    // swapchain directly).
    dvz_images_create(swapchain->images);
}



static void _swapchain_destroy(DvzSwapchain* swapchain)
{
    if (swapchain->images != NULL)
        dvz_images_destroy(swapchain->images);
    if (swapchain->swapchain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(swapchain->gpu->device, swapchain->swapchain, NULL);
}



DvzSwapchain dvz_swapchain(DvzGpu* gpu, DvzWindow* window, uint32_t min_img_count)
{
    ASSERT(gpu != NULL);
    ASSERT(dvz_obj_is_created(&gpu->obj));

    DvzSwapchain swapchain = {0};

    swapchain.gpu = gpu;
    swapchain.window = window;
    swapchain.img_count = min_img_count;
    return swapchain;
}



void dvz_swapchain_format(DvzSwapchain* swapchain, VkFormat format)
{
    ASSERT(swapchain != NULL);
    swapchain->format = format;
}



void dvz_swapchain_present_mode(DvzSwapchain* swapchain, VkPresentModeKHR present_mode)
{
    ASSERT(swapchain != NULL);
    ASSERT(swapchain->gpu != NULL);
    ASSERT(dvz_obj_is_created(&swapchain->gpu->obj));

    swapchain->present_mode = VK_PRESENT_MODE_FIFO_KHR; // default present mode

    for (uint32_t i = 0; i < swapchain->gpu->present_mode_count; i++)
    {
        if (swapchain->gpu->present_modes[i] == present_mode)
        {
            swapchain->present_mode = present_mode;
            return;
        }
    }
    log_warn("unsupported swapchain present mode VkPresentModeKHR #%02d", present_mode);
}



void dvz_swapchain_requested_size(DvzSwapchain* swapchain, uint32_t width, uint32_t height)
{
    ASSERT(swapchain != NULL);
    swapchain->requested_width = width;
    swapchain->requested_height = height;
}



void dvz_swapchain_create(DvzSwapchain* swapchain)
{
    ASSERT(swapchain != NULL);
    ASSERT(swapchain->gpu != NULL);

    log_trace("starting creation of swapchain...");

    // Create the DvzImages struct.
    {
        swapchain->images = calloc(1, sizeof(DvzImages));
        *swapchain->images = dvz_images(swapchain->gpu, VK_IMAGE_TYPE_2D, swapchain->img_count);
        swapchain->images->is_swapchain = true;
        dvz_images_format(swapchain->images, swapchain->format);
    }

    // Create swapchain
    _swapchain_create(swapchain);

    dvz_obj_created(&swapchain->images->obj);
    dvz_obj_created(&swapchain->obj);
    log_trace("swapchain created");
}



void dvz_swapchain_recreate(DvzSwapchain* swapchain)
{
    ASSERT(swapchain != NULL);
    _swapchain_destroy(swapchain);
    _swapchain_create(swapchain);

    dvz_obj_created(&swapchain->images->obj);
    dvz_obj_created(&swapchain->obj);
}



void dvz_swapchain_acquire(
    DvzSwapchain* swapchain, DvzSemaphores* semaphores, uint32_t semaphore_idx, DvzFences* fences,
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
        swapchain->obj.status = DVZ_OBJECT_STATUS_NEED_RECREATE;
        break;
    case VK_SUBOPTIMAL_KHR:
        log_warn("suboptimal frame, recreate swapchain");
        swapchain->obj.status = DVZ_OBJECT_STATUS_NEED_RECREATE;
        break;
    default:
        log_error("failed acquiring the swapchain image");
        // TODO: is that correct? destroying the object if we failed acquiring the swapchain image?
        swapchain->obj.status = DVZ_OBJECT_STATUS_NEED_DESTROY;
        break;
    }
}



void dvz_swapchain_present(
    DvzSwapchain* swapchain, uint32_t queue_idx, DvzSemaphores* semaphores, uint32_t semaphore_idx)
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
        swapchain->obj.status = DVZ_OBJECT_STATUS_NEED_RECREATE;
        break;
    default:
        log_error("failed presenting the swapchain image");
        break;
    }
}



void dvz_swapchain_destroy(DvzSwapchain* swapchain)
{
    ASSERT(swapchain != NULL);
    if (!dvz_obj_is_created(&swapchain->obj))
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

    swapchain->swapchain = VK_NULL_HANDLE;

    dvz_obj_destroyed(&swapchain->obj);
    log_trace("swapchain destroyed");
}



/*************************************************************************************************/
/*  Commands                                                                                     */
/*************************************************************************************************/

DvzCommands dvz_commands(DvzGpu* gpu, uint32_t queue, uint32_t count)
{
    ASSERT(gpu != NULL);
    ASSERT(dvz_obj_is_created(&gpu->obj));

    ASSERT(count <= DVZ_MAX_COMMAND_BUFFERS_PER_SET);
    ASSERT(queue < gpu->queues.queue_count);
    ASSERT(count > 0);
    uint32_t qf = gpu->queues.queue_families[queue];
    ASSERT(qf < gpu->queues.queue_family_count);
    ASSERT(gpu->queues.cmd_pools[qf] != VK_NULL_HANDLE);
    log_trace("creating commands on queue #%d, queue family #%d", queue, qf);

    DvzCommands commands = {0};
    commands.gpu = gpu;
    commands.queue_idx = queue;
    commands.count = count;
    allocate_command_buffers(gpu->device, gpu->queues.cmd_pools[qf], count, commands.cmds);

    dvz_obj_init(&commands.obj);

    return commands;
}



void dvz_cmd_begin(DvzCommands* cmds, uint32_t idx)
{
    ASSERT(cmds != NULL);
    ASSERT(cmds->count > 0);
    ASSERT(idx != cmds->count);

    // log_trace("begin command buffer");
    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CHECK_RESULT(vkBeginCommandBuffer(cmds->cmds[idx], &begin_info));
}



void dvz_cmd_end(DvzCommands* cmds, uint32_t idx)
{
    ASSERT(cmds != NULL);
    ASSERT(cmds->count > 0);
    ASSERT(idx != cmds->count);

    // log_trace("end command buffer");
    VK_CHECK_RESULT(vkEndCommandBuffer(cmds->cmds[idx]));

    dvz_obj_created(&cmds->obj);
}



void dvz_cmd_reset(DvzCommands* cmds, uint32_t idx)
{
    ASSERT(cmds != NULL);
    ASSERT(cmds->count > 0);
    ASSERT(idx != cmds->count);

    log_trace("reset command buffer #%d", idx);
    ASSERT(cmds->cmds[idx] != VK_NULL_HANDLE);
    VK_CHECK_RESULT(vkResetCommandBuffer(cmds->cmds[idx], 0));

    // NOTE: when resetting, we mark the object as not created because it is no longer filled with
    // commands.
    dvz_obj_init(&cmds->obj);
}



void dvz_cmd_free(DvzCommands* cmds)
{
    ASSERT(cmds != NULL);
    ASSERT(cmds->count > 0);
    ASSERT(cmds->gpu != NULL);
    ASSERT(cmds->gpu->device != VK_NULL_HANDLE);

    log_trace("free %d command buffer(s)", cmds->count);
    vkFreeCommandBuffers(
        cmds->gpu->device, cmds->gpu->queues.cmd_pools[cmds->queue_idx], //
        cmds->count, cmds->cmds);

    dvz_obj_init(&cmds->obj);
}



void dvz_cmd_submit_sync(DvzCommands* cmds, uint32_t idx)
{
    log_debug("[SLOW] submit %d command buffer(s)", cmds->count);

    DvzQueues* q = &cmds->gpu->queues;
    VkQueue queue = q->queues[cmds->queue_idx];

    vkQueueWaitIdle(queue);
    VkSubmitInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.commandBufferCount = cmds->count;
    info.pCommandBuffers = cmds->cmds;
    vkQueueSubmit(queue, 1, &info, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
}



void dvz_commands_destroy(DvzCommands* cmds)
{
    ASSERT(cmds != NULL);
    if (!dvz_obj_is_created(&cmds->obj))
    {
        log_trace("skip destruction of already-destroyed commands");
        return;
    }
    log_trace("destroy commands");
    dvz_obj_destroyed(&cmds->obj);
}



/*************************************************************************************************/
/*  Buffers                                                                                      */
/*************************************************************************************************/

DvzBuffer dvz_buffer(DvzGpu* gpu)
{
    ASSERT(gpu != NULL);
    ASSERT(dvz_obj_is_created(&gpu->obj));

    DvzBuffer buffer = {0};
    dvz_obj_init(&buffer.obj);
    buffer.gpu = gpu;

    // Default values.
    // buffer.memory = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    buffer.vma.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    return buffer;
}



void dvz_buffer_size(DvzBuffer* buffer, VkDeviceSize size)
{
    ASSERT(buffer != NULL);
    buffer->size = size;
}



void dvz_buffer_type(DvzBuffer* buffer, DvzBufferType type)
{
    ASSERT(buffer != NULL);
    buffer->type = type;
}



void dvz_buffer_usage(DvzBuffer* buffer, VkBufferUsageFlags usage)
{
    ASSERT(buffer != NULL);
    buffer->usage = usage;
}



void dvz_buffer_vma_usage(DvzBuffer* buffer, VmaMemoryUsage vma_usage)
{
    ASSERT(buffer != NULL);
    buffer->vma.usage = vma_usage;
}



void dvz_buffer_memory(DvzBuffer* buffer, VkMemoryPropertyFlags memory)
{
    ASSERT(buffer != NULL);
    buffer->memory = memory;
}



void dvz_buffer_queue_access(DvzBuffer* buffer, uint32_t queue_idx)
{
    ASSERT(buffer != NULL);
    ASSERT(buffer->gpu != NULL);
    ASSERT(queue_idx < buffer->gpu->queues.queue_count);
    buffer->queues[buffer->queue_count++] = queue_idx;
}



static void _buffer_create(DvzBuffer* buffer)
{
    ASSERT(buffer != NULL);
    DvzGpu* gpu = buffer->gpu;
    ASSERT(gpu != NULL);

    VkBufferCreateInfo buf_info = {0};
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.size = buffer->size;
    buf_info.usage = buffer->usage;

    uint32_t queue_families[DVZ_MAX_QUEUE_FAMILIES];
    make_shared(
        &gpu->queues, buffer->queue_count, buffer->queues, //
        &buf_info.sharingMode, &buf_info.queueFamilyIndexCount, queue_families);
    buf_info.pQueueFamilyIndices = queue_families;

    log_trace(
        "create buffer with size %s, sharing mode %s", pretty_size(buffer->size),
        buf_info.sharingMode == 0 ? "exclusive" : "concurrent");

    // Create the buffer with VMA.
    VmaAllocationCreateInfo alloc_info = {0};
    alloc_info.flags = buffer->vma.flags;
    alloc_info.usage = buffer->vma.usage;
    vmaCreateBuffer(
        gpu->allocator, &buf_info, &alloc_info, &buffer->buffer, //
        &buffer->vma.alloc, &buffer->vma.info);
    ASSERT(buffer->buffer != VK_NULL_HANDLE);

    // Get the memory flags found by VMA and store them in the DvzBuffer instance.
    vmaGetMemoryTypeProperties(gpu->allocator, buffer->vma.info.memoryType, &buffer->memory);
    ASSERT(buffer->memory != 0);

    // Store the alignment requirement in the DvzBuffer.
    VkMemoryRequirements req = {0};
    vkGetBufferMemoryRequirements(gpu->device, buffer->buffer, &req);
    buffer->vma.alignment = req.alignment;
}



static void _buffer_destroy(DvzBuffer* buffer)
{
    ASSERT(buffer != NULL);
    ASSERT(buffer->gpu != NULL);

    // Unmap permanently-mapped buffers before destruction.
    if (buffer->mmap != NULL)
    {
        dvz_buffer_unmap(buffer);
        buffer->mmap = NULL;
    }

    if (buffer->buffer != VK_NULL_HANDLE)
    {
        // vkDestroyBuffer(buffer->gpu->device, buffer->buffer, NULL);
        vmaDestroyBuffer(buffer->gpu->allocator, buffer->buffer, buffer->vma.alloc);
        buffer->buffer = VK_NULL_HANDLE;
    }
    ASSERT(buffer->buffer == VK_NULL_HANDLE);

    // if (buffer->device_memory != VK_NULL_HANDLE)
    // {
    //     vkFreeMemory(buffer->gpu->device, buffer->device_memory, NULL);
    //     buffer->device_memory = VK_NULL_HANDLE;
    // }
    // ASSERT(buffer->device_memory == VK_NULL_HANDLE);
}



static void _buffer_copy(DvzBuffer* buffer0, DvzBuffer* buffer1)
{
    // Copy the parameters of a buffer.
    memcpy(buffer1, buffer0, sizeof(DvzBuffer));
    memcpy(buffer1->queues, buffer0->queues, sizeof(buffer0->queues));
}



void dvz_buffer_create(DvzBuffer* buffer)
{
    ASSERT(buffer != NULL);
    ASSERT(buffer->gpu != NULL);
    ASSERT(buffer->gpu->device != VK_NULL_HANDLE);
    ASSERT(buffer->size > 0);
    ASSERT(buffer->usage != 0);
    ASSERT(buffer->vma.usage != 0);

    log_trace("starting creation of buffer...");
    _buffer_create(buffer);
    ASSERT(buffer->memory != VK_NULL_HANDLE);

    dvz_obj_created(&buffer->obj);
    log_trace("buffer created");
}



void dvz_buffer_resize(DvzBuffer* buffer, VkDeviceSize size)
{
    ASSERT(buffer != NULL);
    DvzGpu* gpu = buffer->gpu;
    if (size <= buffer->size)
    {
        log_trace(
            "skip buffer resizing as the buffer size is large enough:"
            "(requested %s, is %s already)",
            pretty_size(buffer->size), pretty_size(size));
        return;
    }
    log_debug("[SLOW] resize buffer to size %s", pretty_size(size));

    // Create the new buffer with the new size.
    DvzBuffer new_buffer = dvz_buffer(gpu);
    _buffer_copy(buffer, &new_buffer);
    // Make sure we can copy to the new buffer.
    bool proceed = true;
    if ((new_buffer.usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) == 0)
    {
        log_warn("buffer was not created with VK_BUFFER_USAGE_TRANSFER_DST_BIT and therefore the "
                 "data cannot be kept while resizing it");
        proceed = false;
    }
    new_buffer.size = size;
    _buffer_create(&new_buffer);
    // At this point, the new buffer is empty.

    // Handle permanent mapping.
    void* old_mmap = buffer->mmap;
    if (buffer->mmap != NULL)
    {
        // Unmap the to-be-deleted buffer.
        dvz_buffer_unmap(buffer);
        // NOTE: buffer->mmap remains not NULL but invalid: it will need to be reset to a new
        // mapped region after creation of the new buffer.
        buffer->mmap = NULL;
    }

    // If a DvzCommands object was passed for the data transfer, transfer the data from the
    // old buffer to the new, by flushing the corresponding queue and waiting for completion.

    // HACK: use queue 0 for transfers (convention)
    DvzCommands cmds_ = dvz_commands(gpu, 0, 1);
    DvzCommands* cmds = &cmds_;
    if (proceed)
    {
        uint32_t queue_idx = cmds->queue_idx;
        log_debug("copying data from the old buffer to the new one before destroying the old one");
        ASSERT(queue_idx < gpu->queues.queue_count);
        ASSERT(size >= buffer->size);

        dvz_cmd_reset(cmds, 0);
        dvz_cmd_begin(cmds, 0);
        dvz_cmd_copy_buffer(cmds, 0, buffer, 0, &new_buffer, 0, buffer->size);
        dvz_cmd_end(cmds, 0);

        VkQueue queue = gpu->queues.queues[queue_idx];
        dvz_cmd_submit_sync(cmds, 0);
        vkQueueWaitIdle(queue);
    }

    // Delete the old buffer after the transfer has finished.
    _buffer_destroy(buffer);

    // Update the existing buffer's size.
    buffer->size = new_buffer.size;
    ASSERT(buffer->size == size);

    // Update the existing DvzBuffer struct with the newly-created Vulkan objects.
    buffer->buffer = new_buffer.buffer;
    // buffer->device_memory = new_buffer.device_memory;
    buffer->buffer = new_buffer.buffer;
    buffer->vma = new_buffer.vma;

    ASSERT(buffer->buffer != VK_NULL_HANDLE);
    // ASSERT(buffer->device_memory != VK_NULL_HANDLE);

    // If the existing buffer was already mapped, we need to remap the new buffer.
    if (old_mmap != NULL)
    {
        buffer->mmap = dvz_buffer_map(buffer, 0, VK_WHOLE_SIZE);
        // Make sure the permanent memmap has been updated after the buffer resize.
        ASSERT(buffer->mmap != old_mmap);
    }
}



void* dvz_buffer_map(DvzBuffer* buffer, VkDeviceSize offset, VkDeviceSize size)
{
    ASSERT(buffer != NULL);
    ASSERT(buffer->gpu != NULL);
    ASSERT(buffer->gpu->device != VK_NULL_HANDLE);
    ASSERT(dvz_obj_is_created(&buffer->obj));
    if (size < UINT64_MAX)
        ASSERT(offset + size <= buffer->size);
    ASSERT(
        (buffer->memory & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) && //
        (buffer->memory & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

    log_debug("memmap buffer %d", buffer->type);
    ASSERT(buffer->mmap == NULL);
    void* cdata = NULL;
    // VK_CHECK_RESULT(
    //     vkMapMemory(buffer->gpu->device, buffer->device_memory, offset, size, 0, &cdata));

    // if (offset != 0)
    //     log_warn("buffer map offset %d not taken into account with VMA", offset);
    // if (size != buffer->size && size != VK_WHOLE_SIZE)
    //     log_debug("buffer map size %d not taken into account with VMA", size);

    vmaMapMemory(buffer->gpu->allocator, buffer->vma.alloc, &cdata);

    // HACK: since VMA does not map portions of a buffer, we must do it manually.

    return (void*)((uint64_t)cdata + (uint64_t)offset);
}



void dvz_buffer_unmap(DvzBuffer* buffer)
{
    ASSERT(buffer != NULL);
    ASSERT(buffer->gpu != NULL);
    ASSERT(buffer->gpu->device != VK_NULL_HANDLE);
    ASSERT(dvz_obj_is_created(&buffer->obj));

    ASSERT(
        (buffer->memory & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) && //
        (buffer->memory & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

    log_debug("unmap buffer %d", buffer->type);
    // vkUnmapMemory(buffer->gpu->device, buffer->device_memory);
    vmaUnmapMemory(buffer->gpu->allocator, buffer->vma.alloc);
}



void dvz_buffer_upload(DvzBuffer* buffer, VkDeviceSize offset, VkDeviceSize size, const void* data)
{
    ASSERT(buffer != NULL);
    ASSERT(size > 0);
    ASSERT(data != NULL);
    ASSERT(buffer->buffer != VK_NULL_HANDLE);
    ASSERT(offset + size <= buffer->size);

    // log_trace("uploading %s to GPU buffer", pretty_size(size));
    void* mapped = NULL;
    bool need_unmap = false;
    if (buffer->mmap == NULL)
    {
        mapped = dvz_buffer_map(buffer, offset, size);
        need_unmap = true;
    }
    else
    {
        mapped = (void*)((int64_t)buffer->mmap + (int64_t)offset);
        need_unmap = false;
    }

    ASSERT(mapped != NULL);
    memcpy(mapped, data, size);

    if (need_unmap)
        dvz_buffer_unmap(buffer);
}



void dvz_buffer_download(DvzBuffer* buffer, VkDeviceSize offset, VkDeviceSize size, void* data)
{
    log_trace("downloading %s from GPU buffer", pretty_size(size));

    void* mapped = NULL;
    bool need_unmap = false;
    if (buffer->mmap == NULL)
    {
        mapped = dvz_buffer_map(buffer, offset, size);
        need_unmap = true;
    }
    else
    {
        mapped = (void*)((int64_t)buffer->mmap + (int64_t)offset);
        need_unmap = false;
    }
    memcpy(data, mapped, size);
    if (need_unmap)
        dvz_buffer_unmap(buffer);
}



void dvz_buffer_destroy(DvzBuffer* buffer)
{
    ASSERT(buffer != NULL);
    if (!dvz_obj_is_created(&buffer->obj))
    {
        log_trace("skip destruction of already-destroyed buffer");
        return;
    }
    log_trace("destroy buffer");
    _buffer_destroy(buffer);
    dvz_obj_destroyed(&buffer->obj);
}



/*************************************************************************************************/
/*  Buffer regions                                                                               */
/*************************************************************************************************/

DvzBufferRegions dvz_buffer_regions(
    DvzBuffer* buffer, uint32_t count, //
    VkDeviceSize offset, VkDeviceSize size, VkDeviceSize alignment)
{
    ASSERT(buffer != NULL);
    ASSERT(buffer->gpu != NULL);
    ASSERT(buffer->gpu->device != VK_NULL_HANDLE);
    ASSERT(dvz_obj_is_created(&buffer->obj));
    ASSERT(count <= DVZ_MAX_BUFFER_REGIONS_PER_SET);

    DvzBufferRegions regions = {0};
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



void* dvz_buffer_regions_map(
    DvzBufferRegions* br, uint32_t idx, VkDeviceSize offset, VkDeviceSize size)
{
    ASSERT(br != NULL);
    DvzBuffer* buffer = br->buffer;
    ASSERT(idx < br->count);
    ASSERT(size <= br->size);
    ASSERT(br->offsets[idx] + offset + size <= buffer->size);
    return dvz_buffer_map(buffer, br->offsets[idx] + offset, size);
}



void dvz_buffer_regions_unmap(DvzBufferRegions* br)
{
    ASSERT(br != NULL);
    DvzBuffer* buffer = br->buffer;
    ASSERT(buffer != NULL);
    dvz_buffer_unmap(buffer);
}



void dvz_buffer_regions_upload(
    DvzBufferRegions* br, uint32_t idx, VkDeviceSize offset, VkDeviceSize size, const void* data)
{
    ASSERT(br != NULL);
    DvzBuffer* buffer = br->buffer;

    // VkDeviceSize size = br->size;
    // NOTE: size is now passed as an argument to the function

    ASSERT(buffer != NULL);
    ASSERT(size != 0);
    ASSERT(data != NULL);

    log_trace("uploading %s to GPU buffer", pretty_size(size));

    void* mapped = NULL;
    bool need_unmap = false;
    if (buffer->mmap == NULL)
    {
        mapped = dvz_buffer_regions_map(br, idx, offset, size);
        need_unmap = true;
    }
    else
    {
        mapped = buffer->mmap;
        need_unmap = false;
    }
    ASSERT(mapped != NULL);

    memcpy(mapped, data, size);

    if (need_unmap)
        dvz_buffer_regions_unmap(br);
}



void dvz_buffer_regions_download(
    DvzBufferRegions* br, uint32_t idx, VkDeviceSize offset, VkDeviceSize size, void* data)
{
    ASSERT(br != NULL);
    DvzBuffer* buffer = br->buffer;

    // VkDeviceSize size = br->size;
    // NOTE: size is now passed as an argument to the function

    ASSERT(buffer != NULL);
    ASSERT(size != 0);
    ASSERT(data != NULL);

    log_trace("downloading %s from GPU buffer", pretty_size(size));

    void* mapped = NULL;
    bool need_unmap = false;
    if (buffer->mmap == NULL)
    {
        mapped = dvz_buffer_regions_map(br, idx, offset, size);
        need_unmap = true;
    }
    else
    {
        mapped = buffer->mmap;
        need_unmap = false;
    }
    ASSERT(mapped != NULL);

    memcpy(data, mapped, size);

    if (need_unmap)
        dvz_buffer_regions_unmap(br);
}



void dvz_buffer_regions_copy(
    DvzBufferRegions* src, uint32_t src_idx, VkDeviceSize src_offset, //
    DvzBufferRegions* dst, uint32_t dst_idx, VkDeviceSize dst_offset, VkDeviceSize size)
{
    ASSERT(src != NULL);
    ASSERT(dst != NULL);
    ASSERT(src->buffer != NULL);
    ASSERT(dst->buffer != NULL);
    ASSERT(src->buffer->gpu != NULL);
    ASSERT(src->buffer->gpu == dst->buffer->gpu);

    log_debug(
        "request copy from src region #%d (n=%d) to dst region #%d (n=%d)", //
        src_idx, src->count, dst_idx, dst->count);

    DvzGpu* gpu = src->buffer->gpu;
    ASSERT(gpu != NULL);
    ASSERT(size > 0);

    // HACK: use queue 0 for transfers (convention)
    DvzCommands cmds_ = dvz_commands(gpu, 0, 1);
    DvzCommands* cmds = &cmds_;

    dvz_cmd_reset(cmds, 0);
    dvz_cmd_begin(cmds, 0);

    // Copy buffer command.
    VkBufferCopy* regions = (VkBufferCopy*)calloc(src->count, sizeof(VkBufferCopy));
    uint32_t cnt = 0; // how many regions to copy
    cnt = 0;

    // Copy 1 or all regions.
    uint32_t u = 0, v = 0;
    for (uint32_t i = 0; i < MAX(src->count, dst->count); i++)
    {
        u = src_idx >= src->count ? i : src_idx;
        v = dst_idx >= dst->count ? i : dst_idx;
        if (u >= src->count || v >= dst->count)
            break;
        log_debug("copy src region #%d to dst region #%d, size %s", u, v, pretty_size(size));
        ASSERT(u < src->count);
        ASSERT(v < dst->count);
        regions[i].size = size;
        regions[i].srcOffset = src->offsets[u] + src_offset;
        regions[i].dstOffset = dst->offsets[v] + dst_offset;
        cnt++;

        // NOTE: a single region to copy if neither src_idx nor dst_idx is UINT32_MAX
        if (src_idx < src->count && dst_idx < dst->count)
            break;
    }

    ASSERT(cnt > 0);
    vkCmdCopyBuffer(cmds->cmds[0], src->buffer->buffer, dst->buffer->buffer, cnt, regions);

    dvz_cmd_end(cmds, 0);
    FREE(regions);

    // Wait for the render queue to be idle.
    // dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_RENDER);

    // Submit the commands to the transfer queue.
    DvzSubmit submit = dvz_submit(gpu);
    dvz_submit_commands(&submit, cmds);
    // log_debug("copy %s between 2 buffers", pretty_size(size));
    dvz_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    // dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_TRANSFER);
}



/*************************************************************************************************/
/*  Images                                                                                       */
/*************************************************************************************************/

DvzImages dvz_images(DvzGpu* gpu, VkImageType type, uint32_t count)
{
    ASSERT(gpu != NULL);
    ASSERT(dvz_obj_is_created(&gpu->obj));

    DvzImages images = {0};
    dvz_obj_init(&images.obj);

    images.gpu = gpu;
    images.image_type = type;
    ASSERT(type <= VK_IMAGE_TYPE_3D);
    // HACK: find the matching view type.
    images.view_type = (VkImageViewType)type;
    images.count = count;

    // Default options.
    images.tiling = VK_IMAGE_TILING_OPTIMAL;
    // images.memory = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    images.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    for (uint32_t i = 0; i < images.count; i++)
        images.vma[i].usage = VMA_MEMORY_USAGE_GPU_ONLY;

    return images;
}



void dvz_images_format(DvzImages* img, VkFormat format)
{
    ASSERT(img != NULL);
    img->format = format;
}



void dvz_images_layout(DvzImages* img, VkImageLayout layout)
{
    ASSERT(img != NULL);
    img->layout = layout;
}



void dvz_images_size(DvzImages* img, uvec3 shape)
{
    ASSERT(img != NULL);

    log_trace("set image size %dx%dx%d", shape[0], shape[1], shape[2]);
    check_dims(img->image_type, shape);

    _copy_shape(shape, img->shape);
}



void dvz_images_tiling(DvzImages* img, VkImageTiling tiling)
{
    ASSERT(img != NULL);
    img->tiling = tiling;
}



void dvz_images_usage(DvzImages* img, VkImageUsageFlags usage)
{
    ASSERT(img != NULL);
    img->usage = usage;
}



void dvz_images_vma_usage(DvzImages* img, VmaMemoryUsage vma_usage)
{
    ASSERT(img != NULL);
    for (uint32_t i = 0; i < img->count; i++)
        img->vma[i].usage = vma_usage;
}



void dvz_images_memory(DvzImages* img, VkMemoryPropertyFlags memory)
{
    ASSERT(img != NULL);
    img->memory = memory;
}



void dvz_images_aspect(DvzImages* img, VkImageAspectFlags aspect)
{
    ASSERT(img != NULL);
    img->aspect = aspect;
}



void dvz_images_queue_access(DvzImages* img, uint32_t queue_idx)
{
    ASSERT(img != NULL);
    ASSERT(queue_idx < img->gpu->queues.queue_count);
    img->queues[img->queue_count++] = queue_idx;
}



static void _images_create(DvzImages* img)
{
    DvzGpu* gpu = img->gpu;
    VkDeviceSize size = 0;

    // Check whether the image format is supported.
    if (!img->is_swapchain)
    {
        VkImageFormatProperties props = {0};
        VkResult res = vkGetPhysicalDeviceImageFormatProperties(
            gpu->physical_device, img->format, img->image_type, img->tiling, //
            img->usage, 0, &props);
        if (res != VK_SUCCESS)
        {
            log_error("unable to create image, format not supported");
        }
    }

    // Create the images.
    uint32_t width = img->shape[0];
    uint32_t height = img->shape[1];
    uint32_t depth = img->shape[2];

    log_trace("create image %dD %dx%dx%d", img->image_type + 1, width, height, depth);
    ASSERT(width > 0);

    VkImageCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType = img->image_type;
    info.extent.width = width;
    info.extent.height = height;
    info.extent.depth = depth;
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.format = img->format;
    info.tiling = img->tiling;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.usage = img->usage;
    info.samples = VK_SAMPLE_COUNT_1_BIT;

    // Sharing mode, depending on the queues that need to access the image.
    uint32_t queue_families[DVZ_MAX_QUEUE_FAMILIES];
    make_shared(
        &gpu->queues, img->queue_count, img->queues, //
        &info.sharingMode, &info.queueFamilyIndexCount, queue_families);
    info.pQueueFamilyIndices = queue_families;

    // Create the images with VMA.
    VmaAllocationCreateInfo alloc_info = {0};
    // NOTE: we assume all images in the DvzImages set share the same VMA flags and usage.
    alloc_info.flags = img->vma[0].flags;
    alloc_info.usage = img->vma[0].usage;

    for (uint32_t i = 0; i < img->count; i++)
    {
        if (!img->is_swapchain)
        {
            vmaCreateImage(
                gpu->allocator, &info, &alloc_info, &img->images[i], //
                &img->vma[i].alloc, &img->vma[i].info);
            ASSERT(img->images[i] != VK_NULL_HANDLE);

            // Get the memory flags found by VMA and store them in the DvzBuffer instance.
            vmaGetMemoryTypeProperties(gpu->allocator, img->vma[i].info.memoryType, &img->memory);
            ASSERT(img->memory != 0);
        }

        // HACK: staging images do not require an image view
        if (img->tiling != VK_IMAGE_TILING_LINEAR)
            create_image_view(
                gpu->device, img->images[i], img->view_type, img->format, img->aspect,
                &img->image_views[i]);

        // Store the size in bytes of each create image (which should be the same).
        VkMemoryRequirements memRequirements = {0};
        vkGetImageMemoryRequirements(img->gpu->device, img->images[i], &memRequirements);
        if (size == 0)
            size = memRequirements.size;
        else
            ASSERT(size == memRequirements.size);
    }
    img->size = size;
}



static void _images_destroy(DvzImages* img)
{
    ASSERT(img != NULL);
    ASSERT(img->gpu != NULL);

    for (uint32_t i = 0; i < img->count; i++)
    {
        if (img->image_views[i] != VK_NULL_HANDLE)
        {
            vkDestroyImageView(img->gpu->device, img->image_views[i], NULL);
            img->image_views[i] = VK_NULL_HANDLE;
        }
        if (!img->is_swapchain && img->images[i] != VK_NULL_HANDLE)
        {
            vmaDestroyImage(img->gpu->allocator, img->images[i], img->vma[i].alloc);
            img->images[i] = VK_NULL_HANDLE;
        }
    }
}



void dvz_images_create(DvzImages* img)
{
    ASSERT(img != NULL);
    ASSERT(img->gpu != NULL);
    ASSERT(img->gpu->device != VK_NULL_HANDLE);

    check_dims(img->image_type, img->shape);

    log_trace("starting creation of %d images...", img->count);
    _images_create(img);
    dvz_obj_created(&img->obj);
    log_trace("%d images created", img->count);
}



void dvz_images_transition(DvzImages* img)
{
    ASSERT(img != NULL);
    DvzGpu* gpu = img->gpu;
    ASSERT(gpu != NULL);

    // Start the image transition command buffer.
    // HACK: use queue 0 for transfer (convention)
    DvzCommands cmds = dvz_commands(gpu, 0, 1);
    DvzBarrier barrier = dvz_barrier(gpu);

    dvz_cmd_begin(&cmds, 0);
    dvz_barrier_stages(&barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    dvz_barrier_images(&barrier, img);
    dvz_barrier_images_layout(&barrier, VK_IMAGE_LAYOUT_UNDEFINED, img->layout);
    // dvz_barrier_images_access(&barrier, 0, VK_ACCESS_TRANSFER_WRITE_BIT);
    dvz_cmd_barrier(&cmds, 0, &barrier);
    dvz_cmd_end(&cmds, 0);

    dvz_gpu_wait(gpu);
    dvz_cmd_submit_sync(&cmds, 0);
}



void dvz_images_resize(DvzImages* img, uvec3 new_shape)
{
    ASSERT(img != NULL);
    log_debug(
        "[SLOW] resize images to size %dx%dx%d, losing the data in it", //
        new_shape[0], new_shape[1], new_shape[2]);
    _images_destroy(img);
    dvz_images_size(img, new_shape);
    _images_create(img);
}



static void*
_images_download(DvzImages* img, uint32_t idx, bool has_alpha, VkSubresourceLayout* res_layout)
{
    ASSERT(img != NULL);
    ASSERT(img->gpu != NULL);

    VkImageSubresource res = {0};
    res.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vkGetImageSubresourceLayout(img->gpu->device, img->images[idx], &res, res_layout);

    // Map image memory so we can start copying from it
    void* cdata = NULL;
    // vkMapMemory(images->gpu->device, images->memories[idx], 0, VK_WHOLE_SIZE, 0, &cdata);
    vmaMapMemory(img->gpu->allocator, img->vma[idx].alloc, &cdata);
    ASSERT(cdata != NULL);
    VkDeviceSize row_pitch = res_layout->rowPitch;
    ASSERT(row_pitch > 0);

    uint32_t w = img->shape[0];
    uint32_t h = img->shape[1];

    // Size of the buffer to copy.
    VkDeviceSize size = row_pitch * h;
    ASSERT(w > 0);
    ASSERT(h > 0);
    // Ensure that the images buffer has the right size.
    ASSERT(img->size >= size);

    // First, memcopy from the GPU to the CPU.
    void* image = calloc(row_pitch * h, 1);
    memcpy(image, cdata, size);
    vmaUnmapMemory(img->gpu->allocator, img->vma[idx].alloc);
    // vkUnmapMemory(images->gpu->device, images->memories[idx]);

    return image;
}

static void _pack_image_data(
    DvzImages* img, void* imgdata, VkDeviceSize bytes_per_component, //
    VkDeviceSize offset, VkDeviceSize row_pitch,                     //
    bool swizzle, bool has_alpha, void* out)
{
    ASSERT(img != NULL);
    ASSERT(imgdata != NULL);
    ASSERT(out != NULL);
    ASSERT(row_pitch > 0);

    // void* image_orig = images;

    uint32_t n_components = has_alpha ? 4 : 3;
    uint32_t w = img->shape[0];
    uint32_t h = img->shape[1];

    // Then, convert the image to the requested format, into a contiguous array of pixels.
    imgdata = (void*)((uint64_t)imgdata + offset);
    uint32_t src_offset = 0;
    uint32_t dst_offset = 0;
    uint32_t y, x, k, l;
    for (y = 0; y < h; y++)
    {
        src_offset = 0;
        for (x = 0; x < w; x++)
        {
            ASSERT(src_offset + 2 < w * h * 4);
            for (k = 0; k < 4; k++)
            {
                l = k <= 2 ? (swizzle ? 2 - k : k) : 3;
                ASSERT(k <= 3);
                ASSERT(l <= 3);
                ASSERT(k < 3 || l == 3);
                if (k == 3 && n_components == 3)
                    continue;
                memcpy(
                    (void*)((uint64_t)out + (dst_offset + k) * bytes_per_component),
                    (void*)((uint64_t)imgdata + (src_offset + l) * bytes_per_component),
                    bytes_per_component);
            }

            src_offset += 4;            // we assume RGBA in the source array
            dst_offset += n_components; // either RGB or RGBA in the target array
        }
        imgdata = (void*)((uint64_t)imgdata + row_pitch);
    }
    ASSERT(dst_offset == w * h * n_components);
}

void dvz_images_download(
    DvzImages* staging, uint32_t idx, VkDeviceSize bytes_per_component, //
    bool swizzle, bool has_alpha, void* out)
{
    // NOTE: we make the following assumptions:
    // - bytes_per_component is the same between the source and target
    // - source always has alpha
    // - parameter "has_alpha" only refers to the source buffer

    ASSERT(staging != NULL);
    ASSERT(out != NULL);
    ASSERT(bytes_per_component > 0);

    VkSubresourceLayout res_layout = {0};
    // Copy via memory mapping the image memory, which is supposed to be linear.
    void* imgdata = _images_download(staging, idx, has_alpha, &res_layout);
    // Before we can use the copied data, we need to pack it as it may be padded due to internal
    // hardware constraints.
    _pack_image_data(
        staging, imgdata, bytes_per_component, res_layout.offset, res_layout.rowPitch, swizzle,
        has_alpha, out);
    FREE(imgdata);
}



void dvz_images_copy(
    DvzImages* src, uvec3 src_offset, DvzImages* dst, uvec3 dst_offset, uvec3 shape)
{
    ASSERT(src != NULL);
    ASSERT(dst != NULL);
    DvzGpu* gpu = src->gpu;
    ASSERT(gpu != NULL);

    // Take transfer cmd buf.
    DvzCommands cmds_ = dvz_commands(gpu, 0, 1);
    DvzCommands* cmds = &cmds_;
    dvz_cmd_reset(cmds, 0);
    dvz_cmd_begin(cmds, 0);

    DvzBarrier src_barrier = dvz_barrier(gpu);
    dvz_barrier_stages(
        &src_barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    dvz_barrier_images(&src_barrier, src);

    DvzBarrier dst_barrier = dvz_barrier(gpu);
    dvz_barrier_stages(
        &dst_barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    dvz_barrier_images(&dst_barrier, dst);

    // Source image transition.
    if (src->layout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
    {
        log_trace("source image %d transition", src->images[0]);
        dvz_barrier_images_layout(
            &src_barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        dvz_barrier_images_access(&src_barrier, 0, VK_ACCESS_TRANSFER_READ_BIT);
        dvz_cmd_barrier(cmds, 0, &src_barrier);
    }

    // Destination image transition.
    {
        log_trace("destination image %d transition", dst->images[0]);
        dvz_barrier_images_layout(
            &dst_barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        dvz_barrier_images_access(&dst_barrier, 0, VK_ACCESS_TRANSFER_WRITE_BIT);
        dvz_cmd_barrier(cmds, 0, &dst_barrier);
    }

    // Copy texture command.
    VkImageCopy copy = {0};
    copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.srcSubresource.layerCount = 1;
    copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.dstSubresource.layerCount = 1;
    copy.extent.width = shape[0];
    copy.extent.height = shape[1];
    copy.extent.depth = shape[2];
    copy.srcOffset.x = (int32_t)src_offset[0];
    copy.srcOffset.y = (int32_t)src_offset[1];
    copy.srcOffset.z = (int32_t)src_offset[2];
    copy.dstOffset.x = (int32_t)dst_offset[0];
    copy.dstOffset.y = (int32_t)dst_offset[1];
    copy.dstOffset.z = (int32_t)dst_offset[2];

    vkCmdCopyImage(
        cmds->cmds[0],                                        //
        src->images[0], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, //
        dst->images[0], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, //
        1, &copy);

    // Source image transition.
    if (src->layout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL &&
        src->layout != VK_IMAGE_LAYOUT_UNDEFINED)
    {
        log_trace("source image transition back");
        dvz_barrier_images_layout(&src_barrier, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, src->layout);
        dvz_barrier_images_access(
            &src_barrier, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
        dvz_cmd_barrier(cmds, 0, &src_barrier);
    }

    // Destination image transition.
    if (dst->layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
        dst->layout != VK_IMAGE_LAYOUT_UNDEFINED)
    {
        log_trace("destination image transition back");
        dvz_barrier_images_layout(&dst_barrier, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dst->layout);
        dvz_barrier_images_access(&dst_barrier, VK_ACCESS_TRANSFER_WRITE_BIT, 0);
        dvz_cmd_barrier(cmds, 0, &dst_barrier);
    }

    dvz_cmd_end(cmds, 0);

    // Wait for the render queue to be idle.
    // dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_RENDER);

    // Submit the commands to the transfer queue.
    DvzSubmit submit = dvz_submit(gpu);
    dvz_submit_commands(&submit, cmds);
    log_debug("copy %dx%dx%d between 2 textures", shape[0], shape[1], shape[2]);
    dvz_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    // dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_TRANSFER);
}



void dvz_images_copy_from_buffer(
    DvzImages* img, uvec3 tex_offset, uvec3 shape, //
    DvzBufferRegions br, VkDeviceSize buf_offset, VkDeviceSize size)
{
    ASSERT(img != NULL);
    DvzGpu* gpu = img->gpu;
    ASSERT(gpu != NULL);

    DvzBuffer* buffer = br.buffer;
    ASSERT(buffer != NULL);
    buf_offset = br.offsets[0] + buf_offset;

    for (uint32_t i = 0; i < 3; i++)
    {
        ASSERT(shape[i] > 0);
        ASSERT(tex_offset[i] + shape[i] <= img->shape[i]);
    }

    log_debug("copy buffer to image (%s)", pretty_size(size));

    // Take transfer cmd buf.
    DvzCommands cmds_ = dvz_commands(gpu, 0, 1);
    DvzCommands* cmds = &cmds_;
    dvz_cmd_reset(cmds, 0);
    dvz_cmd_begin(cmds, 0);

    // Image transition.
    DvzBarrier barrier = dvz_barrier(gpu);
    dvz_barrier_stages(&barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    ASSERT(img != NULL);
    ASSERT(img != NULL);
    dvz_barrier_images(&barrier, img);
    dvz_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    dvz_barrier_images_access(&barrier, 0, VK_ACCESS_TRANSFER_WRITE_BIT);
    dvz_cmd_barrier(cmds, 0, &barrier);

    // Copy to staging buffer
    dvz_cmd_copy_buffer_to_image(cmds, 0, buffer, buf_offset, img, tex_offset, shape);

    // Image transition.
    dvz_barrier_images_layout(&barrier, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, img->layout);
    dvz_barrier_images_access(&barrier, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT);
    dvz_cmd_barrier(cmds, 0, &barrier);

    dvz_cmd_end(cmds, 0);

    // Wait for the render queue to be idle.
    // dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_RENDER);

    // Submit the commands to the transfer queue.
    DvzSubmit submit = dvz_submit(gpu);
    dvz_submit_commands(&submit, cmds);
    dvz_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    // dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_TRANSFER);
}



void dvz_images_copy_to_buffer(
    DvzImages* img, uvec3 tex_offset, uvec3 shape, //
    DvzBufferRegions br, VkDeviceSize buf_offset, VkDeviceSize size)
{
    ASSERT(img != NULL);
    DvzGpu* gpu = img->gpu;
    ASSERT(gpu != NULL);

    DvzBuffer* buffer = br.buffer;
    ASSERT(buffer != NULL);
    buf_offset = br.offsets[0] + buf_offset;

    for (uint32_t i = 0; i < 3; i++)
    {
        ASSERT(shape[i] > 0);
        ASSERT(tex_offset[i] + shape[i] <= img->shape[i]);
    }

    log_debug("copy image to buffer (%s)", pretty_size(size));

    // Take transfer cmd buf.
    DvzCommands cmds_ = dvz_commands(gpu, 0, 1);
    DvzCommands* cmds = &cmds_;
    dvz_cmd_reset(cmds, 0);
    dvz_cmd_begin(cmds, 0);

    // Image transition.
    DvzBarrier barrier = dvz_barrier(gpu);
    dvz_barrier_stages(&barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    ASSERT(img != NULL);
    ASSERT(img != NULL);
    dvz_barrier_images(&barrier, img);
    dvz_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    dvz_barrier_images_access(&barrier, 0, VK_ACCESS_TRANSFER_READ_BIT);
    dvz_cmd_barrier(cmds, 0, &barrier);

    // Copy to staging buffer
    dvz_cmd_copy_image_to_buffer(cmds, 0, img, tex_offset, shape, buffer, buf_offset);

    // Image transition.
    dvz_barrier_images_layout(&barrier, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, img->layout);
    dvz_barrier_images_access(&barrier, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT);
    dvz_cmd_barrier(cmds, 0, &barrier);

    dvz_cmd_end(cmds, 0);

    // Wait for the render queue to be idle.
    // dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_RENDER);

    // Submit the commands to the transfer queue.
    DvzSubmit submit = dvz_submit(gpu);
    dvz_submit_commands(&submit, cmds);
    dvz_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    // dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_TRANSFER);
}



void dvz_images_destroy(DvzImages* img)
{
    ASSERT(img != NULL);
    if (!dvz_obj_is_created(&img->obj))
    {
        log_trace("skip destruction of already-destroyed images");
        return;
    }
    log_trace("destroy %d image(s) and image view(s)", img->count);
    _images_destroy(img);
    dvz_obj_destroyed(&img->obj);
}



/*************************************************************************************************/
/*  Sampler                                                                                      */
/*************************************************************************************************/

DvzSampler dvz_sampler(DvzGpu* gpu)
{
    ASSERT(gpu != NULL);
    ASSERT(dvz_obj_is_created(&gpu->obj));

    DvzSampler sampler = {0};
    dvz_obj_init(&sampler.obj);
    sampler.gpu = gpu;

    return sampler;
}



void dvz_sampler_min_filter(DvzSampler* sampler, VkFilter filter)
{
    ASSERT(sampler != NULL);
    sampler->min_filter = filter;
}



void dvz_sampler_mag_filter(DvzSampler* sampler, VkFilter filter)
{
    ASSERT(sampler != NULL);
    sampler->mag_filter = filter;
}



void dvz_sampler_address_mode(
    DvzSampler* sampler, DvzSamplerAxis axis, VkSamplerAddressMode address_mode)
{
    ASSERT(sampler != NULL);
    ASSERT(axis <= 2);
    sampler->address_modes[axis] = address_mode;
}



void dvz_sampler_create(DvzSampler* sampler)
{
    ASSERT(sampler != NULL);
    ASSERT(sampler->gpu != NULL);
    ASSERT(sampler->gpu->device != VK_NULL_HANDLE);

    log_trace("starting creation of sampler...");

    create_texture_sampler(
        sampler->gpu->device, sampler->mag_filter, sampler->min_filter, //
        sampler->address_modes, false, &sampler->sampler);

    dvz_obj_created(&sampler->obj);
    log_trace("sampler created");
}



void dvz_sampler_destroy(DvzSampler* sampler)
{
    ASSERT(sampler != NULL);
    if (!dvz_obj_is_created(&sampler->obj))
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
    dvz_obj_destroyed(&sampler->obj);
}



/*************************************************************************************************/
/*  Slots                                                                                        */
/*************************************************************************************************/

DvzSlots dvz_slots(DvzGpu* gpu)
{
    ASSERT(gpu != NULL);
    ASSERT(dvz_obj_is_created(&gpu->obj));

    DvzSlots slots = {0};
    slots.gpu = gpu;
    dvz_obj_init(&slots.obj);

    return slots;
}



void dvz_slots_binding(DvzSlots* slots, uint32_t idx, VkDescriptorType type)
{
    ASSERT(slots != NULL);
    ASSERT(idx == slots->slot_count);
    ASSERT(idx < DVZ_MAX_BINDINGS_SIZE);
    slots->types[slots->slot_count++] = type;
}



void dvz_slots_push(
    DvzSlots* slots, VkDeviceSize offset, VkDeviceSize size, VkShaderStageFlags shaders)
{
    ASSERT(slots != NULL);
    uint32_t idx = slots->push_count;
    ASSERT(idx < DVZ_MAX_PUSH_CONSTANTS);

    slots->push_offsets[idx] = offset;
    slots->push_sizes[idx] = size;
    slots->push_shaders[idx] = shaders;

    slots->push_count++;
}



void dvz_slots_create(DvzSlots* slots)
{
    ASSERT(slots != NULL);
    ASSERT(slots->gpu != NULL);
    ASSERT(slots->gpu->device != VK_NULL_HANDLE);

    log_trace("starting creation of slots...");

    create_descriptor_set_layout(
        slots->gpu->device, slots->slot_count, slots->types, &slots->dset_layout);

    // Push constants.
    VkPushConstantRange push_constants[DVZ_MAX_PUSH_CONSTANTS] = {0};
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

    dvz_obj_created(&slots->obj);
    log_trace("slots created");
}



void dvz_slots_destroy(DvzSlots* slots)
{
    ASSERT(slots != NULL);
    ASSERT(slots->gpu != NULL);
    if (!dvz_obj_is_created(&slots->obj))
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
    dvz_obj_destroyed(&slots->obj);
}



/*************************************************************************************************/
/*  Bindings                                                                                     */
/*************************************************************************************************/

DvzBindings dvz_bindings(DvzSlots* slots, uint32_t dset_count)
{
    ASSERT(slots != NULL);
    DvzGpu* gpu = slots->gpu;
    ASSERT(gpu != NULL);
    ASSERT(dvz_obj_is_created(&gpu->obj));

    DvzBindings bindings = {0};
    bindings.slots = slots;
    bindings.gpu = gpu;

    dvz_obj_init(&bindings.obj);

    if (!dvz_obj_is_created(&slots->obj))
        dvz_slots_create(slots);
    ASSERT(dset_count > 0);
    ASSERT(slots->dset_layout != VK_NULL_HANDLE);

    log_trace("starting creation of bindings with %d descriptor sets...", dset_count);
    bindings.dset_count = dset_count;

    allocate_descriptor_sets(
        gpu->device, gpu->dset_pool, slots->dset_layout, bindings.dset_count, bindings.dsets);

    dvz_obj_created(&bindings.obj);
    log_trace("bindings created");

    return bindings;
}



void dvz_bindings_buffer(DvzBindings* bindings, uint32_t idx, DvzBufferRegions br)
{
    ASSERT(bindings != NULL);
    ASSERT(br.buffer != VK_NULL_HANDLE);
    ASSERT(br.count > 0);
    ASSERT(bindings->dset_count > 0);
    ASSERT(br.count == 1 || br.count == bindings->dset_count);
    log_trace("set bindings with buffer for binding #%d", idx);

    bindings->br[idx] = br;

    if (bindings->obj.status == DVZ_OBJECT_STATUS_CREATED)
        bindings->obj.status = DVZ_OBJECT_STATUS_NEED_UPDATE;
}



void dvz_bindings_texture(DvzBindings* bindings, uint32_t idx, DvzImages* img, DvzSampler* sampler)
{
    ASSERT(bindings != NULL);
    ASSERT(img != NULL);
    ASSERT(sampler != NULL);
    ASSERT(img->count == 1 || img->count == bindings->dset_count);

    log_trace("set bindings with texture for binding #%d", idx);
    bindings->images[idx] = img;
    bindings->samplers[idx] = sampler;

    if (bindings->obj.status == DVZ_OBJECT_STATUS_CREATED)
        bindings->obj.status = DVZ_OBJECT_STATUS_NEED_UPDATE;
}



void dvz_bindings_update(DvzBindings* bindings)
{
    log_trace("update bindings");
    ASSERT(bindings->slots != NULL);
    ASSERT(dvz_obj_is_created(&bindings->slots->obj));
    ASSERT(bindings->slots->dset_layout != VK_NULL_HANDLE);
    ASSERT(bindings->dset_count > 0);
    ASSERT(bindings->dset_count <= DVZ_MAX_SWAPCHAIN_IMAGES);

    for (uint32_t i = 0; i < bindings->dset_count; i++)
    {
        update_descriptor_set(
            bindings->gpu->device, bindings->slots->slot_count, bindings->slots->types,
            bindings->br, bindings->images, bindings->samplers, //
            i, bindings->dsets[i]);
    }

    if (bindings->obj.status == DVZ_OBJECT_STATUS_NEED_UPDATE)
        bindings->obj.status = DVZ_OBJECT_STATUS_CREATED;
}



void dvz_bindings_destroy(DvzBindings* bindings)
{
    ASSERT(bindings != NULL);
    ASSERT(bindings->gpu != NULL);
    if (!dvz_obj_is_created(&bindings->obj))
    {
        log_trace("skip destruction of already-destroyed bindings");
        return;
    }
    log_trace("destroy bindings");
    dvz_obj_destroyed(&bindings->obj);
}



/*************************************************************************************************/
/*  Compute                                                                                      */
/*************************************************************************************************/

DvzCompute dvz_compute(DvzGpu* gpu, const char* shader_path)
{
    ASSERT(gpu != NULL);
    ASSERT(dvz_obj_is_created(&gpu->obj));

    DvzCompute compute = {0};
    dvz_obj_init(&compute.obj);

    compute.gpu = gpu;
    if (shader_path != NULL)
        strcpy(compute.shader_path, shader_path);

    compute.slots = dvz_slots(gpu);

    return compute;
}



void dvz_compute_code(DvzCompute* compute, const char* code)
{
    ASSERT(compute != NULL);
    compute->shader_code = code;
}



void dvz_compute_slot(DvzCompute* compute, uint32_t idx, VkDescriptorType type)
{
    ASSERT(compute != NULL);
    dvz_slots_binding(&compute->slots, idx, type);
}



void dvz_compute_push(
    DvzCompute* compute, VkDeviceSize offset, VkDeviceSize size, VkShaderStageFlags shaders)
{
    ASSERT(compute != NULL);
    dvz_slots_push(&compute->slots, offset, size, shaders);
}



void dvz_compute_bindings(DvzCompute* compute, DvzBindings* bindings)
{
    ASSERT(compute != NULL);
    ASSERT(bindings != NULL);
    compute->bindings = bindings;
}



void dvz_compute_create(DvzCompute* compute)
{
    ASSERT(compute != NULL);
    ASSERT(compute->gpu != NULL);
    ASSERT(compute->gpu->device != VK_NULL_HANDLE);
    ASSERT(compute->shader_path != NULL);
    if (!dvz_obj_is_created(&compute->slots.obj))
        dvz_slots_create(&compute->slots);

    if (compute->bindings == NULL)
    {
        log_error("dvz_compute_bindings() must be called before creating the compute");
        return;
    }

    log_trace("starting creation of compute...");

    if (compute->shader_code != NULL)
    {
        // TODO
        // compute->shader_module =
        //     dvz_shader_compile(compute->gpu, compute->shader_code, VK_SHADER_STAGE_COMPUTE_BIT);
    }
    else
    {
        compute->shader_module =
            create_shader_module_from_file(compute->gpu->device, compute->shader_path);
    }

    create_compute_pipeline(
        compute->gpu->device, compute->shader_module, //
        compute->slots.pipeline_layout, &compute->pipeline);

    dvz_obj_created(&compute->obj);
    log_trace("compute created");
}



void dvz_compute_destroy(DvzCompute* compute)
{
    ASSERT(compute != NULL);
    ASSERT(compute->gpu != NULL);
    if (!dvz_obj_is_created(&compute->obj))
    {
        log_trace("skip destruction of already-destroyed compute");
        return;
    }
    log_trace("destroy compute");

    // Destroy the compute slots.
    if (dvz_obj_is_created(&compute->slots.obj))
        dvz_slots_destroy(&compute->slots);

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

    dvz_obj_destroyed(&compute->obj);
}



/*************************************************************************************************/
/*  Graphics                                                                                     */
/*************************************************************************************************/

DvzGraphics dvz_graphics(DvzGpu* gpu)
{
    ASSERT(gpu != NULL);
    ASSERT(dvz_obj_is_created(&gpu->obj));

    DvzGraphics graphics = {0};
    graphics.gpu = gpu;

    dvz_obj_init(&graphics.obj);

    graphics.slots = dvz_slots(gpu);

    return graphics;
}



void dvz_graphics_renderpass(DvzGraphics* graphics, DvzRenderpass* renderpass, uint32_t subpass)
{
    ASSERT(graphics != NULL);
    graphics->renderpass = renderpass;
    graphics->subpass = subpass;
}



void dvz_graphics_topology(DvzGraphics* graphics, VkPrimitiveTopology topology)
{
    ASSERT(graphics != NULL);
    graphics->topology = topology;
}



void dvz_graphics_shader_glsl(DvzGraphics* graphics, VkShaderStageFlagBits stage, const char* code)
{
    ASSERT(graphics != NULL);
    ASSERT(graphics->gpu != NULL);
    ASSERT(graphics->gpu->device != VK_NULL_HANDLE);

    graphics->shader_stages[graphics->shader_count] = stage;
    // TODO
    // graphics->shader_modules[graphics->shader_count] =
    //     dvz_shader_compile(graphics->gpu, code, stage);
    graphics->shader_count++;
}



void dvz_graphics_shader(
    DvzGraphics* graphics, VkShaderStageFlagBits stage, const char* shader_path)
{
    ASSERT(graphics != NULL);
    ASSERT(graphics->gpu != NULL);
    ASSERT(graphics->gpu->device != VK_NULL_HANDLE);

    graphics->shader_stages[graphics->shader_count] = stage;
    graphics->shader_modules[graphics->shader_count++] =
        create_shader_module_from_file(graphics->gpu->device, shader_path);
}



void dvz_graphics_shader_spirv(
    DvzGraphics* graphics, VkShaderStageFlagBits stage, //
    VkDeviceSize size, const uint32_t* buffer)
{
    ASSERT(graphics != NULL);
    ASSERT(graphics->gpu != NULL);
    ASSERT(graphics->gpu->device != VK_NULL_HANDLE);

    graphics->shader_stages[graphics->shader_count] = stage;
    graphics->shader_modules[graphics->shader_count++] =
        create_shader_module(graphics->gpu->device, size, buffer);
}



void dvz_graphics_vertex_binding(DvzGraphics* graphics, uint32_t binding, VkDeviceSize stride)
{
    ASSERT(graphics != NULL);
    DvzVertexBinding* vb = &graphics->vertex_bindings[graphics->vertex_binding_count++];
    vb->binding = binding;
    vb->stride = stride;
}



void dvz_graphics_vertex_attr(
    DvzGraphics* graphics, uint32_t binding, uint32_t location, VkFormat format,
    VkDeviceSize offset)
{
    ASSERT(graphics != NULL);
    DvzVertexAttr* va = &graphics->vertex_attrs[graphics->vertex_attr_count++];
    va->binding = binding;
    va->location = location;
    va->format = format;
    va->offset = offset;
}



void dvz_graphics_blend(DvzGraphics* graphics, DvzBlendType blend_type)
{
    ASSERT(graphics != NULL);
    graphics->blend_type = blend_type;
}



void dvz_graphics_depth_test(DvzGraphics* graphics, DvzDepthTest depth_test)
{
    ASSERT(graphics != NULL);
    if (depth_test)
        log_debug("enable depth test");
    graphics->depth_test = depth_test;
}



void dvz_graphics_pick(DvzGraphics* graphics, bool support_pick)
{
    ASSERT(graphics != NULL);
    if (support_pick)
        log_debug("enable picking in graphics pipeline");
    graphics->support_pick = support_pick;
}



void dvz_graphics_polygon_mode(DvzGraphics* graphics, VkPolygonMode polygon_mode)
{
    ASSERT(graphics != NULL);
    graphics->polygon_mode = polygon_mode;
}



void dvz_graphics_cull_mode(DvzGraphics* graphics, VkCullModeFlags cull_mode)
{
    ASSERT(graphics != NULL);
    graphics->cull_mode = cull_mode;
}



void dvz_graphics_front_face(DvzGraphics* graphics, VkFrontFace front_face)
{
    ASSERT(graphics != NULL);
    graphics->front_face = front_face;
}



void dvz_graphics_slot(DvzGraphics* graphics, uint32_t idx, VkDescriptorType type)
{
    ASSERT(graphics != NULL);
    dvz_slots_binding(&graphics->slots, idx, type);
}



void dvz_graphics_push(
    DvzGraphics* graphics, VkDeviceSize offset, VkDeviceSize size, VkShaderStageFlags shaders)
{
    ASSERT(graphics != NULL);
    dvz_slots_push(&graphics->slots, offset, size, shaders);
}



void dvz_graphics_create(DvzGraphics* graphics)
{
    ASSERT(graphics != NULL);
    ASSERT(graphics->gpu != NULL);
    ASSERT(graphics->gpu->device != VK_NULL_HANDLE);
    ASSERT(graphics->renderpass != NULL);
    if (!dvz_obj_is_created(&graphics->slots.obj))
        dvz_slots_create(&graphics->slots);

    log_trace("starting creation of graphics pipeline...");

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {0};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    // Vertex bindings.
    VkVertexInputBindingDescription bindings_info[DVZ_MAX_VERTEX_BINDINGS] = {0};
    for (uint32_t i = 0; i < graphics->vertex_binding_count; i++)
    {
        bindings_info[i].binding = graphics->vertex_bindings[i].binding;
        bindings_info[i].stride = graphics->vertex_bindings[i].stride;
        bindings_info[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    }
    vertex_input_info.vertexBindingDescriptionCount = graphics->vertex_binding_count;
    vertex_input_info.pVertexBindingDescriptions = bindings_info;

    // Vertex attributes.
    VkVertexInputAttributeDescription attrs_info[DVZ_MAX_VERTEX_ATTRS] = {0};
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
    VkPipelineShaderStageCreateInfo shader_stages[DVZ_MAX_SHADERS_PER_GRAPHICS] = {0};
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
    VkPipelineRasterizationStateCreateInfo rasterizer =
        create_rasterizer(graphics->cull_mode, graphics->front_face);
    VkPipelineMultisampleStateCreateInfo multisampling = create_multisampling();

    // Blend attachments.
    VkPipelineColorBlendAttachmentState color_attachment = create_color_blend_attachment(true);
    VkPipelineColorBlendAttachmentState pick_attachment = create_color_blend_attachment(false);
    VkPipelineColorBlendStateCreateInfo color_blending = create_color_blending(
        graphics->support_pick ? 2 : 1,
        (VkPipelineColorBlendAttachmentState[]){color_attachment, pick_attachment});

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
    if (graphics->pipeline != VK_NULL_HANDLE)
    {
        log_trace("graphics pipeline created");
        dvz_obj_created(&graphics->obj);
    }
    else
    {
        graphics->obj.status = DVZ_OBJECT_STATUS_INVALID;
    }
}



void dvz_graphics_destroy(DvzGraphics* graphics)
{
    ASSERT(graphics != NULL);
    if (graphics->obj.status <= DVZ_OBJECT_STATUS_INIT || graphics->gpu == NULL)
    {
        // log_trace("skip destruction of already-destroyed graphics");
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
    if (dvz_obj_is_created(&graphics->slots.obj))
        dvz_slots_destroy(&graphics->slots);

    dvz_obj_destroyed(&graphics->obj);
}



/*************************************************************************************************/
/*  Barrier                                                                                      */
/*************************************************************************************************/

DvzBarrier dvz_barrier(DvzGpu* gpu)
{
    ASSERT(gpu != NULL);
    ASSERT(dvz_obj_is_created(&gpu->obj));

    DvzBarrier barrier = {0};
    barrier.gpu = gpu;
    return barrier;
}



void dvz_barrier_stages(
    DvzBarrier* barrier, VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage)
{
    ASSERT(barrier != NULL);
    barrier->src_stage = src_stage;
    barrier->dst_stage = dst_stage;
}



void dvz_barrier_buffer(DvzBarrier* barrier, DvzBufferRegions br)
{
    ASSERT(barrier != NULL);
    DvzBarrierBuffer* b = &barrier->buffer_barriers[barrier->buffer_barrier_count++];
    b->br = br;
}



void dvz_barrier_buffer_queue(DvzBarrier* barrier, uint32_t src_queue, uint32_t dst_queue)
{
    ASSERT(barrier != NULL);

    DvzBarrierBuffer* b = &barrier->buffer_barriers[barrier->buffer_barrier_count - 1];
    ASSERT(b->br.buffer != NULL);

    b->queue_transfer = true;
    b->src_queue = src_queue;
    b->dst_queue = dst_queue;
}



void dvz_barrier_buffer_access(
    DvzBarrier* barrier, VkAccessFlags src_access, VkAccessFlags dst_access)
{
    ASSERT(barrier != NULL);

    DvzBarrierBuffer* b = &barrier->buffer_barriers[barrier->buffer_barrier_count - 1];
    ASSERT(b->br.buffer != NULL);

    b->src_access = src_access;
    b->dst_access = dst_access;
}



void dvz_barrier_images(DvzBarrier* barrier, DvzImages* img)
{
    ASSERT(barrier != NULL);

    DvzBarrierImage* b = &barrier->image_barriers[barrier->image_barrier_count++];

    b->images = img;
}



void dvz_barrier_images_layout(
    DvzBarrier* barrier, VkImageLayout src_layout, VkImageLayout dst_layout)
{
    ASSERT(barrier != NULL);

    DvzBarrierImage* b = &barrier->image_barriers[barrier->image_barrier_count - 1];
    ASSERT(b->images != NULL);

    b->src_layout = src_layout;
    b->dst_layout = dst_layout;
}



void dvz_barrier_images_access(
    DvzBarrier* barrier, VkAccessFlags src_access, VkAccessFlags dst_access)
{
    ASSERT(barrier != NULL);

    DvzBarrierImage* b = &barrier->image_barriers[barrier->image_barrier_count - 1];
    ASSERT(b->images != NULL);

    b->src_access = src_access;
    b->dst_access = dst_access;
}



void dvz_barrier_images_queue(DvzBarrier* barrier, uint32_t src_queue, uint32_t dst_queue)
{
    ASSERT(barrier != NULL);

    DvzBarrierImage* b = &barrier->image_barriers[barrier->image_barrier_count - 1];
    ASSERT(b->images != NULL);

    b->queue_transfer = true;
    b->src_queue = src_queue;
    b->dst_queue = dst_queue;
}



/*************************************************************************************************/
/*  Semaphores                                                                                   */
/*************************************************************************************************/

DvzSemaphores dvz_semaphores(DvzGpu* gpu, uint32_t count)
{
    ASSERT(gpu != NULL);
    ASSERT(dvz_obj_is_created(&gpu->obj));

    ASSERT(count > 0);
    log_trace("create set of %d semaphore(s)", count);

    DvzSemaphores semaphores = {0};
    semaphores.gpu = gpu;
    semaphores.count = count;

    VkSemaphoreCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for (uint32_t i = 0; i < count; i++)
        VK_CHECK_RESULT(vkCreateSemaphore(gpu->device, &info, NULL, &semaphores.semaphores[i]));

    dvz_obj_created(&semaphores.obj);

    return semaphores;
}



void dvz_semaphores_recreate(DvzSemaphores* semaphores)
{
    ASSERT(semaphores != NULL);
    if (!dvz_obj_is_created(&semaphores->obj))
    {
        log_trace("skip destruction of already-destroyed semaphores");
        return;
    }
    DvzGpu* gpu = semaphores->gpu;
    ASSERT(gpu != NULL);

    ASSERT(semaphores->count > 0);
    log_trace("recreate set of %d semaphore(s)", semaphores->count);

    VkSemaphoreCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for (uint32_t i = 0; i < semaphores->count; i++)
    {
        if (semaphores->semaphores[i] != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(gpu->device, semaphores->semaphores[i], NULL);
            VK_CHECK_RESULT(
                vkCreateSemaphore(gpu->device, &info, NULL, &semaphores->semaphores[i]));
        }
    }
}



void dvz_semaphores_destroy(DvzSemaphores* semaphores)
{
    ASSERT(semaphores != NULL);
    if (!dvz_obj_is_created(&semaphores->obj))
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
    dvz_obj_destroyed(&semaphores->obj);
}



/*************************************************************************************************/
/*  Fences                                                                                       */
/*************************************************************************************************/

DvzFences dvz_fences(DvzGpu* gpu, uint32_t count, bool signaled)
{
    ASSERT(gpu != NULL);
    ASSERT(dvz_obj_is_created(&gpu->obj));

    DvzFences fences = {0};

    ASSERT(count > 0);
    log_trace("create set of %d fences(s)", count);

    fences.gpu = gpu;
    fences.count = count;

    VkFenceCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (signaled)
        info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < fences.count; i++)
        VK_CHECK_RESULT(vkCreateFence(fences.gpu->device, &info, NULL, &fences.fences[i]));

    dvz_obj_created(&fences.obj);
    return fences;
}



void dvz_fences_copy(
    DvzFences* src_fences, uint32_t src_idx, DvzFences* dst_fences, uint32_t dst_idx)
{
    ASSERT(src_fences != NULL);
    ASSERT(dst_fences != NULL);

    ASSERT(src_idx < src_fences->count);
    ASSERT(dst_idx < dst_fences->count);

    // Wait for the destination fence first (if it is not null).
    // dvz_fences_wait(dst_fences, dst_idx);

    // log_trace("copy fence %d to %d", src_fences->fences[src_idx], dst_fences->fences[dst_idx]);
    dst_fences->fences[dst_idx] = src_fences->fences[src_idx];
}



void dvz_fences_wait(DvzFences* fences, uint32_t idx)
{
    ASSERT(fences != NULL);
    ASSERT(idx < fences->count);
    if (fences->fences[idx] != VK_NULL_HANDLE)
    {
        // log_trace("wait for fence %u", fences->fences[idx]);
        // dvz_fences_ready(fences, idx));
        vkWaitForFences(fences->gpu->device, 1, &fences->fences[idx], VK_TRUE, 1000000000);
        // log_trace("fence wait finished!");
    }
    else
    {
        log_trace("skip wait for fence %u", fences->fences[idx]);
    }
}



bool dvz_fences_ready(DvzFences* fences, uint32_t idx)
{
    ASSERT(fences != NULL);
    ASSERT(idx < fences->count);
    ASSERT(fences->fences[idx] != VK_NULL_HANDLE);
    VkResult res = vkGetFenceStatus(fences->gpu->device, fences->fences[idx]);
    if (res == VK_SUCCESS)
        return true;
    return false;
}



void dvz_fences_reset(DvzFences* fences, uint32_t idx)
{
    ASSERT(fences != NULL);
    if (fences->fences[idx] != NULL)
    {
        // log_trace("reset fence %d", fences->fences[idx]);
        vkResetFences(fences->gpu->device, 1, &fences->fences[idx]);
    }
}



void dvz_fences_destroy(DvzFences* fences)
{
    ASSERT(fences != NULL);
    if (!dvz_obj_is_created(&fences->obj))
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
    dvz_obj_destroyed(&fences->obj);
}



/*************************************************************************************************/
/*  Renderpass                                                                                   */
/*************************************************************************************************/

DvzRenderpass dvz_renderpass(DvzGpu* gpu)
{
    ASSERT(gpu != NULL);
    ASSERT(dvz_obj_is_created(&gpu->obj));

    DvzRenderpass renderpass = {0};
    renderpass.gpu = gpu;

    return renderpass;
}



void dvz_renderpass_clear(DvzRenderpass* renderpass, VkClearValue value)
{
    ASSERT(renderpass != NULL);
    renderpass->clear_values[renderpass->clear_count++] = value;
}



void dvz_renderpass_attachment(
    DvzRenderpass* renderpass, uint32_t idx, DvzRenderpassAttachmentType type, VkFormat format,
    VkImageLayout ref_layout)
{
    ASSERT(renderpass != NULL);
    renderpass->attachments[idx].ref_layout = ref_layout;
    renderpass->attachments[idx].type = type;
    renderpass->attachments[idx].format = format;
    renderpass->attachment_count = MAX(renderpass->attachment_count, idx + 1);
}



void dvz_renderpass_attachment_layout(
    DvzRenderpass* renderpass, uint32_t idx, VkImageLayout src_layout, VkImageLayout dst_layout)
{
    ASSERT(renderpass != NULL);
    renderpass->attachments[idx].src_layout = src_layout;
    renderpass->attachments[idx].dst_layout = dst_layout;
    renderpass->attachment_count = MAX(renderpass->attachment_count, idx + 1);
}



void dvz_renderpass_attachment_ops(
    DvzRenderpass* renderpass, uint32_t idx, //
    VkAttachmentLoadOp load_op, VkAttachmentStoreOp store_op)
{
    ASSERT(renderpass != NULL);
    renderpass->attachments[idx].load_op = load_op;
    renderpass->attachments[idx].store_op = store_op;
    renderpass->attachment_count = MAX(renderpass->attachment_count, idx + 1);
}



void dvz_renderpass_subpass_attachment(
    DvzRenderpass* renderpass, uint32_t subpass_idx, uint32_t attachment_idx)
{
    ASSERT(renderpass != NULL);
    renderpass->subpasses[subpass_idx]
        .attachments[renderpass->subpasses[subpass_idx].attachment_count++] = attachment_idx;
    renderpass->subpass_count = MAX(renderpass->subpass_count, subpass_idx + 1);
}



void dvz_renderpass_subpass_dependency(
    DvzRenderpass* renderpass, uint32_t dependency_idx, //
    uint32_t src_subpass, uint32_t dst_subpass)
{
    ASSERT(renderpass != NULL);
    renderpass->dependencies[dependency_idx].src_subpass = src_subpass;
    renderpass->dependencies[dependency_idx].dst_subpass = dst_subpass;
    renderpass->dependency_count = MAX(renderpass->dependency_count, dependency_idx + 1);
}



void dvz_renderpass_subpass_dependency_access(
    DvzRenderpass* renderpass, uint32_t dependency_idx, //
    VkAccessFlags src_access, VkAccessFlags dst_access)
{
    ASSERT(renderpass != NULL);
    renderpass->dependencies[dependency_idx].src_access = src_access;
    renderpass->dependencies[dependency_idx].dst_access = dst_access;
}



void dvz_renderpass_subpass_dependency_stage(
    DvzRenderpass* renderpass, uint32_t dependency_idx, //
    VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage)
{
    ASSERT(renderpass != NULL);
    renderpass->dependencies[dependency_idx].src_stage = src_stage;
    renderpass->dependencies[dependency_idx].dst_stage = dst_stage;
}



void dvz_renderpass_create(DvzRenderpass* renderpass)
{
    ASSERT(renderpass != NULL);

    ASSERT(renderpass->gpu != NULL);
    ASSERT(renderpass->gpu->device != VK_NULL_HANDLE);
    log_trace("starting creation of renderpass...");

    // Attachments.
    VkAttachmentDescription attachments[DVZ_MAX_ATTACHMENTS_PER_RENDERPASS] = {0};
    VkAttachmentReference attachment_refs[DVZ_MAX_ATTACHMENTS_PER_RENDERPASS] = {0};
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
    VkSubpassDescription subpasses[DVZ_MAX_SUBPASSES_PER_RENDERPASS] = {0};
    VkAttachmentReference attachment_refs_matrix[DVZ_MAX_ATTACHMENTS_PER_RENDERPASS]
                                                [DVZ_MAX_ATTACHMENTS_PER_RENDERPASS] = {0};
    uint32_t attachment = 0;
    uint32_t k = 0;
    for (uint32_t i = 0; i < renderpass->subpass_count; i++) // i is the subpass index
    {
        k = 0;
        subpasses[i].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        // j is the attachment index
        for (uint32_t j = 0; j < renderpass->subpasses[i].attachment_count; j++)
        {
            attachment = renderpass->subpasses[i].attachments[j];
            ASSERT(attachment < renderpass->attachment_count);
            if (renderpass->attachments[attachment].type == DVZ_RENDERPASS_ATTACHMENT_DEPTH)
            {
                subpasses[i].pDepthStencilAttachment = &attachment_refs[j];
            }
            else
            {
                attachment_refs_matrix[i][k++] =
                    create_attachment_ref(j, renderpass->attachments[i].ref_layout);
            }
        }
        subpasses[i].colorAttachmentCount = k;
        subpasses[i].pColorAttachments = attachment_refs_matrix[i];
    }

    // Dependencies.
    VkSubpassDependency dependencies[DVZ_MAX_DEPENDENCIES_PER_RENDERPASS] = {0};
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
    dvz_obj_created(&renderpass->obj);
}



void dvz_renderpass_destroy(DvzRenderpass* renderpass)
{
    ASSERT(renderpass != NULL);
    if (!dvz_obj_is_created(&renderpass->obj))
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

    dvz_obj_destroyed(&renderpass->obj);
}



/*************************************************************************************************/
/*  Framebuffers                                                                                 */
/*************************************************************************************************/

DvzFramebuffers dvz_framebuffers(DvzGpu* gpu)
{
    ASSERT(gpu != NULL);
    ASSERT(dvz_obj_is_created(&gpu->obj));

    DvzFramebuffers framebuffers = {0};
    framebuffers.gpu = gpu;

    return framebuffers;
}



void dvz_framebuffers_attachment(
    DvzFramebuffers* framebuffers, uint32_t attachment_idx, DvzImages* img)
{
    ASSERT(framebuffers != NULL);

    ASSERT(img != NULL);
    ASSERT(img->count > 0);
    ASSERT(img->shape[0] > 0);
    ASSERT(img->shape[1] > 0);

    ASSERT(attachment_idx < DVZ_MAX_ATTACHMENTS_PER_RENDERPASS);
    framebuffers->attachment_count = MAX(framebuffers->attachment_count, attachment_idx + 1);
    framebuffers->attachments[attachment_idx] = img;

    framebuffers->framebuffer_count = MAX(framebuffers->framebuffer_count, img->count);
}



static void _framebuffers_create(DvzFramebuffers* framebuffers)
{
    DvzRenderpass* renderpass = framebuffers->renderpass;
    ASSERT(renderpass != NULL);

    // The actual framebuffer size in pixels is determined by the first attachment (color images)
    // as these images are created by the swapchain.
    ASSERT(framebuffers->attachment_count > 0);
    uint32_t width = framebuffers->attachments[0]->shape[0];
    uint32_t height = framebuffers->attachments[0]->shape[1];
    log_trace(
        "create %d framebuffer(s) with size %dx%d", framebuffers->framebuffer_count, width,
        height);

    // Loop first over the framebuffers (swapchain images).
    for (uint32_t i = 0; i < framebuffers->framebuffer_count; i++)
    {
        DvzImages* img = NULL;
        VkImageView attachments[DVZ_MAX_ATTACHMENTS_PER_RENDERPASS] = {0};

        // Loop over the attachments.
        for (uint32_t j = 0; j < framebuffers->attachment_count; j++)
        {
            img = framebuffers->attachments[j];
            attachments[j] = img->image_views[MIN(i, img->count - 1)];
        }
        ASSERT(img != NULL);

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



static void _framebuffers_destroy(DvzFramebuffers* framebuffers)
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



void dvz_framebuffers_create(DvzFramebuffers* framebuffers, DvzRenderpass* renderpass)
{
    ASSERT(framebuffers != NULL);

    ASSERT(framebuffers->gpu != NULL);
    ASSERT(framebuffers->gpu->device != VK_NULL_HANDLE);

    ASSERT(renderpass != NULL);
    ASSERT(dvz_obj_is_created(&renderpass->obj));

    framebuffers->renderpass = renderpass;

    ASSERT(framebuffers->attachment_count > 0);
    ASSERT(framebuffers->framebuffer_count > 0);

    ASSERT(renderpass->attachment_count > 0);
    ASSERT(renderpass->attachment_count == framebuffers->attachment_count);

    // Create the framebuffers.
    log_trace("starting creation of %d framebuffer(s)", framebuffers->framebuffer_count);
    _framebuffers_create(framebuffers);
    log_trace("framebuffers created");
    dvz_obj_created(&framebuffers->obj);
}



void dvz_framebuffers_destroy(DvzFramebuffers* framebuffers)
{
    ASSERT(framebuffers != NULL);
    if (!dvz_obj_is_created(&framebuffers->obj))
    {
        log_trace("skip destruction of already-destroyed framebuffers");
        return;
    }
    log_trace("destroying %d framebuffers", framebuffers->framebuffer_count);
    _framebuffers_destroy(framebuffers);
    dvz_obj_destroyed(&framebuffers->obj);
}



/*************************************************************************************************/
/*  Submit                                                                                       */
/*************************************************************************************************/

DvzSubmit dvz_submit(DvzGpu* gpu)
{
    ASSERT(gpu != NULL);
    ASSERT(dvz_obj_is_created(&gpu->obj));

    DvzSubmit submit = {0};
    submit.gpu = gpu;

    return submit;
}



void dvz_submit_commands(DvzSubmit* submit, DvzCommands* commands)
{
    ASSERT(submit != NULL);
    ASSERT(commands != NULL);

    uint32_t n = submit->commands_count;
    ASSERT(n < DVZ_MAX_COMMANDS_PER_SUBMIT);
    // log_trace("adding commands #%d to submit", n);
    submit->commands[n] = commands;
    submit->commands_count++;
}



void dvz_submit_wait_semaphores(
    DvzSubmit* submit, VkPipelineStageFlags stage, DvzSemaphores* semaphores, uint32_t idx)
{
    ASSERT(submit != NULL);
    ASSERT(semaphores != NULL);

    ASSERT(idx < semaphores->count);
    ASSERT(idx < DVZ_MAX_SEMAPHORES_PER_SET);
    uint32_t n = submit->wait_semaphores_count;
    ASSERT(n < DVZ_MAX_SEMAPHORES_PER_SUBMIT);

    ASSERT(semaphores->semaphores[idx] != VK_NULL_HANDLE);

    submit->wait_semaphores[n] = semaphores;
    submit->wait_stages[n] = stage;
    submit->wait_semaphores_idx[n] = idx;

    submit->wait_semaphores_count++;
}



void dvz_submit_signal_semaphores(DvzSubmit* submit, DvzSemaphores* semaphores, uint32_t idx)
{
    ASSERT(submit != NULL);

    ASSERT(idx < DVZ_MAX_SEMAPHORES_PER_SET);
    uint32_t n = submit->signal_semaphores_count;
    ASSERT(n < DVZ_MAX_SEMAPHORES_PER_SUBMIT);

    submit->signal_semaphores[n] = semaphores;
    submit->signal_semaphores_idx[n] = idx;

    submit->signal_semaphores_count++;
}



void dvz_submit_send(DvzSubmit* submit, uint32_t cmd_idx, DvzFences* fences, uint32_t fence_idx)
{
    ASSERT(submit != NULL);
    // log_trace("starting command buffer submission...");

    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[DVZ_MAX_SEMAPHORES_PER_SUBMIT] = {0};
    for (uint32_t i = 0; i < submit->wait_semaphores_count; i++)
    {
        wait_semaphores[i] =
            submit->wait_semaphores[i]->semaphores[submit->wait_semaphores_idx[i]];
        // log_trace("wait for semaphore %d", wait_semaphores[i]);
        ASSERT(submit->wait_stages[i] != VK_NULL_HANDLE);
    }

    VkSemaphore signal_semaphores[DVZ_MAX_SEMAPHORES_PER_SUBMIT] = {0};
    for (uint32_t i = 0; i < submit->signal_semaphores_count; i++)
    {
        signal_semaphores[i] =
            submit->signal_semaphores[i]->semaphores[submit->signal_semaphores_idx[i]];
        // log_trace("signal semaphore %d", signal_semaphores[i]);
    }

    VkCommandBuffer cmd_bufs[DVZ_MAX_COMMANDS_PER_SUBMIT] = {0};

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

    VkFence vfence = fences == NULL ? 0 : fences->fences[fence_idx];

    if (vfence != VK_NULL_HANDLE)
    {
        dvz_fences_wait(fences, fence_idx);
        dvz_fences_reset(fences, fence_idx);
    }
    // log_trace(
    //     "submit queue with %d cmd bufs (%d) and signal fence %d", submit->commands_count,
    //     cmd_idx, vfence);
    VK_CHECK_RESULT(vkQueueSubmit(submit->gpu->queues.queues[queue_idx], 1, &submit_info, vfence));

    // log_trace("submit done");
}



void dvz_submit_reset(DvzSubmit* submit)
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

void dvz_cmd_begin_renderpass(
    DvzCommands* cmds, uint32_t idx, DvzRenderpass* renderpass, DvzFramebuffers* framebuffers)
{
    ASSERT(renderpass != NULL);
    ASSERT(framebuffers != NULL);

    ASSERT(dvz_obj_is_created(&renderpass->obj));
    ASSERT(dvz_obj_is_created(&framebuffers->obj));
    ASSERT(renderpass->renderpass != VK_NULL_HANDLE);

    // Find the framebuffer size.
    ASSERT(framebuffers->attachment_count > 0);
    uint32_t width = framebuffers->attachments[0]->shape[0];
    uint32_t height = framebuffers->attachments[0]->shape[1];
    // log_trace("begin renderpass with size %dx%d", width, height);

    CMD_START_CLIP(cmds->count)
    ASSERT(framebuffers->framebuffers[iclip] != VK_NULL_HANDLE);
    begin_render_pass(
        renderpass->renderpass, cb, framebuffers->framebuffers[iclip], //
        width, height, renderpass->clear_count, renderpass->clear_values);
    CMD_END
}



void dvz_cmd_end_renderpass(DvzCommands* cmds, uint32_t idx)
{
    CMD_START
    vkCmdEndRenderPass(cb);
    CMD_END
}



void dvz_cmd_compute(DvzCommands* cmds, uint32_t idx, DvzCompute* compute, uvec3 size)
{
    ASSERT(compute->bindings != NULL);
    ASSERT(compute->bindings->dsets != NULL);
    ASSERT(compute->pipeline != VK_NULL_HANDLE);
    ASSERT(compute->slots.pipeline_layout != VK_NULL_HANDLE);
    ASSERT(size[0] > 0);
    ASSERT(size[1] > 0);
    ASSERT(size[2] > 0);

    CMD_START

    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, compute->pipeline);
    vkCmdBindDescriptorSets(
        cb, VK_PIPELINE_BIND_POINT_COMPUTE, compute->slots.pipeline_layout, 0, 1,
        compute->bindings->dsets, 0, 0);
    vkCmdDispatch(cb, size[0], size[1], size[2]);
    CMD_END
}



void dvz_cmd_barrier(DvzCommands* cmds, uint32_t idx, DvzBarrier* barrier)
{
    ASSERT(barrier != NULL);
    DvzQueues* q = &cmds->gpu->queues;
    CMD_START

    // Buffer barriers
    VkBufferMemoryBarrier buffer_barriers[DVZ_MAX_BARRIERS_PER_SET] = {0};
    VkBufferMemoryBarrier* buffer_barrier = NULL;
    DvzBarrierBuffer* buffer_info = NULL;

    for (uint32_t j = 0; j < barrier->buffer_barrier_count; j++)
    {
        buffer_barrier = &buffer_barriers[j];
        buffer_info = &barrier->buffer_barriers[j];

        buffer_barrier->sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        buffer_barrier->buffer = buffer_info->br.buffer->buffer;
        buffer_barrier->size = buffer_info->br.size;
        buffer_barrier->offset = buffer_info->br.offsets[MIN(i, cmds->count - 1)];

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
    VkImageMemoryBarrier image_barriers[DVZ_MAX_BARRIERS_PER_SET] = {0};
    VkImageMemoryBarrier* image_barrier = NULL;
    DvzBarrierImage* image_info = NULL;

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



static VkBufferImageCopy
_image_buffer_copy(DvzImages* img, VkDeviceSize buf_offset, uvec3 tex_offset, uvec3 shape)
{
    ASSERT(img != NULL);

    ASSERT(shape[0] > 0);
    ASSERT(shape[1] > 0);
    ASSERT(shape[2] > 0);

    for (uint32_t i = 0; i < 3; i++)
    {
        ASSERT(tex_offset[i] + shape[i] <= img->shape[i]);
    }

    VkBufferImageCopy region = {0};
    region.bufferOffset = buf_offset;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset.x = (int32_t)tex_offset[0];
    region.imageOffset.y = (int32_t)tex_offset[1];
    region.imageOffset.z = (int32_t)tex_offset[2];

    region.imageExtent.width = shape[0];
    region.imageExtent.height = shape[1];
    region.imageExtent.depth = shape[2];

    return region;
}

void dvz_cmd_copy_buffer_to_image(
    DvzCommands* cmds, uint32_t idx,            //
    DvzBuffer* buffer, VkDeviceSize buf_offset, //
    DvzImages* img, uvec3 tex_offset, uvec3 shape)
{
    ASSERT(cmds != NULL);
    ASSERT(buffer != NULL);

    CMD_START_CLIP(img->count)
    VkBufferImageCopy region = _image_buffer_copy(img, buf_offset, tex_offset, shape);
    vkCmdCopyBufferToImage(
        cb, buffer->buffer, img->images[iclip], //
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    CMD_END
}

void dvz_cmd_copy_image_to_buffer(
    DvzCommands* cmds, uint32_t idx,               //
    DvzImages* img, uvec3 tex_offset, uvec3 shape, //
    DvzBuffer* buffer, VkDeviceSize buf_offset     //
)
{
    ASSERT(cmds != NULL);
    ASSERT(buffer != NULL);

    CMD_START_CLIP(img->count)
    VkBufferImageCopy region = _image_buffer_copy(img, buf_offset, tex_offset, shape);
    vkCmdCopyImageToBuffer(
        cb, img->images[iclip], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, //
        buffer->buffer, 1, &region);
    CMD_END
}



void dvz_cmd_copy_image_region(
    DvzCommands* cmds, uint32_t idx,      //
    DvzImages* src_img, ivec3 src_offset, //
    DvzImages* dst_img, ivec3 dst_offset, //
    uvec3 shape)
{
    ASSERT(src_img != NULL);
    ASSERT(dst_img != NULL);

    for (uint32_t i = 0; i < 3; i++)
    {
        ASSERT(src_offset[i] + (int)shape[i] <= (int)src_img->shape[i]);
        ASSERT(dst_offset[i] + (int)shape[i] <= (int)dst_img->shape[i]);
    }

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

    imageCopyRegion.srcOffset.x = src_offset[0];
    imageCopyRegion.srcOffset.y = src_offset[1];
    imageCopyRegion.srcOffset.z = src_offset[2];

    imageCopyRegion.dstOffset.x = dst_offset[0];
    imageCopyRegion.dstOffset.y = dst_offset[1];
    imageCopyRegion.dstOffset.z = dst_offset[2];

    imageCopyRegion.extent.width = shape[0];
    imageCopyRegion.extent.height = shape[1];
    imageCopyRegion.extent.depth = shape[2];
    vkCmdCopyImage(
        cb,                                                        //
        src_img->images[i0], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, //
        dst_img->images[i1], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, //
        1, &imageCopyRegion);
    CMD_END
}



void dvz_cmd_copy_image(DvzCommands* cmds, uint32_t idx, DvzImages* src_img, DvzImages* dst_img)
{
    dvz_cmd_copy_image_region(
        cmds, idx, src_img, (ivec3){0, 0, 0}, dst_img, (ivec3){0, 0, 0}, src_img->shape);
}



void dvz_cmd_viewport(DvzCommands* cmds, uint32_t idx, VkViewport viewport)
{
    CMD_START
    vkCmdSetViewport(cb, 0, 1, &viewport);
    VkRect2D scissor = {
        {viewport.x, viewport.y}, {(uint32_t)viewport.width, (uint32_t)viewport.height}};
    vkCmdSetScissor(cb, 0, 1, &scissor);
    CMD_END
}



void dvz_cmd_bind_graphics(
    DvzCommands* cmds, uint32_t idx, DvzGraphics* graphics, //
    DvzBindings* bindings, uint32_t dynamic_idx)
{
    ASSERT(graphics != NULL);
    DvzSlots* slots = &graphics->slots;
    ASSERT(slots != NULL);
    ASSERT(bindings != NULL);

    // Count the number of dynamic uniforms.
    uint32_t dyn_count = 0;
    uint32_t dyn_offsets[DVZ_MAX_BINDINGS_SIZE] = {0};
    ASSERT(slots->slot_count <= DVZ_MAX_BINDINGS_SIZE);
    for (uint32_t i = 0; i < slots->slot_count; i++)
    {
        if (slots->types[i] == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
        {
            ASSERT(bindings->br[i].aligned_size > 0);
            dyn_offsets[dyn_count++] = dynamic_idx * bindings->br[i].aligned_size;
        }
    }

    CMD_START_CLIP(bindings->dset_count)
    if (dvz_obj_is_created(&graphics->obj))
        vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics->pipeline);
    vkCmdBindDescriptorSets(
        cb, VK_PIPELINE_BIND_POINT_GRAPHICS, slots->pipeline_layout, //
        0, 1, &bindings->dsets[iclip], dyn_count, dyn_offsets);
    CMD_END
}



void dvz_cmd_bind_vertex_buffer(
    DvzCommands* cmds, uint32_t idx, DvzBufferRegions br, VkDeviceSize offset)
{
    CMD_START_CLIP(br.count)
    VkDeviceSize offsets[] = {br.offsets[iclip] + offset};
    vkCmdBindVertexBuffers(cb, 0, 1, &br.buffer->buffer, offsets);
    CMD_END
}



void dvz_cmd_bind_index_buffer(
    DvzCommands* cmds, uint32_t idx, DvzBufferRegions br, VkDeviceSize offset)
{
    CMD_START_CLIP(br.count)
    vkCmdBindIndexBuffer(cb, br.buffer->buffer, br.offsets[iclip] + offset, VK_INDEX_TYPE_UINT32);
    CMD_END
}



void dvz_cmd_draw(DvzCommands* cmds, uint32_t idx, uint32_t first_vertex, uint32_t vertex_count)
{
    ASSERT(vertex_count > 0);
    CMD_START
    vkCmdDraw(cb, vertex_count, 1, first_vertex, 0);
    CMD_END
}



void dvz_cmd_draw_indexed(
    DvzCommands* cmds, uint32_t idx, uint32_t first_index, uint32_t vertex_offset,
    uint32_t index_count)
{
    ASSERT(index_count > 0);
    CMD_START
    vkCmdDrawIndexed(cb, index_count, 1, first_index, (int32_t)vertex_offset, 0);
    CMD_END
}



void dvz_cmd_draw_indirect(DvzCommands* cmds, uint32_t idx, DvzBufferRegions indirect)
{
    CMD_START_CLIP(indirect.count)
    vkCmdDrawIndirect(cb, indirect.buffer->buffer, indirect.offsets[iclip], 1, 0);
    CMD_END
}



void dvz_cmd_draw_indexed_indirect(DvzCommands* cmds, uint32_t idx, DvzBufferRegions indirect)
{
    CMD_START_CLIP(indirect.count)
    vkCmdDrawIndexedIndirect(cb, indirect.buffer->buffer, indirect.offsets[iclip], 1, 0);
    CMD_END
}



void dvz_cmd_copy_buffer(
    DvzCommands* cmds, uint32_t idx,             //
    DvzBuffer* src_buf, VkDeviceSize src_offset, //
    DvzBuffer* dst_buf, VkDeviceSize dst_offset, //
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



void dvz_cmd_push(
    DvzCommands* cmds, uint32_t idx, DvzSlots* slots, VkShaderStageFlagBits shaders, //
    VkDeviceSize offset, VkDeviceSize size, const void* data)
{
    CMD_START
    vkCmdPushConstants(cb, slots->pipeline_layout, shaders, offset, size, data);
    CMD_END
}
