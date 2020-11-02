/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#ifndef ENABLE_VALIDATION_LAYERS
#define ENABLE_VALIDATION_LAYERS 1
#endif

#define STR(r)                                                                                    \
    case VK_##r:                                                                                  \
        str = #r;                                                                                 \
        break
#define noop

static inline void check_result(VkResult res)
{
    char* str = "UNKNOWN_ERROR";
    switch (res)
    {
        STR(NOT_READY);
        STR(TIMEOUT);
        STR(EVENT_SET);
        STR(EVENT_RESET);
        STR(INCOMPLETE);
        STR(ERROR_OUT_OF_HOST_MEMORY);
        STR(ERROR_OUT_OF_DEVICE_MEMORY);
        STR(ERROR_INITIALIZATION_FAILED);
        STR(ERROR_DEVICE_LOST);
        STR(ERROR_MEMORY_MAP_FAILED);
        STR(ERROR_LAYER_NOT_PRESENT);
        STR(ERROR_EXTENSION_NOT_PRESENT);
        STR(ERROR_FEATURE_NOT_PRESENT);
        STR(ERROR_INCOMPATIBLE_DRIVER);
        STR(ERROR_TOO_MANY_OBJECTS);
        STR(ERROR_FORMAT_NOT_SUPPORTED);
        STR(ERROR_SURFACE_LOST_KHR);
        STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
        STR(SUBOPTIMAL_KHR);
        STR(ERROR_OUT_OF_DATE_KHR);
        STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
        STR(ERROR_VALIDATION_FAILED_EXT);
        STR(ERROR_INVALID_SHADER_NV);
    default:
        noop;
    }
    if (res != VK_SUCCESS)
    {
        log_error("VkResult is %s in %s at line %s", str, __FILE__, __LINE__);
    }
}

#define VK_CHECK_RESULT(f)                                                                        \
    {                                                                                             \
        VkResult res = (f);                                                                       \
        check_result(res);                                                                        \
    }



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

// Validation layers.
static const char* layers[] = {"VK_LAYER_KHRONOS_validation"};

// Required device extensions.
static const char* device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};



/*************************************************************************************************/
/*  Misc                                                                                         */
/*************************************************************************************************/

static VkDeviceSize texture_size_bytes(VkyTextureParams params)
{
    return params.width * params.height * params.depth * params.format_bytes;
}


static uint64_t next_pow2(uint64_t x)
{
    uint64_t p = 1;
    while (p < x)
        p *= 2;
    return p;
}


static size_t compute_dynamic_alignment(size_t dynamic_alignment, size_t min_ubo_alignment)
{
    if (min_ubo_alignment > 0)
    {
        dynamic_alignment = (dynamic_alignment + min_ubo_alignment - 1) & ~(min_ubo_alignment - 1);
    }
    dynamic_alignment = next_pow2(dynamic_alignment);
    return dynamic_alignment;
}


static uint32_t find_memory_type(
    uint32_t typeFilter, VkMemoryPropertyFlags properties,
    VkPhysicalDeviceMemoryProperties mem_properties)
{
    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
    {
        if ((typeFilter & (uint32_t)(1 << i)) &&
            (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }
    log_error("could not find an appropriate memory type");
    return 0;
}



/*************************************************************************************************/
/*  Validation layers                                                                            */
/*************************************************************************************************/

static VkResult create_debug_utils_messenger_EXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != NULL)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}


static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    // HACK: hide harmless warning message on Ubuntu:
    // validation layer: /usr/lib/i386-linux-gnu/libvulkan_radeon.so: wrong ELF class: ELFCLASS32
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT &&
        strstr(pCallbackData->pMessage, "ELFCLASS32") == NULL)
        log_error("validation layer: %s", pCallbackData->pMessage);
    return VK_FALSE;
}


static void destroy_debug_utils_messenger_EXT(
    VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger,
    const VkAllocationCallbacks* pAllocator)
{
    PFN_vkDestroyDebugUtilsMessengerEXT func =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != NULL)
    {
        func(instance, debug_messenger, pAllocator);
    }
}


static bool check_validation_layer_support(
    const uint32_t validation_layers_count, const char** validation_layers)
{
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, NULL);

    VkLayerProperties* available_layers = calloc(layer_count, sizeof(VkLayerProperties));
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

    for (uint32_t i = 0; i < validation_layers_count; i++)
    {
        bool layerFound = false;
        const char* layerName = validation_layers[i];
        for (uint32_t j = 0; j < layer_count; j++)
        {
            if (strcmp(layerName, available_layers[j].layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }
        if (!layerFound)
        {
            FREE(available_layers);
            return false;
        }
    }
    FREE(available_layers);
    return true;
}



/*************************************************************************************************/
/*  Instance and device creation                                                                 */
/*************************************************************************************************/

static void create_instance(
    uint32_t required_extension_count, const char** required_extensions, //
    VkInstance* instance, VkDebugUtilsMessengerEXT* debug_messenger)
{
    // Validation layers.
    // const char* layers[] = {"VK_LAYER_KHRONOS_validation"};

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
    log_trace("create instance");
    VK_CHECK_RESULT(vkCreateInstance(&createInfo, NULL, instance));

    // Create the debug utils messenger.
    if (has_validation)
    {
        log_trace("create debug utils messenger");
        VK_CHECK_RESULT(create_debug_utils_messenger_EXT(
            *instance, &debug_create_info, NULL, debug_messenger));
    }
}


static void pick_device(
    VkInstance instance, VkPhysicalDevice* physical_device,
    VkPhysicalDeviceProperties* device_properties, VkPhysicalDeviceFeatures* device_features,
    VkPhysicalDeviceMemoryProperties* memory_properties)
{
    // Pick the physical device.
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, NULL);
    if (device_count == 0)
    {
        log_error("no compatible device found! aborting");
        exit(1);
    }
    VkPhysicalDevice* physical_devices = calloc(device_count, sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(instance, &device_count, physical_devices);

    int i = 0;
    for (i = 0; i < (int)device_count; i++)
    {
        *physical_device = physical_devices[i];
        vkGetPhysicalDeviceProperties(*physical_device, device_properties);
        vkGetPhysicalDeviceFeatures(*physical_device, device_features);
        vkGetPhysicalDeviceMemoryProperties(*physical_device, memory_properties);
        log_debug("found device #%d: %s", i, device_properties->deviceName);
    }
    // By default, select the first device.
    // TODO: better selection of the device depending on the capabililties etc.
    i = vky_env_int("VKY_DEVICE", 0);
    if (i < 0 || i >= (int)device_count)
    {
        log_error("invalid device number %d: should be between 0 and %d", i, 0, device_count - 1);
        i = 0;
    }
    *physical_device = physical_devices[i];
    FREE(physical_devices);

    vkGetPhysicalDeviceProperties(*physical_device, device_properties);
    vkGetPhysicalDeviceFeatures(*physical_device, device_features);
    vkGetPhysicalDeviceMemoryProperties(*physical_device, memory_properties);
    log_info("select device #%d: %s", i, device_properties->deviceName);
}


static VkyQueueFamilyIndices find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface)
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

        if (graphics_found && present_found && compute_found)
            break;
    }

    // Find the number of different queues.
    uint32_t queue_count = 0;
    if (indices.graphics_family == indices.present_family &&
        indices.present_family == indices.compute_family)
        queue_count = 1;
    else
    {
        if (indices.graphics_family != indices.present_family &&
            indices.graphics_family != indices.compute_family &&
            indices.present_family != indices.compute_family)
            queue_count = 3;
        else
            queue_count = 2;
    }
    indices.queue_count = queue_count;
    ASSERT(graphics_found && present_found && compute_found);
    log_trace(
        "%d queue families: graphics %d, present %d, compute %d", //
        queue_count, indices.graphics_family, indices.present_family, indices.compute_family);

    return indices;
}


static void create_queue_info(
    VkPhysicalDevice physical_device, VkyQueueFamilyIndices* indices, VkSurfaceKHR* surface,
    VkDeviceQueueCreateInfo* queue_create_infos)
{
    if (surface != NULL)
        *indices = find_queue_families(physical_device, *surface);
    else
        *indices = (VkyQueueFamilyIndices){0, 0, 0, 1};

    // Queues.
    float queue_priority = 1.0f;
    uint32_t family[3] = {
        indices->graphics_family, indices->present_family, indices->compute_family};

    // HACK: handle the degenerate case where 2 queue indices are equal and the third is different.
    // In this case we must ensure that the second queue create info corresponds to a number
    // that is different from the first queue, so that we correctly create the 2 different queues.
    if (indices->queue_count == 2 && indices->graphics_family == indices->present_family)
        family[1] = indices->compute_family;

    for (uint32_t i = 0; i < indices->queue_count; i++)
    {
        queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos[i].pNext = NULL;
        queue_create_infos[i].flags = 0;
        queue_create_infos[i].queueFamilyIndex = family[i];
        queue_create_infos[i].queueCount = 1;
        queue_create_infos[i].pQueuePriorities = &queue_priority;
    }
}


static void add_device_extensions(VkSurfaceKHR* surface, VkDeviceCreateInfo* device_create_info)
{
    // Device extensions.
    if (surface != NULL)
    {
        device_create_info->enabledExtensionCount = 1;
        device_create_info->ppEnabledExtensionNames = device_extensions;
    }
    else
    {
        device_create_info->enabledExtensionCount = 0;
        device_create_info->ppEnabledExtensionNames = NULL;
    }
}


static void add_device_layers(bool has_validation, VkDeviceCreateInfo* device_create_info)
{
    if (has_validation)
    {
        device_create_info->enabledLayerCount = 1;
        device_create_info->ppEnabledLayerNames = layers;
    }
    else
    {
        device_create_info->enabledLayerCount = 0;
    }
}


static void allocate_command_buffers(
    VkDevice device, VkCommandPool command_pool, uint32_t count, VkCommandBuffer* cmd_bufs)
{
    ASSERT(count > 0);
    log_trace("allocate %d command buffers", count);
    VkCommandBufferAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = count;
    VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &alloc_info, cmd_bufs));
}


static void
create_command_pool(VkDevice device, uint32_t queue_family_index, VkCommandPool* cmd_pool)
{
    log_trace("create command pool");
    VkCommandPoolCreateInfo command_pool_info = {0};
    command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_info.queueFamilyIndex = queue_family_index;
    VK_CHECK_RESULT(vkCreateCommandPool(device, &command_pool_info, NULL, cmd_pool));
}


static void create_render_pass(
    VkDevice device, VkFormat format, VkImageLayout initial_layout, VkImageLayout final_layout,
    VkAttachmentLoadOp load_op, bool has_depth_attachment, VkRenderPass* render_pass)
{
    log_trace("create render pass");
    VkAttachmentDescription colorAttachment = {0};
    colorAttachment.format = format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = load_op;
    // do_clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = initial_layout;
    colorAttachment.finalLayout = final_layout;

    // Color attachment.
    VkAttachmentReference colorAttachmentRef = {0};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Subpass.
    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    // Depth attachment.
    VkAttachmentDescription depthAttachment = {0};
    VkAttachmentReference depthAttachmentRef = {0};
    if (has_depth_attachment)
    {
        depthAttachment.format = VK_FORMAT_D32_SFLOAT;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = load_op;
        // do_clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        subpass.pDepthStencilAttachment = &depthAttachmentRef;
    }

    VkSubpassDependency dependency = {0};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachments[2] = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo render_pass_info = {0};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = has_depth_attachment ? 2 : 1;
    render_pass_info.pAttachments = attachments;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    VK_CHECK_RESULT(vkCreateRenderPass(device, &render_pass_info, NULL, render_pass));
}


static void begin_render_pass(
    VkRenderPass render_pass, VkCommandBuffer cmd_buf, VkFramebuffer framebuffer, uint32_t width,
    uint32_t height, VkyColor* clear_color, bool clear_depth)
{
    VkRenderPassBeginInfo render_pass_info = {0};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = render_pass;
    render_pass_info.framebuffer = framebuffer;
    VkRect2D renderArea = {{0, 0}, {width, height}};
    render_pass_info.renderArea = renderArea;

    VkClearValue clear_color_value = {0};
    if (clear_color != NULL)
    {
        clear_color_value.color.float32[0] = (float)clear_color->rgb[0] / 255.0f;
        clear_color_value.color.float32[1] = (float)clear_color->rgb[1] / 255.0f;
        clear_color_value.color.float32[2] = (float)clear_color->rgb[2] / 255.0f;
        clear_color_value.color.float32[3] = (float)clear_color->alpha / 255.0f;
    }

    VkClearValue clear_depth_value = {0};
    clear_depth_value.depthStencil.depth = 1.0f;
    clear_depth_value.depthStencil.stencil = 0;

    VkClearValue clear_values[] = {clear_color_value, clear_depth_value};
    render_pass_info.clearValueCount =
        (uint32_t)(clear_color != NULL ? 1 : 0) + (uint32_t)(clear_depth ? 1 : 0);
    render_pass_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(cmd_buf, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
}



/*************************************************************************************************/
/*  Data management                                                                              */
/*************************************************************************************************/

static void transition_image_layout(
    VkDevice device, VkCommandPool command_pool, VkQueue graphics_queue, VkImage image,
    VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout)
{
    log_trace("transition image layout");

    // TODO: refactor with vky_texture_barrier()

    VkCommandBuffer command_buffer = begin_single_time_commands(device, command_pool);

    VkImageMemoryBarrier barrier = {0};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags destination_stage;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
        new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (
        old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
        new_layout != VK_IMAGE_LAYOUT_UNDEFINED)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        log_error("image transition failed with new layout %d", new_layout);
    }

    vkCmdPipelineBarrier(
        command_buffer, source_stage, destination_stage, 0, 0, NULL, 0, NULL, 1, &barrier);

    end_single_time_commands(device, command_pool, &command_buffer, graphics_queue);
}


static void copy_buffer(
    VkDevice device, VkCommandPool command_pool, VkQueue graphics_queue, VkBuffer src_buffer,
    VkBuffer dst_buffer, VkBufferCopy copy_region, VkBufferMemoryBarrier* barrier)
{
    VkCommandBuffer command_buffer = begin_single_time_commands(device, command_pool);
    vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);
    // Barrier when updating a storage vertex buffer also used for compute, and when the graphics
    // and compute queue families do not match.
    if (barrier != NULL)
    {
        vkCmdPipelineBarrier(
            command_buffer, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL, 1, barrier, 0, NULL);
    }
    end_single_time_commands(device, command_pool, &command_buffer, graphics_queue);
}


static void copy_buffer_to_image(
    VkDevice device, VkCommandPool command_pool, VkQueue graphics_queue, VkBuffer buffer,
    VkImage image, uint32_t width, uint32_t height, uint32_t depth)
{
    VkCommandBuffer command_buffer = begin_single_time_commands(device, command_pool);
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

    ASSERT(width > 0);
    ASSERT(height > 0);
    ASSERT(depth > 0);

    region.imageExtent.width = width;
    region.imageExtent.height = height;
    region.imageExtent.depth = depth;

    vkCmdCopyBufferToImage(
        command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    end_single_time_commands(device, command_pool, &command_buffer, graphics_queue);
}


static void upload_data_to_buffer(
    VkDevice device, VkBufferCopy copy_region, const void* data, VkBuffer buffer,
    VkPhysicalDeviceMemoryProperties memory_properties, VkCommandPool command_pool,
    VkQueue graphics_queue, VkBufferMemoryBarrier* barrier)
{

    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    VkDeviceSize size = copy_region.size;

    create_buffer(
        device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &staging_buffer, &staging_buffer_memory,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        memory_properties);

    void* cdata = NULL;
    vkMapMemory(device, staging_buffer_memory, 0, size, 0, &cdata);
    memcpy(cdata, data, size);
    vkUnmapMemory(device, staging_buffer_memory);

    copy_buffer(
        device, command_pool, graphics_queue, staging_buffer, buffer, copy_region, barrier);

    vkDestroyBuffer(device, staging_buffer, NULL);
    vkFreeMemory(device, staging_buffer_memory, NULL);
}


static void upload_uniform_data(
    VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size,
    const void* data)
{
    void* cdata = NULL;
    vkMapMemory(device, memory, offset, size, 0, &cdata);
    memcpy(cdata, data, size);
    vkUnmapMemory(device, memory);
}
