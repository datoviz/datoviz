/*************************************************************************************************/
/*  Light wrapper on top of the Vulkan C API                                                     */
/*************************************************************************************************/

#ifndef DVZ_VKLITE_HEADER
#define DVZ_VKLITE_HEADER

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

#define DVZ_MAX_BINDINGS_SIZE    32
#define DVZ_MAX_DESCRIPTOR_SETS  1024
#define DVZ_MAX_PRESENT_MODES    16
#define DVZ_MAX_PUSH_CONSTANTS   16
#define DVZ_MAX_QUEUE_FAMILIES   16
#define DVZ_MAX_QUEUES           16
#define DVZ_MAX_SWAPCHAIN_IMAGES 8

// Maximum number of command buffers per DvzCommands struct
#define DVZ_MAX_COMMAND_BUFFERS_PER_SET     DVZ_MAX_SWAPCHAIN_IMAGES
#define DVZ_MAX_BUFFER_REGIONS_PER_SET      DVZ_MAX_SWAPCHAIN_IMAGES
#define DVZ_MAX_IMAGES_PER_SET              DVZ_MAX_SWAPCHAIN_IMAGES
#define DVZ_MAX_SEMAPHORES_PER_SET          DVZ_MAX_SWAPCHAIN_IMAGES
#define DVZ_MAX_FENCES_PER_SET              DVZ_MAX_SWAPCHAIN_IMAGES
#define DVZ_MAX_COMMANDS_PER_SUBMIT         16
#define DVZ_MAX_BARRIERS_PER_SET            8
#define DVZ_MAX_SEMAPHORES_PER_SUBMIT       8
#define DVZ_MAX_SHADERS_PER_GRAPHICS        8
#define DVZ_MAX_ATTACHMENTS_PER_RENDERPASS  8
#define DVZ_MAX_SUBPASSES_PER_RENDERPASS    8
#define DVZ_MAX_DEPENDENCIES_PER_RENDERPASS 8
#define DVZ_MAX_VERTEX_BINDINGS             16
#define DVZ_MAX_VERTEX_ATTRS                32



/*************************************************************************************************/
/*  Type definitions */
/*************************************************************************************************/

typedef struct DvzQueues DvzQueues;
typedef struct DvzGpu DvzGpu;
typedef struct DvzWindow DvzWindow;
typedef struct DvzSwapchain DvzSwapchain;
typedef struct DvzCommands DvzCommands;
typedef struct DvzBuffer DvzBuffer;
typedef struct DvzBufferRegions DvzBufferRegions;
typedef struct DvzImages DvzImages;
typedef struct DvzSampler DvzSampler;
typedef struct DvzSlots DvzSlots;
typedef struct DvzBindings DvzBindings;
typedef struct DvzCompute DvzCompute;
typedef struct DvzVertexBinding DvzVertexBinding;
typedef struct DvzVertexAttr DvzVertexAttr;
typedef struct DvzGraphics DvzGraphics;
typedef struct DvzBarrierBuffer DvzBarrierBuffer;
typedef struct DvzBarrierImage DvzBarrierImage;
typedef struct DvzBarrier DvzBarrier;
typedef struct DvzSemaphores DvzSemaphores;
typedef struct DvzFences DvzFences;
typedef struct DvzRenderpass DvzRenderpass;
typedef struct DvzRenderpassAttachment DvzRenderpassAttachment;
typedef struct DvzRenderpassSubpass DvzRenderpassSubpass;
typedef struct DvzRenderpassDependency DvzRenderpassDependency;
typedef struct DvzFramebuffers DvzFramebuffers;
typedef struct DvzSubmit DvzSubmit;

// Forward declarations.
typedef struct DvzCanvas DvzCanvas;
typedef struct DvzContext DvzContext;
typedef struct DvzTexture DvzTexture;
typedef struct DvzGraphicsData DvzGraphicsData;

// Callback definitions
typedef void (*DvzGraphicsCallback)(DvzGraphicsData* data, uint32_t item_count, const void* item);



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Queue type.
typedef enum
{
    DVZ_QUEUE_TRANSFER = 0x01,
    DVZ_QUEUE_GRAPHICS = 0x02,
    DVZ_QUEUE_COMPUTE = 0x04,
    DVZ_QUEUE_PRESENT = 0x08,
    DVZ_QUEUE_RENDER = 0x07,
    DVZ_QUEUE_ALL = 0x0F,
} DvzQueueType;



// Command buffer type.
typedef enum
{
    DVZ_COMMAND_TRANSFERS,
    DVZ_COMMAND_GRAPHICS,
    DVZ_COMMAND_COMPUTE,
    DVZ_COMMAND_GUI,
} DvzCommandBufferType;



// Buffer type.
typedef enum
{
    DVZ_BUFFER_TYPE_UNDEFINED,
    DVZ_BUFFER_TYPE_STAGING,
    DVZ_BUFFER_TYPE_VERTEX,
    DVZ_BUFFER_TYPE_INDEX,
    DVZ_BUFFER_TYPE_UNIFORM,
    DVZ_BUFFER_TYPE_STORAGE,
    DVZ_BUFFER_TYPE_UNIFORM_MAPPABLE,
    DVZ_BUFFER_TYPE_COUNT,
} DvzBufferType;



// Graphics builtins
typedef enum
{
    DVZ_GRAPHICS_NONE,
    DVZ_GRAPHICS_POINT,

    DVZ_GRAPHICS_LINE,
    DVZ_GRAPHICS_LINE_STRIP,
    DVZ_GRAPHICS_TRIANGLE,
    DVZ_GRAPHICS_TRIANGLE_STRIP,
    DVZ_GRAPHICS_TRIANGLE_FAN,

    DVZ_GRAPHICS_MARKER,

    DVZ_GRAPHICS_SEGMENT,
    DVZ_GRAPHICS_ARROW,
    DVZ_GRAPHICS_PATH,
    DVZ_GRAPHICS_TEXT,

    DVZ_GRAPHICS_IMAGE,
    DVZ_GRAPHICS_IMAGE_CMAP,

    DVZ_GRAPHICS_VOLUME_SLICE,
    DVZ_GRAPHICS_MESH,

    DVZ_GRAPHICS_FAKE_SPHERE,
    DVZ_GRAPHICS_VOLUME,

    DVZ_GRAPHICS_COUNT,
    DVZ_GRAPHICS_CUSTOM,
} DvzGraphicsType;



// Texture axis.
typedef enum
{
    DVZ_TEXTURE_AXIS_U,
    DVZ_TEXTURE_AXIS_V,
    DVZ_TEXTURE_AXIS_W,
} DvzTextureAxis;



// Blend type.
typedef enum
{
    DVZ_BLEND_DISABLE,
    DVZ_BLEND_ENABLE,
} DvzBlendType;



// Depth test.
typedef enum
{
    DVZ_DEPTH_TEST_DISABLE,
    DVZ_DEPTH_TEST_ENABLE,
} DvzDepthTest;



// Render pass attachment type.
typedef enum
{
    DVZ_RENDERPASS_ATTACHMENT_COLOR,
    DVZ_RENDERPASS_ATTACHMENT_DEPTH,
} DvzRenderpassAttachmentType;



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
        log_warn("mismatch between image count and cmd buf count");                               \
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

struct DvzQueues
{
    DvzObject obj;

    // Hardware supported queues
    // -------------------------
    // Number of different queue families supported by the hardware
    uint32_t queue_family_count;
    // Properties of the queue families
    // VkQueueFamilyProperties queue_families[DVZ_MAX_QUEUE_FAMILIES];
    bool support_transfer[DVZ_MAX_QUEUE_FAMILIES];
    bool support_graphics[DVZ_MAX_QUEUE_FAMILIES];
    bool support_compute[DVZ_MAX_QUEUE_FAMILIES];
    bool support_present[DVZ_MAX_QUEUE_FAMILIES];
    uint32_t max_queue_count[DVZ_MAX_QUEUE_FAMILIES]; // for each queue family, the max # of queues

    // Requested queues
    // ----------------
    // Number of requested queues
    uint32_t queue_count;
    // Requested queue types.
    DvzQueueType queue_types[DVZ_MAX_QUEUES]; // the VKL type of each queue
    // Queues and associated command pools
    uint32_t queue_families[DVZ_MAX_QUEUES]; // for each family, the # of queues
    uint32_t queue_indices[DVZ_MAX_QUEUES];  // for each requested queue, its # within its family
    VkQueue queues[DVZ_MAX_QUEUES];
    VkCommandPool cmd_pools[DVZ_MAX_QUEUE_FAMILIES];
};



struct DvzGpu
{
    DvzObject obj;
    DvzApp* app;

    uint32_t idx; // GPU index within the app
    const char* name;

    VkPhysicalDevice physical_device;
    VkPhysicalDeviceProperties device_properties;
    VkPhysicalDeviceFeatures device_features;
    VkPhysicalDeviceMemoryProperties memory_properties;

    uint32_t present_mode_count;
    VkPresentModeKHR present_modes[DVZ_MAX_PRESENT_MODES];

    DvzQueues queues;
    VkDescriptorPool dset_pool;

    VkPhysicalDeviceFeatures requested_features;
    VkDevice device;

    DvzContext* context;
};



struct DvzWindow
{
    DvzObject obj;
    DvzApp* app;

    void* backend_window;
    uint32_t width, height; // in screen coordinates

    bool close_on_esc;
    VkSurfaceKHR surface;
    VkSurfaceCapabilitiesKHR caps; // current extent in pixel coordinates (framebuffers)
};



struct DvzSwapchain
{
    DvzObject obj;
    DvzGpu* gpu;
    DvzWindow* window;

    VkFormat format;
    VkPresentModeKHR present_mode;
    bool support_transfer; // whether the swapchain supports copying the image to another

    // extent in pixel coordinates if caps.currentExtent is not available
    uint32_t requested_width, requested_height;

    uint32_t img_count;
    uint32_t img_idx;
    VkSwapchainKHR swapchain;

    // The actual framebuffer size in pixels is found in the images size
    DvzImages* images;
};



struct DvzCommands
{
    DvzObject obj;
    DvzGpu* gpu;

    uint32_t queue_idx;
    uint32_t count;
    VkCommandBuffer cmds[DVZ_MAX_COMMAND_BUFFERS_PER_SET];
};



struct DvzBuffer
{
    DvzObject obj;
    DvzGpu* gpu;

    DvzBufferType type;
    VkBuffer buffer;
    VkDeviceMemory device_memory;

    // Queues that need access to the buffer.
    uint32_t queue_count;
    uint32_t queues[DVZ_MAX_QUEUES];

    VkDeviceSize size;
    VkDeviceSize allocated_size;
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags memory;

    void* mmap;
};



struct DvzBufferRegions
{
    DvzBuffer* buffer;
    uint32_t count;
    VkDeviceSize size;
    VkDeviceSize aligned_size; // NOTE: is non-null only for aligned arrays
    VkDeviceSize alignment;
    VkDeviceSize offsets[DVZ_MAX_BUFFER_REGIONS_PER_SET];
};



struct DvzImages
{
    DvzObject obj;
    DvzGpu* gpu;

    uint32_t count;
    bool is_swapchain;

    // Queues that need access to the buffer.
    uint32_t queue_count;
    uint32_t queues[DVZ_MAX_QUEUES];

    VkImageType image_type;
    VkImageViewType view_type;
    uint32_t width, height, depth;
    VkFormat format;
    VkImageLayout layout;
    VkImageTiling tiling;
    VkImageUsageFlags usage;
    VkMemoryPropertyFlags memory;
    VkImageAspectFlags aspect;

    VkImage images[DVZ_MAX_IMAGES_PER_SET];
    VkDeviceMemory memories[DVZ_MAX_IMAGES_PER_SET];
    VkImageView image_views[DVZ_MAX_IMAGES_PER_SET];
};



struct DvzSampler
{
    DvzObject obj;
    DvzGpu* gpu;

    VkFilter min_filter;
    VkFilter mag_filter;
    VkSamplerAddressMode address_modes[3];
    VkSampler sampler;
};



struct DvzSlots
{
    DvzObject obj;
    DvzGpu* gpu;

    uint32_t slot_count;
    VkDescriptorType types[DVZ_MAX_BINDINGS_SIZE];

    uint32_t push_count;
    VkDeviceSize push_offsets[DVZ_MAX_PUSH_CONSTANTS];
    VkDeviceSize push_sizes[DVZ_MAX_PUSH_CONSTANTS];
    VkShaderStageFlags push_shaders[DVZ_MAX_PUSH_CONSTANTS];

    VkPipelineLayout pipeline_layout;
    VkDescriptorSetLayout dset_layout;
};



struct DvzBindings
{
    DvzObject obj;
    DvzGpu* gpu;

    DvzSlots* slots;

    // a Bindings struct holds multiple almost-identical copies of descriptor sets
    // with the same layout, but possibly with the different idx in the DvzBuffer
    uint32_t dset_count;
    VkDescriptorSet dsets[DVZ_MAX_SWAPCHAIN_IMAGES];

    DvzBufferRegions br[DVZ_MAX_BINDINGS_SIZE];
    DvzImages* images[DVZ_MAX_BINDINGS_SIZE];
    DvzSampler* samplers[DVZ_MAX_BINDINGS_SIZE];
};



struct DvzCompute
{
    DvzObject obj;
    DvzGpu* gpu;
    DvzContext* context;

    char shader_path[1024];
    const char* shader_code;

    VkPipeline pipeline;
    DvzSlots slots;
    DvzBindings* bindings;
    VkShaderModule shader_module;
};



struct DvzVertexBinding
{
    uint32_t binding;
    VkDeviceSize stride;
};



struct DvzVertexAttr
{
    uint32_t binding;
    uint32_t location;
    VkFormat format;
    VkDeviceSize offset;
};



struct DvzGraphics
{
    DvzObject obj;
    DvzGpu* gpu;

    DvzGraphicsType type;
    int flags;

    DvzRenderpass* renderpass;
    uint32_t subpass;

    VkPrimitiveTopology topology;
    DvzBlendType blend_type;
    DvzDepthTest depth_test;
    VkPolygonMode polygon_mode;
    VkCullModeFlags cull_mode;
    VkFrontFace front_face;

    VkPipeline pipeline;
    DvzSlots slots;

    uint32_t vertex_binding_count;
    DvzVertexBinding vertex_bindings[DVZ_MAX_VERTEX_BINDINGS];

    uint32_t vertex_attr_count;
    DvzVertexAttr vertex_attrs[DVZ_MAX_VERTEX_ATTRS];

    uint32_t shader_count;
    VkShaderStageFlagBits shader_stages[DVZ_MAX_SHADERS_PER_GRAPHICS];
    VkShaderModule shader_modules[DVZ_MAX_SHADERS_PER_GRAPHICS];

    DvzGraphicsCallback callback;
};



struct DvzBarrierBuffer
{
    DvzBufferRegions br;
    bool queue_transfer;

    VkAccessFlags src_access;
    uint32_t src_queue;

    VkAccessFlags dst_access;
    uint32_t dst_queue;
};



struct DvzBarrierImage
{
    DvzImages* images;
    bool queue_transfer;

    VkAccessFlags src_access;
    uint32_t src_queue;
    VkImageLayout src_layout;

    VkAccessFlags dst_access;
    uint32_t dst_queue;
    VkImageLayout dst_layout;
};



struct DvzBarrier
{
    DvzObject obj;
    DvzGpu* gpu;

    // uint32_t idx; // index within the buffer regions or images

    VkPipelineStageFlagBits src_stage;
    VkPipelineStageFlagBits dst_stage;

    uint32_t buffer_barrier_count;
    DvzBarrierBuffer buffer_barriers[DVZ_MAX_BARRIERS_PER_SET];

    uint32_t image_barrier_count;
    DvzBarrierImage image_barriers[DVZ_MAX_BARRIERS_PER_SET];
};



struct DvzFences
{
    DvzObject obj;
    DvzGpu* gpu;

    uint32_t count;
    VkFence fences[DVZ_MAX_FENCES_PER_SET];
};



struct DvzSemaphores
{
    DvzObject obj;
    DvzGpu* gpu;

    uint32_t count;
    VkSemaphore semaphores[DVZ_MAX_SEMAPHORES_PER_SET];
};



struct DvzRenderpassAttachment
{
    VkImageLayout ref_layout;
    DvzRenderpassAttachmentType type;
    VkFormat format;

    VkImageLayout src_layout;
    VkImageLayout dst_layout;

    VkAttachmentLoadOp load_op;
    VkAttachmentStoreOp store_op;
};



struct DvzRenderpassSubpass
{
    uint32_t attachment_count;
    uint32_t attachments[DVZ_MAX_ATTACHMENTS_PER_RENDERPASS];
};



struct DvzRenderpassDependency
{
    uint32_t src_subpass;
    VkPipelineStageFlags src_stage;
    VkAccessFlags src_access;

    uint32_t dst_subpass;
    VkPipelineStageFlags dst_stage;
    VkAccessFlags dst_access;
};



struct DvzFramebuffers
{
    DvzObject obj;
    DvzGpu* gpu;
    DvzRenderpass* renderpass;

    uint32_t attachment_count;
    // by definition, the framebuffers size = the first attachment's size
    DvzImages* attachments[DVZ_MAX_ATTACHMENTS_PER_RENDERPASS];

    uint32_t framebuffer_count;
    VkFramebuffer framebuffers[DVZ_MAX_SWAPCHAIN_IMAGES];
};



struct DvzRenderpass
{
    DvzObject obj;
    DvzGpu* gpu;

    uint32_t attachment_count;
    DvzRenderpassAttachment attachments[DVZ_MAX_ATTACHMENTS_PER_RENDERPASS];

    uint32_t clear_count;
    VkClearValue clear_values[DVZ_MAX_ATTACHMENTS_PER_RENDERPASS];

    uint32_t subpass_count;
    DvzRenderpassSubpass subpasses[DVZ_MAX_SUBPASSES_PER_RENDERPASS];

    uint32_t dependency_count;
    DvzRenderpassDependency dependencies[DVZ_MAX_DEPENDENCIES_PER_RENDERPASS];

    VkRenderPass renderpass;
};



struct DvzSubmit
{
    DvzObject obj;
    DvzGpu* gpu;

    uint32_t commands_count;
    DvzCommands* commands[DVZ_MAX_COMMANDS_PER_SUBMIT];

    uint32_t wait_semaphores_count;
    uint32_t wait_semaphores_idx[DVZ_MAX_SEMAPHORES_PER_SUBMIT];
    DvzSemaphores* wait_semaphores[DVZ_MAX_SEMAPHORES_PER_SUBMIT];
    VkPipelineStageFlags wait_stages[DVZ_MAX_SEMAPHORES_PER_SUBMIT];

    uint32_t signal_semaphores_count;
    uint32_t signal_semaphores_idx[DVZ_MAX_SEMAPHORES_PER_SUBMIT];
    DvzSemaphores* signal_semaphores[DVZ_MAX_SEMAPHORES_PER_SUBMIT];
};



struct DvzTexture
{
    DvzObject obj;

    DvzContext* context;

    DvzImages* image;
    DvzSampler* sampler;
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
DVZ_EXPORT DvzApp* dvz_app(DvzBackend backend);

/**
 * Destroy the application.
 *
 * This function automatically destroys all objects created within the application.
 *
 * @param app the application to destroy
 */
DVZ_EXPORT int dvz_app_destroy(DvzApp* app);



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

DVZ_EXPORT DvzGpu* dvz_gpu(DvzApp* app, uint32_t idx);

/**
 * Request some features before creating the GPU instance.
 *
 * This function needs to be called before creating the GPU with ` dvz_gpu_create()`.
 *
 * @param gpu the GPU
 * @param requested_features the list of requested features
 */
DVZ_EXPORT void dvz_gpu_request_features(DvzGpu* gpu, VkPhysicalDeviceFeatures requested_features);

/**
 * Request a new Vulkan queue before creating the GPU.
 *
 * @param gpu the GPU
 * @param idx the queue index (should be regularly increasing: 0, 1, 2...)
 * @param type the queue type
 */
DVZ_EXPORT void dvz_gpu_queue(DvzGpu* gpu, uint32_t idx, DvzQueueType type);

/**
 * Create a GPU once the features and queues have been set up.
 *
 * @param gpu the GPU
 * @param surface the surface on which the GPU will need to render
 */
DVZ_EXPORT void dvz_gpu_create(DvzGpu* gpu, VkSurfaceKHR surface);

/**
 * Wait for a queue to be idle.
 *
 * This is one of the different GPU synchronization methods. It is not efficient as it waits until
 * the queue is idle.
 *
 * @param gpu the GPU
 * @param queue_idx the queue index
 */
DVZ_EXPORT void dvz_queue_wait(DvzGpu* gpu, uint32_t queue_idx);

/**
 * Full synchronization on all GPUs.
 *
 * This function waits on all queues of all GPUs. The strongest, least efficient of the
 * synchronization methods.
 *
 * @param app the application instance
 */
DVZ_EXPORT void dvz_app_wait(DvzApp* app);

/**
 * Full synchronization on a given GPU.
 *
 * This function waits on all queues of a given GPU.
 *
 * @param gpu the GPU
 */
DVZ_EXPORT void dvz_gpu_wait(DvzGpu* gpu);

/**
 * Destroy the resources associated to a GPU.
 *
 * @param gpu the GPU
 */
DVZ_EXPORT void dvz_gpu_destroy(DvzGpu* gpu);



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
DVZ_EXPORT DvzWindow* dvz_window(DvzApp* app, uint32_t width, uint32_t height);

/**
 * Get the window size, in pixels.
 *
 * @param window the window
 * @param[out] framebuffer_width the width, in pixels
 * @param[out] framebuffer_height the height, in pixels
 */
DVZ_EXPORT void
dvz_window_get_size(DvzWindow* window, uint32_t* framebuffer_width, uint32_t* framebuffer_height);

/**
 * Process the pending windowing events by the backend (glfw by default).
 *
 * @param window the window
 */
DVZ_EXPORT void dvz_window_poll_events(DvzWindow* window);

/**
 * Destroy a window.
 *
 * !!! warning
 *     This function must be imperatively called *after* `dvz_swapchain_destroy()`.
 *
 * @param window the window
 */
DVZ_EXPORT void dvz_window_destroy(DvzWindow* window);

/**
 * Destroy a canvas.
 *
 * @param canvas the canvas
 */
DVZ_EXPORT void dvz_canvas_destroy(DvzCanvas* canvas);

/**
 * Destroy all canvases.
 *
 * @param canvases the container with the canvases.
 */
DVZ_EXPORT void dvz_canvases_destroy(DvzContainer* canvases);



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
DVZ_EXPORT DvzSwapchain dvz_swapchain(DvzGpu* gpu, DvzWindow* window, uint32_t min_img_count);

/**
 * Set the swapchain image format.
 *
 * @param swapchain the swapchain
 * @param format the format
 */
DVZ_EXPORT void dvz_swapchain_format(DvzSwapchain* swapchain, VkFormat format);

/**
 * Set the swapchain present mode.
 *
 * @param swapchain the swapchain
 * @param present_mode the present mode
 */
DVZ_EXPORT void dvz_swapchain_present_mode(DvzSwapchain* swapchain, VkPresentModeKHR present_mode);

/**
 * Set the swapchain requested image size.
 *
 * @param swapchain the swapchain
 * @param width the requested width
 * @param height the requested height
 */
DVZ_EXPORT void
dvz_swapchain_requested_size(DvzSwapchain* swapchain, uint32_t width, uint32_t height);

/**
 * Create the swapchain once it has been set up.
 *
 * @param swapchain the swapchain
 */
DVZ_EXPORT void dvz_swapchain_create(DvzSwapchain* swapchain);

/**
 * Recreate a swapchain (for example after a window resize).
 *
 * @param swapchain the swapchain
 */
DVZ_EXPORT void dvz_swapchain_recreate(DvzSwapchain* swapchain);

/**
 * Acquire a swapchain image.
 *
 * @param swapchain the swapchain
 * @param semaphores the set of signal semaphores
 * @param semaphore_idx the index of the semaphore to signal after image acquisition
 * @param fences the set of signal fences
 * @param fence_idx the index of the fence to signal after image acquisition
 */
DVZ_EXPORT void dvz_swapchain_acquire(
    DvzSwapchain* swapchain, DvzSemaphores* semaphores, uint32_t semaphore_idx, DvzFences* fences,
    uint32_t fence_idx);

/**
 * Present a swapchain image to the screen after it has been rendered.
 *
 * @param swapchain the swapchain
 * @param queue_idx the index of the present queue
 * @param semaphores the set of waiting semaphores
 * @param semaphore_idx the index of the semaphore to wait on before presentation
 */
DVZ_EXPORT void dvz_swapchain_present(
    DvzSwapchain* swapchain, uint32_t queue_idx, DvzSemaphores* semaphores,
    uint32_t semaphore_idx);

/**
 * Destroy a swapchain
 *
 * !!! warning
 *     This function must imperatively be called *before* `dvz_window_destroy()`.
 *
 * @param swapchain the swapchain
 */
DVZ_EXPORT void dvz_swapchain_destroy(DvzSwapchain* swapchain);



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
DVZ_EXPORT DvzCommands dvz_commands(DvzGpu* gpu, uint32_t queue, uint32_t count);

/**
 * Start recording a command buffer.
 *
 * @param cmds the set of command buffers
 * @param idx the index of the command buffer to begin recording on
 */
DVZ_EXPORT void dvz_cmd_begin(DvzCommands* cmds, uint32_t idx);

/**
 * Stop recording a command buffer.
 *
 * @param cmds the set of command buffers
 * @param idx the index of the command buffer to stop the recording on
 */
DVZ_EXPORT void dvz_cmd_end(DvzCommands* cmds, uint32_t idx);

/**
 * Reset a command buffer.
 *
 * @param cmds the set of command buffers
 * @param idx the index of the command buffer to reset
 */
DVZ_EXPORT void dvz_cmd_reset(DvzCommands* cmds, uint32_t idx);

/**
 * Free a set of command buffers.
 *
 * @param cmds the set of command buffers
 */
DVZ_EXPORT void dvz_cmd_free(DvzCommands* cmds);

/**
 * Submit a command buffer on its queue with inefficient full synchronization.
 *
 * This function is relatively inefficient because it calls `dvz_queue_wait()`.
 *
 * @param cmds the set of command buffers
 * @param idx the index of the command buffer to submit
 */
DVZ_EXPORT void dvz_cmd_submit_sync(DvzCommands* cmds, uint32_t idx);

/**
 * Destroy a set of command buffers.
 *
 * @param cmds the set of command buffers
 */
DVZ_EXPORT void dvz_commands_destroy(DvzCommands* cmds);



/*************************************************************************************************/
/*  Buffers                                                                                      */
/*************************************************************************************************/

/**
 * Initialize a GPU buffer.
 *
 * @param gpu the GPU
 * @returns the buffer
 */
DVZ_EXPORT DvzBuffer dvz_buffer(DvzGpu* gpu);

/**
 * Set the buffer size.
 *
 * @param buffer the buffer
 * @param size the buffer size, in bytes
 */
DVZ_EXPORT void dvz_buffer_size(DvzBuffer* buffer, VkDeviceSize size);

/**
 * Set the buffer type.
 *
 * @param buffer the buffer
 * @param type the buffer type
 */
DVZ_EXPORT void dvz_buffer_type(DvzBuffer* buffer, DvzBufferType type);

/**
 * Set the buffer usage.
 *
 * @param buffer the buffer
 * @param usage the buffer usage
 */
DVZ_EXPORT void dvz_buffer_usage(DvzBuffer* buffer, VkBufferUsageFlags usage);

/**
 * Set the buffer memory properties.
 *
 * @param buffer the buffer
 * @param memory the memory properties
 */
DVZ_EXPORT void dvz_buffer_memory(DvzBuffer* buffer, VkMemoryPropertyFlags memory);

/**
 * Set the buffer queue access.
 *
 * @param buffer the buffer
 * @param queue_idx the queue index
 */
DVZ_EXPORT void dvz_buffer_queue_access(DvzBuffer* buffer, uint32_t queue_idx);

/**
 * Create the buffer after it has been set.
 *
 * @param buffer the buffer
 */
DVZ_EXPORT void dvz_buffer_create(DvzBuffer* buffer);

/**
 * Resize a buffer.
 *
 * @param buffer the buffer
 * @param size the new buffer size, in bytes
 * @param cmds the command buffers to use for the GPU-GPU data copy transfer
 */
DVZ_EXPORT void dvz_buffer_resize(DvzBuffer* buffer, VkDeviceSize size, DvzCommands* cmds);

/**
 * Memory-map a buffer.
 *
 * @param buffer the buffer
 * @param offset the offset within the buffer, in bytes
 * @param size the size to map, in bytes
 */
DVZ_EXPORT void* dvz_buffer_map(DvzBuffer* buffer, VkDeviceSize offset, VkDeviceSize size);

/**
 * Unmap a buffer.
 *
 * @param buffer the buffer
 */
DVZ_EXPORT void dvz_buffer_unmap(DvzBuffer* buffer);

/**
 * Download a buffer data to the CPU.
 *
 * @param buffer the buffer
 * @param offset the offset within the buffer, in bytes
 * @param size the size of the region to download, in bytes
 * @param[out] data the buffer to download on (must be allocated with the appropriate size)
 */
DVZ_EXPORT void
dvz_buffer_download(DvzBuffer* buffer, VkDeviceSize offset, VkDeviceSize size, void* data);

/**
 * Upload data to a GPU buffer.
 *
 * @param buffer the buffer
 * @param offset the offset within the buffer, in bytes
 * @param size the buffer size, in bytes
 * @param data the data to upload
 */
DVZ_EXPORT void
dvz_buffer_upload(DvzBuffer* buffer, VkDeviceSize offset, VkDeviceSize size, const void* data);

/**
 * Destroy a buffer
 *
 * @param buffer the buffer
 */
DVZ_EXPORT void dvz_buffer_destroy(DvzBuffer* buffer);



/**
 * Create buffer regions on an existing GPU buffer.
 *
 * @param buffer the buffer
 * @param count the number of successive regions
 * @param offset the offset within the buffer
 * @param size the size of each region, in bytes
 * @param alignment the alignment requirement for the region offsets
 */
DVZ_EXPORT DvzBufferRegions dvz_buffer_regions(
    DvzBuffer* buffer, uint32_t count, //
    VkDeviceSize offset, VkDeviceSize size, VkDeviceSize alignment);

/**
 * Map a buffer region.
 *
 * @param br the buffer regions
 * @param idx the index of the buffer region to map
 */
DVZ_EXPORT void* dvz_buffer_regions_map(DvzBufferRegions* br, uint32_t idx);

/**
 * Unmap a set of buffer regions.
 *
 * @param br the buffer regions
 */
DVZ_EXPORT void dvz_buffer_regions_unmap(DvzBufferRegions* br);

/**
 * Upload data to a buffer region.
 *
 * @param br the set of buffer regions
 * @param idx the index of the buffer region to upload data to
 * @param data the data to upload
 */
DVZ_EXPORT void dvz_buffer_regions_upload(DvzBufferRegions* br, uint32_t idx, const void* data);



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
DVZ_EXPORT DvzImages dvz_images(DvzGpu* gpu, VkImageType type, uint32_t count);

/**
 * Set the images format.
 *
 * @param images the images
 * @param format the image format
 */
DVZ_EXPORT void dvz_images_format(DvzImages* images, VkFormat format);

/**
 * Set the images layout.
 *
 * @param images the images
 * @param layout the image layout
 */
DVZ_EXPORT void dvz_images_layout(DvzImages* images, VkImageLayout layout);

/**
 * Set the images size.
 *
 * @param images the images
 * @param width the image width
 * @param height the image height
 * @param depth the image depth
 */
DVZ_EXPORT void
dvz_images_size(DvzImages* images, uint32_t width, uint32_t height, uint32_t depth);

/**
 * Set the images tiling.
 *
 * @param images the images
 * @param tiling the image tiling
 */
DVZ_EXPORT void dvz_images_tiling(DvzImages* images, VkImageTiling tiling);

/**
 * Set the images usage.
 *
 * @param images the images
 * @param usage the image usage
 */
DVZ_EXPORT void dvz_images_usage(DvzImages* images, VkImageUsageFlags usage);

/**
 * Set the images memory properties.
 *
 * @param images the images
 * @param memory the memory properties
 */
DVZ_EXPORT void dvz_images_memory(DvzImages* images, VkMemoryPropertyFlags memory);

/**
 * Set the images aspect.
 *
 * @param images the images
 * @param aspect the image aspect
 */
DVZ_EXPORT void dvz_images_aspect(DvzImages* images, VkImageAspectFlags aspect);

/**
 * Set the images queue access.
 *
 * This parameter specifies which queues may access the image from command buffers submitted to
 * them.
 *
 * @param images the images
 * @param queue_idx the queue index
 */
DVZ_EXPORT void dvz_images_queue_access(DvzImages* images, uint32_t queue_idx);

/**
 * Create the images after they have been set up.
 *
 * @param images the images
 */
DVZ_EXPORT void dvz_images_create(DvzImages* images);

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
DVZ_EXPORT void
dvz_images_resize(DvzImages* images, uint32_t width, uint32_t height, uint32_t depth);

/**
 * Transition the images to their layout after creation.
 *
 * This function performs a hard synchronization on the queue and submits a command buffer with the
 * image transition.
 *
 * @param images the images
 */
DVZ_EXPORT void dvz_images_transition(DvzImages* images);

/**
 * Download the data from a staging GPU image.
 *
 * @param staging the images to download the data from
 * @param idx the index of the image
 * @param swizzle whether the RGB(A) values need to be transposed
 * @param has_alpha whether there is an Alpha component in the output buffer
 * @param[out] out the buffer that will be filled with the image data (must be already allocated)
 */
DVZ_EXPORT void
dvz_images_download(DvzImages* staging, uint32_t idx, bool swizzle, bool has_alpha, uint8_t* out);

/**
 * Destroy images.
 *
 * @param images the images
 */
DVZ_EXPORT void dvz_images_destroy(DvzImages* images);



/*************************************************************************************************/
/*  Sampler                                                                                      */
/*************************************************************************************************/

/**
 * Initialize a texture sampler.
 *
 * @param gpu the GPU
 * @returns the sampler object
 */
DVZ_EXPORT DvzSampler dvz_sampler(DvzGpu* gpu);

/**
 * Set the sampler min filter.
 *
 * @param sampler the sampler
 * @param filter the filter
 */
DVZ_EXPORT void dvz_sampler_min_filter(DvzSampler* sampler, VkFilter filter);

/**
 * Set the sampler mag filter.
 *
 * @param sampler the sampler
 * @param filter the filter
 */
DVZ_EXPORT void dvz_sampler_mag_filter(DvzSampler* sampler, VkFilter filter);

/**
 * Set the sampler address mode
 *
 * @param sampler the sampler
 * @param axis the sampler axis
 * @param address_mode the address mode
 */
DVZ_EXPORT void dvz_sampler_address_mode(
    DvzSampler* sampler, DvzTextureAxis axis, VkSamplerAddressMode address_mode);

/**
 * Create the sampler after it has been set up.
 *
 * @param sampler the sampler
 */
DVZ_EXPORT void dvz_sampler_create(DvzSampler* sampler);

/**
 * Destroy a sampler
 *
 * @param sampler the sampler
 */
DVZ_EXPORT void dvz_sampler_destroy(DvzSampler* sampler);



/*************************************************************************************************/
/*  Slots                                                                                        */
/*************************************************************************************************/

/**
 * Initialize pipeline slots (aka Vulkan descriptor set layout).
 *
 * @param gpu the GPU
 * @returns the slots
 */
DVZ_EXPORT DvzSlots dvz_slots(DvzGpu* gpu);

/**
 * Set the slots binding.
 *
 * @param slots the slots
 * @param idx the slot index to set up
 * @param type the descriptor type for that slot
 */
DVZ_EXPORT void dvz_slots_binding(DvzSlots* slots, uint32_t idx, VkDescriptorType type);

/**
 * Set up push constants.
 *
 * @param slots the slots
 * @param offset the push constant offset, in bytes
 * @param size the push constant size, in bytes
 * @param shaders the shader stages that will access the push constant
 */
DVZ_EXPORT void dvz_slots_push(
    DvzSlots* slots, VkDeviceSize offset, VkDeviceSize size, VkShaderStageFlags shaders);

/**
 * Create the slots after they have been set up.
 *
 * @param slots the slots
 */
DVZ_EXPORT void dvz_slots_create(DvzSlots* slots);

/**
 * Destroy the slots
 *
 * @param slots the slots
 */
DVZ_EXPORT void dvz_slots_destroy(DvzSlots* slots);



/*************************************************************************************************/
/*  Bindings                                                                                     */
/*************************************************************************************************/

/**
 * Initialize bindings corresponding to slots.
 *
 * @param slots the slots
 * @param dset_count the number of descriptor sets (number of swapchain images)
 */
DVZ_EXPORT DvzBindings dvz_bindings(DvzSlots* slots, uint32_t dset_count);

/**
 * Bind a buffer to a slot.
 *
 * @param bindings the bindings
 * @param idx the slot index
 * @param br the buffer regions to bind to that slot
 */
DVZ_EXPORT void dvz_bindings_buffer(DvzBindings* bindings, uint32_t idx, DvzBufferRegions br);

/**
 * Bind a texture to a slot.
 *
 * @param bindings the bindings
 * @param idx the slot index
 * @param br the texture to bind to that slot
 */
DVZ_EXPORT void dvz_bindings_texture(DvzBindings* bindings, uint32_t idx, DvzTexture* texture);

/**
 * Update the bindings after the buffers/textures have been set up.
 *
 * @param bindings the bindings
 */
DVZ_EXPORT void dvz_bindings_update(DvzBindings* bindings);

/**
 * Destroy bindings.
 *
 * @param bindings the bindings
 */
DVZ_EXPORT void dvz_bindings_destroy(DvzBindings* bindings);



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
DVZ_EXPORT DvzCompute dvz_compute(DvzGpu* gpu, const char* shader_path);

/**
 * Create a compute pipeline after it has been set up.
 *
 * @param compute the compute pipeline
 */
DVZ_EXPORT void dvz_compute_create(DvzCompute* compute);

/**
 * Set the GLSL code directly (the library will compile it automatically to SPIRV).
 *
 * @param compute the compute pipeline
 * @param code the GLSL code defining the compute shader
 */
DVZ_EXPORT void dvz_compute_code(DvzCompute* compute, const char* code);

/**
 * Declare a slot for the compute pipeline.
 *
 * @param compute the compute pipeline
 * @param idx the slot index
 * @param type the descriptor type
 */
DVZ_EXPORT void dvz_compute_slot(DvzCompute* compute, uint32_t idx, VkDescriptorType type);

/**
 * Set up push constant.
 *
 * @param compute the compute pipeline
 * @param offset the push constant offset, in bytes
 * @param size the push constant size, in bytes
 * @param shaders the shaders that will need to access the push constant
 */
DVZ_EXPORT void dvz_compute_push(
    DvzCompute* compute, VkDeviceSize offset, VkDeviceSize size, VkShaderStageFlags shaders);

/**
 * Associate a bindings object to a compute pipeline.
 *
 * @param compute the compute pipeline
 * @param bindings the bindings
 */
DVZ_EXPORT void dvz_compute_bindings(DvzCompute* compute, DvzBindings* bindings);

/**
 * Destroy a compute pipeline.
 *
 * @param compute the compute pipeline
 */
DVZ_EXPORT void dvz_compute_destroy(DvzCompute* compute);



/*************************************************************************************************/
/*  Pipeline                                                                                     */
/*************************************************************************************************/

/**
 * Initialize a graphics pipeline.
 *
 * @param gpu the GPU
 * @returns the graphics pipeline
 */
DVZ_EXPORT DvzGraphics dvz_graphics(DvzGpu* gpu);

/**
 * Set the renderpass of a graphics pipeline.
 *
 * @param graphics the graphics pipeline
 * @param renderpass the render pass
 * @param subpass the subpass index
 */
DVZ_EXPORT void
dvz_graphics_renderpass(DvzGraphics* graphics, DvzRenderpass* renderpass, uint32_t subpass);

/**
 * Set the graphics pipeline primitive topology
 *
 * @param graphics the graphics pipeline
 * @param topology the primitive topology
 */
DVZ_EXPORT void dvz_graphics_topology(DvzGraphics* graphics, VkPrimitiveTopology topology);

/**
 * Set the GLSL code of a graphics pipeline.
 *
 * @param graphics the graphics pipeline
 * @param stage the shader stage
 * @param code the GLSL code of the shader
 */
DVZ_EXPORT void
dvz_graphics_shader_glsl(DvzGraphics* graphics, VkShaderStageFlagBits stage, const char* code);

/**
 * Set the SPIRV code of a graphics pipeline.
 *
 * @param graphics the graphics pipeline
 * @param stage the shader stage
 * @param size the size of the SPIRV buffer, in bytes
 * @param buffer the binary buffer with the SPIRV code
 */
DVZ_EXPORT void dvz_graphics_shader_spirv(
    DvzGraphics* graphics, VkShaderStageFlagBits stage, VkDeviceSize size, const uint32_t* buffer);

/**
 * Set the path to a shader for a graphics pipeline.
 *
 * @param graphics the graphics pipeline
 * @param stage the shader stage
 * @param shader_path the path to the `.spirv` shader file
 */
DVZ_EXPORT void
dvz_graphics_shader(DvzGraphics* graphics, VkShaderStageFlagBits stage, const char* shader_path);

/**
 * Set the vertex binding.
 *
 * @param graphics the graphics pipeline
 * @param binding the binding index
 * @param stride the stride in the vertex buffer, in bytes
 */
DVZ_EXPORT void
dvz_graphics_vertex_binding(DvzGraphics* graphics, uint32_t binding, VkDeviceSize stride);

/**
 * Add a vertex attribute.
 *
 * @param graphics the graphics pipeline
 * @param binding the binding index (as specified in the vertex shader)
 * @param location the location index (as specified in the vertex shader)
 * @param format the format
 * @param offset the offset, in bytes
 */
DVZ_EXPORT void dvz_graphics_vertex_attr(
    DvzGraphics* graphics, uint32_t binding, uint32_t location, VkFormat format,
    VkDeviceSize offset);

/**
 * Set the graphics blend type.
 *
 * @param graphics the graphics pipeline
 * @param blend_type the blend type
 */
DVZ_EXPORT void dvz_graphics_blend(DvzGraphics* graphics, DvzBlendType blend_type);

/**
 * Set the graphics depth test.
 *
 * @param graphics the graphics pipeline
 * @param depth_test the depth test
 */
DVZ_EXPORT void dvz_graphics_depth_test(DvzGraphics* graphics, DvzDepthTest depth_test);

/**
 * Set the graphics polygon mode.
 *
 * @param graphics the graphics pipeline
 * @param polygon_mode the polygon mode
 */
DVZ_EXPORT void dvz_graphics_polygon_mode(DvzGraphics* graphics, VkPolygonMode polygon_mode);

/**
 * Set the graphics cull mode.
 *
 * @param graphics the graphics pipeline
 * @param cull_mode the cull mode
 */
DVZ_EXPORT void dvz_graphics_cull_mode(DvzGraphics* graphics, VkCullModeFlags cull_mode);

/**
 * Set the graphics front face.
 *
 * @param graphics the graphics pipeline
 * @param front_face the front face
 */
DVZ_EXPORT void dvz_graphics_front_face(DvzGraphics* graphics, VkFrontFace front_face);

/**
 * Create a graphics pipeline after it has been set up.
 *
 * @param graphics the graphics pipeline
 */
DVZ_EXPORT void dvz_graphics_create(DvzGraphics* graphics);

/**
 * Set a binding slot for a graphics pipeline.
 *
 * @param graphics the graphics pipeline
 * @param idx the slot index
 * @param type the descriptor type
 */
DVZ_EXPORT void dvz_graphics_slot(DvzGraphics* graphics, uint32_t idx, VkDescriptorType type);

/**
 * Set a graphics pipeline push constant.
 *
 * @param graphics the graphics pipeline
 * @param offset the push constant offset, in bytes
 * @param offset the push size, in bytes
 * @param shaders the shader stages that will access the push constant
 */
DVZ_EXPORT void dvz_graphics_push(
    DvzGraphics* graphics, VkDeviceSize offset, VkDeviceSize size, VkShaderStageFlags shaders);

/**
 * Destroy a graphics pipeline.
 *
 * @param graphics the graphics pipeline
 */
DVZ_EXPORT void dvz_graphics_destroy(DvzGraphics* graphics);



/*************************************************************************************************/
/*  Barrier                                                                                      */
/*************************************************************************************************/

/**
 * Initialize a synchronization barrier (usedwithin a command buffer).
 *
 * @param gpu the GPU
 * @returns the barrier
 */
DVZ_EXPORT DvzBarrier dvz_barrier(DvzGpu* gpu);

/**
 * Set the barrier stages.
 *
 * @param barrier the barrier
 * @param src_stage the source stage
 * @param dst_stage the destination stage
 */
DVZ_EXPORT void dvz_barrier_stages(
    DvzBarrier* barrier, VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage);

/**
 * Set the barrier buffer.
 *
 * @param barrier the barrier
 * @param br the buffer regions
 */
DVZ_EXPORT void dvz_barrier_buffer(DvzBarrier* barrier, DvzBufferRegions br);

/**
 * Set the barrier buffer queue.
 *
 * @param barrier the barrier
 * @param src_queue the source queue index
 * @param dst_queue the destination queue index
 */
DVZ_EXPORT void
dvz_barrier_buffer_queue(DvzBarrier* barrier, uint32_t src_queue, uint32_t dst_queue);

/**
 * Set the barrier buffer access.
 *
 * @param barrier the barrier
 * @param src_access the source access flags
 * @param dst_access the destination access flags
 */
DVZ_EXPORT void
dvz_barrier_buffer_access(DvzBarrier* barrier, VkAccessFlags src_access, VkAccessFlags dst_access);

/**
 * Set the barrier images.
 *
 * @param barrier the barrier
 * @param images the images
 */
DVZ_EXPORT void dvz_barrier_images(DvzBarrier* barrier, DvzImages* images);

/**
 * Set the barrier images layout.
 *
 * @param barrier the barrier
 * @param src_layout the source layout
 * @param dst_layout the destination layout
 */
DVZ_EXPORT void
dvz_barrier_images_layout(DvzBarrier* barrier, VkImageLayout src_layout, VkImageLayout dst_layout);

/**
 * Set the barrier images queue.
 *
 * @param barrier the barrier
 * @param src_queue the source queue index
 * @param dst_queue the destination queue index
 */
DVZ_EXPORT void
dvz_barrier_images_queue(DvzBarrier* barrier, uint32_t src_queue, uint32_t dst_queue);

/**
 * Set the barrier images access.
 *
 * @param barrier the barrier
 * @param src_access the source access flags
 * @param dst_access the destination access flags
 */
DVZ_EXPORT void
dvz_barrier_images_access(DvzBarrier* barrier, VkAccessFlags src_access, VkAccessFlags dst_access);



/*************************************************************************************************/
/*  Semaphores                                                                                   */
/*************************************************************************************************/

/**
 * Initialize a set of semaphores (GPU-GPU synchronization).
 *
 * @param gpu the GPU
 * @param count the number of semaphores
 * @returns the semaphores
 */
DVZ_EXPORT DvzSemaphores dvz_semaphores(DvzGpu* gpu, uint32_t count);

/**
 * Destroy semaphores.
 *
 * @param semaphores the semaphores
 */
DVZ_EXPORT void dvz_semaphores_destroy(DvzSemaphores* semaphores);



/*************************************************************************************************/
/*  Fences                                                                                       */
/*************************************************************************************************/

/**
 * Initialize a set of fences (CPU-GPU synchronization).
 *
 * @param gpu the GPU
 * @param count the number of fences
 * @param signaled whether the fences are created in the signaled state or not
 * @returns the fences
 */
DVZ_EXPORT DvzFences dvz_fences(DvzGpu* gpu, uint32_t count, bool signaled);

/**
 * Copy a fence from a set of fences to another.
 *
 * @param src_fences the source fences
 * @param src_idx the fence index within the source fences
 * @param dst_fences the destination fences
 * @param dst_idx the fence index within the destination fences
 */
DVZ_EXPORT void
dvz_fences_copy(DvzFences* src_fences, uint32_t src_idx, DvzFences* dst_fences, uint32_t dst_idx);

/**
 * Wait on the GPU until a fence is signaled.
 *
 * @param fences the fences
 * @param idx the fence index
 */
DVZ_EXPORT void dvz_fences_wait(DvzFences* fences, uint32_t idx);

/**
 * Return whether a fence is ready.
 *
 * @param fences the fences
 * @param idx the fence index
 */
DVZ_EXPORT bool dvz_fences_ready(DvzFences* fences, uint32_t idx);

/**
 * Rset the state of a fence.
 *
 * @param fences the fences
 * @param idx the fence index
 */
DVZ_EXPORT void dvz_fences_reset(DvzFences* fences, uint32_t idx);

/**
 * Destroy fences.
 *
 * @param fences the fences
 */
DVZ_EXPORT void dvz_fences_destroy(DvzFences* fences);



/*************************************************************************************************/
/*  Renderpass                                                                                   */
/*************************************************************************************************/

/**
 * Initialize a render pass.
 *
 * @param gpu the GPU
 * @returns the render pass
 */
DVZ_EXPORT DvzRenderpass dvz_renderpass(DvzGpu* gpu);

/**
 * Set the clear value of a render pass.
 *
 * @param renderpass the render pass
 * @param value the clear value
 */
DVZ_EXPORT void dvz_renderpass_clear(DvzRenderpass* renderpass, VkClearValue value);

/**
 * Specify a render pass attachment.
 *
 * @param renderpass the render pass
 * @param idx the attachment index
 * @param type the attachment type
 * @param format the attachment image format
 * @param ref_layout the image layout
 */
DVZ_EXPORT void dvz_renderpass_attachment(
    DvzRenderpass* renderpass, uint32_t idx, DvzRenderpassAttachmentType type, VkFormat format,
    VkImageLayout ref_layout);

/**
 * Set the attachment layout.
 *
 * @param renderpass the render pass
 * @param idx the attachment index
 * @param src_layout the source layout
 * @param dst_layout the destination layout
 */
DVZ_EXPORT void dvz_renderpass_attachment_layout(
    DvzRenderpass* renderpass, uint32_t idx, VkImageLayout src_layout, VkImageLayout dst_layout);

/**
 * Set the attachment load and store operations.
 *
 * @param renderpass the render pass
 * @param idx the attachment index
 * @param load_op the load operation
 * @param store_op the store operation
 */
DVZ_EXPORT void dvz_renderpass_attachment_ops(
    DvzRenderpass* renderpass, uint32_t idx, //
    VkAttachmentLoadOp load_op, VkAttachmentStoreOp store_op);

/**
 * Set a subpass attachment.
 *
 * @param renderpass the render pass
 * @param subpass_idx the subpass index
 * @param attachment_idx the attachment index
 */
DVZ_EXPORT void dvz_renderpass_subpass_attachment(
    DvzRenderpass* renderpass, uint32_t subpass_idx, uint32_t attachment_idx);

/**
 * Set a subpass dependency.
 *
 * @param renderpass the render pass
 * @param dependency_idx the dependency index
 * @param src_subpass the source subpass index
 * @param dst_subpass the destination subpass index
 */
DVZ_EXPORT void dvz_renderpass_subpass_dependency(
    DvzRenderpass* renderpass, uint32_t dependency_idx, //
    uint32_t src_subpass, uint32_t dst_subpass);

/**
 * Set a subpass dependency access.
 *
 * @param renderpass the render pass
 * @param dependency_idx the dependency index
 * @param src_access the source access flags
 * @param dst_access the destinationaccess flags
 */
DVZ_EXPORT void dvz_renderpass_subpass_dependency_access(
    DvzRenderpass* renderpass, uint32_t dependency_idx, //
    VkAccessFlags src_access, VkAccessFlags dst_access);

/**
 * Set a subpass dependency stage.
 *
 * @param renderpass the render pass
 * @param dependency_idx the dependency index
 * @param src_stage the source pipeline stages
 * @param dst_stage the destination pipeline stages
 */
DVZ_EXPORT void dvz_renderpass_subpass_dependency_stage(
    DvzRenderpass* renderpass, uint32_t dependency_idx, //
    VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage);

/**
 * Create a render pass after it has been set up.
 *
 * @param renderpass the render pass
 */
DVZ_EXPORT void dvz_renderpass_create(DvzRenderpass* renderpass);

/**
 * Destroy a render pass.
 *
 * @param renderpass the render pass
 */
DVZ_EXPORT void dvz_renderpass_destroy(DvzRenderpass* renderpass);



/*************************************************************************************************/
/*  Framebuffers                                                                                 */
/*************************************************************************************************/

/**
 * Initialize a set of framebuffers.
 *
 * @param gpu the GPU
 * @returns the framebuffers
 */
DVZ_EXPORT DvzFramebuffers dvz_framebuffers(DvzGpu* gpu);

/**
 * Set framebuffers attachment.
 *
 * @param framebuffers the framebuffers
 * @param attachment_idx the attachment index
 * @param images the images
 */

DVZ_EXPORT void dvz_framebuffers_attachment(
    DvzFramebuffers* framebuffers, uint32_t attachment_idx, DvzImages* images);

/**
 * Create a set of framebuffers after it has been set up.
 *
 * @param framebuffers the framebuffers
 * @param renderpass the render pass
 */
DVZ_EXPORT void dvz_framebuffers_create(DvzFramebuffers* framebuffers, DvzRenderpass* renderpass);

/**
 * Destroy a set of framebuffers.
 *
 * @param framebuffers the framebuffers
 */
DVZ_EXPORT void dvz_framebuffers_destroy(DvzFramebuffers* framebuffers);



/*************************************************************************************************/
/*  Submit                                                                                       */
/*************************************************************************************************/

/**
 * Create a submit object, used to submit command buffers to a GPU queue.
 *
 * @param gpu the GPU
 * @returns the submit
 */
DVZ_EXPORT DvzSubmit dvz_submit(DvzGpu* gpu);

/**
 * Set the command buffers to submit.
 *
 * @param submit the submit object
 * @param cmds the set of command buffers
 */
DVZ_EXPORT void dvz_submit_commands(DvzSubmit* submit, DvzCommands* commands);

/**
 * Set the wait semaphores
 *
 * @param submit the submit object
 * @param stage the pipeline stage
 * @param semaphores the set of semaphores to wait on
 * @param idx the semaphore index to wait on
 */
DVZ_EXPORT void dvz_submit_wait_semaphores(
    DvzSubmit* submit, VkPipelineStageFlags stage, DvzSemaphores* semaphores, uint32_t idx);

/**
 * Set the signal semaphores
 *
 * @param submit the submit object
 * @param semaphores the set of semaphores to signal after the commands have completed
 * @param idx the semaphore index to signal
 */
DVZ_EXPORT void
dvz_submit_signal_semaphores(DvzSubmit* submit, DvzSemaphores* semaphores, uint32_t idx);

/**
 * Submit the command buffers to their queue.
 *
 * @param submit the submit object
 * @param cmd_idx the command buffer index to submit
 * @param fences the fences to signal after completion
 * @param fence_idx the fence index to signal
 */
DVZ_EXPORT void
dvz_submit_send(DvzSubmit* submit, uint32_t cmd_idx, DvzFences* fences, uint32_t fence_idx);

/**
 * Reset a submit object.
 *
 * @param submit the submit object
 */
DVZ_EXPORT void dvz_submit_reset(DvzSubmit* submit);



/*************************************************************************************************/
/*  Command buffer filling                                                                       */
/*************************************************************************************************/

/**
 * Begin a render pass.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param renderpass the render pass
 * @param framebuffers the framebuffers
 */
DVZ_EXPORT void dvz_cmd_begin_renderpass(
    DvzCommands* cmds, uint32_t idx, DvzRenderpass* renderpass, DvzFramebuffers* framebuffers);

/**
 * End a render pass.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 */
DVZ_EXPORT void dvz_cmd_end_renderpass(DvzCommands* cmds, uint32_t idx);

/**
 * Launch a compute task.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param compute the computer pipeline
 * @param size the task shape
 */
DVZ_EXPORT void dvz_cmd_compute(DvzCommands* cmds, uint32_t idx, DvzCompute* compute, uvec3 size);

/**
 * Register a barrier.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param barrier the barrier
 */
DVZ_EXPORT void dvz_cmd_barrier(DvzCommands* cmds, uint32_t idx, DvzBarrier* barrier);

/**
 * Copy a GPU buffer to a GPU image.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param buffer the buffer
 * @param images the image
 */
DVZ_EXPORT void dvz_cmd_copy_buffer_to_image(
    DvzCommands* cmds, uint32_t idx, DvzBuffer* buffer, DvzImages* images);

/**
 * Copy a GPU image to a GPU buffer.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param images the image
 * @param buffer the buffer
 */
DVZ_EXPORT void dvz_cmd_copy_image_to_buffer(
    DvzCommands* cmds, uint32_t idx, DvzImages* images, DvzBuffer* buffer);

/**
 * Copy a GPU image to another.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param src_img the source image
 * @param dst_img the destination image
 */
DVZ_EXPORT void
dvz_cmd_copy_image(DvzCommands* cmds, uint32_t idx, DvzImages* src_img, DvzImages* dst_img);

/**
 * Set the viewport.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param viewport the viewport
 */
DVZ_EXPORT void dvz_cmd_viewport(DvzCommands* cmds, uint32_t idx, VkViewport viewport);

/**
 * Bind a graphics pipeline.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param graphics the graphics pipeline
 * @param bindings the bindings associated to the pipeline
 * @param dynamic_idx the dynamic uniform buffer index
 */
DVZ_EXPORT void dvz_cmd_bind_graphics(
    DvzCommands* cmds, uint32_t idx, DvzGraphics* graphics, //
    DvzBindings* bindings, uint32_t dynamic_idx);

/**
 * Bind a vertex buffer.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param br the buffer regions
 * @param offset the offset within the buffer regions, in bytes
 */
DVZ_EXPORT void dvz_cmd_bind_vertex_buffer(
    DvzCommands* cmds, uint32_t idx, DvzBufferRegions br, VkDeviceSize offset);

/**
 * Bind an index buffer.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param br the buffer regions
 * @param offset the offset within the buffer regions, in bytes
 */
DVZ_EXPORT void dvz_cmd_bind_index_buffer(
    DvzCommands* cmds, uint32_t idx, DvzBufferRegions br, VkDeviceSize offset);

/**
 * Direct draw.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param first_vertex index of the first vertex
 * @param vertex_count number of vertices to draw
 */
DVZ_EXPORT void
dvz_cmd_draw(DvzCommands* cmds, uint32_t idx, uint32_t first_vertex, uint32_t vertex_count);

/**
 * Direct indexed draw.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param first_index index of the first index
 * @param vertex_offset offset of the vertex
 * @param index_count number of indices to draw
 */
DVZ_EXPORT void dvz_cmd_draw_indexed(
    DvzCommands* cmds, uint32_t idx, uint32_t first_index, uint32_t vertex_offset,
    uint32_t index_count);

/**
 * Indirect draw.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param indirect buffer regions with the indirect draw info
 */
DVZ_EXPORT void dvz_cmd_draw_indirect(DvzCommands* cmds, uint32_t idx, DvzBufferRegions indirect);

/**
 * Indirect indexed draw.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param indirect buffer regions with the indirect draw info
 */
DVZ_EXPORT void
dvz_cmd_draw_indexed_indirect(DvzCommands* cmds, uint32_t idx, DvzBufferRegions indirect);

/**
 * Copy a GPU buffer to another.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param src_buf the source buffer
 * @param src_offset the offset in the source buffer
 * @param dst_buf the destination buffer, in bytes
 * @param dst_offset the offset in the destination buffer, in bytes
 * @param size the size of the region to copy, in bytes
 */
DVZ_EXPORT void dvz_cmd_copy_buffer(
    DvzCommands* cmds, uint32_t idx,             //
    DvzBuffer* src_buf, VkDeviceSize src_offset, //
    DvzBuffer* dst_buf, VkDeviceSize dst_offset, //
    VkDeviceSize size);

/**
 * Push constants.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param slots the slots
 * @param shaders the shader stages that have access to the push constant
 * @param offset the offset in the push constant, in bytes
 * @param size the size in the push constant, in bytes
 * @param data the data to send via the push constant
 */
DVZ_EXPORT void dvz_cmd_push(
    DvzCommands* cmds, uint32_t idx, DvzSlots* slots, VkShaderStageFlagBits shaders, //
    VkDeviceSize offset, VkDeviceSize size, const void* data);



/*************************************************************************************************/
/*  Context                                                                                      */
/*************************************************************************************************/

/**
 * Destroy a context.
 *
 * @param context the context
 */
DVZ_EXPORT void dvz_context_destroy(DvzContext* context);



#ifdef __cplusplus
}
#endif

#endif
