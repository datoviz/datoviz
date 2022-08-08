/*************************************************************************************************/
/*  Vklite utils                                                                                 */
/*************************************************************************************************/

#ifndef DVZ_HEADER_VKUTILS
#define DVZ_HEADER_VKUTILS



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#ifndef ENABLE_VALIDATION_LAYERS
#define ENABLE_VALIDATION_LAYERS 1
#endif

// Validation layers.
static const char* DVZ_LAYERS[] = {"VK_LAYER_KHRONOS_validation"};

// Ignore some validation warnings.
static const char* VALIDATION_IGNORES[] = {

    // HACK: hide harmless warning message on Ubuntu:
    // validation layer: /usr/lib/i386-linux-gnu/libvulkan_radeon.so: wrong ELF class: ELFCLASS32
    "ELFCLASS32",

    "BestPractices-vkBindMemory-small-dedicated-allocation",
    "BestPractices-vkAllocateMemory-small-allocation",
    "BestPractices-vkCreateCommandPool-command-buffer-reset",
    "BestPractices-vkCreateInstance-specialuse-extension",

    // prevent unnecessary error messages when quickly resizing a window (race condition, fix to be
    // done probably in the validation layers)
    // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/624
    "VUID-VkSwapchainCreateInfoKHR-imageExtent",

    // https://github.com/datoviz/datoviz/issues/17#issuecomment-849213008
    // https://www.gitmemory.com/issue/KhronosGroup/Vulkan-ValidationLayers/2729/824406355
    // "invalid layer manifest file",

    "but only supports loader interface version 4",

    "VkImageMemoryBarrier is being submitted with oldLayout VK_IMAGE_LAYOUT_UNDEFINED",
};



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define STR(r)                                                                                    \
    case VK_##r:                                                                                  \
        str = #r;                                                                                 \
        break
#define noop

static inline int check_result(VkResult res)
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
        log_error("VkResult is %s in %s at line %d", str, __FILE__, __LINE__);
        return 1;
    }
    return 0;
}

#define VK_CHECK_RESULT(f)                                                                        \
    {                                                                                             \
        VkResult res = (f);                                                                       \
        check_result(res);                                                                        \
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



static inline int _log_level(VkDebugUtilsMessageSeverityFlagBitsEXT sev)
{
    switch (sev)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        return LOG_TRACE;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        return LOG_DEBUG;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        return LOG_WARN;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        return LOG_ERROR;
        break;
    default:
        break;
    }
    return LOG_ERROR;
}



static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    // Hide long list of extensions.
    if (strstr(pCallbackData->pMessage, "Extension: VK_") != NULL)
        return VK_FALSE;

    int level = _log_level(messageSeverity);

    // NOTE: force TRACE level if ignored message.
    for (uint32_t i = 0; i < ARRAY_COUNT(VALIDATION_IGNORES); i++)
    {
        if (strstr(pCallbackData->pMessage, VALIDATION_IGNORES[i]) != NULL)
        {
            level = LOG_TRACE;
            break;
        }
    }

    log_log(level, __FILENAME__, __LINE__, "validation layer: %s", pCallbackData->pMessage);
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT && pUserData != NULL)
    {
        uint32_t* n_errors = (uint32_t*)pUserData;
        (*n_errors)++;
    }
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



static void create_instance(
    uint32_t required_extension_count, const char** required_extensions, //
    VkInstance* instance, VkDebugUtilsMessengerEXT* debug_messenger, void* debug_data)
{
    log_trace("starting creation of instance...");

    // Add ext debug extension.
    bool has_validation = false;
    if (ENABLE_VALIDATION_LAYERS)
    {
        has_validation = check_validation_layer_support(ARRAY_COUNT(DVZ_LAYERS), DVZ_LAYERS);
        if (!has_validation)
            log_warn(
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
    appInfo.engineVersion = APPLICATION_VERSION;
    appInfo.apiVersion = DVZ_VULKAN_API;

    VkInstanceCreateInfo info_inst = {0};
    info_inst.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info_inst.pApplicationInfo = &appInfo;
    info_inst.enabledExtensionCount = extension_count;
    info_inst.ppEnabledExtensionNames = extensions;

    // Validation layers.
    VkDebugUtilsMessengerCreateInfoEXT info_debug = {0};
    info_debug.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info_debug.flags = 0;
    info_debug.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info_debug.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info_debug.pfnUserCallback = debug_callback;
    info_debug.pUserData = debug_data;

    VkValidationFeatureEnableEXT enables[16] = {0};
    VkValidationFeaturesEXT features = {0};

    if (has_validation)
    {
        log_debug("enable Vulkan validation layer");
        info_inst.enabledLayerCount = ARRAY_COUNT(DVZ_LAYERS);
        info_inst.ppEnabledLayerNames = DVZ_LAYERS;

        // https://vulkan.lunarg.com/doc/sdk/1.2.170.0/linux/best_practices.html
        enables[0] = VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT;
        enables[1] = VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT;
        enables[2] = VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT;

        features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
        features.enabledValidationFeatureCount = 3;
        features.pEnabledValidationFeatures = enables;

        // pNext chain
        features.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&info_debug;
        info_inst.pNext = &features;
    }
    else
    {
        log_warn("disable Vulkan validation layer");
        info_inst.enabledLayerCount = 0;
        info_inst.pNext = NULL;
    }

    // Create the Vulkan instance.
    log_trace("create instance");
    VK_CHECK_RESULT(vkCreateInstance(&info_inst, NULL, instance));

    // Create the debug utils messenger.
    if (has_validation)
    {
        log_trace("create debug utils messenger");
        VK_CHECK_RESULT(
            create_debug_utils_messenger_EXT(*instance, &info_debug, NULL, debug_messenger));
    }

    log_trace("instance created");
}



// static void destroy_instance(DvzHost* host)
// {
//     log_trace("starting destruction of instance...");

//     if (host->debug_messenger)
//         destroy_debug_utils_messenger_EXT(host->instance, host->debug_messenger, NULL);

//     // Destroy the instance.
//     log_trace("destroy instance");
//     if (host->instance != VK_NULL_HANDLE)
//     {
//         vkDestroyInstance(host->instance, NULL);
//         host->instance = VK_NULL_HANDLE;
//     }

//     log_trace("instance destroyed");
// }



static void find_queue_families(VkPhysicalDevice device, DvzQueues* queues)
{
    ASSERT(device != VK_NULL_HANDLE);
    ASSERT(queues != NULL);

    // Get the queue family properties.
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queues->queue_family_count, NULL);
    log_trace("found %d queue families", queues->queue_family_count);
    ASSERT(queues->queue_family_count > 0);
    ASSERT(queues->queue_family_count <= DVZ_MAX_QUEUE_FAMILIES);
    VkQueueFamilyProperties queue_families[DVZ_MAX_QUEUE_FAMILIES];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queues->queue_family_count, queue_families);
    ASSERT(queues->queue_family_count <= DVZ_MAX_QUEUE_FAMILIES);

    for (uint32_t i = 0; i < queues->queue_family_count; i++)
    {
        queues->support_transfer[i] = queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT;
        queues->support_graphics[i] = queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
        queues->support_compute[i] = queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT;
        queues->max_queue_count[i] = queue_families[i].queueCount;
        log_trace(
            "queue family #%d (max %d): transfer %d, graphics %d, compute %d", //
            i, queues->max_queue_count[i],                                     //
            queues->support_transfer[i], queues->support_graphics[i], queues->support_compute[i]);
    }
}



static void discover_gpu(VkPhysicalDevice physical_device, DvzGpu* gpu)
{
    vkGetPhysicalDeviceProperties(physical_device, &gpu->device_properties);
    vkGetPhysicalDeviceFeatures(physical_device, &gpu->device_features);
    vkGetPhysicalDeviceMemoryProperties(physical_device, &gpu->memory_properties);

    gpu->physical_device = physical_device;
    gpu->name = gpu->device_properties.deviceName;

    // Find the total amount of VRAM.
    VkMemoryHeap* heap = NULL;
    for (uint32_t j = 0; j < gpu->memory_properties.memoryHeapCount; j++)
    {
        heap = &gpu->memory_properties.memoryHeaps[j];
        if (heap->size > 0 && ((heap->flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0))
        {
            ASSERT(heap->size > 0);
            gpu->vram += heap->size;
        }
    }

    find_queue_families(gpu->physical_device, &gpu->queues);
}



#endif
