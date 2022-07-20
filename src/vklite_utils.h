/*************************************************************************************************/
/*  Vklite utils                                                                                 */
/*************************************************************************************************/

#ifndef DVZ_HEADER_VKLITE_UTILS
#define DVZ_HEADER_VKLITE_UTILS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "fileio.h"
#include "vklite.h"
#include "vkutils.h"
#include "window.h"

#include <GLFW/glfw3.h>



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

// Required device extensions.
static const char* DVZ_DEVICE_EXTENSIONS[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzPointer DvzPointer;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzPointer
{
    void* pointer;
    bool aligned;
};



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static VkDeviceSize get_alignment(VkDeviceSize alignment, VkDeviceSize min_alignment)
{
    if (min_alignment > 0)
        alignment = (alignment + min_alignment - 1) & ~(min_alignment - 1);
    alignment = dvz_next_pow2(alignment);
    ASSERT(alignment >= min_alignment);
    return alignment;
}



/*
BIG FAT WARNING: never FREE a pointer returned by aligned_malloc(), use aligned_free() instead.
Otherwise direct crash on Windows.
*/
static void* aligned_malloc(VkDeviceSize size, VkDeviceSize alignment)
{
    void* data = NULL;
    // Allocate the aligned buffer.
#if OS_MACOS
    posix_memalign((void**)&data, alignment, size);
#elif OS_WIN32
    data = _aligned_malloc(size, alignment);
#else
    data = aligned_alloc(alignment, size);
#endif

    if (data == NULL)
        log_error("failed making the aligned allocation of the dynamic uniform buffer");
    return data;
}



/*
WARNING : on Windows, only works on aligned pointers. */
static void aligned_free(void* pointer)
{
#if OS_WIN32
    _aligned_free(pointer);
#else
    FREE(pointer)
#endif
}



static void* aligned_pointer(const void* data, VkDeviceSize alignment, uint32_t idx)
{
    // Get a pointer to a given item in the dynamic uniform buffer, to update it.
    return (void*)(((uint64_t)data + (idx * alignment)));
}



static VkDeviceSize aligned_size(VkDeviceSize size, VkDeviceSize alignment)
{
    if (alignment == 0)
        return size;
    ASSERT(alignment > 0);
    if (size % alignment == 0)
        return size;
    ASSERT(size % alignment < alignment);
    size += (alignment - (size % alignment));
    ASSERT(size % alignment == 0);
    return size;
}



/*
WARNING: returns a wrapped pointer specifiying whether the pointer was aligned or not.
This is needed on Windows because aligned pointers must be freed with _aligned_free(), whereas
normal pointers must be freed with free(). Without a wrapper DvzPointer struct, this function
wouldn't be able to say whether its returned pointer was aligned-allocated or normally allocated.
*/
static DvzPointer
aligned_repeat(VkDeviceSize size, const void* data, uint32_t count, VkDeviceSize alignment)
{
    // Take any buffer and make `count` consecutive aligned copies of it.
    // The caller must free the result.
    VkDeviceSize alsize = alignment > 0 ? get_alignment(size, alignment) : size;
    VkDeviceSize rep_size = alsize * count;
    void* repeated = NULL;
    if (alignment > 0)
        repeated = aligned_malloc(rep_size, alignment);
    else
        repeated = malloc(rep_size);
    ASSERT(repeated != NULL);
    memset(repeated, 0, rep_size);
    for (uint32_t i = 0; i < count; i++)
    {
        memcpy((void*)(((int64_t)repeated) + (int64_t)(i * alsize)), data, size);
    }
    return (DvzPointer){repeated, alignment > 0};
}



static inline VkClearColorValue get_clear_color(cvec4 color)
{
    VkClearColorValue value = {0};
    value.float32[0] = color[0] * M_INV_255;
    value.float32[1] = color[1] * M_INV_255;
    value.float32[2] = color[2] * M_INV_255;
    value.float32[3] = color[3] * M_INV_255;
    return value;
}



/*************************************************************************************************/
/*  Instance and devices                                                                         */
/*************************************************************************************************/

static void
find_present_queue_family(VkPhysicalDevice device, VkSurfaceKHR surface, DvzQueues* queues)
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
    log_trace("create command pool for queue family index #%d", queue_family_index);
    VkCommandPoolCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    info.queueFamilyIndex = queue_family_index;
    VK_CHECK_RESULT(vkCreateCommandPool(device, &info, NULL, cmd_pool));
}



static void create_device(DvzGpu* gpu, VkSurfaceKHR surface)
{
    log_trace("starting creation of device...");
    ASSERT(gpu != NULL);
    ASSERT(gpu->host != NULL);

    bool has_surface = surface != VK_NULL_HANDLE;
    // bool has_validation = gpu->app->debug_messenger != NULL;

    // Find the supported present modes.
    if (surface != VK_NULL_HANDLE)
    {
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            gpu->physical_device, surface, &gpu->present_mode_count, NULL);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            gpu->physical_device, surface, &gpu->present_mode_count, gpu->present_modes);
    }

    // NOTE: we needed to wait until we had a handle to the surface in order to find
    // the queue families supporting PRESENT.
    find_present_queue_family(gpu->physical_device, surface, &gpu->queues);

    // Here, we need to determine the queue family and queue index of every requested queue,
    // as a function of the requested queue type, and the discovered queue families.
    DvzQueues* q = &gpu->queues;

    // First, we compute, for each queue family, the number of queue types it supports
    log_trace("computing queue family scores");
    uint32_t queue_family_score[DVZ_MAX_QUEUE_FAMILIES] = {0};
    ASSERT(q->queue_family_count <= DVZ_MAX_QUEUE_FAMILIES);
    for (uint32_t i = 0; i < q->queue_family_count; i++)
    {
        queue_family_score[i] += (uint32_t)q->support_transfer[i];
        queue_family_score[i] += (uint32_t)q->support_graphics[i];
        queue_family_score[i] += (uint32_t)q->support_compute[i];
        queue_family_score[i] += (uint32_t)q->support_present[i];
    }

    // DEBUG to check when there is only 1 family queue with 1 queue.
    // q->queue_family_count = 1;
    // queue_family_score[0] = 4;
    // q->support_transfer[0] = 1;
    // q->support_graphics[0] = 1;
    // q->support_compute[0] = 1;
    // q->support_present[0] = 1;
    // q->max_queue_count[0] = 1;

    // Then, for each requested queue, we find the matching queue family with the lowest score.
    {
        bool qf_match;
        uint32_t lowest_score;
        uint32_t queues_per_family[DVZ_MAX_QUEUE_FAMILIES] = {0};
        ASSERT(q->queue_count <= DVZ_MAX_QUEUES);
        for (uint32_t i = 0; i < q->queue_count; i++)
        {
            // log_trace("starting search of queue family for requested queue #%d", i);
            lowest_score = 1000;
            // For each possible queue family, determine whether it would fit for the current
            // requested queue.
            uint32_t qf = 0;

            // a queue family that would match but that is full, so not possible to get a
            // new queue, but could still use an existing queue
            uint32_t qf_fallback = 0;

            for (uint32_t qfi = 0; qfi < q->queue_family_count; qfi++)
            {
                // NOTE: go through queue families in reversed order so that, if multiple matching
                // queues have the same score, we take the *first* one instead of the last.
                qf = q->queue_family_count - qfi - 1;
                qf_match = true;
                // log_trace("looking at queue family %d with score %d", qf,
                // queue_family_score[qf]);
                if ((q->queue_types[i] & DVZ_QUEUE_TRANSFER) && !q->support_transfer[qf])
                    qf_match = false;
                if ((q->queue_types[i] & DVZ_QUEUE_GRAPHICS) && !q->support_graphics[qf])
                    qf_match = false;
                if ((q->queue_types[i] & DVZ_QUEUE_COMPUTE) && !q->support_compute[qf])
                    qf_match = false;
                if ((q->queue_types[i] & DVZ_QUEUE_PRESENT) && !q->support_present[qf])
                    qf_match = false;

                if (queues_per_family[qf] >= q->max_queue_count[qf])
                {
                    log_trace(
                        "the current queue family %d doesn't match because it's full (%d >= %d)",
                        qf, queues_per_family[qf], q->max_queue_count[qf]);
                    qf_match = false;
                    qf_fallback = qf;
                }

                // This queue family does not match, skipping.
                if (!qf_match)
                {
                    // log_trace("queue family #%d does not match", qf);
                    continue;
                }
                // The current queue family matches, what is its score?
                if (queue_family_score[qf] <= lowest_score)
                {
                    // The best matching queue family so far for the current queue, saving it for
                    // now.
                    lowest_score = queue_family_score[qf];
                    q->queue_families[i] = qf;
                    // log_trace("queue family #%d matches requested queue #%d", qf, i);
                }
                else
                {
                    // log_trace(
                    //    "queue family #%d would match but its score %d is larger than %d", qf,
                    //    queue_family_score[qf], lowest_score);
                }
            }
            // Here, qfis the best matching family for the current queue, and qf is saved in
            // queue_families[i].
            queues_per_family[qf]++;
            // At the next iteration, we will take this queue family assignment into account
            // when finding the best queue family for the next queue, so that we don't exceed the
            // max number of queues per family.
            if (lowest_score == 1000)
            {
                log_debug(
                    "could not find a matching queue family for requested queue #%d, reusing an "
                    "existing queue",
                    i);
                q->queue_families[i] = qf_fallback;
            }
        }
    }

    // Create the queue info structure.
    float queue_priority = 1.0f;

    // Count the number of queues requested, for each queue family.
    log_trace("counting the number of requested queues for each queue family");
    uint32_t queues_per_family[DVZ_MAX_QUEUE_FAMILIES] = {0};
    uint32_t qf = 0;         // queue family of the current queue
    uint32_t max_queues = 0; // max number of queues in the family of the current queue
    for (uint32_t i = 0; i < q->queue_count; i++)
    {
        qf = q->queue_families[i]; // the queue family of the current queue
        max_queues = q->max_queue_count[qf];
        ASSERT(qf < q->queue_family_count);
        ASSERT(qf < DVZ_MAX_QUEUE_FAMILIES);
        if (queues_per_family[qf] < max_queues)
        {
            // Also determine the queue index of each requested queue within its queue family.
            q->queue_indices[i] = queues_per_family[qf];
            // Count the number of queues in each family.
            queues_per_family[qf]++;
        }
        else
        {
            log_debug(
                "maximum number of queues (%d) reached for queue #%d in family %d", //
                max_queues, i, qf);
            q->queue_indices[i] = max_queues - 1;
        }
    }

    // Count the number of queue families with at least 1 queue to create.
    log_trace("determining the queue families to create and the number of queues in each");
    uint32_t queue_family_count = 0;
    uint32_t queues_per_family_to_create[DVZ_MAX_QUEUE_FAMILIES] = {0};
    // the queue family index of each queue family to create
    uint32_t queue_family_indices[DVZ_MAX_QUEUE_FAMILIES] = {0};
    for (qf = 0; qf < q->queue_family_count; qf++)
    {
        if (queues_per_family[qf] > 0)
        {
            queues_per_family_to_create[queue_family_count] = queues_per_family[qf];
            queue_family_indices[queue_family_count] = qf;
            queue_family_count++;
            log_trace("will create queue family #%d with %d queue(s)", qf, queues_per_family[qf]);
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
        queue_families_info[i].queueFamilyIndex = queue_family_indices[i];
    }

    VkDeviceCreateInfo device_info = {0};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.queueCreateInfoCount = queue_family_count;
    device_info.pQueueCreateInfos = queue_families_info;

    // Requested features
    device_info.pEnabledFeatures = &gpu->requested_features;

    // Device extensions and layers
    char* extensions[16] = {0};
    uint32_t n_extensions = 0;

    if (has_surface)
    {
        uint32_t n = ARRAY_COUNT(DVZ_DEVICE_EXTENSIONS);
        log_trace("has surface, will add %d extensions", n);
        ASSERT(n < 16);
        memcpy(extensions, DVZ_DEVICE_EXTENSIONS, n * sizeof(char*));
        n_extensions += n;
    }

    // Fix for the following validation error (macOS):
    // If the [VK_KHR_portability_subset] extension is included in pProperties of
    // vkEnumerateDeviceExtensionProperties, ppEnabledExtensions must include
    // "VK_KHR_portability_subset"
    {
        log_trace("getting device extensions properties");
        uint32_t n = 0;
        vkEnumerateDeviceExtensionProperties(gpu->physical_device, NULL, &n, NULL);
        VkExtensionProperties* ext = calloc(n, sizeof(VkExtensionProperties));
        VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(gpu->physical_device, NULL, &n, ext));
        for (uint32_t i = 0; i < n; i++)
        {
            if (strncmp(
                    ext[i].extensionName, "VK_KHR_portability_subset",
                    strnlen("VK_KHR_portability_subset", 32)) == 0)
            {
                log_trace("found portability subset, will need to add extension "
                          "VK_KHR_portability_subset");
                // extensions[n_extensions++] = "VK_KHR_get_physical_device_properties2";
                extensions[n_extensions++] = "VK_KHR_portability_subset";
                break;
            }
        }
        FREE(ext);
    }

    device_info.enabledExtensionCount = n_extensions;
    device_info.ppEnabledExtensionNames = (const char* const*)extensions;

    // device_info.enabledLayerCount = has_validation ? ARRAY_COUNT(DVZ_LAYERS) : 0;
    // device_info.ppEnabledLayerNames = has_validation ? DVZ_LAYERS : NULL;

    // Create the device
    VK_CHECK_RESULT(vkCreateDevice(gpu->physical_device, &device_info, NULL, &gpu->device));
    FREE(queue_families_info);
    log_trace("device created");
}



/*************************************************************************************************/
/*  Swapchain                                                                                    */
/*************************************************************************************************/

static bool check_surface_format(VkPhysicalDevice pdevice, VkSurfaceKHR surface, VkFormat format)
{
    uint32_t n_formats = 0;
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(pdevice, surface, &n_formats, NULL));
    ASSERT(n_formats > 0);
    VkSurfaceFormatKHR* formats = calloc(n_formats, sizeof(VkSurfaceFormatKHR));
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(pdevice, surface, &n_formats, formats));
    bool found = false;
    for (uint32_t i = 0; i < n_formats; i++)
    {
        if (formats[i].format == format)
        {
            found = true;
            break;
        }
    }
    if (!found)
        log_error("format %d is not supported by the surface", format);
    FREE(formats);
    return found;
}



static void create_swapchain(
    VkDevice device, VkPhysicalDevice pdevice,                              //
    VkSurfaceKHR surface, uint32_t image_count,                             //
    VkFormat format, VkPresentModeKHR present_mode,                         //
    DvzQueues* queues, uint32_t requested_width, uint32_t requested_height, //
    VkSurfaceCapabilitiesKHR* caps, VkSwapchainKHR* swapchain,              //
    uint32_t* width, uint32_t* height) // final actual swapchain size in pixels
{
    // The output width and height values are determined by the surface itself, as queried by
    // the Vulkan function vkGetPhysicalDeviceSurfaceCapabilitiesKHR().

    ASSERT(surface != VK_NULL_HANDLE);
    ASSERT(format != 0);
    ASSERT(image_count > 0);

    // NOTE: this call is necessary when creating multiple canvases, otherwise we get the message
    // below:
    //
    // MessageID = 0xa183744d | vkCreateSwapchainKHR(): pCreateInfo->surface is not known at
    // this time to be supported for presentation by this device. The
    // vkGetPhysicalDeviceSurfaceSupportKHR() must be called beforehand, and it must return VK_TRUE
    // support with this surface for at least one queue family of this device. The Vulkan spec
    // states: surface must be a surface that is supported by the device as determined using
    // vkGetPhysicalDeviceSurfaceSupportKHR
    find_present_queue_family(pdevice, surface, queues);

    // Find formats supported by the surface.
    ASSERT(check_surface_format(pdevice, surface, format));

    // Swap chain.
    VkSwapchainCreateInfoKHR info = {0};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = surface;
    info.minImageCount = image_count;
    info.imageFormat = format;
    info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = present_mode;
    info.clipped = VK_TRUE;

    // Determine which queue families have access to the swapchain images.
    // If there is at least one queue family that supports PRESENT but not GRAPHICS, then the
    // sharing mode will be concurrent, otherwise it is exclusive.
    uint32_t queue_families[DVZ_MAX_QUEUE_FAMILIES] = {0};
    uint32_t n = 0;
    uint32_t qf = 0;
    bool qf_counted[DVZ_MAX_QUEUE_FAMILIES] = {0};
    for (uint32_t i = 0; i < queues->queue_count; i++)
    {
        qf = queues->queue_families[i];
        if (!qf_counted[qf] && (queues->support_graphics[qf] || queues->support_present[qf]))
        {
            queue_families[n++] = qf;
            qf_counted[qf] = true;
        }
    }
    log_trace("found %d created queue familie(s) needing to access the swapchain images", n);
    if (n >= 2)
    {
        log_trace("creating swapchain in concurrent image sharing mode");
        info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        info.queueFamilyIndexCount = n;
        info.pQueueFamilyIndices = queue_families;
    }
    else
    {
        log_trace("creating swapchain in exclusive image sharing mode");
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    ASSERT(pdevice != VK_NULL_HANDLE);
    ASSERT(caps != NULL);
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdevice, surface, caps));
    log_trace("caps window size is %dx%d", caps->currentExtent.width, caps->currentExtent.height);

    // Handle special case where glfw should give the framebuffer size here as image extent.
    if (caps->currentExtent.width == UINT32_MAX)
    {
        info.imageExtent.width =
            CLIP(requested_width, caps->minImageExtent.width, caps->maxImageExtent.width);
        info.imageExtent.height =
            CLIP(requested_height, caps->minImageExtent.height, caps->maxImageExtent.height);
        log_trace(
            "set swapchain extent to %dx%d", //
            info.imageExtent.width, info.imageExtent.height);
    }
    else
    {
        info.imageExtent = caps->currentExtent;
    }

    // Check.
    ASSERT(info.imageExtent.width >= caps->minImageExtent.width);
    ASSERT(info.imageExtent.height >= caps->minImageExtent.height);
    ASSERT(info.imageExtent.width <= caps->maxImageExtent.width);
    ASSERT(info.imageExtent.height <= caps->maxImageExtent.height);

    // We return the final actual swapchain size.
    *width = info.imageExtent.width;
    *height = info.imageExtent.height;

    info.preTransform = caps->currentTransform;

#if SWIFTSHADER
    log_trace("NOT swapchain as using swiftshader");
#else
    log_trace("create swapchain");
    VK_CHECK_RESULT(vkCreateSwapchainKHR(device, &info, NULL, swapchain));
#endif
}



/*************************************************************************************************/
/*  Command buffers                                                                              */
/*************************************************************************************************/

static void allocate_command_buffers(
    VkDevice device, VkCommandPool command_pool, uint32_t count, VkCommandBuffer* cmd_bufs)
{
    ASSERT(count > 0);
    log_trace("allocate %d command buffer(s)", count);
    ASSERT(command_pool != VK_NULL_HANDLE);
    ASSERT(count > 0);

    VkCommandBufferAllocateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool = command_pool;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = count;
    VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &info, cmd_bufs));
}



/*************************************************************************************************/
/*  Buffers                                                                                      */
/*************************************************************************************************/

static uint32_t find_memory_type(
    uint32_t typeFilter, VkMemoryPropertyFlags properties,
    VkPhysicalDeviceMemoryProperties mem_properties)
{
    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
    {
        if ((typeFilter & (uint32_t)(1 << i)) &&
            (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }
    log_error("could not find an appropriate memory type");
    return 0;
}



static void make_shared(
    DvzQueues* queues, uint32_t queue_count, const uint32_t* queue_indices, //
    VkSharingMode* sharing_mode, uint32_t* queue_family_count, uint32_t* queue_families)
{
    ASSERT(queues != NULL);
    ASSERT(sharing_mode != NULL);
    if (queue_count == 0)
    {
        return;
    }
    ASSERT(queue_families != NULL);

    // Go through the requested queues, check their queue family, and count the total number of
    // different queue families. If >= 2, mode is concurrent, otherwise it is exclusive.
    uint32_t n = 0;
    uint32_t qf = 0;
    uint32_t qfs[DVZ_MAX_QUEUE_FAMILIES] = {0}; // for each queue family, the number of queues
    for (uint32_t i = 0; i < queue_count; i++)
    {
        // Get the family of the current requested queue.
        qf = queues->queue_families[queue_indices[i]];
        // If this queue family is first encountered, add it to the supplied output array.
        if (qfs[qf] == 0)
            queue_families[n++] = qf;
        // Count the number of queues in that family.
        qfs[qf]++;
    }
    // Now, n is the number of *different* queue families.
    log_trace(
        "requested %d queue(s), corresponding to %d distinct queue families", queue_count, n);
    *queue_family_count = n;

    // DEBUG: always use exclusive for now (require queue owernship transfer with barriers)
    // if (true || n <= 1)
    if (n <= 1)
    {
        *sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
    }
    else
    {
        *sharing_mode = VK_SHARING_MODE_CONCURRENT;
    }
}



/*************************************************************************************************/
/*  Images                                                                                       */
/*************************************************************************************************/

static void check_dims(VkImageType image_type, uvec3 shape)
{
    ASSERT(shape[0] != 0);
    if (image_type == VK_IMAGE_TYPE_1D)
    {
        ASSERT(shape[1] == 1);
        ASSERT(shape[2] == 1);
    }
    else if (image_type == VK_IMAGE_TYPE_2D)
    {
        ASSERT(shape[1] != 0);
        ASSERT(shape[2] == 1);
    }
    else if (image_type == VK_IMAGE_TYPE_3D)
    {
        ASSERT(shape[2] != 0);
    }
    else
    {
        log_error("unknown image type %d", image_type);
    }
}



static inline void _copy_shape(uvec3 src, uvec3 dst) { memcpy(dst, src, sizeof(uvec3)); }



static void create_image_view(
    VkDevice device, VkImage image, VkImageViewType view_type, VkFormat format,
    VkImageAspectFlags aspect_flags, VkImageView* image_view)
{
    log_trace("create image view %dD", view_type + 1);

    ASSERT(aspect_flags != 0);
    ASSERT(image != VK_NULL_HANDLE);
    ASSERT(format != 0);

    VkImageViewCreateInfo viewInfo = {0};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = view_type;
    viewInfo.format = format;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.subresourceRange.aspectMask = aspect_flags;

    VK_CHECK_RESULT(vkCreateImageView(device, &viewInfo, NULL, image_view));
}



/*************************************************************************************************/
/*  Sampler                                                                                      */
/*************************************************************************************************/

static void create_texture_sampler(
    VkDevice device, VkFilter mag_filter, VkFilter min_filter, //
    VkSamplerAddressMode* address_modes, bool anisotropy, VkSampler* sampler)
{
    log_trace("create texture sampler");
    VkSamplerCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    info.magFilter = mag_filter;
    info.minFilter = min_filter;

    info.addressModeU = address_modes[0];
    info.addressModeV = address_modes[1];
    info.addressModeW = address_modes[2];

    info.anisotropyEnable = anisotropy;
    info.maxAnisotropy = 16;
    info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    info.unnormalizedCoordinates = VK_FALSE;
    info.compareEnable = VK_FALSE;
    info.compareOp = VK_COMPARE_OP_ALWAYS;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.mipLodBias = 0.0f;
    info.minLod = 0.0f;
    info.maxLod = 0.0f;

    VK_CHECK_RESULT(vkCreateSampler(device, &info, NULL, sampler));
}



/*************************************************************************************************/
/*  Bindings                                                                                     */
/*************************************************************************************************/

static void create_descriptor_pool(VkDevice device, VkDescriptorPool* dset_pool)
{
    // Descriptor pool.
    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, DVZ_MAX_DESCRIPTOR_SETS},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, DVZ_MAX_DESCRIPTOR_SETS},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, DVZ_MAX_DESCRIPTOR_SETS},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, DVZ_MAX_DESCRIPTOR_SETS},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, DVZ_MAX_DESCRIPTOR_SETS},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, DVZ_MAX_DESCRIPTOR_SETS},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, DVZ_MAX_DESCRIPTOR_SETS},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, DVZ_MAX_DESCRIPTOR_SETS},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, DVZ_MAX_DESCRIPTOR_SETS},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, DVZ_MAX_DESCRIPTOR_SETS},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, DVZ_MAX_DESCRIPTOR_SETS}};
    VkDescriptorPoolCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    info.poolSizeCount = 11;
    info.pPoolSizes = poolSizes;
    info.maxSets = DVZ_MAX_DESCRIPTOR_SETS * info.poolSizeCount;

    // Create descriptor pool.
    log_trace("create descriptor pool");
    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &info, NULL, dset_pool));
}



static void create_pipeline_layout(
    VkDevice device,                                                    //
    uint32_t push_constants_count, VkPushConstantRange* push_constants, //
    VkDescriptorSetLayout* dset_layout, VkPipelineLayout* pipeline_layout)
{
    // Pipeline layout.
    VkPipelineLayoutCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.setLayoutCount = dset_layout != NULL ? 1 : 0;
    info.pSetLayouts = dset_layout;
    info.pushConstantRangeCount = push_constants_count;
    info.pPushConstantRanges = push_constants;

    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &info, NULL, pipeline_layout));
}



static void create_descriptor_set_layout(
    VkDevice device, uint32_t binding_count, VkDescriptorType* binding_types,
    VkDescriptorSetLayout* dset_layout)
{
    // Descriptor set layout.
    VkDescriptorSetLayoutBinding* layout_bindings =
        calloc(binding_count, sizeof(VkDescriptorSetLayoutBinding));

    for (uint32_t i = 0; i < binding_count; i++)
    {
        VkDescriptorType dtype = binding_types[i];
        layout_bindings[i].binding = i;
        layout_bindings[i].descriptorType = dtype;
        layout_bindings[i].descriptorCount = 1;
        layout_bindings[i].stageFlags = VK_SHADER_STAGE_ALL;
        layout_bindings[i].pImmutableSamplers = NULL; // Optional
    }

    // Create descriptor set layout.
    VkDescriptorSetLayoutCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = binding_count;
    info.pBindings = layout_bindings;

    log_trace("create descriptor set layout");
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &info, NULL, dset_layout));
    FREE(layout_bindings);
}



static void allocate_descriptor_sets(
    VkDevice device, VkDescriptorPool dset_pool, VkDescriptorSetLayout dset_layout, uint32_t count,
    VkDescriptorSet* dsets)
{
    // Allocate descriptor sets.
    VkDescriptorSetLayout* layouts = calloc(count, sizeof(VkDescriptorSetLayout));
    for (uint32_t i = 0; i < count; i++)
        layouts[i] = dset_layout;

    VkDescriptorSetAllocateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ASSERT(dset_pool != VK_NULL_HANDLE);
    info.descriptorPool = dset_pool;
    info.descriptorSetCount = count;
    info.pSetLayouts = layouts;

    log_trace("allocate descriptor sets");
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &info, dsets));
    FREE(layouts);
}



static bool is_descriptor_type_buffer(VkDescriptorType binding_type)
{
    return binding_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
           binding_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
           binding_type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
           binding_type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
}



static bool is_descriptor_type_image(VkDescriptorType binding_type)
{
    return binding_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
           binding_type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
}



static void update_descriptor_set(
    VkDevice device, uint32_t binding_count, VkDescriptorType* types,            //
    DvzBufferRegions* buffer_regions, DvzImages** images, DvzSampler** samplers, //
    uint32_t idx, VkDescriptorSet dset)
{
    log_trace("update descriptor set #%d", idx);
    VkWriteDescriptorSet* descriptor_writes = calloc(binding_count, sizeof(VkWriteDescriptorSet));

    VkDescriptorBufferInfo buffer_infos[DVZ_MAX_BINDINGS_SIZE] = {0};
    VkDescriptorImageInfo image_infos[DVZ_MAX_BINDINGS_SIZE] = {0};

    VkDescriptorType binding_type = {0};
    DvzBufferRegions* br = NULL;

    for (uint32_t i = 0; i < binding_count; i++)
    {
        binding_type = types[i];

        if (is_descriptor_type_buffer(binding_type))
        {
            // log_trace("bind buffer for binding point %d", i);
            if (buffer_regions[i].buffer == NULL)
            {
                log_error("buffer of type %d #%d is not set", binding_type, i);
            }
            br = &buffer_regions[i];
            ASSERT(buffer_regions[i].buffer != NULL);
            ASSERT(br->size > 0);

            uint32_t idx_clip = MIN(idx, br->count - 1);
            buffer_infos[i].buffer = br->buffer->buffer;
            buffer_infos[i].offset = br->offsets[idx_clip];
            buffer_infos[i].range = br->size;
        }
        else if (is_descriptor_type_image(binding_type))
        {
            // log_trace("bind texture for binding point %d", i);
            ASSERT(images[i] != NULL);
            // if (images[i] != NULL)
            // {
            uint32_t idx_clip = MIN(idx, images[i]->count - 1);
            image_infos[i].imageLayout = images[i]->layout;
            image_infos[i].imageView = images[i]->image_views[idx_clip];
            image_infos[i].sampler = samplers[i]->sampler;
            // }
            // else
            // {
            //     log_warn("empty texture for binding point %d", i);
            // }
        }
        else
        {
            log_error("unsupported descriptor type %d", binding_type);
            return;
        }
        descriptor_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[i].pNext = VK_NULL_HANDLE;
        descriptor_writes[i].dstSet = dset;
        descriptor_writes[i].dstBinding = i;
        descriptor_writes[i].dstArrayElement = 0;
        descriptor_writes[i].descriptorCount = 1;
        descriptor_writes[i].descriptorType = binding_type;
        descriptor_writes[i].pImageInfo = &image_infos[i];
        descriptor_writes[i].pBufferInfo = &buffer_infos[i];
        descriptor_writes[i].pTexelBufferView = VK_NULL_HANDLE;
    }

    vkUpdateDescriptorSets(device, binding_count, descriptor_writes, 0, NULL);
    FREE(descriptor_writes);
}



/*************************************************************************************************/
/*  Shaders                                                                                      */
/*************************************************************************************************/

static VkShaderModule
create_shader_module(VkDevice device, VkDeviceSize size, const uint32_t* buffer)
{
    ASSERT(device != VK_NULL_HANDLE);
    ASSERT(size > 0);
    ASSERT(buffer != NULL);

    VkShaderModuleCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = (size_t)size;
    createInfo.pCode = buffer;

    VkShaderModule module = {0};
    VK_CHECK_RESULT(vkCreateShaderModule(device, &createInfo, NULL, &module));
    return module;
}



static VkShaderModule create_shader_module_from_file(VkDevice device, const char* filename)
{
    log_trace("create shader module from file %s", filename);
    size_t size = 0;
    uint32_t* shader_code = (uint32_t*)dvz_read_file(filename, &size);
    ASSERT(shader_code != NULL);
    ASSERT(size > 0);
    VkShaderModule module = create_shader_module(device, size, shader_code);
    FREE(shader_code);
    return module;
}



/*************************************************************************************************/
/*  Compute                                                                                      */
/*************************************************************************************************/

static void create_compute_pipeline(
    VkDevice device, VkShaderModule shader_module, VkPipelineLayout pipeline_layout,
    VkPipeline* pipeline)
{
    // Create the shader and pipeline.
    VkComputePipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = pipeline_layout;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.pName = "main";
    pipelineInfo.stage.module = shader_module;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    VK_CHECK_RESULT(
        vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, pipeline));
}



/*************************************************************************************************/
/*  Graphics                                                                                     */
/*************************************************************************************************/

static VkPipelineInputAssemblyStateCreateInfo create_input_assembly(VkPrimitiveTopology topology)
{
    VkPipelineInputAssemblyStateCreateInfo input_assembly = {0};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = topology;
#if OS_MACOS
    if (topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN)
    {
        log_error("macOS does not support triangle fan topology, falling back to triangle list!");
        input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
#endif
    input_assembly.primitiveRestartEnable = VK_FALSE;
    return input_assembly;
}


static VkPipelineRasterizationStateCreateInfo
create_rasterizer(VkCullModeFlags cull_mode, VkFrontFace front_face)
{
    VkPipelineRasterizationStateCreateInfo rasterizer = {0};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = cull_mode;
    rasterizer.frontFace = front_face;
    rasterizer.depthBiasEnable = VK_FALSE;
    return rasterizer;
}


static VkPipelineMultisampleStateCreateInfo create_multisampling()
{
    VkPipelineMultisampleStateCreateInfo multisampling = {0};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    return multisampling;
}


static VkPipelineColorBlendAttachmentState create_color_blend_attachment(bool enable)
{
    VkPipelineColorBlendAttachmentState attachment = {0};
    attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    attachment.blendEnable = enable;
    if (enable)
    {
        attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        attachment.colorBlendOp = VK_BLEND_OP_ADD;
        attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }
    return attachment;
}


static VkPipelineColorBlendStateCreateInfo
create_color_blending(uint32_t count, VkPipelineColorBlendAttachmentState* attachments)
{
    VkPipelineColorBlendStateCreateInfo color_blending = {0};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY;
    color_blending.attachmentCount = count;
    color_blending.pAttachments = attachments;
    color_blending.blendConstants[0] = 0.0f;
    color_blending.blendConstants[1] = 0.0f;
    color_blending.blendConstants[2] = 0.0f;
    color_blending.blendConstants[3] = 0.0f;
    return color_blending;
}


static VkPipelineDepthStencilStateCreateInfo create_depth_stencil(bool enable)
{
    VkPipelineDepthStencilStateCreateInfo depth_stencil = {0};
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = enable;
    depth_stencil.depthWriteEnable = VK_TRUE;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
    // depth_stencil.depthBoundsTestEnable = VK_FALSE;
    // depth_stencil.minDepthBounds = 0.0f; // Optional
    // depth_stencil.maxDepthBounds = 1.0f; // Optional
    // depth_stencil.stencilTestEnable = VK_FALSE;
    // depth_stencil.front = (VkStencilOpState){0}; // Optional
    // depth_stencil.back = (VkStencilOpState){0};  // Optional
    return depth_stencil;
}


static VkPipelineViewportStateCreateInfo create_viewport_state()
{
    VkPipelineViewportStateCreateInfo viewport_state = {0};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    // NOTE: unused because the viewport/scissor are set in the dynamic states
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;
    return viewport_state;
}


static VkPipelineDynamicStateCreateInfo
create_dynamic_states(uint32_t count, VkDynamicState* dynamic_states)
{
    VkPipelineDynamicStateCreateInfo dynamic_state = {0};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.pNext = NULL;
    dynamic_state.pDynamicStates = dynamic_states;
    dynamic_state.dynamicStateCount = count;
    return dynamic_state;
}



/*************************************************************************************************/
/*  Renderpass                                                                                   */
/*************************************************************************************************/

static VkAttachmentDescription create_attachment(
    VkFormat format, VkAttachmentLoadOp load_op, VkAttachmentStoreOp store_op,
    VkImageLayout src_layout, VkImageLayout dst_layout)
{
    VkAttachmentDescription attachment = {0};
    attachment.format = format;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = load_op;
    attachment.storeOp = store_op;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = src_layout;
    attachment.finalLayout = dst_layout;

    return attachment;
}



static VkAttachmentReference create_attachment_ref(uint32_t attachment, VkImageLayout ref_layout)
{
    VkAttachmentReference attachment_ref = {0};
    attachment_ref.attachment = attachment;
    attachment_ref.layout = ref_layout;
    return attachment_ref;
}



static void begin_render_pass(
    VkRenderPass renderpass, VkCommandBuffer cmd_buf, VkFramebuffer framebuffer, //
    uint32_t width, uint32_t height, uint32_t clear_count, VkClearValue* clear_colors)
{
    ASSERT(renderpass != VK_NULL_HANDLE);
    ASSERT(framebuffer != VK_NULL_HANDLE);
    ASSERT(width > 0);
    ASSERT(height > 0);
    // ASSERT(clear_count > 0);
    // ASSERT(clear_colors != NULL);

    VkRenderPassBeginInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.renderPass = renderpass;
    info.framebuffer = framebuffer;
    VkRect2D renderArea = {{0, 0}, {width, height}};
    info.renderArea = renderArea;
    info.clearValueCount = clear_count;
    info.pClearValues = clear_colors;
    vkCmdBeginRenderPass(cmd_buf, &info, VK_SUBPASS_CONTENTS_INLINE);
}



static void make_renderpass(
    DvzGpu* gpu, DvzRenderpass* renderpass, DvzFormat format, VkImageLayout layout,
    VkClearColorValue clear_color)
{
    ASSERT(gpu != NULL);
    ASSERT(renderpass != NULL);
    ASSERT(format != 0);

    log_trace("making renderpass");
    *renderpass = dvz_renderpass(gpu);

    VkClearValue clear_depth = {0};
    clear_depth.depthStencil.depth = 1.0f;
    dvz_renderpass_clear(renderpass, (VkClearValue){.color = clear_color});
    dvz_renderpass_clear(renderpass, clear_depth);

    // Color attachment.
    dvz_renderpass_attachment(
        renderpass, 0, //
        DVZ_RENDERPASS_ATTACHMENT_COLOR, (VkFormat)format,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    dvz_renderpass_attachment_layout(renderpass, 0, VK_IMAGE_LAYOUT_UNDEFINED, layout);
    dvz_renderpass_attachment_ops(
        renderpass, 0, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);

    // Depth attachment.
    dvz_renderpass_attachment(
        renderpass, 1, //
        DVZ_RENDERPASS_ATTACHMENT_DEPTH, VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    dvz_renderpass_attachment_layout(
        renderpass, 1, //
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    dvz_renderpass_attachment_ops(
        renderpass, 1, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE);

    // Subpass.
    dvz_renderpass_subpass_attachment(renderpass, 0, 0);
    dvz_renderpass_subpass_attachment(renderpass, 0, 1);

    // Create renderpass.
    dvz_renderpass_create(renderpass);
}



#endif
