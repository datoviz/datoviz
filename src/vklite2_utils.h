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

    if (surface != 0)
    {
        log_trace("destroy surface");
        vkDestroySurfaceKHR(instance, surface, NULL);
    }

    switch (backend)
    {
    case VKL_BACKEND_GLFW:
        glfwPollEvents();
        glfwDestroyWindow(window);
        break;
    default:
        break;
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
    ASSERT(queues->indices != NULL);

    // Get the queue family properties.
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queues->queue_count, NULL);
    ASSERT(queues->queue_count);

    VkQueueFamilyProperties* queue_families =
        calloc(queues->queue_count, sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queues->queue_count, queue_families);

    queues->indices[0] = queues->indices[1] = queues->indices[2] = -1;

    for (int32_t i = 0; i < (int)queues->queue_count; i++)
    {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            queues->indices[VKL_QUEUE_GRAPHICS] = i;
        if (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
            queues->indices[VKL_QUEUE_COMPUTE] = i;
        if (queues->indices[VKL_QUEUE_GRAPHICS] >= 0 && queues->indices[VKL_QUEUE_COMPUTE] >= 0)
            break;
    }

    ASSERT(queues->indices[VKL_QUEUE_GRAPHICS] >= 0);
    ASSERT(queues->indices[VKL_QUEUE_COMPUTE] >= 0);

    log_trace(
        "queue families: graphics %d, compute %d", //
        queues->indices[VKL_QUEUE_GRAPHICS], queues->indices[VKL_QUEUE_COMPUTE]);

    FREE(queue_families);
}



static void discover_gpu(VkPhysicalDevice physical_device, VklGpu* gpu)
{
    vkGetPhysicalDeviceProperties(physical_device, &gpu->device_properties);
    vkGetPhysicalDeviceFeatures(physical_device, &gpu->device_features);
    vkGetPhysicalDeviceMemoryProperties(physical_device, &gpu->memory_properties);

    gpu->physical_device = physical_device;
    gpu->name = gpu->device_properties.deviceName;
    gpu->cmd_pool_count = 2;

    find_queue_families(gpu->physical_device, &gpu->queues);
}



static void
find_present_queue_family(VkPhysicalDevice device, VkSurfaceKHR surface, VklQueues* queues)
{
    if (surface == 0)
        return;
    VkBool32 presentSupport = false;
    for (int32_t i = 0; i < (int)queues->queue_count; i++)
    {
        vkGetPhysicalDeviceSurfaceSupportKHR(device, (uint32_t)i, surface, &presentSupport);
        if (presentSupport)
        {
            queues->indices[VKL_QUEUE_PRESENT] = i;
            break;
        }
    }
}



static void
create_command_pool(VkDevice device, int32_t queue_family_index, VkCommandPool* cmd_pool)
{
    log_trace("starting creation of command pool...");
    ASSERT(queue_family_index >= 0);
    VkCommandPoolCreateInfo command_pool_info = {0};
    command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_info.queueFamilyIndex = (uint32_t)queue_family_index;
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

    find_present_queue_family(gpu->physical_device, surface, &gpu->queues);

    // Create the queue info structure.
    VkDeviceQueueCreateInfo queues_info[3] = {0};
    float queue_priority = 1.0f;
    for (uint32_t i = 0; i < 3; i++)
    {
        queues_info[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queues_info[i].queueCount = 1;
        queues_info[i].pQueuePriorities = &queue_priority;
    }
    // Need to create unique queues only.
    uint32_t queue_count = 1;
    // There is at least one queue.
    queues_info[0].queueFamilyIndex = (uint32_t)gpu->queues.indices[0];
    // We add a second one if its index is different.
    if (gpu->queues.indices[1] >= 0 && gpu->queues.indices[1] != gpu->queues.indices[0])
    {
        queues_info[1].queueFamilyIndex = (uint32_t)gpu->queues.indices[1];
        queue_count++;
    }
    // We add a third one if its index is different.
    if (gpu->queues.indices[2] >= 0 && gpu->queues.indices[2] != gpu->queues.indices[0] &&
        gpu->queues.indices[2] != gpu->queues.indices[1])
    {
        queues_info[2].queueFamilyIndex = (uint32_t)gpu->queues.indices[2];
        queue_count++;
    }

    VkDeviceCreateInfo device_info = {0};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    // If there is no surface, the third queue (present queue) is not created.
    log_trace("creating %d distinct queue(s)", queue_count);
    device_info.queueCreateInfoCount = queue_count;
    device_info.pQueueCreateInfos = queues_info;

    // Requested features
    device_info.pEnabledFeatures = &gpu->requested_features;

    // Device extensions and layers
    device_info.enabledExtensionCount = (uint32_t)has_surface;
    device_info.ppEnabledExtensionNames = has_surface ? device_extensions : NULL;
    device_info.enabledLayerCount = (uint32_t)has_validation;
    device_info.ppEnabledLayerNames = has_validation ? layers : NULL;

    // Create the device
    VK_CHECK_RESULT(vkCreateDevice(gpu->physical_device, &device_info, NULL, &gpu->device));

    // Get device queues
    for (uint32_t i = 0; i < 2 + (uint32_t)has_surface; i++)
    {
        ASSERT(gpu->queues.indices[i] >= 0);
        vkGetDeviceQueue(gpu->device, (uint32_t)gpu->queues.indices[i], 0, &gpu->queues.queues[i]);
    }
    log_trace("device created");
}



/*************************************************************************************************/
/*  Window                                                                                       */
/*************************************************************************************************/



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
