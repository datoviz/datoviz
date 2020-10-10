#include "../include/visky/visky.h"
#include "vkutils.h"
#include <stdlib.h>

BEGIN_INCL_NO_WARN
#include <stb_image.h>
END_INCL_NO_WARN



/*************************************************************************************************/
/*  Helper Vulkan functions                                                                      */
/*************************************************************************************************/

VkSampler create_texture_sampler(
    VkDevice device, VkFilter mag_filter, VkFilter min_filter, VkSamplerAddressMode address_mode)
{
    log_trace("create texture sampler");
    VkSamplerCreateInfo sampler_info = {0};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    sampler_info.magFilter = mag_filter;
    sampler_info.minFilter = min_filter;

    sampler_info.addressModeU = address_mode;
    sampler_info.addressModeV = address_mode;
    sampler_info.addressModeW = address_mode;

    sampler_info.anisotropyEnable = VK_TRUE;
    sampler_info.maxAnisotropy = 16;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 0.0f;

    VkSampler sampler = {0};
    VK_CHECK_RESULT(vkCreateSampler(device, &sampler_info, NULL, &sampler));
    return sampler;
}

void create_buffer(
    VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer* buffer,
    VkDeviceMemory* bufferMemory, VkMemoryPropertyFlags properties,
    VkPhysicalDeviceMemoryProperties memory_properties)
{
    log_trace("create buffer");
    VkBufferCreateInfo bufferInfo = {0};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK_RESULT(vkCreateBuffer(device, &bufferInfo, NULL, buffer));

    VkMemoryRequirements memRequirements = {0};
    vkGetBufferMemoryRequirements(device, *buffer, &memRequirements);

    VkMemoryAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = memRequirements.size;
    alloc_info.memoryTypeIndex =
        find_memory_type(memRequirements.memoryTypeBits, properties, memory_properties);

    VK_CHECK_RESULT(vkAllocateMemory(device, &alloc_info, NULL, bufferMemory));

    vkBindBufferMemory(device, *buffer, *bufferMemory, 0);
}

VkCommandBuffer begin_single_time_commands(VkDevice device, VkCommandPool command_pool)
{
    VkCommandBufferAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = command_pool;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer = {0};
    vkAllocateCommandBuffers(device, &alloc_info, &command_buffer);

    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer, &begin_info);

    return command_buffer;
}

void end_single_time_commands(
    VkDevice device, VkCommandPool command_pool, VkCommandBuffer* command_buffer,
    VkQueue graphics_queue)
{
    vkEndCommandBuffer(*command_buffer);

    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = command_buffer;

    vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue);

    vkFreeCommandBuffers(device, command_pool, 1, command_buffer);
}

void add_image_transition(
    VkCommandBuffer copyCmd, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout,
    VkAccessFlags src_mask, VkAccessFlags dst_mask)
{

    // TODO: refactor with vky_texture_barrier()

    VkImageMemoryBarrier barrier = {0};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    barrier.image = image;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcAccessMask = src_mask;
    barrier.dstAccessMask = dst_mask;
    vkCmdPipelineBarrier(
        copyCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0,
        NULL, 1, &barrier);
}

void create_image(
    VkDevice device, uint32_t width, uint32_t height, uint32_t depth, VkFormat format,
    VkImageTiling tiling, VkImageUsageFlags usage, VkImage* image, VkDeviceMemory* imageMemory,
    VkMemoryPropertyFlags properties, VkPhysicalDeviceMemoryProperties memory_properties)
{
    log_trace("create image");
    ASSERT(width > 0);
    ASSERT(height > 0);
    ASSERT(depth > 0);

    VkImageType image_type = VK_IMAGE_TYPE_2D;
    if (height == 1 && depth == 1)
        image_type = VK_IMAGE_TYPE_1D;
    else if (depth > 1)
        image_type = VK_IMAGE_TYPE_3D;

    VkImageCreateInfo imageInfo = {0};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = image_type;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = depth;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK_RESULT(vkCreateImage(device, &imageInfo, NULL, image));

    VkMemoryRequirements memRequirements = {0};
    vkGetImageMemoryRequirements(device, *image, &memRequirements);

    VkMemoryAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = memRequirements.size;
    alloc_info.memoryTypeIndex =
        find_memory_type(memRequirements.memoryTypeBits, properties, memory_properties);

    VK_CHECK_RESULT(vkAllocateMemory(device, &alloc_info, NULL, imageMemory));

    vkBindImageMemory(device, *image, *imageMemory, 0);
}

VkImageView create_image_view(
    VkDevice device, VkImage image, VkImageViewType image_type, VkFormat format,
    VkImageAspectFlags aspect_flags)
{
    log_trace("create image view");
    VkImageViewCreateInfo viewInfo = {0};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = image_type;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.subresourceRange.aspectMask = aspect_flags;

    VkImageView imageView = {0};
    VK_CHECK_RESULT(vkCreateImageView(device, &viewInfo, NULL, &imageView));

    return imageView;
}



/*************************************************************************************************/
/*  vklite API                                                                                   */
/*************************************************************************************************/

// Validation layers.
const char* layers[] = {"VK_LAYER_KHRONOS_validation"};

// Required device extensions.
const char* device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

void vky_show_fps(VkyCanvas* canvas)
{
    uint64_t fps = vky_get_fps(canvas->frame_count);
    if (fps > 0)
    {
        printf("\rFPS: %d", (int)fps);
        fflush(stdout);
    }
}

VkyQueueFamilyIndices vky_find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    VkyQueueFamilyIndices indices = {0, 0, 0};
    bool graphics_found = false, present_found = false, compute_found = false;

    uint32_t queue_family_count = 0;
    VkQueueFamilyProperties queueFamilies[100];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);
    ASSERT(queue_family_count <= 100);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queueFamilies);

    for (uint32_t i = 0; i < queue_family_count; i++)
    {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphics_family = i;
            graphics_found = true;
        }
        if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            indices.compute_family = i;
            compute_found = true;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport)
        {
            indices.present_family = i;
            present_found = true;
        }

        if (graphics_found && present_found)
        {
            break;
        }
    }

    ASSERT(graphics_found && present_found && compute_found);
    log_trace(
        "queue families: graphics %d, present %d, compute %d", indices.graphics_family,
        indices.present_family, indices.compute_family);

    return indices;
}

VkyGpu vky_create_device(uint32_t required_extension_count, const char** required_extensions)
{
    log_trace("create GPU");
    // TODO: mechanism to specify extensions
    // Also: this function should be glfw independent

    // Add ext debug extension.
    bool has_validation = false;
    if (ENABLE_VALIDATION_LAYERS)
    {
        has_validation = check_validation_layer_support(1, layers);
        if (!has_validation)
            log_error(
                "validation layer support missing, make sure you have exported the environment "
                "variable VK_LAYER_PATH=\"$VULKAN_SDK/etc/vulkan/explicit_layer.d\"");
    }

    uint32_t extension_count = required_extension_count;
    if (has_validation)
    {
        extension_count++;
    }
    ASSERT(extension_count <= 100);
    const char* extensions[100];
    for (uint32_t i = 0; i < required_extension_count; i++)
    {
        extensions[i] = required_extensions[i];
    }
    if (has_validation)
    {
        extensions[required_extension_count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    }


    // Prepare the creation of the Vulkan instance.
    VkApplicationInfo appInfo = {0};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = APPLICATION_NAME;
    appInfo.applicationVersion = APPLICATION_VERSION;
    appInfo.pEngineName = ENGINE_NAME;
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = extension_count;
    createInfo.ppEnabledExtensionNames = extensions;

    // Validation layers.
    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {0};
    debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_create_info.flags = 0;
    debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_create_info.pfnUserCallback = debug_callback;

    if (has_validation)
    {
        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = layers;
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = NULL;
    }

    // Create the Vulkan instance.
    VkInstance instance = {0};
    log_trace("create instance");
    VK_CHECK_RESULT(vkCreateInstance(&createInfo, NULL, &instance));

    // Create the debug utils messenger.
    VkDebugUtilsMessengerEXT debug_messenger = {0};
    if (has_validation)
    {
        log_trace("create debug utils messenger");
        VK_CHECK_RESULT(create_debug_utils_messenger_EXT(
            instance, &debug_create_info, NULL, &debug_messenger));
    }

    // Create the VkyGpu struct.
    VkyGpu gpu = {0};
    gpu.instance = instance;
    if (debug_messenger != 0)
        gpu.debug_messenger = debug_messenger;
    gpu.has_validation = has_validation;

    // Pick the physical device.
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, NULL);
    ASSERT(device_count <= 100);
    VkPhysicalDevice physical_devices[100];
    vkEnumeratePhysicalDevices(instance, &device_count, physical_devices);
    // TODO: select the appropriate GPU instead of just the first one.
    VkPhysicalDevice physical_device = physical_devices[0];

    VkPhysicalDeviceProperties device_properties = {0};
    VkPhysicalDeviceFeatures device_features = {0};
    VkPhysicalDeviceMemoryProperties memory_properties = {0};

    vkGetPhysicalDeviceProperties(physical_device, &device_properties);
    vkGetPhysicalDeviceFeatures(physical_device, &device_features);
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

    gpu.physical_device = physical_device;
    gpu.device_properties = device_properties;
    gpu.device_features = device_features;
    gpu.memory_properties = memory_properties;

    gpu.buffer_count = 0;
    gpu.buffers = calloc(VKY_MAX_BUFFER_COUNT, sizeof(VkyBuffer));
    gpu.texture_count = 0;
    gpu.textures = calloc(VKY_MAX_TEXTURE_COUNT, sizeof(VkyTexture));

    log_debug("successfully created device %s", device_properties.deviceName);

    return gpu;
}

void vky_create_descriptor_pool(VkyGpu* gpu)
{
    // Descriptor pool.
    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};
    VkDescriptorPoolCreateInfo descriptor_pool_info = {0};
    descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptor_pool_info.poolSizeCount = 11;
    descriptor_pool_info.pPoolSizes = poolSizes;
    descriptor_pool_info.maxSets = 1000 * descriptor_pool_info.poolSizeCount;

    // Create descriptor pool.
    log_trace("create descriptor pool");
    VK_CHECK_RESULT(
        vkCreateDescriptorPool(gpu->device, &descriptor_pool_info, NULL, &gpu->descriptor_pool));
}

void vky_create_command_pool(VkyGpu* gpu)
{
    // Create the command pool.
    log_trace("create graphics command pool");
    VkCommandPoolCreateInfo command_pool_info = {0};
    command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_info.queueFamilyIndex = gpu->queue_indices.graphics_family;
    VK_CHECK_RESULT(
        vkCreateCommandPool(gpu->device, &command_pool_info, NULL, &gpu->command_pool));

    log_trace("create compute command pool");
    command_pool_info.queueFamilyIndex = gpu->queue_indices.compute_family;
    VK_CHECK_RESULT(
        vkCreateCommandPool(gpu->device, &command_pool_info, NULL, &gpu->compute_command_pool));
}

void vky_prepare_gpu(VkyGpu* gpu, VkSurfaceKHR* surface)
{
    log_trace("prepare the GPU from a surface");

    // Queue family indices.
    VkyQueueFamilyIndices indices;
    if (surface != NULL)
        indices = vky_find_queue_families(gpu->physical_device, *surface);
    else
        indices = (VkyQueueFamilyIndices){0, 0}; // TODO

    // Queues.
    uint32_t queue_count = 1;
    if (indices.present_family != indices.graphics_family)
    {
        queue_count = 2;
    }
    ASSERT(queue_count <= 100);
    VkDeviceQueueCreateInfo queue_create_infos[100];
    float queue_priority = 1.0f;

    queue_create_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_infos[0].pNext = NULL;
    queue_create_infos[0].flags = 0;
    queue_create_infos[0].queueFamilyIndex = indices.graphics_family;
    queue_create_infos[0].queueCount = 1;
    queue_create_infos[0].pQueuePriorities = &queue_priority;

    if (queue_count > 1)
    {
        queue_create_infos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos[1].pNext = NULL;
        queue_create_infos[1].flags = 0;
        queue_create_infos[1].queueFamilyIndex = indices.present_family;
        queue_create_infos[1].queueCount = 1;
        queue_create_infos[1].pQueuePriorities = &queue_priority;
    }

    VkDeviceCreateInfo device_create_info = {0};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = queue_count;
    device_create_info.pQueueCreateInfos = queue_create_infos;

    // Device features.
    VkPhysicalDeviceFeatures enabled_features = {0};
    enabled_features.samplerAnisotropy = VK_TRUE;
    enabled_features.geometryShader = VK_FALSE;
    device_create_info.pEnabledFeatures = &enabled_features;

    // Device extensions.
    if (surface != NULL)
    {
        device_create_info.enabledExtensionCount = 1;
        device_create_info.ppEnabledExtensionNames = device_extensions;
        gpu->has_graphics = true;
    }
    else
    {
        device_create_info.enabledExtensionCount = 0;
        device_create_info.ppEnabledExtensionNames = NULL;
        gpu->has_graphics = false;
    }

    if (gpu->has_validation)
    {
        device_create_info.enabledLayerCount = 1;
        device_create_info.ppEnabledLayerNames = layers;
    }
    else
    {
        device_create_info.enabledLayerCount = 0;
    }

    // Create the device.
    VkDevice device = {0};
    log_trace("create device");
    VK_CHECK_RESULT(vkCreateDevice(gpu->physical_device, &device_create_info, NULL, &device));

    // Create the queues.
    vkGetDeviceQueue(device, indices.graphics_family, 0, &gpu->graphics_queue);
    vkGetDeviceQueue(device, indices.present_family, 0, &gpu->present_queue);
    vkGetDeviceQueue(device, indices.compute_family, 0, &gpu->compute_queue);

    // TODO: choose image_count dynamically.
    gpu->image_count = 3;
    gpu->image_format = VKY_IMAGE_FORMAT;
    gpu->device = device;
    gpu->queue_indices = indices;

    // Create command pool and descriptor pool.
    vky_create_command_pool(gpu);
    vky_create_descriptor_pool(gpu);

    // Create the compute command buffer.
    log_trace("allocate compute command buffers");
    VkCommandBufferAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = gpu->compute_command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;
    VK_CHECK_RESULT(
        vkAllocateCommandBuffers(gpu->device, &alloc_info, &gpu->compute_command_buffer));
    ASSERT(gpu->compute_command_buffer != 0);

    vky_create_shared_objects(gpu);
}

void vky_create_shared_objects(VkyGpu* gpu)
{
    // The color texture is uploaded only once and is shared by all canvases.
    // It is always the first texture registered.
    vky_get_color_texture(gpu);
    vky_get_font_texture(gpu);

    // Indirect buffer.
    // NOTE: this indirect buffer is the first buffer. Visuals use this fact.
    vky_add_buffer(
        gpu, VKY_MAX_VISUAL_COUNT * sizeof(VkDrawIndexedIndirectCommand),
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
}

VkyCanvas* vky_create_canvas_from_surface(VkyApp* app, void* window, VkSurfaceKHR* surface)
{
    log_trace("create canvas from surface");
    VkyGpu* gpu = app->gpu;
    ASSERT(gpu != NULL);
    // TODO: add dpi_scaling_factor param to create_canvas

    // HACK: avoid following warning:
    // validation layer: vkQueuePresentKHR: Presenting image without calling
    // vkGetPhysicalDeviceSurfaceSupportKHR
    vky_find_queue_families(gpu->physical_device, *surface);

    VkyCanvas canvas = {0};
    canvas.app = app;
    canvas.window = window;
    canvas.gpu = gpu;
    canvas.dpi_factor = VKY_DPI_SCALING_FACTOR;
    canvas.image_count = gpu->image_count;
    canvas.surface = *surface;
    canvas.depth_format = VK_FORMAT_D32_SFLOAT;
    canvas.image_format = gpu->image_format;

    // Create the render pass.
    log_trace("create render pass");
    VkRenderPass render_pass = {0};
    VkFormat image_format = VKY_IMAGE_FORMAT;
    create_render_pass(
        gpu->device, image_format, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, &render_pass, true);
    canvas.render_pass = render_pass;

    // Create the live render pass.
    log_trace("create live render pass");
    VkRenderPass live_render_pass = {0};
    create_render_pass(
        gpu->device, image_format, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, &live_render_pass, false);
    canvas.live_render_pass = live_render_pass;

    // Two per swap chain frame buffer: regular command buffer, and live command buffer (to be
    // updated at every frame).
    canvas.command_buffers = calloc(2 * canvas.image_count, sizeof(VkCommandBuffer));

    // Wait until the window is fully resized after initialization.
    vky_wait_canvas_ready(&canvas);
    // canvas.window_size.lw = canvas.window_size.w;
    // canvas.window_size.lh = canvas.window_size.h;

    // Create the swap chain and all objects that will need to be recreated upon resize.
    vky_create_swapchain_resources(&canvas);

    // Create the command buffers.
    // One command buffer per swap chain framebuffer, + the live command buffers.
    vky_create_command_buffers(canvas.gpu, 2 * canvas.image_count, canvas.command_buffers);
    canvas.live_command_buffers = &canvas.command_buffers[canvas.image_count];

    // Create sync resources.
    vky_create_sync_resources(&canvas);

    VkyCanvas* canvas_ptr = calloc(1, sizeof(VkyCanvas));
    *canvas_ptr = canvas;
    return canvas_ptr;
}

void vky_reset_canvas(VkyCanvas* canvas)
{
    canvas->local_time = 0;
    canvas->dt = 0;
    canvas->frame_count = 0;
    canvas->fps = 0;

    // If a test uses compute, the next may not. This variable will be set to true by
    // any test using compute.
    canvas->gpu->has_compute = false;

    vky_reset_event_controller(canvas->event_controller);
}

void vky_create_sync_resources(VkyCanvas* canvas)
{
    log_trace("create draw sync objects");
    VkyGpu* gpu = canvas->gpu;
    VkDevice device = canvas->gpu->device;

    // Synchronization primitives.
    VkSemaphore* image_available_semaphores =
        calloc(VKY_MAX_FRAMES_IN_FLIGHT, sizeof(VkSemaphore));
    VkSemaphore* render_finished_semaphores =
        calloc(VKY_MAX_FRAMES_IN_FLIGHT, sizeof(VkSemaphore));
    VkFence* in_flight_fences = calloc(VKY_MAX_FRAMES_IN_FLIGHT, sizeof(VkFence));
    VkFence* images_in_flight = calloc(canvas->image_count, sizeof(VkFence));

    VkSemaphoreCreateInfo semaphoreInfo = {0};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {0};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < VKY_MAX_FRAMES_IN_FLIGHT; i++)
    {
        VK_CHECK_RESULT(
            vkCreateSemaphore(device, &semaphoreInfo, NULL, &image_available_semaphores[i]));
        VK_CHECK_RESULT(
            vkCreateSemaphore(device, &semaphoreInfo, NULL, &render_finished_semaphores[i]));
        VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, NULL, &in_flight_fences[i]));
    }

    VkyDrawSync draw_sync = {0};
    draw_sync.image_available_semaphores = image_available_semaphores;
    draw_sync.render_finished_semaphores = render_finished_semaphores;
    draw_sync.in_flight_fences = in_flight_fences;
    draw_sync.images_in_flight = images_in_flight;
    canvas->draw_sync = draw_sync;

    // Create semaphore (used for synchronization between graphics and compute).
    if (gpu->graphics_semaphore == 0)
    {
        log_trace("create graphics and compute semaphores");
        VkSemaphoreCreateInfo semaphoreCreateInfo = {0};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(device, &semaphoreCreateInfo, NULL, &gpu->graphics_semaphore);
        semaphoreCreateInfo = (VkSemaphoreCreateInfo){0};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(device, &semaphoreCreateInfo, NULL, &gpu->compute_semaphore);
    }
}

VkDeviceSize vky_compute_dynamic_alignment(VkyGpu* gpu, size_t size)
{
    return compute_dynamic_alignment(
        size, gpu->device_properties.limits.minUniformBufferOffsetAlignment);
}

void vky_create_swapchain_resources(VkyCanvas* canvas)
{
    VkDevice device = canvas->gpu->device;

    // Swap chain.
    VkSwapchainCreateInfoKHR screateInfo = {0};
    screateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    screateInfo.surface = canvas->surface;
    screateInfo.minImageCount = canvas->image_count;
    screateInfo.imageFormat = canvas->image_format;
    screateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    VkSurfaceCapabilitiesKHR caps = {0};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        canvas->gpu->physical_device, canvas->surface, &caps);

    screateInfo.imageExtent = caps.currentExtent;
    screateInfo.imageArrayLayers = 1;
    screateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    screateInfo.preTransform = caps.currentTransform;
    screateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if (VKY_VSYNC)
    {
        log_trace("enable vsync");
        screateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    }
    else
    {
        log_trace("disable vsync");
        screateInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    }
    screateInfo.clipped = VK_TRUE;

    VkyQueueFamilyIndices indices = canvas->gpu->queue_indices;
    uint32_t queue_family_indices[] = {indices.graphics_family, indices.present_family};
    if (indices.graphics_family != indices.present_family)
    {
        screateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        screateInfo.queueFamilyIndexCount = 2;
        screateInfo.pQueueFamilyIndices = queue_family_indices;
    }
    else
    {
        screateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    VkSwapchainKHR swapchain = {0};
    log_trace("create swapchain");
    VK_CHECK_RESULT(vkCreateSwapchainKHR(device, &screateInfo, NULL, &swapchain));

    // Swap chain images and image views.
    uint32_t image_count = canvas->image_count;
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, NULL);
    VkImage* swap_images = calloc(image_count, sizeof(VkImage));
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, swap_images);
    log_trace("create %d swapchain images", image_count);
    VkImageView* swap_image_views = calloc(image_count, sizeof(VkImageView));
    for (uint32_t i = 0; i < image_count; i++)
    {
        swap_image_views[i] = create_image_view(
            device, swap_images[i], VK_IMAGE_VIEW_TYPE_2D, canvas->image_format,
            VK_IMAGE_ASPECT_COLOR_BIT);
    }

    // Create the depth objects.
    VkImage depth_image = {0};
    VkImageView depth_image_view = {0};
    VkDeviceMemory depth_image_memory = {0};
    create_image(
        device, caps.currentExtent.width, caps.currentExtent.height, 1, canvas->depth_format,
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, &depth_image,
        &depth_image_memory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, canvas->gpu->memory_properties);
    depth_image_view = create_image_view(
        device, depth_image, VK_IMAGE_VIEW_TYPE_2D, canvas->depth_format,
        VK_IMAGE_ASPECT_DEPTH_BIT);

    // Create the frame buffers.
    log_trace("create %d swapchain framebuffers", image_count);
    VkFramebuffer* swap_framebuffers = calloc(image_count, sizeof(VkFramebuffer));
    for (uint32_t i = 0; i < image_count; i++)
    {
        // Create FrameBuffer
        VkImageView attachments[] = {swap_image_views[i], depth_image_view};

        VkFramebufferCreateInfo framebuffer_info = {0};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = canvas->render_pass;
        framebuffer_info.attachmentCount = 2;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = caps.currentExtent.width;
        framebuffer_info.height = caps.currentExtent.height;
        framebuffer_info.layers = 1;

        VK_CHECK_RESULT(
            vkCreateFramebuffer(device, &framebuffer_info, NULL, &swap_framebuffers[i]));
    }

    // Fill the VkyCanvas struct.
    canvas->size.framebuffer_width = caps.currentExtent.width;
    canvas->size.framebuffer_height = caps.currentExtent.height;
    log_trace(
        "create swapchain framebuffer %dx%d", canvas->size.framebuffer_width,
        canvas->size.framebuffer_height);
    canvas->depth_image = depth_image;
    canvas->depth_image_view = depth_image_view;
    canvas->depth_image_memory = depth_image_memory;

    // One per swap image.
    canvas->swapchain = swapchain;
    canvas->images = swap_images;
    canvas->image_views = swap_image_views;
    canvas->framebuffers = swap_framebuffers;
}



/*************************************************************************************************/
/*  Command buffers and render pass                                                              */
/*************************************************************************************************/

void vky_create_command_buffers(
    VkyGpu* gpu, uint32_t command_buffer_count, VkCommandBuffer* command_buffers)
{
    log_trace("create command buffers");

    // Create the command buffers.
    VkCommandBufferAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = gpu->command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = command_buffer_count;

    VK_CHECK_RESULT(vkAllocateCommandBuffers(gpu->device, &alloc_info, command_buffers));
}

void vky_begin_command_buffer(VkCommandBuffer command_buffer, VkyGpu* gpu)
{
    // log_trace("begin command buffer");

    // Fill the command buffer.
    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VK_CHECK_RESULT(vkBeginCommandBuffer(command_buffer, &begin_info));
}

void vky_begin_render_pass(VkCommandBuffer command_buffer, VkyCanvas* canvas, VkyColor clear_color)
{
    // log_trace("begin render pass");

    VkRenderPassBeginInfo render_pass_info = {0};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = canvas->render_pass;
    render_pass_info.framebuffer = canvas->framebuffers[canvas->current_command_buffer_index];
    VkRect2D renderArea = {
        {0, 0}, {canvas->size.framebuffer_width, canvas->size.framebuffer_height}};
    render_pass_info.renderArea = renderArea;

    VkClearValue clear_color_value = {0};
    clear_color_value.color.float32[0] = (float)clear_color.rgb[0] / 255.0f;
    clear_color_value.color.float32[1] = (float)clear_color.rgb[1] / 255.0f;
    clear_color_value.color.float32[2] = (float)clear_color.rgb[2] / 255.0f;
    clear_color_value.color.float32[3] = (float)clear_color.alpha / 255.0f;

    VkClearValue clear_depth_value = {0};
    clear_depth_value.depthStencil.depth = 1.0f;
    clear_depth_value.depthStencil.stencil = 0;

    VkClearValue clear_values[] = {clear_color_value, clear_depth_value};
    render_pass_info.clearValueCount = 2;
    render_pass_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
}

void vky_begin_live_render_pass(VkCommandBuffer command_buffer, VkyCanvas* canvas)
{
    // log_trace("begin live render pass");

    VkRenderPassBeginInfo render_pass_info = {0};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = canvas->live_render_pass;
    render_pass_info.framebuffer = canvas->framebuffers[canvas->current_command_buffer_index];
    VkRect2D renderArea = {
        {0, 0}, {canvas->size.framebuffer_width, canvas->size.framebuffer_height}};
    render_pass_info.renderArea = renderArea;

    vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
}

void vky_end_render_pass(VkCommandBuffer command_buffer, VkyCanvas* canvas)
{
    // log_trace("end render pass");
    vkCmdEndRenderPass(command_buffer);
}

void vky_end_command_buffer(VkCommandBuffer command_buffer, VkyGpu* gpu)
{
    // log_trace("end command buffer");
    VK_CHECK_RESULT(vkEndCommandBuffer(command_buffer));
}

bool is_canvas_offscreen(VkyCanvas* canvas)
{
    VkyBackendType backend = canvas->app->backend;
    return (
        backend == VKY_BACKEND_VIDEO || backend == VKY_BACKEND_SCREENSHOT ||
        backend == VKY_BACKEND_OFFSCREEN);
}

void vky_submit_command_buffers(
    VkyCanvas* canvas, uint32_t command_buffer_count, VkCommandBuffer* command_buffers)
{
    // log_trace("submit command buffers");

    // The application can dynamically change the command buffers creation code
    // in the fill_command_buffer() callback, it then needs to set need_refill=true
    // so that the buffers are recreated at the right moment in the event loop.
    if (canvas->need_refill)
    {
        vky_fill_command_buffers(canvas);
        canvas->need_refill = false;
    }

    // Determine whether there is a compute command buffer.
    bool do_compute = canvas->gpu->has_compute;
    // log_trace("submit %d command buffers, compute: %d", command_buffer_count, do_compute);

    ASSERT(!canvas->need_recreation);

    VkyGpu* gpu = canvas->gpu;
    VkyDrawSync* draw_sync = &canvas->draw_sync;

    // Submit the command buffers.
    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    ASSERT(command_buffer_count > 0);
    ASSERT(command_buffers != NULL);

    if (!is_canvas_offscreen(canvas))
    {
        if (!do_compute)
        {
            VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
            VkSemaphore waitSemaphores[] = {
                draw_sync->image_available_semaphores[canvas->current_frame]};
            VkSemaphore signalSemaphores[] = {
                draw_sync->render_finished_semaphores[canvas->current_frame]};

            submit_info.waitSemaphoreCount = 1;
            submit_info.pWaitSemaphores = waitSemaphores;
            submit_info.pWaitDstStageMask = waitStages;
            submit_info.commandBufferCount = command_buffer_count;
            submit_info.pCommandBuffers = command_buffers;

            submit_info.signalSemaphoreCount = 1;
            submit_info.pSignalSemaphores = signalSemaphores;
        }
        else
        {
            // Graphics and compute.
            VkPipelineStageFlags waitStages[] = {
                VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
            VkSemaphore waitSemaphores[] = {
                gpu->compute_semaphore,
                draw_sync->image_available_semaphores[canvas->current_frame]};
            VkSemaphore signalSemaphores[] = {
                gpu->graphics_semaphore,
                draw_sync->render_finished_semaphores[canvas->current_frame]};

            submit_info.commandBufferCount = command_buffer_count;
            submit_info.pCommandBuffers = command_buffers;
            submit_info.waitSemaphoreCount = 2;
            submit_info.pWaitSemaphores = waitSemaphores;
            submit_info.pWaitDstStageMask = waitStages;
            submit_info.signalSemaphoreCount = 2;
            submit_info.pSignalSemaphores = signalSemaphores;
        }

        vkResetFences(gpu->device, 1, &(draw_sync->in_flight_fences[canvas->current_frame]));

        VK_CHECK_RESULT(vkQueueSubmit(
            gpu->graphics_queue, 1, &submit_info,
            draw_sync->in_flight_fences[canvas->current_frame]));
    }
    else
    {
        // Offscreen canvas.
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = NULL;
        submit_info.pWaitDstStageMask = NULL;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = canvas->command_buffers;

        VK_CHECK_RESULT(
            vkQueueSubmit(canvas->gpu->graphics_queue, 1, &submit_info, VK_NULL_HANDLE));
    }
}

void vky_fill_command_buffers(VkyCanvas* canvas)
{
    log_trace("clean and refill command buffers");
    // Clean all command bufers.
    // vkResetCommandPool(canvas->gpu->device, canvas->gpu->command_pool, 0);
    // Refill the main command buffers (one per swap chain image).
    for (uint32_t image_index = 0; image_index < canvas->image_count; image_index++)
    {
        // NOTE: we set the current command buffer index being constructed here so that
        // it is used by vky_begin_render_pass(). Otherwise an error message related
        // to the swapchain image layout occurs.
        canvas->current_command_buffer_index = image_index;
        VkCommandBuffer command_buffer = canvas->command_buffers[image_index];
        vkResetCommandBuffer(command_buffer, 0);
        vky_begin_command_buffer(command_buffer, canvas->gpu);
        if (canvas->cb_fill_command_buffer != NULL)
        {
            canvas->cb_fill_command_buffer(canvas, command_buffer);
        }
        vky_end_command_buffer(command_buffer, canvas->gpu);
    }
}

void vky_fill_live_command_buffers(VkyCanvas* canvas)
{
    if (canvas->cb_fill_live_command_buffer == NULL)
        return;
    // log_trace("fill live command buffers");
    ASSERT(canvas->live_command_buffers != NULL);
    uint32_t image_index = canvas->image_index;
    canvas->current_command_buffer_index = image_index;
    VkCommandBuffer command_buffer = canvas->live_command_buffers[image_index];
    vkResetCommandBuffer(command_buffer, 0);
    vky_begin_command_buffer(command_buffer, canvas->gpu);
    canvas->cb_fill_live_command_buffer(canvas, command_buffer);
    vky_end_command_buffer(command_buffer, canvas->gpu);
}



/*************************************************************************************************/
/*  Draw                                                                                         */
/*************************************************************************************************/

void vky_set_viewport(VkCommandBuffer command_buffer, int32_t x, int32_t y, uint32_t w, uint32_t h)
{
    VkRect2D scissor = {{x, y}, {w, h}};
    VkViewport viewport = {x, y, w, h, 0.0f, 1.0f};

    vkCmdSetViewport(command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);
}

void vky_draw(VkCommandBuffer command_buffer, uint32_t first_vertex, uint32_t vertex_count)
{
    log_trace("draw %d", vertex_count);
    vkCmdDraw(command_buffer, vertex_count, 1, first_vertex, 0);
}

void vky_draw_indirect(VkCommandBuffer command_buffer, VkyBufferRegion indirect_buffer)
{
    log_trace("draw indirect");
    ASSERT(indirect_buffer.buffer->raw_buffer != 0);
    if ((indirect_buffer.buffer->flags & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT) !=
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT)
    {
        log_error("by convention, the first created buffer should be an indirect buffer");
        return;
    }
    vkCmdDrawIndirect(
        command_buffer, indirect_buffer.buffer->raw_buffer, indirect_buffer.offset, 1, 0);
}

void vky_draw_indexed(
    VkCommandBuffer command_buffer, uint32_t first_index, uint32_t vertex_offset,
    uint32_t index_count)
{
    log_trace("draw indexed %d", index_count);
    vkCmdDrawIndexed(command_buffer, index_count, 1, first_index, (int32_t)vertex_offset, 0);
}

void vky_draw_indexed_indirect(VkCommandBuffer command_buffer, VkyBufferRegion indirect_buffer)
{
    log_trace("draw indexed indirect");
    ASSERT(indirect_buffer.buffer->raw_buffer != 0);
    vkCmdDrawIndexedIndirect(
        command_buffer, indirect_buffer.buffer->raw_buffer, indirect_buffer.offset, 1, 0);
}



/*************************************************************************************************/
/*  Destroy                                                                                      */
/*************************************************************************************************/

void vky_destroy_swapchain_resources(VkyCanvas* canvas)
{
    log_trace("destroy swapchain resources");
    VkDevice device = canvas->gpu->device;

    // log_trace("free command buffers");
    // vkFreeCommandBuffers(canvas->gpu->device, canvas->gpu->command_pool, canvas->image_count,
    // canvas->command_buffers);

    // Destroy the framebuffers.
    log_trace("destroy swapchain");
    for (uint32_t i = 0; i < canvas->image_count; i++)
    {
        vkDestroyFramebuffer(device, canvas->framebuffers[i], NULL);
    }

    // Destroy the depth objects.
    vkDestroyImageView(device, canvas->depth_image_view, NULL);
    vkDestroyImage(device, canvas->depth_image, NULL);
    vkFreeMemory(device, canvas->depth_image_memory, NULL);

    // Destroy the swap chain image views and the swap chain.
    for (uint32_t i = 0; i < canvas->image_count; i++)
    {
        vkDestroyImageView(device, canvas->image_views[i], NULL);
    }

    if (canvas->swapchain != NULL)
    {
        vkDestroySwapchainKHR(device, canvas->swapchain, NULL);
    }
    else
    {
        // Offscreen rendering.
        vkDestroyImage(device, canvas->images[0], NULL);
        vkFreeMemory(device, canvas->image_memory, NULL);
    }

    free(canvas->framebuffers);
    free(canvas->image_views);
    free(canvas->images);
}

void vky_destroy_canvas(VkyCanvas* canvas)
{
    log_trace("destroy canvas");

    if (canvas->scene != NULL)
    {
        log_trace("destroy scene while destroying canvas");
        vky_destroy_scene(canvas->scene);
    }

    VkDevice device = canvas->gpu->device;
    vky_destroy_swapchain_resources(canvas);

    if (canvas->surface != 0)
    {
        log_trace("destroy surface");
        vkDestroySurfaceKHR(canvas->gpu->instance, canvas->surface, NULL);
    }

    // Destroy the sync objects.
    VkyDrawSync* draw_sync = &canvas->draw_sync;
    if (draw_sync->image_available_semaphores != NULL)
    {
        log_trace("destroy %d sync objects", VKY_MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < VKY_MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(device, draw_sync->render_finished_semaphores[i], NULL);
            vkDestroySemaphore(device, draw_sync->image_available_semaphores[i], NULL);
            vkDestroyFence(device, draw_sync->in_flight_fences[i], NULL);
        }

        free(draw_sync->render_finished_semaphores);
        free(draw_sync->image_available_semaphores);
        free(draw_sync->in_flight_fences);
        free(draw_sync->images_in_flight);
    }

    log_trace("free command buffers");
    vkFreeCommandBuffers(
        canvas->gpu->device, canvas->gpu->command_pool, canvas->image_count,
        canvas->command_buffers);

    // Destroy the render pass.
    vkDestroyRenderPass(device, canvas->render_pass, NULL);
    vkDestroyRenderPass(device, canvas->live_render_pass, NULL);

    if (canvas->compute_pipeline != NULL)
    {
        vky_destroy_compute_pipeline(canvas->compute_pipeline);
        free(canvas->compute_pipeline);
        canvas->compute_pipeline = NULL;
    }

    free(canvas->command_buffers);
}

void vky_destroy_device(VkyGpu* gpu)
{
    log_trace("destroy device");
    VkDevice device = gpu->device;

    // Destroy buffers.
    log_trace("destroy buffers");
    for (uint32_t i = 0; i < gpu->buffer_count; i++)
    {
        vky_destroy_buffer(&gpu->buffers[i]);
    }
    free(gpu->buffers);
    gpu->buffers = NULL;

    // Destroy textures.
    log_trace("destroy textures");
    for (uint32_t i = 0; i < gpu->texture_count; i++)
    {
        vky_destroy_texture(&gpu->textures[i]);
    }
    free(gpu->textures);
    gpu->textures = NULL;

    log_trace("destroy graphics and compute semaphores");
    if (gpu->graphics_semaphore)
        vkDestroySemaphore(device, gpu->graphics_semaphore, NULL);
    if (gpu->compute_semaphore)
        vkDestroySemaphore(device, gpu->compute_semaphore, NULL);

    // Destroy the command pool.
    log_trace("destroy command pools");
    if (gpu->command_pool)
        vkDestroyCommandPool(device, gpu->command_pool, NULL);
    if (gpu->compute_command_pool)
        vkDestroyCommandPool(device, gpu->compute_command_pool, NULL);

    if (gpu->has_validation)
    {
        destroy_debug_utils_messenger_EXT(gpu->instance, gpu->debug_messenger, NULL);
    }

    log_trace("destroy descriptor pool");
    if (gpu->descriptor_pool)
        vkDestroyDescriptorPool(gpu->device, gpu->descriptor_pool, NULL);

    // Destroy the device.
    log_trace("destroy device");
    vkDestroyDevice(device, NULL);

    // Destroy the instance.
    log_trace("destroy instance");
    vkDestroyInstance(gpu->instance, NULL);
}



/*************************************************************************************************/
/*  Vertex layout                                                                                */
/*************************************************************************************************/

VkyVertexLayout vky_create_vertex_layout(VkyGpu* gpu, uint32_t binding, uint32_t stride)
{
    VkyVertexLayout layout = {0};
    layout.gpu = gpu;
    layout.attribute_count = 0;
    layout.attribute_offsets = calloc(VKY_MAX_ATTRIBUTE_COUNT, sizeof(uint32_t));
    layout.attribute_formats = calloc(VKY_MAX_ATTRIBUTE_COUNT, sizeof(VkFormat));
    layout.binding = binding;
    layout.stride = stride;
    return layout;
}

void vky_add_vertex_attribute(
    VkyVertexLayout* layout, uint32_t location, VkFormat format, uint32_t offset)
{
    layout->attribute_formats[location] = format;
    layout->attribute_offsets[location] = offset;
    layout->attribute_count++;
}

void vky_destroy_vertex_layout(VkyVertexLayout* layout)
{
    free(layout->attribute_offsets);
    free(layout->attribute_formats);
}



/*************************************************************************************************/
/*  Resource layout                                                                              */
/*************************************************************************************************/

VkyResourceLayout vky_create_resource_layout(VkyGpu* gpu, uint32_t image_count)
{
    VkyResourceLayout layout = {0};
    layout.gpu = gpu;
    layout.binding_count = 0;
    layout.dynamic_binding_count = 0;
    layout.binding_types = calloc(VKY_MAX_BINDING_COUNT, sizeof(VkDescriptorType));
    layout.image_count = image_count;
    return layout;
}

void vky_add_resource_binding(VkyResourceLayout* layout, uint32_t binding, VkDescriptorType type)
{
    // NOTE: binding should be 0, 1, 2, ... at each call.
    layout->binding_types[binding] = type;

    // Increment the counters.
    layout->binding_count++;
    if (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
    {
        layout->dynamic_binding_count++;
    }
}

void vky_destroy_resource_layout(VkyResourceLayout* layout)
{
    free(layout->binding_types);
    // free(layout->alignedSizes);
}



/*************************************************************************************************/
/*  Shaders                                                                                      */
/*************************************************************************************************/

VkShaderModule vky_create_shader_module(VkyGpu* gpu, uint32_t size, const uint32_t* buffer)
{
    VkShaderModuleCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = size;
    createInfo.pCode = buffer;

    VkShaderModule module = {0};
    VK_CHECK_RESULT(vkCreateShaderModule(gpu->device, &createInfo, NULL, &module));

    return module;
}

VkShaderModule vky_create_shader_module_from_file(VkyGpu* gpu, char* filename)
{
    log_trace("create shader module from file %s", filename);
    uint32_t size = 0;
    uint32_t* shader_code = (uint32_t*)read_file(filename, &size);
    VkShaderModule module = vky_create_shader_module(gpu, size, shader_code);
    free(shader_code);
    return module;
}

VkyShaders vky_create_shaders(VkyGpu* gpu)
{
    log_trace("create shaders");
    VkyShaders shaders = {0};
    shaders.gpu = gpu;
    shaders.shader_count = 0;
    shaders.modules = calloc(VKY_MAX_SHADER_COUNT, sizeof(VkShaderModule));
    shaders.stages = calloc(VKY_MAX_SHADER_COUNT, sizeof(VkShaderStageFlagBits));
    return shaders;
}

void vky_add_shader(VkyShaders* shaders, VkShaderStageFlagBits stage, const char* filename)
{
    log_trace("add shader");
    char path[1024];
    snprintf(path, sizeof(path), "%s/spirv/%s", DATA_DIR, filename);
    shaders->modules[shaders->_index] = vky_create_shader_module_from_file(shaders->gpu, path);

    shaders->stages[shaders->_index] = stage;
    shaders->_index++;
    shaders->shader_count++;
}

void vky_destroy_shaders(VkyShaders* shaders)
{
    log_trace("destroy shaders");
    for (uint32_t i = 0; i < shaders->shader_count; i++)
    {
        log_trace("destroy shader %d", i);
        vkDestroyShaderModule(shaders->gpu->device, shaders->modules[i], NULL);
    }
    free(shaders->modules);
    free(shaders->stages);
}



/*************************************************************************************************/
/*  Graphics pipeline                                                                            */
/*************************************************************************************************/

VkyGraphicsPipeline vky_create_graphics_pipeline(
    VkyCanvas* canvas, VkPrimitiveTopology topology, VkyShaders shaders,
    VkyVertexLayout vertex_layout, VkyResourceLayout resource_layout,
    VkyGraphicsPipelineParams params)
{

    log_trace("create graphics pipeline");
    VkyGpu* gpu = canvas->gpu;

    VkyGraphicsPipeline gp = {0};
    gp.gpu = gpu;
    gp.topology = topology;
    gp.vertex_layout = vertex_layout;
    gp.resource_layout = resource_layout;
    gp.shaders = shaders;

    // Descriptor set layout.
    ASSERT(gp.resource_layout.binding_count <= 100);
    VkDescriptorSetLayoutBinding layout_bindings[100];
    for (size_t i = 0; i < gp.resource_layout.binding_count; i++)
    {
        VkDescriptorType dtype = gp.resource_layout.binding_types[i];
        layout_bindings[i].binding = i;
        layout_bindings[i].descriptorType = dtype;
        layout_bindings[i].descriptorCount = 1;
        layout_bindings[i].stageFlags = VK_SHADER_STAGE_ALL;
        layout_bindings[i].pImmutableSamplers = NULL; // Optional
    }

    // Create descriptor set layout.
    VkDescriptorSetLayoutCreateInfo layout_info = {0};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = gp.resource_layout.binding_count;
    layout_info.pBindings = layout_bindings;

    log_trace("create descriptor set layout");
    VK_CHECK_RESULT(
        vkCreateDescriptorSetLayout(gpu->device, &layout_info, NULL, &gp.descriptor_set_layout));

    // Pipeline layout.
    VkPipelineLayoutCreateInfo pipeline_layout_info = {0};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &gp.descriptor_set_layout;

    // Push constants
    VkPushConstantRange push_constants = {0};
    push_constants.offset = 0;
    push_constants.size = params.push_constant_size;
    push_constants.stageFlags = VK_SHADER_STAGE_ALL;
    if (params.push_constant_size == 0)
    {
        pipeline_layout_info.pushConstantRangeCount = 0;
        pipeline_layout_info.pPushConstantRanges = NULL;
    }
    else
    {
        pipeline_layout_info.pushConstantRangeCount = 1;
        pipeline_layout_info.pPushConstantRanges = &push_constants;
    }

    log_trace("create pipeline layout");
    VK_CHECK_RESULT(
        vkCreatePipelineLayout(gpu->device, &pipeline_layout_info, NULL, &gp.pipeline_layout));

    // Pipeline.
    VkPipelineInputAssemblyStateCreateInfo input_assembly = {0};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = topology;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineRasterizationStateCreateInfo rasterizer = {0};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {0};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState color_blend_attachment = {0};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_TRUE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blending = {0};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;
    color_blending.blendConstants[0] = 0.0f;
    color_blending.blendConstants[1] = 0.0f;
    color_blending.blendConstants[2] = 0.0f;
    color_blending.blendConstants[3] = 0.0f;

    VkPipelineDepthStencilStateCreateInfo depth_stencil = {0};
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = params.depth_test_enable;
    depth_stencil.depthWriteEnable = VK_TRUE;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.minDepthBounds = 0.0f; // Optional
    depth_stencil.maxDepthBounds = 1.0f; // Optional
    depth_stencil.stencilTestEnable = VK_FALSE;
    depth_stencil.front = (VkStencilOpState){0}; // Optional
    depth_stencil.back = (VkStencilOpState){0};  // Optional

    VkPipelineViewportStateCreateInfo viewport_state = {0};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount =
        1; // NOTE: unused because the viewport/scissor are set in the dynamic states
    viewport_state.scissorCount = 1;

    // Dynamic state
    VkPipelineDynamicStateCreateInfo dynamic_state = {0};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.pNext = NULL;

    VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    dynamic_state.pDynamicStates = dynamic_states;
    dynamic_state.dynamicStateCount = 2;

    // Vertex input state.
    VkPipelineVertexInputStateCreateInfo vertex_input_info = {0};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkVertexInputBindingDescription binding_description = {0};
    binding_description.binding = gp.vertex_layout.binding;
    binding_description.stride = gp.vertex_layout.stride;
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    ASSERT(gp.vertex_layout.attribute_count <= 100);
    VkVertexInputAttributeDescription attribute_descriptions[100];
    for (uint32_t i = 0; i < gp.vertex_layout.attribute_count; i++)
    {
        attribute_descriptions[i].binding = gp.vertex_layout.binding;
        attribute_descriptions[i].location = i;
        attribute_descriptions[i].format = gp.vertex_layout.attribute_formats[i];
        attribute_descriptions[i].offset = gp.vertex_layout.attribute_offsets[i];
    }

    vertex_input_info.vertexBindingDescriptionCount = 1; // TODO: support multiple bindings
    vertex_input_info.vertexAttributeDescriptionCount = gp.vertex_layout.attribute_count;
    vertex_input_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions;

    // Create the shader stages.
    ASSERT(shaders.shader_count <= 100);
    VkPipelineShaderStageCreateInfo shader_stages[100];
    for (uint32_t i = 0; i < shaders.shader_count; i++)
    {
        shader_stages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stages[i].stage = shaders.stages[i];
        shader_stages[i].module = shaders.modules[i];
        shader_stages[i].pName = "main";
        // NOTE: important to initialize empty fields as the memory has not been allocated
        // for the array, so fields could contain garbage values..
        shader_stages[i].pNext = NULL;
        shader_stages[i].flags = 0;
        shader_stages[i].pSpecializationInfo = NULL;
    }

    // Finally create the pipeline.
    VkGraphicsPipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = shaders.shader_count;
    pipelineInfo.pStages = shader_stages;
    pipelineInfo.pVertexInputState = &vertex_input_info;
    pipelineInfo.pInputAssemblyState = &input_assembly;
    pipelineInfo.pViewportState = &viewport_state;
    pipelineInfo.pDynamicState = &dynamic_state;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &color_blending;
    pipelineInfo.pDepthStencilState = &depth_stencil;
    pipelineInfo.layout = gp.pipeline_layout;
    pipelineInfo.renderPass = canvas->render_pass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    log_trace("create graphics pipeline");
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(
        gpu->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &gp.pipeline));

    // Allocate descriptor sets.
    ASSERT(gp.resource_layout.image_count <= 100);
    VkDescriptorSetLayout layouts[100];
    for (uint32_t i = 0; i < gp.resource_layout.image_count; i++)
    {
        layouts[i] = gp.descriptor_set_layout;
    }
    VkDescriptorSetAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ASSERT(gpu->descriptor_pool != 0);
    alloc_info.descriptorPool = gpu->descriptor_pool;
    alloc_info.descriptorSetCount = gp.resource_layout.image_count;
    alloc_info.pSetLayouts = layouts;

    gp.descriptor_set_count = gp.resource_layout.image_count;
    gp.descriptor_sets = calloc(gp.resource_layout.image_count, sizeof(VkDescriptorSet));
    log_trace("allocate descriptor sets");
    if (gp.resource_layout.binding_count > 0)
    {
        VK_CHECK_RESULT(vkAllocateDescriptorSets(gpu->device, &alloc_info, gp.descriptor_sets));
    }

    return gp;
}

void vky_push_constants(
    VkCommandBuffer command_buffer, VkyGraphicsPipeline* pipeline, uint32_t size, const void* data)
{
    vkCmdPushConstants(
        command_buffer, pipeline->pipeline_layout, VK_SHADER_STAGE_ALL, 0, size, data);
}

void vky_bind_graphics_pipeline(VkCommandBuffer command_buffer, VkyGraphicsPipeline* pipeline)
{
    log_trace("bind pipeline");
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
}

void vky_destroy_graphics_pipeline(VkyGraphicsPipeline* gp)
{
    log_trace("destroy graphics pipeline");

    vkDestroyDescriptorSetLayout(gp->gpu->device, gp->descriptor_set_layout, NULL);
    vkDestroyPipelineLayout(gp->gpu->device, gp->pipeline_layout, NULL);
    vkDestroyPipeline(gp->gpu->device, gp->pipeline, NULL);
    free(gp->descriptor_sets);
}



/*************************************************************************************************/
/*  Compute pipeline                                                                             */
/*************************************************************************************************/

static void vky_buffer_barrier(
    VkCommandBuffer cmd_buf, VkyBufferRegion* buffer, VkAccessFlags src_access,
    VkAccessFlags dst_access, VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage)
{
    VkyGpu* gpu = buffer->buffer->gpu;
    if (gpu->queue_indices.graphics_family == gpu->queue_indices.compute_family)
        return;

    log_warn("Compute resource synchronization has never been tested yet on GPUs with different "
             "graphics/compute queues!");

    uint32_t src_family = gpu->queue_indices.graphics_family;
    uint32_t dst_family = gpu->queue_indices.graphics_family;

    if (src_stage & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT)
    {
        src_family = gpu->queue_indices.compute_family;
    }
    else if (dst_stage & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT)
    {
        dst_family = gpu->queue_indices.compute_family;
    }

    VkBufferMemoryBarrier barrier = {
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        NULL,
        src_access,
        dst_access,
        src_family,
        dst_family,
        buffer->buffer->raw_buffer,
        buffer->offset,
        buffer->size,
    };

    vkCmdPipelineBarrier(cmd_buf, src_stage, dst_stage, 0, 0, NULL, 1, &barrier, 0, NULL);
}

static void vky_texture_barrier(
    VkCommandBuffer cmd_buf, VkyTexture* texture, VkAccessFlags src_access,
    VkAccessFlags dst_access, VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage)
{
    VkyGpu* gpu = texture->gpu;
    if (gpu->queue_indices.graphics_family == gpu->queue_indices.compute_family)
        return;

    uint32_t src_family = gpu->queue_indices.graphics_family;
    uint32_t dst_family = gpu->queue_indices.graphics_family;

    if (src_stage & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT)
    {
        src_family = gpu->queue_indices.compute_family;
    }
    else if (dst_stage & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT)
    {
        dst_family = gpu->queue_indices.compute_family;
    }

    VkImageMemoryBarrier barrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        NULL,
        src_access,
        dst_access,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_GENERAL,
        src_family,
        dst_family,
        texture->image,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    };
    vkCmdPipelineBarrier(cmd_buf, src_stage, dst_stage, 0, 0, NULL, 0, NULL, 1, &barrier);
}

static void vky_resource_barrier(
    VkCommandBuffer cmd_buf, VkDescriptorType descriptor_type, void* resource,
    VkAccessFlags src_access, VkAccessFlags dst_access, VkPipelineStageFlags src_stage,
    VkPipelineStageFlags dst_stage)
{
    switch (descriptor_type)
    {

    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        vky_buffer_barrier(
            cmd_buf, (VkyBufferRegion*)resource, src_access, dst_access, src_stage, dst_stage);
        break;

    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        vky_texture_barrier(
            cmd_buf, (VkyTexture*)resource, src_access, dst_access, src_stage, dst_stage);
        break;

    default:
        log_error("resource type not supported: %d", descriptor_type);
        break;
    }
}

static void
vky_release_compute_resource(VkyGpu* gpu, VkDescriptorType descriptor_type, void* resource)
{
    if (gpu->queue_indices.graphics_family == gpu->queue_indices.compute_family)
        return;

    // Create a transient command buffer for setting up the initial buffer transfer state
    log_trace("release compute resources");
    VkCommandBuffer transferCmd = {0};
    VkCommandBufferAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = gpu->compute_command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;
    VK_CHECK_RESULT(vkAllocateCommandBuffers(gpu->device, &alloc_info, &transferCmd));

    vky_resource_barrier(
        transferCmd, descriptor_type, resource, 0, VK_ACCESS_SHADER_WRITE_BIT,
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    vky_resource_barrier(
        transferCmd, descriptor_type, resource, VK_ACCESS_SHADER_WRITE_BIT, 0,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);

    vkEndCommandBuffer(transferCmd);

    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &transferCmd;

    // Create fence to ensure that the command buffer has finished executing
    VkFenceCreateInfo fenceInfo = {0};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence fence = {0};
    vkCreateFence(gpu->device, &fenceInfo, NULL, &fence);
    vkQueueSubmit(gpu->compute_queue, 1, &submit_info, fence);
    // Wait for the fence to signal that command buffer has finished executing
    vkWaitForFences(gpu->device, 1, &fence, VK_TRUE, UINT64_MAX);
    vkDestroyFence(gpu->device, fence, NULL);
    vkFreeCommandBuffers(gpu->device, gpu->compute_command_pool, 1, &transferCmd);
}

VkyComputePipeline
vky_create_compute_pipeline(VkyGpu* gpu, const char* filename, VkyResourceLayout resource_layout)
{

    log_trace("create compute pipeline");
    VkyComputePipeline gp = {0};
    gp.gpu = gpu;
    gp.resource_layout = resource_layout;

    // Descriptor set layout.
    ASSERT(gp.resource_layout.binding_count <= 100);
    VkDescriptorSetLayoutBinding layout_bindings[100];
    for (size_t i = 0; i < gp.resource_layout.binding_count; i++)
    {
        VkDescriptorType dtype = gp.resource_layout.binding_types[i];
        layout_bindings[i].binding = i;
        layout_bindings[i].descriptorType = dtype;
        layout_bindings[i].descriptorCount = 1;
        layout_bindings[i].stageFlags = VK_SHADER_STAGE_ALL;
        layout_bindings[i].pImmutableSamplers = NULL; // Optional
    }

    // Create descriptor set layout.
    VkDescriptorSetLayoutCreateInfo layout_info = {0};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = gp.resource_layout.binding_count;
    layout_info.pBindings = layout_bindings;

    log_trace("create descriptor set layout");
    VK_CHECK_RESULT(
        vkCreateDescriptorSetLayout(gpu->device, &layout_info, NULL, &gp.descriptor_set_layout));

    // Pipeline layout.
    VkPipelineLayoutCreateInfo pipeline_layout_info = {0};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &gp.descriptor_set_layout;

    VK_CHECK_RESULT(
        vkCreatePipelineLayout(gpu->device, &pipeline_layout_info, NULL, &gp.pipeline_layout));

    // Allocate descriptor sets.
    ASSERT(gp.resource_layout.image_count == 1);
    ASSERT(gp.resource_layout.image_count <= 100);
    VkDescriptorSetLayout layouts[100]; // NOTE: should be 1
    for (uint32_t i = 0; i < gp.resource_layout.image_count; i++)
    {
        layouts[i] = gp.descriptor_set_layout;
    }
    VkDescriptorSetAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ASSERT(gpu->descriptor_pool != 0);
    alloc_info.descriptorPool = gpu->descriptor_pool;
    alloc_info.descriptorSetCount = gp.resource_layout.image_count;
    alloc_info.pSetLayouts = layouts;

    gp.descriptor_set_count = gp.resource_layout.image_count;
    gp.descriptor_sets = calloc(gp.resource_layout.image_count, sizeof(VkDescriptorSet));
    log_trace("allocate descriptor sets");
    if (gp.resource_layout.binding_count > 0)
    {
        VK_CHECK_RESULT(vkAllocateDescriptorSets(gpu->device, &alloc_info, gp.descriptor_sets));
    }
    ASSERT(gp.descriptor_sets != NULL);

    // Create the shader and pipeline.
    VkComputePipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = gp.pipeline_layout;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.pName = "main";
    char path[1024];
    snprintf(path, sizeof(path), "%s/spirv/%s", DATA_DIR, filename);
    pipelineInfo.stage.module = gp.shader = vky_create_shader_module_from_file(gpu, path);
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    VK_CHECK_RESULT(vkCreateComputePipelines(
        gpu->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &gp.pipeline));

    return gp;
}

void vky_begin_compute(VkyGpu* gpu)
{
    log_trace("begin compute");

    if (gpu->has_graphics)
    {
        // Signal the semaphore
        VkSubmitInfo submitInfo = {0};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &gpu->compute_semaphore;
        vkQueueSubmit(gpu->graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(gpu->graphics_queue);
    }
    // Build compute command buffer.
    VkCommandBufferBeginInfo cmdBufInfo = {0};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VkCommandBuffer cmd_buf = gpu->compute_command_buffer;
    ASSERT(cmd_buf != 0);
    vkBeginCommandBuffer(cmd_buf, &cmdBufInfo);
}

void vky_compute_acquire(
    VkyComputePipeline* pipeline, VkDescriptorType descriptor_type, void* resource,
    VkPipelineStageFlagBits stage)
{
    vky_resource_barrier(
        pipeline->gpu->compute_command_buffer, descriptor_type, resource, 0,
        VK_ACCESS_SHADER_WRITE_BIT, stage, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
}

void vky_compute_release(
    VkyComputePipeline* pipeline, VkDescriptorType descriptor_type, void* resource,
    VkPipelineStageFlagBits stage)
{
    vky_resource_barrier(
        pipeline->gpu->compute_command_buffer, descriptor_type, resource,
        VK_ACCESS_SHADER_WRITE_BIT, 0, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, stage);
}

void vky_compute(VkyComputePipeline* pipeline, uint32_t nx, uint32_t ny, uint32_t nz)
{
    log_trace("make compute");
    // NOTE: the passed buffer MUST be the same as the storage buffer bound via
    // vky_bind_resources();

    VkyGpu* gpu = pipeline->gpu;
    VkCommandBuffer cmd_buf = gpu->compute_command_buffer;

    // Dispatch the compute job
    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->pipeline);
    vkCmdBindDescriptorSets(
        cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->pipeline_layout, 0, 1,
        pipeline->descriptor_sets, 0, 0);
    vkCmdDispatch(cmd_buf, nx, ny, nz);
}

void vky_end_compute(
    VkyGpu* gpu, uint32_t resource_count, VkDescriptorType* descriptor_types, void** resources)
{
    log_trace("end compute");

    vkEndCommandBuffer(gpu->compute_command_buffer);

    for (uint32_t i = 0; i < resource_count; i++)
    {
        vky_release_compute_resource(gpu, descriptor_types[i], resources[i]);
    }

    gpu->has_compute = true;
}

void vky_compute_submit(VkyGpu* gpu)
{
    if (!gpu->has_compute)
        return;

    vkQueueWaitIdle(gpu->graphics_queue);

    // Wait for rendering finished
    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

    // Submit compute commands
    // log_trace("submit compute command buffer");
    VkSubmitInfo computeSubmitInfo = {0};
    computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    computeSubmitInfo.commandBufferCount = 1;
    computeSubmitInfo.pCommandBuffers = &gpu->compute_command_buffer;

    if (gpu->has_graphics)
    {
        computeSubmitInfo.waitSemaphoreCount = 1;
        computeSubmitInfo.pWaitSemaphores = &gpu->graphics_semaphore;
        computeSubmitInfo.pWaitDstStageMask = &waitStageMask;
        computeSubmitInfo.signalSemaphoreCount = 1;
        computeSubmitInfo.pSignalSemaphores = &gpu->compute_semaphore;
    }

    vkQueueSubmit(gpu->compute_queue, 1, &computeSubmitInfo, VK_NULL_HANDLE);
}

void vky_compute_wait(VkyGpu* gpu) { vkQueueWaitIdle(gpu->compute_queue); }

void vky_destroy_compute_pipeline(VkyComputePipeline* pipeline)
{
    if (pipeline == NULL)
    {
        log_trace("skip destruction of null compute pipeline");
        return;
    }
    log_trace("destroy compute pipeline");
    VkDevice device = pipeline->gpu->device;
    ASSERT(pipeline->shader != 0);
    vkDestroyShaderModule(device, pipeline->shader, NULL);
    vkDestroyPipelineLayout(device, pipeline->pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(device, pipeline->descriptor_set_layout, NULL);
    vkDestroyPipeline(device, pipeline->pipeline, NULL);
}



/*************************************************************************************************/
/*  Buffers                                                                                      */
/*************************************************************************************************/

VkyBuffer vky_create_buffer(
    VkyGpu* gpu, VkDeviceSize size, VkBufferUsageFlagBits flags, VkMemoryPropertyFlagBits memory)
{
    log_debug("create buffer %d with size %d", flags, size);
    ASSERT(size > 0);

    VkyBuffer buffer = {0};
    buffer.gpu = gpu;
    buffer.size = size;
    buffer.flags = flags;
    buffer.allocated_size = 0;

    // Default memory property
    if (memory == 0)
        memory = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    create_buffer(
        gpu->device, buffer.size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | flags, &buffer.raw_buffer,
        &buffer.memory, memory, gpu->memory_properties);

    return buffer;
}

VkyBufferRegion vky_allocate_buffer(VkyBuffer* buffer, VkDeviceSize size)
{
    log_trace("allocate buffer %d with size %d", buffer->flags, size);
    VkyBufferRegion buffer_region = {0};
    buffer_region.buffer = buffer;
    buffer_region.offset = buffer->allocated_size;
    buffer_region.size = size;
    buffer->allocated_size += size;
    return buffer_region;
}

void vky_clear_buffer(VkyBuffer* buffer) { buffer->allocated_size = 0; }

void vky_clear_all_buffers(VkyGpu* gpu)
{
    for (uint32_t i = 0; i < gpu->buffer_count; i++)
    {
        vky_clear_buffer(&gpu->buffers[i]);
    }
}

void vky_upload_buffer(
    VkyBufferRegion buffer, VkDeviceSize offset, VkDeviceSize size, const void* data)
{
    // log_trace("upload buffer with offset %d and size %d", offset, size);
    VkyGpu* gpu = buffer.buffer->gpu;

    // Checks.
    ASSERT(size > 0);
    // Check that the buffer is large enough.
    ASSERT(offset + size <= buffer.size);
    // Check that the underlying buffer is large enough.
    ASSERT(buffer.offset + offset + size <= buffer.buffer->size);

    VkBufferCopy copy_region = {0};
    copy_region.srcOffset = 0;
    // NOTE: we must take into account the offset of the buffer within the underlying buffer.
    copy_region.dstOffset = buffer.offset + offset;
    copy_region.size = size;

    VkBufferMemoryBarrier* p_barrier = NULL;
    // Compute
    if (buffer.buffer->flags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT &&
        gpu->queue_indices.graphics_family != gpu->queue_indices.compute_family)
    {
        VkBufferMemoryBarrier barrier = {0};
        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        barrier.dstAccessMask = 0;
        barrier.srcQueueFamilyIndex = gpu->queue_indices.graphics_family;
        barrier.dstQueueFamilyIndex = gpu->queue_indices.compute_family;
        barrier.buffer = buffer.buffer->raw_buffer;
        barrier.offset = buffer.offset;
        barrier.size = buffer.size;
        *p_barrier = barrier;
    }
    if (p_barrier != NULL)
    {
        ASSERT(p_barrier->sType == VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER);
    }

    upload_data_to_buffer(
        gpu->device, copy_region, data, buffer.buffer->raw_buffer, gpu->memory_properties,
        gpu->command_pool, gpu->graphics_queue, p_barrier);
}

void vky_download_buffer(VkyBufferRegion* buffer, void* out)
{
    // NOTE: only for host coherent/visible buffers (ie storage buffers for compute).
    VkyGpu* gpu = buffer->buffer->gpu;
    void* buf = NULL;
    vkMapMemory(gpu->device, buffer->buffer->memory, 0, buffer->size, 0, &buf);
    memcpy(out, buf, buffer->size);
    vkUnmapMemory(gpu->device, buffer->buffer->memory);
}

void vky_bind_vertex_buffer(
    VkCommandBuffer command_buffer, VkyBufferRegion buffer, VkDeviceSize offset)
{
    log_trace("bind vertex buffer");
    VkBuffer vertex_buffers[] = {buffer.buffer->raw_buffer};
    // NOTE: we must take into account the offset of the buffer within the underlying buffer.
    VkDeviceSize offsets[] = {buffer.offset + offset};
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
}

void vky_bind_index_buffer(
    VkCommandBuffer command_buffer, VkyBufferRegion buffer, VkDeviceSize offset)
{
    log_trace("bind index buffer");
    // NOTE: uint32 dtype for indices is hard-coded here.
    // NOTE: we must take into account the offset of the buffer within the underlying buffer.
    vkCmdBindIndexBuffer(
        command_buffer, buffer.buffer->raw_buffer, buffer.offset + offset, VK_INDEX_TYPE_UINT32);
}

void vky_destroy_buffer(VkyBuffer* buffer)
{
    log_trace("destroy buffer");
    vkDestroyBuffer(buffer->gpu->device, buffer->raw_buffer, NULL);
    vkFreeMemory(buffer->gpu->device, buffer->memory, NULL);
}



/*************************************************************************************************/
/*  Data management                                                                              */
/*************************************************************************************************/

VkyBuffer* vky_add_buffer(VkyGpu* gpu, VkDeviceSize size, VkBufferUsageFlagBits flags)
{
    ASSERT(size > 0);
    VkyBuffer buffer = vky_create_buffer(gpu, size, flags, 0);
    gpu->buffers[gpu->buffer_count] = buffer;
    VkyBuffer* out = &gpu->buffers[gpu->buffer_count];
    gpu->buffer_count++;
    return out;
}

VkyBuffer* vky_add_vertex_buffer(VkyGpu* gpu, VkDeviceSize size)
{
    return vky_add_buffer(gpu, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

VkyBuffer* vky_add_index_buffer(VkyGpu* gpu, VkDeviceSize size)
{
    return vky_add_buffer(gpu, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

VkyTexture* vky_add_texture(VkyGpu* gpu, const VkyTextureParams* params)
{
    VkyTexture texture = vky_create_texture(gpu, params);
    gpu->textures[gpu->texture_count] = texture;
    VkyTexture* out = &gpu->textures[gpu->texture_count];
    gpu->texture_count++;
    return out;
}

VkyBuffer* vky_find_buffer(VkyGpu* gpu, VkDeviceSize size, VkBufferUsageFlagBits flags)
{
    // find the first buffer with the given flag and sufficient size
    for (uint32_t i = 0; i < gpu->buffer_count; i++)
    {
        VkyBuffer* gbuf = &gpu->buffers[i];
        if ((gbuf->flags & flags) != 0 && gbuf->size - gbuf->allocated_size >= size)
        {
            return gbuf;
        }
    }
    log_debug("automatically creating a new buffer as an existing one as requested did not exist");
    return vky_add_buffer(gpu, size, flags);
}


VkyTextureParams vky_default_texture_params(uint32_t width, uint32_t height, uint32_t depth)
{
    VkyTextureParams params = {
        width,
        height,
        depth,
        4,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FILTER_NEAREST,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        VK_IMAGE_LAYOUT_UNDEFINED,
        false};
    return params;
}

/*************************************************************************************************/
/*  Common textures                                                                              */
/*************************************************************************************************/

VkyTexture* vky_get_color_texture(VkyGpu* gpu)
{
    // NOTE: texture #0 is colormap, texture #1 is the font texture
    if (gpu->texture_count == 0)
    {
        log_trace("create color texture");
        // Create the color texture only once.
        VkyTextureParams tex_params = {256,
                                       256,
                                       1,
                                       4,
                                       VK_FORMAT_R8G8B8A8_UNORM,
                                       VK_FILTER_NEAREST,
                                       VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                       0,
                                       false};
        vky_add_texture(gpu, &tex_params);
        // Load the color texture from file on disk if it is empty.
        uint8_t empty[256] = {0};
        // NOTE: this function is implemented in colormaps.c.
        if (memcmp(VKY_COLOR_TEXTURE, empty, 256) == 0)
            vky_load_color_texture();
        // Now the color texture should not be empty.
        ASSERT(memcmp(VKY_COLOR_TEXTURE, empty, 256) != 0);

        log_trace("upload the color texture to the GPU");
        vky_upload_texture(&gpu->textures[0], VKY_COLOR_TEXTURE);
    }
    ASSERT(gpu->texture_count >= 1);
    ASSERT(&gpu->textures[0] != NULL);
    return &gpu->textures[0];
}

VkyTexture* vky_get_font_texture(VkyGpu* gpu)
{
    // NOTE: texture #0 is colormap, texture #1 is the font texture
    if (gpu->texture_count <= 1)
    {
        log_trace("create font texture");

        // Load the font texture from disk.
        int width, height, depth;
        char path[1024];
        snprintf(path, sizeof(path), "%s/textures/%s", DATA_DIR, VKY_FONT_MAP_FILENAME);
        stbi_uc* pixels = stbi_load(path, &width, &height, &depth, STBI_rgb_alpha);
        if (pixels == NULL)
        {
            log_error("font texture file `%s` not found", path);
            width = height = 2;
            depth = 4;
            pixels = calloc(4, 4);
        }

        // Create the font texture.
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
        VkFilter filter = VK_FILTER_LINEAR;
        VkSamplerAddressMode address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkyTextureParams tex_params = (VkyTextureParams){(uint32_t)width,
                                                         (uint32_t)height,
                                                         1,
                                                         (uint32_t)depth,
                                                         format,
                                                         filter,
                                                         address_mode,
                                                         0,
                                                         false};

        // Create the texture.
        VkyTexture* texture = vky_add_texture(gpu, &tex_params);

        // Upload the font texture.
        vky_upload_texture(texture, pixels);
        stbi_image_free(pixels);
    }
    return &gpu->textures[1];
}



/*************************************************************************************************/
/*  Textures                                                                                     */
/*************************************************************************************************/

VkyTexture vky_create_texture(VkyGpu* gpu, const VkyTextureParams* p_params)
{
    log_trace("create texture, compute %d", p_params->enable_compute);
    VkyTextureParams params = *p_params;
    VkyTexture texture = {0};
    texture.gpu = gpu;

    // Usage flags.
    VkImageUsageFlagBits flags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    if (params.enable_compute)
    {
        flags |= VK_IMAGE_USAGE_STORAGE_BIT;
    }

    // Default image layout.
    if (params.layout == 0)
    {
        params.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    // Update the texture params.
    texture.params = params;

    create_image(
        gpu->device, params.width, params.height, params.depth, params.format,
        VK_IMAGE_TILING_OPTIMAL, flags, &texture.image, &texture.image_memory,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, gpu->memory_properties);

    VkImageViewType image_type = VK_IMAGE_VIEW_TYPE_2D;
    if (params.height == 1 && params.depth == 1)
        image_type = VK_IMAGE_VIEW_TYPE_1D;
    else if (params.depth > 1)
        image_type = VK_IMAGE_VIEW_TYPE_3D;

    texture.image_view = create_image_view(
        gpu->device, texture.image, image_type, params.format, VK_IMAGE_ASPECT_COLOR_BIT);

    // Create the texture sampler.
    texture.sampler =
        create_texture_sampler(gpu->device, params.filter, params.filter, params.address_mode);

    return texture;
}

void vky_upload_texture(VkyTexture* texture, const void* pixels)
{
    // TODO: support partial upload
    VkyGpu* gpu = texture->gpu;

    VkBuffer staging_buffer = {0};
    VkDeviceMemory staging_buffer_memory = {0};

    uint32_t image_size = texture_size_bytes(texture->params);

    create_buffer(
        gpu->device, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &staging_buffer,
        &staging_buffer_memory,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        gpu->memory_properties);

    void* data;
    if (pixels != NULL)
    {
        vkMapMemory(gpu->device, staging_buffer_memory, 0, image_size, 0, &data);
        memcpy(data, pixels, image_size);
        vkUnmapMemory(gpu->device, staging_buffer_memory);
    }

    transition_image_layout(
        gpu->device, gpu->command_pool, gpu->graphics_queue, texture->image,
        texture->params.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    copy_buffer_to_image(
        gpu->device, gpu->command_pool, gpu->graphics_queue, staging_buffer, texture->image,
        texture->params.width, texture->params.height, texture->params.depth);

    ASSERT(texture->params.layout != 0);
    transition_image_layout(
        gpu->device, gpu->command_pool, gpu->graphics_queue, texture->image,
        texture->params.format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texture->params.layout);

    vkDestroyBuffer(gpu->device, staging_buffer, NULL);
    vkFreeMemory(gpu->device, staging_buffer_memory, NULL);
}

void vky_destroy_texture(VkyTexture* texture)
{
    log_trace("destroy texture");
    VkDevice device = texture->gpu->device;
    vkDestroyImageView(device, texture->image_view, NULL);
    vkDestroyImage(device, texture->image, NULL);
    vkFreeMemory(device, texture->image_memory, NULL);

    vkDestroySampler(device, texture->sampler, NULL);
}



/*************************************************************************************************/
/*  Uniform buffers                                                                              */
/*************************************************************************************************/

VkyUniformBuffer
vky_create_uniform_buffer(VkyGpu* gpu, uint32_t image_count, VkDeviceSize type_size)
{
    log_trace("create ubo");
    VkyUniformBuffer ubo = {0};
    ubo.gpu = gpu;

    ubo.image_count = image_count;
    ubo.type_size = type_size;

    // NOTE: 3 values below unused, only for dynamic UBOs.
    ubo.item_count = 1;
    ubo.alignment = 0;
    ubo.buffer_size = type_size;

    ubo.buffers = calloc(image_count, sizeof(VkBuffer));
    ubo.memories = calloc(image_count, sizeof(VkDeviceMemory));

    // Create the buffers.
    for (size_t i = 0; i < image_count; i++)
    {
        create_buffer(
            gpu->device, type_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &ubo.buffers[i],
            &ubo.memories[i],
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            gpu->memory_properties);
    }

    return ubo;
}

void vky_upload_uniform_buffer(VkyUniformBuffer* ubo, uint32_t current_image, const void* data)
{
    // log_trace("upload ubo");
    upload_uniform_data(ubo->gpu->device, ubo->memories[current_image], 0, ubo->type_size, data);
}

void vky_bind_resources(
    VkyResourceLayout resource_layout, VkDescriptorSet* descriptor_sets, void** resources)
{
    log_trace("bind resources");
    VkyGpu* gpu = resource_layout.gpu;

    ASSERT(resource_layout.binding_count <= 100);

    for (size_t i = 0; i < resource_layout.image_count; i++)
    {
        VkWriteDescriptorSet descriptor_writes[100];

        // NOTE: need to instantiate this here and not in the inner loop
        // to ensure the descriptor writes get the right buffers.
        VkDescriptorBufferInfo buffer_infos[100];
        VkDescriptorImageInfo image_infos[100];

        for (size_t j = 0; j < resource_layout.binding_count; j++)
        {

            VkDescriptorType binding_type = resource_layout.binding_types[j];

            if (binding_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
                binding_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
            {
                log_trace("bind uniform for binding point %d", j);
                // Uniform buffers.
                VkyUniformBuffer* ubo = (VkyUniformBuffer*)resources[j];

                buffer_infos[j].buffer = ubo->buffers[i];
                buffer_infos[j].offset = 0;
                buffer_infos[j].range = ubo->type_size;
            }
            else if (binding_type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
            {
                VkyBufferRegion* buffer = (VkyBufferRegion*)resources[j];
                buffer_infos[j].buffer = buffer->buffer->raw_buffer;
                buffer_infos[j].offset = buffer->offset;
                buffer_infos[j].range = buffer->size;
            }
            else
            {
                // Samplers.
                // typically here, binding_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                log_trace("bind texture for binding point %d", j);
                VkyTexture* texture = (VkyTexture*)resources[j];
                if (texture != NULL)
                {
                    ASSERT(texture->image_view != NULL);
                    image_infos[j].imageLayout = texture->params.layout;
                    image_infos[j].imageView = texture->image_view;
                    image_infos[j].sampler = texture->sampler;
                }
                else
                {
                    log_error("no texture bound");
                }
            }
            descriptor_writes[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_writes[j].pNext = VK_NULL_HANDLE;
            descriptor_writes[j].dstSet = descriptor_sets[i];
            descriptor_writes[j].dstBinding = j;
            descriptor_writes[j].dstArrayElement = 0;
            descriptor_writes[j].descriptorCount = 1;
            descriptor_writes[j].descriptorType = binding_type;
            descriptor_writes[j].pImageInfo = &image_infos[j];
            descriptor_writes[j].pBufferInfo = &buffer_infos[j];
            descriptor_writes[j].pTexelBufferView = VK_NULL_HANDLE;
        }

        vkUpdateDescriptorSets(
            gpu->device, resource_layout.binding_count, descriptor_writes, 0, NULL);
    }
}

void vky_destroy_uniform_buffer(VkyUniformBuffer* ubo)
{
    log_trace("destroy ubo");
    for (size_t i = 0; i < ubo->image_count; i++)
    {
        vkDestroyBuffer(ubo->gpu->device, ubo->buffers[i], NULL);
        vkFreeMemory(ubo->gpu->device, ubo->memories[i], NULL);
    }
    free(ubo->buffers);
    free(ubo->memories);
}



/*************************************************************************************************/
/*  Dynamic uniform buffers                                                                      */
/*************************************************************************************************/

VkyUniformBuffer vky_create_dynamic_uniform_buffer(
    VkyGpu* gpu, uint32_t image_count, uint32_t item_count, VkDeviceSize type_size)
{
    VkyUniformBuffer dubo = {0};
    dubo.gpu = gpu;

    dubo.image_count = image_count;
    dubo.type_size = type_size;
    dubo.alignment = vky_compute_dynamic_alignment(gpu, type_size);
    dubo.item_count = item_count;
    dubo.buffer_size = item_count * dubo.alignment;
    log_trace("create dynamic ubo %d", dubo.buffer_size);

    ASSERT(item_count >= 1);
    ASSERT(dubo.type_size <= 256);
    ASSERT(dubo.alignment >= 256);

    dubo.buffers = calloc(image_count, sizeof(VkBuffer));
    dubo.memories = calloc(image_count, sizeof(VkDeviceMemory));
    dubo.cdata = calloc(image_count, sizeof(void*));

    // Allocate the aligned buffer.
#if OS_MACOS
    posix_memalign((void**)&dubo.data, dubo.alignment, dubo.buffer_size);
#elif OS_WIN32
    dubo.data = _aligned_malloc(dubo.buffer_size, dubo.alignment);
#else
    dubo.data = aligned_alloc(dubo.alignment, dubo.buffer_size);
#endif

    if (dubo.data == NULL)
    {
        log_error("failed making the aligned allocation of the dynamic uniform buffer");
    }

    // Create the buffers.
    for (size_t i = 0; i < image_count; i++)
    {
        create_buffer(
            gpu->device, dubo.buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &dubo.buffers[i],
            &dubo.memories[i],
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            gpu->memory_properties);

        // Map the dynamic uniform buffers at the beginning.
        vkMapMemory(gpu->device, dubo.memories[i], 0, dubo.buffer_size, 0, &dubo.cdata[i]);
    }

    return dubo;
}

void* vky_get_dynamic_uniform_pointer(VkyUniformBuffer* dubo, uint32_t item_index)
{
    // Get a pointer to a given item in the dynamic uniform buffer, to update it.
    return (void*)(((uint64_t)dubo->data + (item_index * dubo->alignment)));
}

void vky_upload_dynamic_uniform_buffer(VkyUniformBuffer* dubo, uint32_t current_image)
{
    // log_trace("upload dubo");
    // NOTE: the cdata is an array of void* pointers, initialized at creation and freed at
    // destruction. They are mapped and unmapped at creation and destruction.
    memcpy(dubo->cdata[current_image], dubo->data, dubo->buffer_size);
}

void vky_bind_dynamic_uniform(
    VkCommandBuffer command_buffer, VkyGraphicsPipeline* pipeline, VkyUniformBuffer* dubo,
    uint32_t current_image, uint32_t item_index)
{
    ASSERT(pipeline->resource_layout.dynamic_binding_count <= 100);
    uint32_t dynamicOffsets[100];
    // TODO: different item_index for different dynamic uniform descriptor sets.
    // Compute the dynamic offset in each dynamic uniform.
    for (uint32_t i = 0; i < pipeline->resource_layout.dynamic_binding_count; i++)
    {
        // HACK: we assume that all dynamic uniform buffers have the same alignment here.
        if (i >= 1)
        {
            log_trace("WARN: multiple dynamic uniform buffers with different alignments not "
                      "supported currently");
        }
        dynamicOffsets[i] = item_index * dubo->alignment;
    }
    vkCmdBindDescriptorSets(
        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline_layout, 0, 1,
        &pipeline->descriptor_sets[current_image], pipeline->resource_layout.dynamic_binding_count,
        dynamicOffsets);
}

void vky_destroy_dynamic_uniform_buffer(VkyUniformBuffer* dubo)
{
    log_trace("destroy dubo");
    for (size_t i = 0; i < dubo->image_count; i++)
    {
        vkUnmapMemory(dubo->gpu->device, dubo->memories[i]);

        vkDestroyBuffer(dubo->gpu->device, dubo->buffers[i], NULL);
        vkFreeMemory(dubo->gpu->device, dubo->memories[i], NULL);
    }
    free(dubo->buffers);
    free(dubo->memories);
    free(dubo->data);
    free(dubo->cdata);
}
