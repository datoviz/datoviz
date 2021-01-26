/*************************************************************************************************/
/*  Light wrapper on top of the Vulkan C API                                                     */
/*************************************************************************************************/

#ifndef VKL_VKLITE_HEADER
#define VKL_VKLITE_HEADER

#define GLM_FORCE_RADIANS
// #define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// #include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "app.h"
#include "common.h"

BEGIN_INCL_NO_WARN
#include <cglm/struct.h>
END_INCL_NO_WARN

#ifdef __cplusplus
extern "C" {
#endif



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define VKL_MAX_BINDINGS_SIZE    32
#define VKL_MAX_DESCRIPTOR_SETS  64
#define VKL_MAX_PRESENT_MODES    16
#define VKL_MAX_PUSH_CONSTANTS   16
#define VKL_MAX_QUEUE_FAMILIES   16
#define VKL_MAX_QUEUES           16
#define VKL_MAX_SWAPCHAIN_IMAGES 8

// Maximum number of command buffers per VklCommands struct
#define VKL_MAX_COMMAND_BUFFERS_PER_SET     VKL_MAX_SWAPCHAIN_IMAGES
#define VKL_MAX_BUFFER_REGIONS_PER_SET      VKL_MAX_SWAPCHAIN_IMAGES
#define VKL_MAX_IMAGES_PER_SET              VKL_MAX_SWAPCHAIN_IMAGES
#define VKL_MAX_SEMAPHORES_PER_SET          VKL_MAX_SWAPCHAIN_IMAGES
#define VKL_MAX_FENCES_PER_SET              VKL_MAX_SWAPCHAIN_IMAGES
#define VKL_MAX_COMMANDS_PER_SUBMIT         16
#define VKL_MAX_BARRIERS_PER_SET            8
#define VKL_MAX_SEMAPHORES_PER_SUBMIT       8
#define VKL_MAX_SHADERS_PER_GRAPHICS        8
#define VKL_MAX_ATTACHMENTS_PER_RENDERPASS  8
#define VKL_MAX_SUBPASSES_PER_RENDERPASS    8
#define VKL_MAX_DEPENDENCIES_PER_RENDERPASS 8
#define VKL_MAX_VERTEX_BINDINGS             16
#define VKL_MAX_VERTEX_ATTRS                32



/*************************************************************************************************/
/*  Type definitions */
/*************************************************************************************************/

typedef struct VklQueues VklQueues;
typedef struct VklGpu VklGpu;
typedef struct VklWindow VklWindow;
typedef struct VklSwapchain VklSwapchain;
typedef struct VklCommands VklCommands;
typedef struct VklBuffer VklBuffer;
typedef struct VklBufferRegions VklBufferRegions;
typedef struct VklImages VklImages;
typedef struct VklSampler VklSampler;
typedef struct VklSlots VklSlots;
typedef struct VklBindings VklBindings;
typedef struct VklCompute VklCompute;
typedef struct VklVertexBinding VklVertexBinding;
typedef struct VklVertexAttr VklVertexAttr;
typedef struct VklGraphics VklGraphics;
typedef struct VklBarrierBuffer VklBarrierBuffer;
typedef struct VklBarrierImage VklBarrierImage;
typedef struct VklBarrier VklBarrier;
typedef struct VklSemaphores VklSemaphores;
typedef struct VklFences VklFences;
typedef struct VklRenderpass VklRenderpass;
typedef struct VklRenderpassAttachment VklRenderpassAttachment;
typedef struct VklRenderpassSubpass VklRenderpassSubpass;
typedef struct VklRenderpassDependency VklRenderpassDependency;
typedef struct VklFramebuffers VklFramebuffers;
typedef struct VklSubmit VklSubmit;

// Forward declarations.
typedef struct VklCanvas VklCanvas;
typedef struct VklContext VklContext;
typedef struct VklTexture VklTexture;
typedef struct VklGraphicsData VklGraphicsData;

// Callback definitions
typedef void (*VklGraphicsCallback)(VklGraphicsData* data, uint32_t item_count, const void* item);



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Queue type.
typedef enum
{
    VKL_QUEUE_TRANSFER = 0x01,
    VKL_QUEUE_GRAPHICS = 0x02,
    VKL_QUEUE_COMPUTE = 0x04,
    VKL_QUEUE_PRESENT = 0x08,
    VKL_QUEUE_RENDER = 0x07,
    VKL_QUEUE_ALL = 0x0F,
} VklQueueType;



// Command buffer type.
typedef enum
{
    VKL_COMMAND_TRANSFERS,
    VKL_COMMAND_GRAPHICS,
    VKL_COMMAND_COMPUTE,
    VKL_COMMAND_GUI,
} VklCommandBufferType;



// Buffer type.
typedef enum
{
    VKL_BUFFER_TYPE_UNDEFINED,
    VKL_BUFFER_TYPE_STAGING,
    VKL_BUFFER_TYPE_VERTEX,
    VKL_BUFFER_TYPE_INDEX,
    VKL_BUFFER_TYPE_UNIFORM,
    VKL_BUFFER_TYPE_STORAGE,
    VKL_BUFFER_TYPE_UNIFORM_MAPPABLE,
    VKL_BUFFER_TYPE_COUNT,
} VklBufferType;



// Graphics builtins
typedef enum
{
    VKL_GRAPHICS_NONE,
    VKL_GRAPHICS_POINTS,

    VKL_GRAPHICS_LINES,
    VKL_GRAPHICS_LINE_STRIP,
    VKL_GRAPHICS_TRIANGLES,
    VKL_GRAPHICS_TRIANGLE_STRIP,
    VKL_GRAPHICS_TRIANGLE_FAN,

    VKL_GRAPHICS_MARKER_RAW,
    VKL_GRAPHICS_MARKER,

    VKL_GRAPHICS_SEGMENT,
    VKL_GRAPHICS_ARROW,
    VKL_GRAPHICS_PATH,
    VKL_GRAPHICS_TEXT,

    VKL_GRAPHICS_IMAGE,
    VKL_GRAPHICS_VOLUME_SLICE,
    VKL_GRAPHICS_MESH,

    VKL_GRAPHICS_FAKE_SPHERE,
    VKL_GRAPHICS_VOLUME,

    VKL_GRAPHICS_COUNT,
} VklGraphicsType;



// Texture axis.
typedef enum
{
    VKL_TEXTURE_AXIS_U,
    VKL_TEXTURE_AXIS_V,
    VKL_TEXTURE_AXIS_W,
} VklTextureAxis;



// Blend type.
typedef enum
{
    VKL_BLEND_DISABLE,
    VKL_BLEND_ENABLE,
} VklBlendType;



// Depth test.
typedef enum
{
    VKL_DEPTH_TEST_DISABLE,
    VKL_DEPTH_TEST_ENABLE,
} VklDepthTest;



// Render pass attachment type.
typedef enum
{
    VKL_RENDERPASS_ATTACHMENT_COLOR,
    VKL_RENDERPASS_ATTACHMENT_DEPTH,
} VklRenderpassAttachmentType;



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
    ASSERT((cnt) == 1 || (cnt) == cmds->count);                                                   \
    VkCommandBuffer cb = {0};                                                                     \
    uint32_t iclip = 0;                                                                           \
    uint32_t i = idx;                                                                             \
    iclip = (cnt) == 1 ? 0 : (MIN(i, (cnt)-1));                                                   \
    ASSERT(iclip < (cnt));                                                                        \
    cb = cmds->cmds[i];


#define CMD_END //

#define GB 1073741824
#define MB 1048576
#define KB 1024

static char _PRETTY_SIZE[64] = {0};

static inline char* pretty_size(VkDeviceSize size)
{
    float s = (float)size;
    const char* u;
    if (size >= GB)
    {
        s /= GB;
        u = "GB";
    }
    else if (size >= MB)
    {
        s /= MB;
        u = "MB";
    }
    else if (size >= KB)
    {
        s /= KB;
        u = "KB";
    }
    else
    {
        u = "bytes";
    }
    snprintf(_PRETTY_SIZE, 64, "%.1f %s", s, u);
    return _PRETTY_SIZE;
}



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VklQueues
{
    VklObject obj;

    // Hardware supported queues
    // -------------------------
    // Number of different queue families supported by the hardware
    uint32_t queue_family_count;
    // Properties of the queue families
    // VkQueueFamilyProperties queue_families[VKL_MAX_QUEUE_FAMILIES];
    bool support_transfer[VKL_MAX_QUEUE_FAMILIES];
    bool support_graphics[VKL_MAX_QUEUE_FAMILIES];
    bool support_compute[VKL_MAX_QUEUE_FAMILIES];
    bool support_present[VKL_MAX_QUEUE_FAMILIES];
    uint32_t max_queue_count[VKL_MAX_QUEUE_FAMILIES]; // for each queue family, the max # of queues

    // Requested queues
    // ----------------
    // Number of requested queues
    uint32_t queue_count;
    // Requested queue types.
    VklQueueType queue_types[VKL_MAX_QUEUES]; // the VKL type of each queue
    // Queues and associated command pools
    uint32_t queue_families[VKL_MAX_QUEUES]; // for each family, the # of queues
    uint32_t queue_indices[VKL_MAX_QUEUES];  // for each requested queue, its # within its family
    VkQueue queues[VKL_MAX_QUEUES];
    VkCommandPool cmd_pools[VKL_MAX_QUEUE_FAMILIES];
};



struct VklGpu
{
    VklObject obj;
    VklApp* app;

    uint32_t idx; // GPU index within the app
    const char* name;

    VkPhysicalDevice physical_device;
    VkPhysicalDeviceProperties device_properties;
    VkPhysicalDeviceFeatures device_features;
    VkPhysicalDeviceMemoryProperties memory_properties;

    uint32_t present_mode_count;
    VkPresentModeKHR present_modes[VKL_MAX_PRESENT_MODES];

    VklQueues queues;
    VkDescriptorPool dset_pool;

    VkPhysicalDeviceFeatures requested_features;
    VkDevice device;

    VklContext* context;
};



struct VklWindow
{
    VklObject obj;
    VklApp* app;

    void* backend_window;
    uint32_t width, height; // in screen coordinates

    bool close_on_esc;
    VkSurfaceKHR surface;
    VkSurfaceCapabilitiesKHR caps; // current extent in pixel coordinates (framebuffers)
};



struct VklSwapchain
{
    VklObject obj;
    VklGpu* gpu;
    VklWindow* window;

    VkFormat format;
    VkPresentModeKHR present_mode;
    bool support_transfer; // whether the swapchain supports copying the image to another

    // extent in pixel coordinates if caps.currentExtent is not available
    uint32_t requested_width, requested_height;

    uint32_t img_count;
    uint32_t img_idx;
    VkSwapchainKHR swapchain;

    // The actual framebuffer size in pixels is found in the images size
    VklImages* images;
};



struct VklCommands
{
    VklObject obj;
    VklGpu* gpu;

    uint32_t queue_idx;
    uint32_t count;
    VkCommandBuffer cmds[VKL_MAX_COMMAND_BUFFERS_PER_SET];
};



struct VklBuffer
{
    VklObject obj;
    VklGpu* gpu;

    VklBufferType type;
    VkBuffer buffer;
    VkDeviceMemory device_memory;

    // Queues that need access to the buffer.
    uint32_t queue_count;
    uint32_t queues[VKL_MAX_QUEUES];

    VkDeviceSize size;
    VkDeviceSize allocated_size;
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags memory;

    void* mmap;
};



struct VklBufferRegions
{
    VklBuffer* buffer;
    uint32_t count;
    VkDeviceSize size;
    VkDeviceSize aligned_size; // NOTE: is non-null only for aligned arrays
    VkDeviceSize alignment;
    VkDeviceSize offsets[VKL_MAX_BUFFER_REGIONS_PER_SET];
};



struct VklImages
{
    VklObject obj;
    VklGpu* gpu;

    uint32_t count;
    bool is_swapchain;

    // Queues that need access to the buffer.
    uint32_t queue_count;
    uint32_t queues[VKL_MAX_QUEUES];

    VkImageType image_type;
    VkImageViewType view_type;
    uint32_t width, height, depth;
    VkFormat format;
    VkImageLayout layout;
    VkImageTiling tiling;
    VkImageUsageFlags usage;
    VkMemoryPropertyFlags memory;
    VkImageAspectFlags aspect;

    VkImage images[VKL_MAX_IMAGES_PER_SET];
    VkDeviceMemory memories[VKL_MAX_IMAGES_PER_SET];
    VkImageView image_views[VKL_MAX_IMAGES_PER_SET];
};



struct VklSampler
{
    VklObject obj;
    VklGpu* gpu;

    VkFilter min_filter;
    VkFilter mag_filter;
    VkSamplerAddressMode address_modes[3];
    VkSampler sampler;
};



struct VklSlots
{
    VklObject obj;
    VklGpu* gpu;

    uint32_t slot_count;
    VkDescriptorType types[VKL_MAX_BINDINGS_SIZE];

    uint32_t push_count;
    VkDeviceSize push_offsets[VKL_MAX_PUSH_CONSTANTS];
    VkDeviceSize push_sizes[VKL_MAX_PUSH_CONSTANTS];
    VkShaderStageFlags push_shaders[VKL_MAX_PUSH_CONSTANTS];

    VkPipelineLayout pipeline_layout;
    VkDescriptorSetLayout dset_layout;
};



struct VklBindings
{
    VklObject obj;
    VklGpu* gpu;

    VklSlots* slots;

    // a Bindings struct holds multiple almost-identical copies of descriptor sets
    // with the same layout, but possibly with the different idx in the VklBuffer
    uint32_t dset_count;
    VkDescriptorSet dsets[VKL_MAX_SWAPCHAIN_IMAGES];

    VklBufferRegions br[VKL_MAX_BINDINGS_SIZE];
    VklImages* images[VKL_MAX_BINDINGS_SIZE];
    VklSampler* samplers[VKL_MAX_BINDINGS_SIZE];
};



struct VklCompute
{
    VklObject obj;
    VklGpu* gpu;
    VklContext* context;

    char shader_path[1024];
    const char* shader_code;

    VkPipeline pipeline;
    VklSlots slots;
    VklBindings* bindings;
    VkShaderModule shader_module;
};



struct VklVertexBinding
{
    uint32_t binding;
    VkDeviceSize stride;
};



struct VklVertexAttr
{
    uint32_t binding;
    uint32_t location;
    VkFormat format;
    VkDeviceSize offset;
};



struct VklGraphics
{
    VklObject obj;
    VklGpu* gpu;

    VklGraphicsType type;
    int flags;

    VklRenderpass* renderpass;
    uint32_t subpass;

    VkPrimitiveTopology topology;
    VklBlendType blend_type;
    VklDepthTest depth_test;
    VkPolygonMode polygon_mode;
    VkCullModeFlags cull_mode;
    VkFrontFace front_face;

    VkPipeline pipeline;
    VklSlots slots;

    uint32_t vertex_binding_count;
    VklVertexBinding vertex_bindings[VKL_MAX_VERTEX_BINDINGS];

    uint32_t vertex_attr_count;
    VklVertexAttr vertex_attrs[VKL_MAX_VERTEX_ATTRS];

    uint32_t shader_count;
    VkShaderStageFlagBits shader_stages[VKL_MAX_SHADERS_PER_GRAPHICS];
    VkShaderModule shader_modules[VKL_MAX_SHADERS_PER_GRAPHICS];

    VklGraphicsCallback callback;
};



struct VklBarrierBuffer
{
    VklBufferRegions br;
    bool queue_transfer;

    VkAccessFlags src_access;
    uint32_t src_queue;

    VkAccessFlags dst_access;
    uint32_t dst_queue;
};



struct VklBarrierImage
{
    VklImages* images;
    bool queue_transfer;

    VkAccessFlags src_access;
    uint32_t src_queue;
    VkImageLayout src_layout;

    VkAccessFlags dst_access;
    uint32_t dst_queue;
    VkImageLayout dst_layout;
};



struct VklBarrier
{
    VklObject obj;
    VklGpu* gpu;

    // uint32_t idx; // index within the buffer regions or images

    VkPipelineStageFlagBits src_stage;
    VkPipelineStageFlagBits dst_stage;

    uint32_t buffer_barrier_count;
    VklBarrierBuffer buffer_barriers[VKL_MAX_BARRIERS_PER_SET];

    uint32_t image_barrier_count;
    VklBarrierImage image_barriers[VKL_MAX_BARRIERS_PER_SET];
};



struct VklFences
{
    VklObject obj;
    VklGpu* gpu;

    uint32_t count;
    VkFence fences[VKL_MAX_FENCES_PER_SET];
};



struct VklSemaphores
{
    VklObject obj;
    VklGpu* gpu;

    uint32_t count;
    VkSemaphore semaphores[VKL_MAX_SEMAPHORES_PER_SET];
};



struct VklRenderpassAttachment
{
    VkImageLayout ref_layout;
    VklRenderpassAttachmentType type;
    VkFormat format;

    VkImageLayout src_layout;
    VkImageLayout dst_layout;

    VkAttachmentLoadOp load_op;
    VkAttachmentStoreOp store_op;
};



struct VklRenderpassSubpass
{
    uint32_t attachment_count;
    uint32_t attachments[VKL_MAX_ATTACHMENTS_PER_RENDERPASS];
};



struct VklRenderpassDependency
{
    uint32_t src_subpass;
    VkPipelineStageFlags src_stage;
    VkAccessFlags src_access;

    uint32_t dst_subpass;
    VkPipelineStageFlags dst_stage;
    VkAccessFlags dst_access;
};



struct VklFramebuffers
{
    VklObject obj;
    VklGpu* gpu;
    VklRenderpass* renderpass;

    uint32_t attachment_count;
    // by definition, the framebuffers size = the first attachment's size
    VklImages* attachments[VKL_MAX_ATTACHMENTS_PER_RENDERPASS];

    uint32_t framebuffer_count;
    VkFramebuffer framebuffers[VKL_MAX_SWAPCHAIN_IMAGES];
};



struct VklRenderpass
{
    VklObject obj;
    VklGpu* gpu;

    uint32_t attachment_count;
    VklRenderpassAttachment attachments[VKL_MAX_ATTACHMENTS_PER_RENDERPASS];

    uint32_t clear_count;
    VkClearValue clear_values[VKL_MAX_ATTACHMENTS_PER_RENDERPASS];

    uint32_t subpass_count;
    VklRenderpassSubpass subpasses[VKL_MAX_SUBPASSES_PER_RENDERPASS];

    uint32_t dependency_count;
    VklRenderpassDependency dependencies[VKL_MAX_DEPENDENCIES_PER_RENDERPASS];

    VkRenderPass renderpass;
};



struct VklSubmit
{
    VklObject obj;
    VklGpu* gpu;

    uint32_t commands_count;
    VklCommands* commands[VKL_MAX_COMMANDS_PER_SUBMIT];

    uint32_t wait_semaphores_count;
    uint32_t wait_semaphores_idx[VKL_MAX_SEMAPHORES_PER_SUBMIT];
    VklSemaphores* wait_semaphores[VKL_MAX_SEMAPHORES_PER_SUBMIT];
    VkPipelineStageFlags wait_stages[VKL_MAX_SEMAPHORES_PER_SUBMIT];

    uint32_t signal_semaphores_count;
    uint32_t signal_semaphores_idx[VKL_MAX_SEMAPHORES_PER_SUBMIT];
    VklSemaphores* signal_semaphores[VKL_MAX_SEMAPHORES_PER_SUBMIT];
};



struct VklTexture
{
    VklObject obj;

    VklContext* context;

    VklImages* image;
    VklSampler* sampler;
};



/*************************************************************************************************/
/*  App                                                                                          */
/*************************************************************************************************/

/**
 * Create an application instance.
 *
 * There is typically only one App object in a given application. This object holds a pointer to
 * the Vulkan instance and is responsible for discovering the available GPUs.
 *
 * @param backend the backend
 * @returns a pointer to the created app
 */
VKY_EXPORT VklApp* vkl_app(VklBackend backend);

/**
 * Destroy the application.
 *
 * This function automatically destroys all objects created within the application.
 *
 * @param app the application to destroy
 */
VKY_EXPORT int vkl_app_destroy(VklApp* app);



/*************************************************************************************************/
/*  GPU                                                                                          */
/*************************************************************************************************/
/**
 * Initialize a GPU.
 *
 * A GPU object is the interface to one of the GPUs on the current system.
 *
 * @param app the app
 * @param idx the GPU index among the system's GPUs
 * @returns a pointer to the created GPU object
 */

VKY_EXPORT VklGpu* vkl_gpu(VklApp* app, uint32_t idx);

/**
 * Request some features before creating the GPU instance.
 *
 * This function needs to be called before creating the GPU with ` vkl_gpu_create()`.
 *
 * @param gpu the GPU
 * @param requested_features the list of requested features
 */
VKY_EXPORT void vkl_gpu_request_features(VklGpu* gpu, VkPhysicalDeviceFeatures requested_features);

/**
 * Request a new Vulkan queue before creating the GPU.
 *
 * @param gpu the GPU
 * @param type the queue type
 * @param type the queue index (should be regularly increasing: 0, 1, 2...)
 */
VKY_EXPORT void vkl_gpu_queue(VklGpu* gpu, VklQueueType type, uint32_t idx);

/**
 * Create a GPU once the features and queues have been set up.
 *
 * @param gpu the GPU
 * @param surface the surface on which the GPU will need to render
 */
VKY_EXPORT void vkl_gpu_create(VklGpu* gpu, VkSurfaceKHR surface);

/**
 * Wait for a queue to be idle.
 *
 * This is one of the different GPU synchronization methods. It is not efficient as it waits until
 * the queue is idle.
 *
 * @param gpu the GPU
 * @param queue_idx the queue index
 */
VKY_EXPORT void vkl_queue_wait(VklGpu* gpu, uint32_t queue_idx);

/**
 * Full synchronization on all GPUs.
 *
 * This function waits on all queues of all GPUs. The strongest, least efficient of the
 * synchronization methods.
 *
 * @param app the application instance
 */
VKY_EXPORT void vkl_app_wait(VklApp* app);

/**
 * Full synchronization on a given GPU.
 *
 * This function waits on all queues of a given GPU.
 *
 * @param gpu the GPU
 */
VKY_EXPORT void vkl_gpu_wait(VklGpu* gpu);

/**
 * Destroy the resources associated to a GPU.
 *
 * @param gpu the GPU
 */
VKY_EXPORT void vkl_gpu_destroy(VklGpu* gpu);



/*************************************************************************************************/
/*  Window                                                                                       */
/*************************************************************************************************/

/**
 * Create a blank window.
 *
 * This function is rarely used on its own. A bare window offers
 * no functionality that allows one to render to it with Vulkan. One needs a swapchain, an event
 * loop, and so on, which are provided instead at the level of the Canvas.
 *
 * @param app the application instance
 * @param width the window width, in pixels
 * @param height the window height, in pixels
 * @returns the window
 */
VKY_EXPORT VklWindow* vkl_window(VklApp* app, uint32_t width, uint32_t height);

/**
 * Get the window size, in pixels.
 *
 * @param window the window
 * @param[out] framebuffer_width the width, in pixels
 * @param[out] framebuffer_height the height, in pixels
 */
VKY_EXPORT void
vkl_window_get_size(VklWindow* window, uint32_t* framebuffer_width, uint32_t* framebuffer_height);

/**
 * Process the pending windowing events by the backend (glfw by default).
 *
 * @param window the window
 */
VKY_EXPORT void vkl_window_poll_events(VklWindow* window);

/**
 * Destroy a window.
 *
 * !!! warning
 *     This function must be imperatively called *after* `vkl_swapchain_destroy()`.
 *
 * @param window the window
 */
VKY_EXPORT void vkl_window_destroy(VklWindow* window);

/**
 * Destroy a canvas.
 *
 * @param canvas the canvas
 */
VKY_EXPORT void vkl_canvas_destroy(VklCanvas* canvas);

/**
 * Destroy all canvases.
 *
 * @param canvases the container with the canvases.
 */
VKY_EXPORT void vkl_canvases_destroy(VklContainer* canvases);



/*************************************************************************************************/
/*  Swapchain                                                                                    */
/*************************************************************************************************/

/**
 * Initialize a swapchain.
 *
 * @param gpu the GPU
 * @param window the window
 * @param min_img_count the minimum acceptable number of images in the swapchain
 * @returns the swapchain
 */
VKY_EXPORT VklSwapchain vkl_swapchain(VklGpu* gpu, VklWindow* window, uint32_t min_img_count);

/**
 * Set the swapchain image format.
 *
 * @param swapchain the swapchain
 * @param format the format
 */
VKY_EXPORT void vkl_swapchain_format(VklSwapchain* swapchain, VkFormat format);

/**
 * Set the swapchain present mode.
 *
 * @param swapchain the swapchain
 * @param present_mode the present mode
 */
VKY_EXPORT void vkl_swapchain_present_mode(VklSwapchain* swapchain, VkPresentModeKHR present_mode);

/**
 * Set the swapchain requested image size.
 *
 * @param swapchain the swapchain
 * @param width the requested width
 * @param height the requested height
 */
VKY_EXPORT void
vkl_swapchain_requested_size(VklSwapchain* swapchain, uint32_t width, uint32_t height);

/**
 * Create the swapchain once it has been set up.
 *
 * @param swapchain the swapchain
 */
VKY_EXPORT void vkl_swapchain_create(VklSwapchain* swapchain);

/**
 * Recreate a swapchain (for example after a window resize).
 *
 * @param swapchain the swapchain
 */
VKY_EXPORT void vkl_swapchain_recreate(VklSwapchain* swapchain);

/**
 * Acquire a swapchain image.
 *
 * @param swapchain the swapchain
 * @param semaphores the set of signal semaphores
 * @param semaphore_idx the index of the semaphore to signal after image acquisition
 * @param fences the set of signal fences
 * @param fence_idx the index of the fence to signal after image acquisition
 */
VKY_EXPORT void vkl_swapchain_acquire(
    VklSwapchain* swapchain, VklSemaphores* semaphores, uint32_t semaphore_idx, VklFences* fences,
    uint32_t fence_idx);

/**
 * Present a swapchain image to the screen after it has been rendered.
 *
 * @param swapchain the swapchain
 * @param queue_idx the index of the present queue
 * @param semaphores the set of waiting semaphores
 * @param semaphore_idx the index of the semaphore to wait on before presentation
 */
VKY_EXPORT void vkl_swapchain_present(
    VklSwapchain* swapchain, uint32_t queue_idx, VklSemaphores* semaphores,
    uint32_t semaphore_idx);

/**
 * Destroy a swapchain
 *
 * !!! warning
 *     This function must imperatively be called *before* `vkl_window_destroy()`.
 *
 * @param swapchain the swapchain
 */
VKY_EXPORT void vkl_swapchain_destroy(VklSwapchain* swapchain);



/*************************************************************************************************/
/*  Commands                                                                                     */
/*************************************************************************************************/

/**
 * Create a set of command buffers.
 *
 * @param gpu the GPU
 * @param queue the queue index within the GPU
 * @param count the number of command buffers to create
 * @returns the set of command buffers
 */
VKY_EXPORT VklCommands vkl_commands(VklGpu* gpu, uint32_t queue, uint32_t count);

/**
 * Start recording a command buffer.
 *
 * @param cmds the set of command buffers
 * @param idx the index of the command buffer to begin recording on
 */
VKY_EXPORT void vkl_cmd_begin(VklCommands* cmds, uint32_t idx);

/**
 * Stop recording a command buffer.
 *
 * @param cmds the set of command buffers
 * @param idx the index of the command buffer to stop the recording on
 */
VKY_EXPORT void vkl_cmd_end(VklCommands* cmds, uint32_t idx);

/**
 * Reset a command buffer.
 *
 * @param cmds the set of command buffers
 * @param idx the index of the command buffer to reset
 */
VKY_EXPORT void vkl_cmd_reset(VklCommands* cmds, uint32_t idx);

/**
 * Free a set of command buffers.
 *
 * @param cmds the set of command buffers
 */
VKY_EXPORT void vkl_cmd_free(VklCommands* cmds);

/**
 * Submit a command buffer on its queue with inefficient full synchronization.
 *
 * This function is relatively inefficient because it calls `vkl_queue_wait()`.
 *
 * @param cmds the set of command buffers
 * @param idx the index of the command buffer to submit
 */
VKY_EXPORT void vkl_cmd_submit_sync(VklCommands* cmds, uint32_t idx);

/**
 * Destroy a set of command buffers.
 *
 * @param cmds the set of command buffers
 */
VKY_EXPORT void vkl_commands_destroy(VklCommands* cmds);



/*************************************************************************************************/
/*  Buffers                                                                                      */
/*************************************************************************************************/

/**
 * Initialize a GPU buffer.
 *
 * @param gpu the GPU
 * @returns the buffer
 */
VKY_EXPORT VklBuffer vkl_buffer(VklGpu* gpu);

/**
 * Set the buffer size.
 *
 * @param buffer the buffer
 * @param size the buffer size, in bytes
 */
VKY_EXPORT void vkl_buffer_size(VklBuffer* buffer, VkDeviceSize size);

/**
 * Set the buffer type.
 *
 * @param buffer the buffer
 * @param type the buffer type
 */
VKY_EXPORT void vkl_buffer_type(VklBuffer* buffer, VklBufferType type);

/**
 * Set the buffer usage.
 *
 * @param buffer the buffer
 * @param usage the buffer usage
 */
VKY_EXPORT void vkl_buffer_usage(VklBuffer* buffer, VkBufferUsageFlags usage);

/**
 * Set the buffer memory properties.
 *
 * @param buffer the buffer
 * @param memory the memory properties
 */
VKY_EXPORT void vkl_buffer_memory(VklBuffer* buffer, VkMemoryPropertyFlags memory);

/**
 * Set the buffer queue access.
 *
 * @param buffer the buffer
 * @param queue_idx the queue index
 */
VKY_EXPORT void vkl_buffer_queue_access(VklBuffer* buffer, uint32_t queue_idx);

/**
 * Create the buffer after it has been set.
 *
 * @param buffer the buffer
 */
VKY_EXPORT void vkl_buffer_create(VklBuffer* buffer);

/**
 * Resize a buffer.
 *
 * @param buffer the buffer
 * @param size the new buffer size, in bytes
 * @param cmds the command buffers to use for the GPU-GPU data copy transfer
 */
VKY_EXPORT void vkl_buffer_resize(VklBuffer* buffer, VkDeviceSize size, VklCommands* cmds);

/**
 * Memory-map a buffer.
 *
 * @param buffer the buffer
 * @param offset the offset within the buffer, in bytes
 * @param size the size to map, in bytes
 */
VKY_EXPORT void* vkl_buffer_map(VklBuffer* buffer, VkDeviceSize offset, VkDeviceSize size);

/**
 * Unmap a buffer.
 *
 * @param buffer the buffer
 */
VKY_EXPORT void vkl_buffer_unmap(VklBuffer* buffer);

/**
 * Download a buffer data to the CPU.
 *
 * @param buffer the buffer
 * @param offset the offset within the buffer, in bytes
 * @param size the size of the region to download, in bytes
 * @param[out] data the buffer to download on (must be allocated with the appropriate size)
 */
VKY_EXPORT void
vkl_buffer_download(VklBuffer* buffer, VkDeviceSize offset, VkDeviceSize size, void* data);

/**
 * Upload data to a GPU buffer.
 *
 * @param buffer the buffer
 * @param offset the offset within the buffer, in bytes
 * @param size the buffer size, in bytes
 * @param data the data to upload
 */
VKY_EXPORT void
vkl_buffer_upload(VklBuffer* buffer, VkDeviceSize offset, VkDeviceSize size, const void* data);

/**
 * Destroy a buffer
 *
 * @param buffer the buffer
 */
VKY_EXPORT void vkl_buffer_destroy(VklBuffer* buffer);



/**
 * Create buffer regions on an existing GPU buffer.
 *
 * @param buffer the buffer
 * @param count the number of successive regions
 * @param offset the offset within the buffer
 * @param size the size of each region, in bytes
 * @param alignment the alignment requirement for the region offsets
 */
VKY_EXPORT VklBufferRegions vkl_buffer_regions(
    VklBuffer* buffer, uint32_t count, //
    VkDeviceSize offset, VkDeviceSize size, VkDeviceSize alignment);

/**
 * Map a buffer region.
 *
 * @param br the buffer regions
 * @param idx the index of the buffer region to map
 */
VKY_EXPORT void* vkl_buffer_regions_map(VklBufferRegions* br, uint32_t idx);

/**
 * Unmap a set of buffer regions.
 *
 * @param br the buffer regions
 */
VKY_EXPORT void vkl_buffer_regions_unmap(VklBufferRegions* br);

/**
 * Upload data to a buffer region.
 *
 * @param br the set of buffer regions
 * @param idx the index of the buffer region to upload data to
 * @param data the data to upload
 */
VKY_EXPORT void vkl_buffer_regions_upload(VklBufferRegions* br, uint32_t idx, const void* data);



/*************************************************************************************************/
/*  Images                                                                                       */
/*************************************************************************************************/

/**
 * Initialize a set of GPU images.
 *
 * @param gpu the GPU
 * @param type the image type
 * @param count the number of images
 * @returns the images
 */
VKY_EXPORT VklImages vkl_images(VklGpu* gpu, VkImageType type, uint32_t count);

/**
 * Set the images format.
 *
 * @param images the images
 * @param format the image format
 */
VKY_EXPORT void vkl_images_format(VklImages* images, VkFormat format);

/**
 * Set the images layout.
 *
 * @param images the images
 * @param layout the image layout
 */
VKY_EXPORT void vkl_images_layout(VklImages* images, VkImageLayout layout);

/**
 * Set the images size.
 *
 * @param images the images
 * @param width the image width
 * @param height the image height
 * @param depth the image depth
 */
VKY_EXPORT void
vkl_images_size(VklImages* images, uint32_t width, uint32_t height, uint32_t depth);

/**
 * Set the images tiling.
 *
 * @param images the images
 * @param tiling the image tiling
 */
VKY_EXPORT void vkl_images_tiling(VklImages* images, VkImageTiling tiling);

/**
 * Set the images usage.
 *
 * @param images the images
 * @param usage the image usage
 */
VKY_EXPORT void vkl_images_usage(VklImages* images, VkImageUsageFlags usage);

/**
 * Set the images memory properties.
 *
 * @param images the images
 * @param memory the memory properties
 */
VKY_EXPORT void vkl_images_memory(VklImages* images, VkMemoryPropertyFlags memory);

/**
 * Set the images aspect.
 *
 * @param images the images
 * @param aspect the image aspect
 */
VKY_EXPORT void vkl_images_aspect(VklImages* images, VkImageAspectFlags aspect);

/**
 * Set the images queue access.
 *
 * This parameter specifies which queues may access the image from command buffers submitted to
 * them.
 *
 * @param images the images
 * @param queue_idx the queue index
 */
VKY_EXPORT void vkl_images_queue_access(VklImages* images, uint32_t queue_idx);

/**
 * Create the images after they have been set up.
 *
 * @param images the images
 */
VKY_EXPORT void vkl_images_create(VklImages* images);

/**
 * Resize images.
 *
 * !!! warning
 *     This function deletes the images contents when resizing.
 *
 * @param images the images
 * @param width the new width
 * @param height the new height
 * @param depth the new depth
 */
VKY_EXPORT void
vkl_images_resize(VklImages* images, uint32_t width, uint32_t height, uint32_t depth);

/**
 * Transition the images to their layout after creation.
 *
 * This function performs a hard synchronization on the queue and submits a command buffer with the
 * image transition.
 *
 * @param images the images
 */
VKY_EXPORT void vkl_images_transition(VklImages* images);

/**
 * Download the data from a staging GPU image.
 *
 * @param staging the images to download the data from
 * @param idx the index of the image
 * @param swizzle whether the RGBA values need to be transposed
 * @param[out] rgba the buffer that will be filled with the image data (must be already allocated)
 */
VKY_EXPORT void vkl_images_download(VklImages* staging, uint32_t idx, bool swizzle, uint8_t* rgba);

/**
 * Destroy images.
 *
 * @param images the images
 */
VKY_EXPORT void vkl_images_destroy(VklImages* images);



/*************************************************************************************************/
/*  Sampler                                                                                      */
/*************************************************************************************************/

/**
 * Initialize a texture sampler.
 *
 * @param gpu the GPU
 * @returns the sampler object
 */
VKY_EXPORT VklSampler vkl_sampler(VklGpu* gpu);

/**
 * Set the sampler min filter.
 *
 * @param sampler the sampler
 * @param filter the filter
 */
VKY_EXPORT void vkl_sampler_min_filter(VklSampler* sampler, VkFilter filter);

/**
 * Set the sampler mag filter.
 *
 * @param sampler the sampler
 * @param filter the filter
 */
VKY_EXPORT void vkl_sampler_mag_filter(VklSampler* sampler, VkFilter filter);

/**
 * Set the sampler address mode
 *
 * @param sampler the sampler
 * @param axis the sampler axis
 * @param address_mode the address mode
 */
VKY_EXPORT void vkl_sampler_address_mode(
    VklSampler* sampler, VklTextureAxis axis, VkSamplerAddressMode address_mode);

/**
 * Create the sampler after it has been set up.
 *
 * @param sampler the sampler
 */
VKY_EXPORT void vkl_sampler_create(VklSampler* sampler);

/**
 * Destroy a sampler
 *
 * @param sampler the sampler
 */
VKY_EXPORT void vkl_sampler_destroy(VklSampler* sampler);



/*************************************************************************************************/
/*  Slots                                                                                        */
/*************************************************************************************************/

/**
 * Initialize pipeline slots (aka Vulkan descriptor set layout).
 *
 * @param gpu the GPU
 * @returns the slots
 */
VKY_EXPORT VklSlots vkl_slots(VklGpu* gpu);

/**
 * Set the slots binding.
 *
 * @param slots the slots
 * @param idx the slot index to set up
 * @param type the descriptor type for that slot
 */
VKY_EXPORT void vkl_slots_binding(VklSlots* slots, uint32_t idx, VkDescriptorType type);

/**
 * Set up push constants.
 *
 * @param slots the slots
 * @param offset the push constant offset, in bytes
 * @param size the push constant size, in bytes
 * @param shaders the shader stages that will access the push constant
 */
VKY_EXPORT void vkl_slots_push(
    VklSlots* slots, VkDeviceSize offset, VkDeviceSize size, VkShaderStageFlags shaders);

/**
 * Create the slots after they have been set up.
 *
 * @param slots the slots
 */
VKY_EXPORT void vkl_slots_create(VklSlots* slots);

/**
 * Destroy the slots
 *
 * @param slots the slots
 */
VKY_EXPORT void vkl_slots_destroy(VklSlots* slots);



/*************************************************************************************************/
/*  Bindings                                                                                     */
/*************************************************************************************************/

/**
 * Initialize bindings corresponding to slots.
 *
 * @param slots the slots
 * @param dset_count the number of descriptor sets (number of swapchain images)
 */
VKY_EXPORT VklBindings vkl_bindings(VklSlots* slots, uint32_t dset_count);

/**
 * Bind a buffer to a slot.
 *
 * @param bindings the bindings
 * @param idx the slot index
 * @param br the buffer regions to bind to that slot
 */
VKY_EXPORT void vkl_bindings_buffer(VklBindings* bindings, uint32_t idx, VklBufferRegions br);

/**
 * Bind a texture to a slot.
 *
 * @param bindings the bindings
 * @param idx the slot index
 * @param br the texture to bind to that slot
 */
VKY_EXPORT void vkl_bindings_texture(VklBindings* bindings, uint32_t idx, VklTexture* texture);

/**
 * Update the bindings after the buffers/textures have been set up.
 *
 * @param bindings the bindings
 */
VKY_EXPORT void vkl_bindings_update(VklBindings* bindings);

/**
 * Destroy bindings.
 *
 * @param bindings the bindings
 */
VKY_EXPORT void vkl_bindings_destroy(VklBindings* bindings);



/*************************************************************************************************/
/*  Compute                                                                                      */
/*************************************************************************************************/

/**
 * Initialize a compute pipeline.
 *
 * @param gpu the GPU
 * @param shader_path (optional) the path to the `.spirv` file with the compute shader
 * @returns the compute pipeline
 */
VKY_EXPORT VklCompute vkl_compute(VklGpu* gpu, const char* shader_path);

/**
 * Create a compute pipeline after it has been set up.
 *
 * @param compute the compute pipeline
 */
VKY_EXPORT void vkl_compute_create(VklCompute* compute);

/**
 * Set the GLSL code directly (the library will compile it automatically to SPIRV).
 *
 * @param compute the compute pipeline
 * @param code the GLSL code defining the compute shader
 */
VKY_EXPORT void vkl_compute_code(VklCompute* compute, const char* code);

/**
 * Declare a slot for the compute pipeline.
 *
 * @param compute the compute pipeline
 * @param idx the slot index
 * @param type the descriptor type
 */
VKY_EXPORT void vkl_compute_slot(VklCompute* compute, uint32_t idx, VkDescriptorType type);

/**
 * Set up push constant.
 *
 * @param compute the compute pipeline
 * @param offset the push constant offset, in bytes
 * @param size the push constant size, in bytes
 * @param shaders the shaders that will need to access the push constant
 */
VKY_EXPORT void vkl_compute_push(
    VklCompute* compute, VkDeviceSize offset, VkDeviceSize size, VkShaderStageFlags shaders);

/**
 * Associate a bindings object to a compute pipeline.
 *
 * @param compute the compute pipeline
 * @param bindings the bindings
 */
VKY_EXPORT void vkl_compute_bindings(VklCompute* compute, VklBindings* bindings);

/**
 * Destroy a compute pipeline.
 *
 * @param compute the compute pipeline
 */
VKY_EXPORT void vkl_compute_destroy(VklCompute* compute);



/*************************************************************************************************/
/*  Pipeline                                                                                     */
/*************************************************************************************************/

/**
 * Initialize a graphics pipeline.
 *
 * @param gpu the GPU
 * @returns the graphics pipeline
 */
VKY_EXPORT VklGraphics vkl_graphics(VklGpu* gpu);

/**
 * Set the renderpass of a graphics pipeline.
 *
 * @param graphics the graphics pipeline
 * @param renderpass the render pass
 * @param subpass the subpass index
 */
VKY_EXPORT void
vkl_graphics_renderpass(VklGraphics* graphics, VklRenderpass* renderpass, uint32_t subpass);

/**
 * Set the graphics pipeline primitive topology
 *
 * @param graphics the graphics pipeline
 * @param topology the primitive topology
 */
VKY_EXPORT void vkl_graphics_topology(VklGraphics* graphics, VkPrimitiveTopology topology);

/**
 * Set the GLSL code of a graphics pipeline.
 *
 * @param graphics the graphics pipeline
 * @param stage the shader stage
 * @param code the GLSL code of the shader
 */
VKY_EXPORT void
vkl_graphics_shader_glsl(VklGraphics* graphics, VkShaderStageFlagBits stage, const char* code);

/**
 * Set the SPIRV code of a graphics pipeline.
 *
 * @param graphics the graphics pipeline
 * @param stage the shader stage
 * @param size the size of the SPIRV buffer, in bytes
 * @param buffer the binary buffer with the SPIRV code
 */
VKY_EXPORT void vkl_graphics_shader_spirv(
    VklGraphics* graphics, VkShaderStageFlagBits stage, VkDeviceSize size, const uint32_t* buffer);

/**
 * Set the path to a shader for a graphics pipeline.
 *
 * @param graphics the graphics pipeline
 * @param stage the shader stage
 * @param shader_path the path to the `.spirv` shader file
 */
VKY_EXPORT void
vkl_graphics_shader(VklGraphics* graphics, VkShaderStageFlagBits stage, const char* shader_path);

/**
 * Set the vertex binding.
 *
 * @param graphics the graphics pipeline
 * @param binding the binding index
 * @param stride the stride in the vertex buffer, in bytes
 */
VKY_EXPORT void
vkl_graphics_vertex_binding(VklGraphics* graphics, uint32_t binding, VkDeviceSize stride);

/**
 * Add a vertex attribute.
 *
 * @param graphics the graphics pipeline
 * @param binding the binding index (as specified in the vertex shader)
 * @param location the location index (as specified in the vertex shader)
 * @param format the format
 * @param offset the offset, in bytes
 */
VKY_EXPORT void vkl_graphics_vertex_attr(
    VklGraphics* graphics, uint32_t binding, uint32_t location, VkFormat format,
    VkDeviceSize offset);

/**
 * Set the graphics blend type.
 *
 * @param graphics the graphics pipeline
 * @param blend_type the blend type
 */
VKY_EXPORT void vkl_graphics_blend(VklGraphics* graphics, VklBlendType blend_type);

/**
 * Set the graphics depth test.
 *
 * @param graphics the graphics pipeline
 * @param depth_test the depth test
 */
VKY_EXPORT void vkl_graphics_depth_test(VklGraphics* graphics, VklDepthTest depth_test);

/**
 * Set the graphics polygon mode.
 *
 * @param graphics the graphics pipeline
 * @param polygon_mode the polygon mode
 */
VKY_EXPORT void vkl_graphics_polygon_mode(VklGraphics* graphics, VkPolygonMode polygon_mode);

/**
 * Set the graphics cull mode.
 *
 * @param graphics the graphics pipeline
 * @param cull_mode the cull mode
 */
VKY_EXPORT void vkl_graphics_cull_mode(VklGraphics* graphics, VkCullModeFlags cull_mode);

/**
 * Set the graphics front face.
 *
 * @param graphics the graphics pipeline
 * @param front_face the front face
 */
VKY_EXPORT void vkl_graphics_front_face(VklGraphics* graphics, VkFrontFace front_face);

/**
 * Create a graphics pipeline after it has been set up.
 *
 * @param graphics the graphics pipeline
 */
VKY_EXPORT void vkl_graphics_create(VklGraphics* graphics);

/**
 * Set a binding slot for a graphics pipeline.
 *
 * @param graphics the graphics pipeline
 * @param idx the slot index
 * @param type the descriptor type
 */
VKY_EXPORT void vkl_graphics_slot(VklGraphics* graphics, uint32_t idx, VkDescriptorType type);

/**
 * Set a graphics pipeline push constant.
 *
 * @param graphics the graphics pipeline
 * @param offset the push constant offset, in bytes
 * @param offset the push size, in bytes
 * @param shaders the shader stages that will access the push constant
 */
VKY_EXPORT void vkl_graphics_push(
    VklGraphics* graphics, VkDeviceSize offset, VkDeviceSize size, VkShaderStageFlags shaders);

/**
 * Destroy a graphics pipeline.
 *
 * @param graphics the graphics pipeline
 */
VKY_EXPORT void vkl_graphics_destroy(VklGraphics* graphics);



/*************************************************************************************************/
/*  Barrier                                                                                      */
/*************************************************************************************************/

/**
 * Initialize a synchronization barrier (usedwithin a command buffer).
 *
 * @param gpu the GPU
 * @returns the barrier
 */
VKY_EXPORT VklBarrier vkl_barrier(VklGpu* gpu);

/**
 * Set the barrier stages.
 *
 * @param barrier the barrier
 * @param src_stage the source stage
 * @param dst_stage the destination stage
 */
VKY_EXPORT void vkl_barrier_stages(
    VklBarrier* barrier, VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage);

/**
 * Set the barrier buffer.
 *
 * @param barrier the barrier
 * @param br the buffer regions
 */
VKY_EXPORT void vkl_barrier_buffer(VklBarrier* barrier, VklBufferRegions br);

/**
 * Set the barrier buffer queue.
 *
 * @param barrier the barrier
 * @param src_queue the source queue index
 * @param dst_queue the destination queue index
 */
VKY_EXPORT void
vkl_barrier_buffer_queue(VklBarrier* barrier, uint32_t src_queue, uint32_t dst_queue);

/**
 * Set the barrier buffer access.
 *
 * @param barrier the barrier
 * @param src_access the source access flags
 * @param dst_access the destination access flags
 */
VKY_EXPORT void
vkl_barrier_buffer_access(VklBarrier* barrier, VkAccessFlags src_access, VkAccessFlags dst_access);

/**
 * Set the barrier images.
 *
 * @param barrier the barrier
 * @param images the images
 */
VKY_EXPORT void vkl_barrier_images(VklBarrier* barrier, VklImages* images);

/**
 * Set the barrier images layout.
 *
 * @param barrier the barrier
 * @param src_layout the source layout
 * @param dst_layout the destination layout
 */
VKY_EXPORT void
vkl_barrier_images_layout(VklBarrier* barrier, VkImageLayout src_layout, VkImageLayout dst_layout);

/**
 * Set the barrier images queue.
 *
 * @param barrier the barrier
 * @param src_queue the source queue index
 * @param dst_queue the destination queue index
 */
VKY_EXPORT void
vkl_barrier_images_queue(VklBarrier* barrier, uint32_t src_queue, uint32_t dst_queue);

/**
 * Set the barrier images access.
 *
 * @param barrier the barrier
 * @param src_access the source access flags
 * @param dst_access the destination access flags
 */
VKY_EXPORT void
vkl_barrier_images_access(VklBarrier* barrier, VkAccessFlags src_access, VkAccessFlags dst_access);



/*************************************************************************************************/
/*  Semaphores                                                                                   */
/*************************************************************************************************/

VKY_EXPORT VklSemaphores vkl_semaphores(VklGpu* gpu, uint32_t count);

VKY_EXPORT void vkl_semaphores_destroy(VklSemaphores* semaphores);



/*************************************************************************************************/
/*  Fences                                                                                       */
/*************************************************************************************************/

VKY_EXPORT VklFences vkl_fences(VklGpu* gpu, uint32_t count);

VKY_EXPORT void
vkl_fences_copy(VklFences* src_fences, uint32_t src_idx, VklFences* dst_fences, uint32_t dst_idx);

VKY_EXPORT void vkl_fences_wait(VklFences* fences, uint32_t idx);

VKY_EXPORT bool vkl_fences_ready(VklFences* fences, uint32_t idx);

VKY_EXPORT void vkl_fences_reset(VklFences* fences, uint32_t idx);

VKY_EXPORT void vkl_fences_destroy(VklFences* fences);



/*************************************************************************************************/
/*  Renderpass                                                                                   */
/*************************************************************************************************/

VKY_EXPORT VklRenderpass vkl_renderpass(VklGpu* gpu);

VKY_EXPORT void vkl_renderpass_clear(VklRenderpass* renderpass, VkClearValue value);

VKY_EXPORT void vkl_renderpass_attachment(
    VklRenderpass* renderpass, uint32_t idx, VklRenderpassAttachmentType type, VkFormat format,
    VkImageLayout ref_layout);

VKY_EXPORT void vkl_renderpass_attachment_layout(
    VklRenderpass* renderpass, uint32_t idx, VkImageLayout src_layout, VkImageLayout dst_layout);

VKY_EXPORT void vkl_renderpass_attachment_ops(
    VklRenderpass* renderpass, uint32_t idx, //
    VkAttachmentLoadOp load_op, VkAttachmentStoreOp store_op);

VKY_EXPORT void vkl_renderpass_subpass_attachment(
    VklRenderpass* renderpass, uint32_t subpass_idx, uint32_t attachment_idx);

VKY_EXPORT void vkl_renderpass_subpass_dependency(
    VklRenderpass* renderpass, uint32_t dependency_idx, //
    uint32_t src_subpass, uint32_t dst_subpass);

VKY_EXPORT void vkl_renderpass_subpass_dependency_access(
    VklRenderpass* renderpass, uint32_t dependency_idx, //
    VkAccessFlags src_access, VkAccessFlags dst_access);

VKY_EXPORT void vkl_renderpass_subpass_dependency_stage(
    VklRenderpass* renderpass, uint32_t dependency_idx, //
    VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage);

VKY_EXPORT void vkl_renderpass_create(VklRenderpass* renderpass);

VKY_EXPORT void vkl_renderpass_destroy(VklRenderpass* renderpass);



/*************************************************************************************************/
/*  Framebuffers                                                                                 */
/*************************************************************************************************/

VKY_EXPORT VklFramebuffers vkl_framebuffers(VklGpu* gpu);

VKY_EXPORT void vkl_framebuffers_attachment(
    VklFramebuffers* framebuffers, uint32_t attachment_idx, VklImages* images);

VKY_EXPORT void vkl_framebuffers_create(VklFramebuffers* framebuffers, VklRenderpass* renderpass);

VKY_EXPORT void vkl_framebuffers_destroy(VklFramebuffers* framebuffers);



/*************************************************************************************************/
/*  Submit                                                                                       */
/*************************************************************************************************/

VKY_EXPORT VklSubmit vkl_submit(VklGpu* gpu);

VKY_EXPORT void vkl_submit_commands(VklSubmit* submit, VklCommands* commands);

VKY_EXPORT void vkl_submit_wait_semaphores(
    VklSubmit* submit, VkPipelineStageFlags stage, VklSemaphores* semaphores, uint32_t idx);

VKY_EXPORT void
vkl_submit_signal_semaphores(VklSubmit* submit, VklSemaphores* semaphores, uint32_t idx);

VKY_EXPORT void
vkl_submit_send(VklSubmit* submit, uint32_t cmd_idx, VklFences* fence, uint32_t fence_idx);

VKY_EXPORT void vkl_submit_reset(VklSubmit* submit);



/*************************************************************************************************/
/*  Command buffer filling                                                                       */
/*************************************************************************************************/

VKY_EXPORT void vkl_cmd_begin_renderpass(
    VklCommands* cmds, uint32_t idx, VklRenderpass* renderpass, VklFramebuffers* framebuffers);

VKY_EXPORT void vkl_cmd_end_renderpass(VklCommands* cmds, uint32_t idx);

VKY_EXPORT void vkl_cmd_compute(VklCommands* cmds, uint32_t idx, VklCompute* compute, uvec3 size);

VKY_EXPORT void vkl_cmd_barrier(VklCommands* cmds, uint32_t idx, VklBarrier* barrier);

VKY_EXPORT void vkl_cmd_copy_buffer_to_image(
    VklCommands* cmds, uint32_t idx, VklBuffer* buffer, VklImages* images);

VKY_EXPORT void vkl_cmd_copy_image_to_buffer(
    VklCommands* cmds, uint32_t idx, VklImages* images, VklBuffer* buffer);

VKY_EXPORT void
vkl_cmd_copy_image(VklCommands* cmds, uint32_t idx, VklImages* src_img, VklImages* dst_img);

VKY_EXPORT void vkl_cmd_viewport(VklCommands* cmds, uint32_t idx, VkViewport viewport);

VKY_EXPORT void vkl_cmd_bind_graphics(
    VklCommands* cmds, uint32_t idx, VklGraphics* graphics, //
    VklBindings* bindings, uint32_t dynamic_idx);

VKY_EXPORT void vkl_cmd_bind_vertex_buffer(
    VklCommands* cmds, uint32_t idx, VklBufferRegions br, VkDeviceSize offset);

VKY_EXPORT void vkl_cmd_bind_index_buffer(
    VklCommands* cmds, uint32_t idx, VklBufferRegions br, VkDeviceSize offset);

VKY_EXPORT void
vkl_cmd_draw(VklCommands* cmds, uint32_t idx, uint32_t first_vertex, uint32_t vertex_count);

VKY_EXPORT void vkl_cmd_draw_indexed(
    VklCommands* cmds, uint32_t idx, uint32_t first_index, uint32_t vertex_offset,
    uint32_t index_count);

VKY_EXPORT void vkl_cmd_draw_indirect(VklCommands* cmds, uint32_t idx, VklBufferRegions indirect);

VKY_EXPORT void
vkl_cmd_draw_indexed_indirect(VklCommands* cmds, uint32_t idx, VklBufferRegions indirect);

VKY_EXPORT void vkl_cmd_copy_buffer(
    VklCommands* cmds, uint32_t idx,             //
    VklBuffer* src_buf, VkDeviceSize src_offset, //
    VklBuffer* dst_buf, VkDeviceSize dst_offset, //
    VkDeviceSize size);

VKY_EXPORT void vkl_cmd_push(
    VklCommands* cmds, uint32_t idx, VklSlots* slots, VkShaderStageFlagBits shaders, //
    VkDeviceSize offset, VkDeviceSize size, const void* data);



/*************************************************************************************************/
/*  Context                                                                                      */
/*************************************************************************************************/

VKY_EXPORT void vkl_context_destroy(VklContext* context);



#ifdef __cplusplus
}
#endif

#endif
