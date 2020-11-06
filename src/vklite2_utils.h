#include "../include/visky/vklite2.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#ifndef ENABLE_VALIDATION_LAYERS
#define ENABLE_VALIDATION_LAYERS 1
#endif

// Validation layers.
static const char* layers[] = {"VK_LAYER_KHRONOS_validation"};

// Required device extensions.
static const char* device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

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
/*  Backend-specific code                                                                        */
/*************************************************************************************************/

static const char** backend_extensions(VklBackend backend, uint32_t* required_extension_count)
{
    const char** required_extensions = NULL;

    // Backend initialization and required extensions.
    switch (backend)
    {
    case VKL_BACKEND_GLFW:

        glfwInit();
        ASSERT(glfwVulkanSupported() != 0);
        required_extensions = glfwGetRequiredInstanceExtensions(required_extension_count);
        log_trace("%d extension(s) required by backend GLFW", *required_extension_count);

        break;
    default:
        break;
    }

    return required_extensions;
}



static void* backend_window(
    VkInstance instance, VklBackend backend, uint32_t width, uint32_t height,
    VkSurfaceKHR* surface)
{
    log_trace("create canvas with size %dx%d", width, height);

    switch (backend)
    {
    case VKL_BACKEND_GLFW:
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow* window =
            glfwCreateWindow((int)width, (int)height, APPLICATION_NAME, NULL, NULL);
        ASSERT(window != NULL);
        if (glfwCreateWindowSurface(instance, window, NULL, surface) != VK_SUCCESS)
            log_error("error creating the GLFW surface");
        return window;
        break;
    default:
        break;
    }

    return NULL;
}


static void
backend_window_destroy(VkInstance instance, VklBackend backend, void* window, VkSurfaceKHR surface)
{
    log_trace("starting destruction of backend window...");
    // NOTE TODO: need to vkDeviceWaitIdle(device) on all devices before calling this

    switch (backend)
    {
    case VKL_BACKEND_GLFW:
        glfwPollEvents();
        ASSERT(window != NULL);
        log_trace("destroy GLFW window");
        glfwDestroyWindow(window);
        break;
    default:
        break;
    }

    if (surface != 0)
    {
        log_trace("destroy surface");
        vkDestroySurfaceKHR(instance, surface, NULL);
    }

    log_trace("backend window destroyed");
}



/*************************************************************************************************/
/*  Instance and devices                                                                         */
/*************************************************************************************************/

static void create_instance(
    uint32_t required_extension_count, const char** required_extensions, //
    VkInstance* instance, VkDebugUtilsMessengerEXT* debug_messenger)
{
    log_trace("starting creation of instance...");

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

    log_trace("instance created");
}



static void destroy_instance(VklApp* app)
{
    log_trace("starting destruction of instance...");

    if (app->debug_messenger)
        destroy_debug_utils_messenger_EXT(app->instance, app->debug_messenger, NULL);

    // Destroy the instance.
    log_trace("destroy instance");
    if (app->instance != 0)
    {
        vkDestroyInstance(app->instance, NULL);
        app->instance = 0;
    }

    log_trace("instance destroyed");
}



static void find_queue_families(VkPhysicalDevice device, VklQueues* queues)
{
    ASSERT(device != 0);
    ASSERT(queues != NULL);

    // Get the queue family properties.
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queues->queue_family_count, NULL);
    log_trace("found %d queue families", queues->queue_family_count);
    ASSERT(queues->queue_family_count > 0);
    ASSERT(queues->queue_family_count <= VKL_MAX_QUEUE_FAMILIES);
    VkQueueFamilyProperties queue_families[VKL_MAX_QUEUE_FAMILIES];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queues->queue_family_count, queue_families);
    ASSERT(queues->queue_family_count <= VKL_MAX_QUEUE_FAMILIES);

    for (uint32_t i = 0; i < queues->queue_family_count; i++)
    {
        queues->support_transfer[i] = queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT;
        queues->support_graphics[i] = queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
        queues->support_compute[i] = queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT;
        log_trace(
            "queue family #%d: transfer %d, graphics %d, compute %d", i,
            queues->support_transfer[i], queues->support_graphics[i], queues->support_compute[i]);
    }
}



static void discover_gpu(VkPhysicalDevice physical_device, VklGpu* gpu)
{
    vkGetPhysicalDeviceProperties(physical_device, &gpu->device_properties);
    vkGetPhysicalDeviceFeatures(physical_device, &gpu->device_features);
    vkGetPhysicalDeviceMemoryProperties(physical_device, &gpu->memory_properties);

    gpu->physical_device = physical_device;
    gpu->name = gpu->device_properties.deviceName;

    find_queue_families(gpu->physical_device, &gpu->queues);
}



static void
find_present_queue_family(VkPhysicalDevice device, VkSurfaceKHR surface, VklQueues* queues)
{
    if (surface == 0)
        return;
    VkBool32 presentSupport = false;
    for (uint32_t i = 0; i < queues->queue_family_count; i++)
    {
        presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport)
            log_trace("queue family #%d supports present", i);
        queues->support_present[i] = presentSupport;
    }
}



static void
create_command_pool(VkDevice device, uint32_t queue_family_index, VkCommandPool* cmd_pool)
{
    log_trace("starting creation of command pool...");
    VkCommandPoolCreateInfo command_pool_info = {0};
    command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_info.queueFamilyIndex = queue_family_index;
    VK_CHECK_RESULT(vkCreateCommandPool(device, &command_pool_info, NULL, cmd_pool));
    log_trace("command pool created");
}



static void create_device(VklGpu* gpu, VkSurfaceKHR surface)
{
    log_trace("starting creation of device...");
    ASSERT(gpu != NULL);
    ASSERT(gpu->app != NULL);

    bool has_surface = surface != NULL;
    bool has_validation = gpu->app->debug_messenger != NULL;

    // NOTE: we needed to wait until we had a handle to the surface in order to find
    // the queue families supporting PRESENT.
    find_present_queue_family(gpu->physical_device, surface, &gpu->queues);

    // Here, we need to determine the queue family and queue index of every requested queue,
    // as a function of the requested queue type, and the discovered queue families.
    VklQueues* q = &gpu->queues;

    // First, we compute, for each queue family, the number of queue types it supports
    log_trace("computing queue family scores");
    uint32_t queue_family_score[VKL_MAX_QUEUE_FAMILIES] = {0};
    ASSERT(q->queue_family_count <= VKL_MAX_QUEUE_FAMILIES);
    for (uint32_t i = 0; i < q->queue_family_count; i++)
    {
        queue_family_score[i] += (int)q->support_transfer[i];
        queue_family_score[i] += (int)q->support_graphics[i];
        queue_family_score[i] += (int)q->support_compute[i];
        queue_family_score[i] += (int)q->support_present[i];
    }
    // Then, for each requested queue, we find the matching queue family with the lowest score.
    bool qf_match;
    uint32_t lowest_score;
    ASSERT(q->queue_count <= VKL_MAX_QUEUES);
    for (uint32_t i = 0; i < q->queue_count; i++)
    {
        log_trace("starting search of queue family for requested queue #%d", i);
        qf_match = true;
        lowest_score = 1000;
        // For each possible queue family, determine whether it would fit for the current requested
        // queue.
        for (uint32_t qf = 0; qf < q->queue_family_count; qf++)
        {
            log_trace("looking at queue family %d with score %d", qf, queue_family_score[qf]);
            if ((q->queue_types[i] & VKL_QUEUE_TRANSFER) && !q->support_transfer[qf])
                qf_match = false;
            if ((q->queue_types[i] & VKL_QUEUE_GRAPHICS) && !q->support_graphics[qf])
                qf_match = false;
            if ((q->queue_types[i] & VKL_QUEUE_COMPUTE) && !q->support_compute[qf])
                qf_match = false;
            if ((q->queue_types[i] & VKL_QUEUE_PRESENT) && !q->support_present[qf])
                qf_match = false;
            // This queue family does not match, skipping.
            if (!qf_match)
                continue;
            // The current queue family matches, what is its score?
            if (queue_family_score[qf] <= lowest_score)
            {
                // The best matching queue family so far for the current queue, saving it for now.
                lowest_score = queue_family_score[qf];
                q->queue_families[i] = qf;
                log_trace("queue family %d matches requested queue #%d", qf, i);
            }
        }
        if (lowest_score == 1000)
        {
            log_error("could not find a matching queue family for requested queue #%d", i);
            exit(1);
        }
    }

    // Create the queue info structure.
    float queue_priority = 1.0f;

    // Count the number of queues requested, for each queue family.
    log_trace("counting the number of requested queues for each queue family");
    uint32_t queues_per_family[VKL_MAX_QUEUE_FAMILIES] = {0};
    for (uint32_t i = 0; i < q->queue_count; i++)
    {
        ASSERT(q->queue_families[i] < q->queue_family_count);
        ASSERT(q->queue_families[i] < VKL_MAX_QUEUE_FAMILIES);
        // Also determine the queue index of each requested queue within its queue family.
        q->queue_indices[i] = queues_per_family[q->queue_families[i]];
        // Count the number of queues in each family.
        queues_per_family[q->queue_families[i]]++;
    }

    // Count the number of queue families with at least 1 queue to create.
    log_trace("determining the queue families to create and the number of queues in each");
    uint32_t queue_family_count = 0;
    uint32_t queues_per_family_to_create[VKL_MAX_QUEUE_FAMILIES] = {0};
    uint32_t queue_indices[VKL_MAX_QUEUE_FAMILIES] = {0};
    for (uint32_t i = 0; i < q->queue_family_count; i++)
    {
        if (queues_per_family[i] > 0)
        {
            queues_per_family_to_create[queue_family_count] = queues_per_family[i];
            queue_indices[queue_family_count] = i;
            queue_family_count++;
            log_trace("will create queue family #%d with %d queue(s)", i, queues_per_family[i]);
        }
    }

    // Allocate the queue families info struct array.
    ASSERT(queue_family_count > 0);
    VkDeviceQueueCreateInfo* queue_families_info =
        calloc(queue_family_count, sizeof(VkDeviceQueueCreateInfo));
    for (uint32_t i = 0; i < queue_family_count; i++)
    {
        queue_families_info[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_families_info[i].queueCount = queues_per_family_to_create[i];
        queue_families_info[i].pQueuePriorities = &queue_priority;
        queue_families_info[i].queueFamilyIndex = queue_indices[i];
    }

    VkDeviceCreateInfo device_info = {0};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.queueCreateInfoCount = queue_family_count;
    device_info.pQueueCreateInfos = queue_families_info;

    // Requested features
    device_info.pEnabledFeatures = &gpu->requested_features;

    // Device extensions and layers
    device_info.enabledExtensionCount = (uint32_t)has_surface;
    device_info.ppEnabledExtensionNames = has_surface ? device_extensions : NULL;
    device_info.enabledLayerCount = (uint32_t)has_validation;
    device_info.ppEnabledLayerNames = has_validation ? layers : NULL;

    // Create the device
    VK_CHECK_RESULT(vkCreateDevice(gpu->physical_device, &device_info, NULL, &gpu->device));
    FREE(queue_families_info);
    log_trace("device created");
}



/*************************************************************************************************/
/*  Swapchain                                                                                    */
/*************************************************************************************************/

static void create_swapchain(
    VkDevice device, VkPhysicalDevice pdevice,      //
    VkSurfaceKHR surface, uint32_t image_count,     //
    VkFormat format, VkPresentModeKHR present_mode, //
    VklQueues* queues,                              //
    VkSurfaceCapabilitiesKHR* caps, VkSwapchainKHR* swapchain)
{
    // Swap chain.
    VkSwapchainCreateInfoKHR screateInfo = {0};
    screateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    screateInfo.surface = surface;
    screateInfo.minImageCount = image_count;
    screateInfo.imageFormat = format;
    screateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdevice, surface, caps);

    screateInfo.imageExtent = caps->currentExtent;
    screateInfo.imageArrayLayers = 1;
    screateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    screateInfo.preTransform = caps->currentTransform;
    screateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    screateInfo.presentMode = present_mode;
    screateInfo.clipped = VK_TRUE;

    // Determine which queue families have access to the swapchain images.
    // If there is at least one queue family that supports PRESENT but not GRAPHICS, then the
    // sharing mode will be concurrent, otherwise it is exclusive.
    uint32_t queue_families[VKL_MAX_QUEUE_FAMILIES] = {0};
    uint32_t n = 0;
    for (uint32_t i = 0; i < queues->queue_family_count; i++)
    {
        if (queues->support_graphics[i] || queues->support_present)
            queue_families[n++] = i;
    }
    if (n >= 2)
    {
        screateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        screateInfo.queueFamilyIndexCount = n;
        screateInfo.pQueueFamilyIndices = queue_families;
    }
    else
    {
        screateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    log_trace("create swapchain");
    VK_CHECK_RESULT(vkCreateSwapchainKHR(device, &screateInfo, NULL, swapchain));
}



/*************************************************************************************************/
/*  Canvas                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Command buffers                                                                              */
/*************************************************************************************************/

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
