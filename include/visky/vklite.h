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
 * @param backend the backend, typically either VKL_BACKEND_GLFW or VKL_BACKEND_OFFSCREEN
 * @returns a pointer to the created VklApp struct
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

VKY_EXPORT VklGpu* vkl_gpu(VklApp* app, uint32_t idx);

VKY_EXPORT void vkl_gpu_request_features(VklGpu* gpu, VkPhysicalDeviceFeatures requested_features);

VKY_EXPORT void vkl_gpu_queue(VklGpu* gpu, VklQueueType type, uint32_t idx);

VKY_EXPORT void vkl_gpu_create(VklGpu* gpu, VkSurfaceKHR surface);

VKY_EXPORT void vkl_queue_wait(VklGpu* gpu, uint32_t queue_idx);

VKY_EXPORT void vkl_app_wait(VklApp* app);

VKY_EXPORT void vkl_gpu_wait(VklGpu* gpu);

VKY_EXPORT void vkl_gpu_destroy(VklGpu* gpu);



/*************************************************************************************************/
/*  Window                                                                                       */
/*************************************************************************************************/

VKY_EXPORT VklWindow* vkl_window(VklApp* app, uint32_t width, uint32_t height);

VKY_EXPORT void
vkl_window_get_size(VklWindow* window, uint32_t* framebuffer_width, uint32_t* framebuffer_height);

VKY_EXPORT void vkl_window_poll_events(VklWindow* window);

// NOTE: to be called AFTER vkl_swapchain_destroy()
VKY_EXPORT void vkl_window_destroy(VklWindow* window);

VKY_EXPORT void vkl_canvas_destroy(VklCanvas* canvas);

VKY_EXPORT void vkl_canvases_destroy(VklContainer* canvases);



/*************************************************************************************************/
/*  Swapchain                                                                                    */
/*************************************************************************************************/

VKY_EXPORT VklSwapchain vkl_swapchain(VklGpu* gpu, VklWindow* window, uint32_t min_img_count);

VKY_EXPORT void vkl_swapchain_format(VklSwapchain* swapchain, VkFormat format);

VKY_EXPORT void vkl_swapchain_present_mode(VklSwapchain* swapchain, VkPresentModeKHR present_mode);

VKY_EXPORT void
vkl_swapchain_requested_size(VklSwapchain* swapchain, uint32_t width, uint32_t height);

VKY_EXPORT void vkl_swapchain_create(VklSwapchain* swapchain);

VKY_EXPORT void vkl_swapchain_recreate(VklSwapchain* swapchain);

VKY_EXPORT void vkl_swapchain_acquire(
    VklSwapchain* swapchain, VklSemaphores* semaphores, uint32_t semaphore_idx, VklFences* fences,
    uint32_t fence_idx);

VKY_EXPORT void vkl_swapchain_present(
    VklSwapchain* swapchain, uint32_t queue_idx, VklSemaphores* semaphores,
    uint32_t semaphore_idx);

// NOTE: to be called BEFORE vkl_window_destroy()
VKY_EXPORT void vkl_swapchain_destroy(VklSwapchain* swapchain);



/*************************************************************************************************/
/*  Commands                                                                                     */
/*************************************************************************************************/

VKY_EXPORT VklCommands vkl_commands(VklGpu* gpu, uint32_t queue, uint32_t count);

VKY_EXPORT void vkl_cmd_begin(VklCommands* cmds, uint32_t idx);

VKY_EXPORT void vkl_cmd_end(VklCommands* cmds, uint32_t idx);

VKY_EXPORT void vkl_cmd_reset(VklCommands* cmds, uint32_t idx);

VKY_EXPORT void vkl_cmd_free(VklCommands* cmds);

VKY_EXPORT void vkl_cmd_submit_sync(VklCommands* cmds, uint32_t idx);

VKY_EXPORT void vkl_commands_destroy(VklCommands* cmds);



/*************************************************************************************************/
/*  Buffers                                                                                      */
/*************************************************************************************************/

VKY_EXPORT VklBuffer vkl_buffer(VklGpu* gpu);

VKY_EXPORT void vkl_buffer_size(VklBuffer* buffer, VkDeviceSize size);

VKY_EXPORT void vkl_buffer_type(VklBuffer* buffer, VklBufferType type);

VKY_EXPORT void vkl_buffer_usage(VklBuffer* buffer, VkBufferUsageFlags usage);

VKY_EXPORT void vkl_buffer_memory(VklBuffer* buffer, VkMemoryPropertyFlags memory);

VKY_EXPORT void vkl_buffer_queue_access(VklBuffer* buffer, uint32_t queues);

VKY_EXPORT void vkl_buffer_create(VklBuffer* buffer);

VKY_EXPORT void vkl_buffer_resize(VklBuffer* buffer, VkDeviceSize size, VklCommands* cmds);

VKY_EXPORT void* vkl_buffer_map(VklBuffer* buffer, VkDeviceSize offset, VkDeviceSize size);

VKY_EXPORT void vkl_buffer_unmap(VklBuffer* buffer);

VKY_EXPORT void
vkl_buffer_download(VklBuffer* buffer, VkDeviceSize offset, VkDeviceSize size, void* data);

VKY_EXPORT void
vkl_buffer_upload(VklBuffer* buffer, VkDeviceSize offset, VkDeviceSize size, const void* data);

VKY_EXPORT void vkl_buffer_destroy(VklBuffer* buffer);



VKY_EXPORT VklBufferRegions vkl_buffer_regions(
    VklBuffer* buffer, uint32_t count, //
    VkDeviceSize offset, VkDeviceSize size, VkDeviceSize alignment);

VKY_EXPORT void* vkl_buffer_regions_map(VklBufferRegions* br, uint32_t idx);

VKY_EXPORT void vkl_buffer_regions_unmap(VklBufferRegions* br);

VKY_EXPORT void vkl_buffer_regions_upload(VklBufferRegions* br, uint32_t idx, const void* data);



/*************************************************************************************************/
/*  Images                                                                                       */
/*************************************************************************************************/

VKY_EXPORT VklImages vkl_images(VklGpu* gpu, VkImageType type, uint32_t count);

VKY_EXPORT void vkl_images_format(VklImages* images, VkFormat format);

VKY_EXPORT void vkl_images_layout(VklImages* images, VkImageLayout layout);

VKY_EXPORT void
vkl_images_size(VklImages* images, uint32_t width, uint32_t height, uint32_t depth);

VKY_EXPORT void vkl_images_tiling(VklImages* images, VkImageTiling tiling);

VKY_EXPORT void vkl_images_usage(VklImages* images, VkImageUsageFlags usage);

VKY_EXPORT void vkl_images_memory(VklImages* images, VkMemoryPropertyFlags memory);

VKY_EXPORT void vkl_images_aspect(VklImages* images, VkImageAspectFlags aspect);

VKY_EXPORT void vkl_images_queue_access(VklImages* images, uint32_t queue);

VKY_EXPORT void vkl_images_create(VklImages* images);

VKY_EXPORT void
vkl_images_resize(VklImages* images, uint32_t width, uint32_t height, uint32_t depth);

VKY_EXPORT void vkl_images_transition(VklImages* images);

VKY_EXPORT void vkl_images_download(VklImages* staging, uint32_t idx, bool swizzle, uint8_t* rgba);

VKY_EXPORT void vkl_images_destroy(VklImages* images);



/*************************************************************************************************/
/*  Sampler                                                                                      */
/*************************************************************************************************/

VKY_EXPORT VklSampler vkl_sampler(VklGpu* gpu);

VKY_EXPORT void vkl_sampler_min_filter(VklSampler* sampler, VkFilter filter);

VKY_EXPORT void vkl_sampler_mag_filter(VklSampler* sampler, VkFilter filter);

VKY_EXPORT void vkl_sampler_address_mode(
    VklSampler* sampler, VklTextureAxis axis, VkSamplerAddressMode address_mode);

VKY_EXPORT void vkl_sampler_create(VklSampler* sampler);

VKY_EXPORT void vkl_sampler_destroy(VklSampler* sampler);



/*************************************************************************************************/
/*  Slots                                                                                        */
/*************************************************************************************************/

VKY_EXPORT VklSlots vkl_slots(VklGpu* gpu);

VKY_EXPORT void vkl_slots_binding(VklSlots* slots, uint32_t idx, VkDescriptorType type);

VKY_EXPORT void vkl_slots_push(
    VklSlots* slots, VkDeviceSize offset, VkDeviceSize size, VkShaderStageFlags shaders);

VKY_EXPORT void vkl_slots_create(VklSlots* slots);

VKY_EXPORT void vkl_slots_destroy(VklSlots* slots);



/*************************************************************************************************/
/*  Bindings                                                                                     */
/*************************************************************************************************/

VKY_EXPORT VklBindings vkl_bindings(VklSlots* slots, uint32_t dset_count);

VKY_EXPORT void vkl_bindings_buffer(VklBindings* bindings, uint32_t idx, VklBufferRegions br);

VKY_EXPORT void vkl_bindings_texture(VklBindings* bindings, uint32_t idx, VklTexture* texture);

VKY_EXPORT void vkl_bindings_update(VklBindings* bindings);

VKY_EXPORT void vkl_bindings_destroy(VklBindings* bindings);



/*************************************************************************************************/
/*  Compute                                                                                      */
/*************************************************************************************************/

VKY_EXPORT VklCompute vkl_compute(VklGpu* gpu, const char* shader_path);

VKY_EXPORT void vkl_compute_create(VklCompute* compute);

VKY_EXPORT void vkl_compute_code(VklCompute* compute, const char* code);

VKY_EXPORT void vkl_compute_slot(VklCompute* compute, uint32_t idx, VkDescriptorType type);

VKY_EXPORT void vkl_compute_push(
    VklCompute* compute, VkDeviceSize offset, VkDeviceSize size, VkShaderStageFlags shaders);

VKY_EXPORT void vkl_compute_bindings(VklCompute* compute, VklBindings* bindings);

VKY_EXPORT void vkl_compute_destroy(VklCompute* compute);



/*************************************************************************************************/
/*  Pipeline                                                                                     */
/*************************************************************************************************/

VKY_EXPORT VklGraphics vkl_graphics(VklGpu* gpu);

VKY_EXPORT void
vkl_graphics_renderpass(VklGraphics* graphics, VklRenderpass* renderpass, uint32_t subpass);

VKY_EXPORT void vkl_graphics_topology(VklGraphics* graphics, VkPrimitiveTopology topology);

VKY_EXPORT void
vkl_graphics_shader_glsl(VklGraphics* graphics, VkShaderStageFlagBits stage, const char* code);

VKY_EXPORT void vkl_graphics_shader_spirv(
    VklGraphics* graphics, VkShaderStageFlagBits stage, //
    VkDeviceSize size, const uint32_t* buffer);

VKY_EXPORT void
vkl_graphics_shader(VklGraphics* graphics, VkShaderStageFlagBits stage, const char* shader_path);

VKY_EXPORT void
vkl_graphics_vertex_binding(VklGraphics* graphics, uint32_t binding, VkDeviceSize stride);

VKY_EXPORT void vkl_graphics_vertex_attr(
    VklGraphics* graphics, uint32_t binding, uint32_t location, VkFormat format,
    VkDeviceSize offset);

VKY_EXPORT void vkl_graphics_blend(VklGraphics* graphics, VklBlendType blend_type);

VKY_EXPORT void vkl_graphics_depth_test(VklGraphics* graphics, VklDepthTest depth_test);

VKY_EXPORT void vkl_graphics_polygon_mode(VklGraphics* graphics, VkPolygonMode polygon_mode);

VKY_EXPORT void vkl_graphics_cull_mode(VklGraphics* graphics, VkCullModeFlags cull_mode);

VKY_EXPORT void vkl_graphics_front_face(VklGraphics* graphics, VkFrontFace front_face);

VKY_EXPORT void vkl_graphics_create(VklGraphics* graphics);

VKY_EXPORT void vkl_graphics_slot(VklGraphics* graphics, uint32_t idx, VkDescriptorType type);

VKY_EXPORT void vkl_graphics_push(
    VklGraphics* graphics, VkDeviceSize offset, VkDeviceSize size, VkShaderStageFlags shaders);

VKY_EXPORT void vkl_graphics_destroy(VklGraphics* graphics);



/*************************************************************************************************/
/*  Barrier                                                                                      */
/*************************************************************************************************/

VKY_EXPORT VklBarrier vkl_barrier(VklGpu* gpu);

VKY_EXPORT void vkl_barrier_stages(
    VklBarrier* barrier, VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage);

VKY_EXPORT void vkl_barrier_buffer(VklBarrier* barrier, VklBufferRegions br);

VKY_EXPORT void
vkl_barrier_buffer_queue(VklBarrier* barrier, uint32_t src_queue, uint32_t dst_queue);

VKY_EXPORT void
vkl_barrier_buffer_access(VklBarrier* barrier, VkAccessFlags src_access, VkAccessFlags dst_access);

VKY_EXPORT void vkl_barrier_images(VklBarrier* barrier, VklImages* images);

VKY_EXPORT void
vkl_barrier_images_layout(VklBarrier* barrier, VkImageLayout src_layout, VkImageLayout dst_layout);

VKY_EXPORT void
vkl_barrier_images_queue(VklBarrier* barrier, uint32_t src_queue, uint32_t dst_queue);

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
