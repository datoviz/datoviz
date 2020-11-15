#ifndef VKL_VKLITE2_HEADER
#define VKL_VKLITE2_HEADER

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// #include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "common.h"

BEGIN_INCL_NO_WARN
#include <cglm/struct.h>
END_INCL_NO_WARN


/*
TODO later
- rename Vkl/VKL/vkl to Vkl
- move constants to constants.c
- put glfw-specific code in the same place
*/



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define VKL_MAX_BINDINGS         1024
#define VKL_MAX_BINDINGS_SIZE    32
#define VKL_MAX_COMMANDS         1024
#define VKL_MAX_DESCRIPTOR_SETS  1024
#define VKL_MAX_FENCES           1024
#define VKL_MAX_FRAMEBUFFERS     32
#define VKL_MAX_GPUS             64
#define VKL_MAX_GRAPHICS         1024
#define VKL_MAX_PRESENT_MODES    16
#define VKL_MAX_QUEUE_FAMILIES   16
#define VKL_MAX_QUEUES           16
#define VKL_MAX_RENDERPASSES     32
#define VKL_MAX_SEMAPHORES       1024
#define VKL_MAX_SWAPCHAIN_IMAGES 16
#define VKL_MAX_WINDOWS          1024

// Maximum number of command buffers per VklCommands struct
#define VKL_MAX_COMMAND_BUFFERS_PER_SET     VKL_MAX_SWAPCHAIN_IMAGES
#define VKL_MAX_BUFFER_REGIONS_PER_SET      VKL_MAX_SWAPCHAIN_IMAGES
#define VKL_MAX_IMAGES_PER_SET              VKL_MAX_SWAPCHAIN_IMAGES
#define VKL_MAX_SEMAPHORES_PER_SET          VKL_MAX_SWAPCHAIN_IMAGES
#define VKL_MAX_FENCES_PER_SET              VKL_MAX_SWAPCHAIN_IMAGES
#define VKL_MAX_COMMANDS_PER_SUBMIT         16
#define VKL_MAX_BARRIERS_PER_SET            16
#define VKL_MAX_SEMAPHORES_PER_SUBMIT       16
#define VKL_MAX_SHADERS_PER_GRAPHICS        8
#define VKL_MAX_ATTACHMENTS_PER_RENDERPASS  16
#define VKL_MAX_SUBPASSES_PER_RENDERPASS    16
#define VKL_MAX_DEPENDENCIES_PER_RENDERPASS 16
#define VKL_MAX_VERTEX_BINDINGS             16
#define VKL_MAX_VERTEX_ATTRS                32



/*************************************************************************************************/
/*  Type definitions */
/*************************************************************************************************/

typedef struct VklObject VklObject;
typedef struct VklApp VklApp;
typedef struct VklQueues VklQueues;
typedef struct VklGpu VklGpu;
typedef struct VklWindow VklWindow;
typedef struct VklSwapchain VklSwapchain;
typedef struct VklCommands VklCommands;
typedef struct VklBuffer VklBuffer;
typedef struct VklBufferRegions VklBufferRegions;
typedef struct VklImages VklImages;
typedef struct VklSampler VklSampler;
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



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/


typedef enum
{
    VKL_OBJECT_TYPE_UNDEFINED,
    VKL_OBJECT_TYPE_APP,
    VKL_OBJECT_TYPE_GPU,
    VKL_OBJECT_TYPE_WINDOW,
    VKL_OBJECT_TYPE_SWAPCHAIN,
    VKL_OBJECT_TYPE_CANVAS,
    VKL_OBJECT_TYPE_COMMANDS,
    VKL_OBJECT_TYPE_BUFFER,
    VKL_OBJECT_TYPE_IMAGES,
    VKL_OBJECT_TYPE_SAMPLER,
    VKL_OBJECT_TYPE_BINDINGS,
    VKL_OBJECT_TYPE_COMPUTE,
    VKL_OBJECT_TYPE_GRAPHICS,
    VKL_OBJECT_TYPE_BARRIER,
    VKL_OBJECT_TYPE_FENCES,
    VKL_OBJECT_TYPE_SEMAPHORES,
    VKL_OBJECT_TYPE_RENDERPASS,
    VKL_OBJECT_TYPE_FRAMEBUFFER,
    VKL_OBJECT_TYPE_SUBMIT,
    VKL_OBJECT_TYPE_CUSTOM,
} VklObjectType;


// NOTE: the order is important, status >= CREATED means the object has been created
typedef enum
{
    VKL_OBJECT_STATUS_NONE,          // after allocation
    VKL_OBJECT_STATUS_DESTROYED,     // after destruction
    VKL_OBJECT_STATUS_INIT,          // after struct initialization but before Vulkan creation
    VKL_OBJECT_STATUS_CREATED,       // after proper creation on the GPU
    VKL_OBJECT_STATUS_NEED_RECREATE, // need to be recreated
    VKL_OBJECT_STATUS_NEED_UPDATE,   // need to be updated
    VKL_OBJECT_STATUS_NEED_DESTROY,  // need to be destroyed
    VKL_OBJECT_STATUS_INVALID,       // invalid
} VklObjectStatus;


typedef enum
{
    VKL_BACKEND_NONE,
    VKL_BACKEND_GLFW,
    VKL_BACKEND_OFFSCREEN,
} VklBackend;


typedef enum
{
    VKL_QUEUE_TRANSFER = 0x01,
    VKL_QUEUE_GRAPHICS = 0x02,
    VKL_QUEUE_COMPUTE = 0x04,
    VKL_QUEUE_PRESENT = 0x08,
    VKL_QUEUE_RENDER = 0x07,
    VKL_QUEUE_ALL = 0x0F,
} VklQueueType;


typedef enum
{
    VKL_COMMAND_TRANSFERS,
    VKL_COMMAND_GRAPHICS,
    VKL_COMMAND_COMPUTE,
    VKL_COMMAND_GUI,
} VklCommandBufferType;


typedef enum
{
    VKL_TEXTURE_AXIS_U,
    VKL_TEXTURE_AXIS_V,
    VKL_TEXTURE_AXIS_W,
} VklTextureAxis;


typedef enum
{
    VKL_BLEND_DISABLE,
    VKL_BLEND_ENABLE,
} VklBlendType;


typedef enum
{
    VKL_DEPTH_TEST_DISABLE,
    VKL_DEPTH_TEST_ENABLE,
} VklDepthTest;


typedef enum
{
    VKL_RENDERPASS_ATTACHMENT_COLOR,
    VKL_RENDERPASS_ATTACHMENT_DEPTH,
} VklRenderpassAttachmentType;



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define INSTANCES_INIT(s, o, p, c, n, t)                                                          \
    log_trace("init %d object(s) %s", n, #s);                                                     \
    o->p = calloc(n, sizeof(s));                                                                  \
    for (uint32_t i = 0; i < n; i++)                                                              \
    {                                                                                             \
        o->p[i].obj.type = t;                                                                     \
    }                                                                                             \
    o->c = n;


#define INSTANCE_NEW(s, o, instances, n)                                                          \
    s* o = NULL;                                                                                  \
    for (uint32_t i = 0; i < n; i++)                                                              \
        if (instances[i].obj.status < VKL_OBJECT_STATUS_INIT)                                     \
        {                                                                                         \
            o = &instances[i];                                                                    \
            o->obj.status = VKL_OBJECT_STATUS_INIT;                                               \
            break;                                                                                \
        }                                                                                         \
    if (o == NULL)                                                                                \
    {                                                                                             \
        log_error("maximum number of %s instances reached", #s);                                  \
        exit(1);                                                                                  \
    }


#define INSTANCES_DESTROY(o)                                                                      \
    log_trace("destroy objects %s", #o);                                                          \
    FREE(o);                                                                                      \
    o = NULL;


#define CMD_START                                                                                 \
    ASSERT(cmds != NULL);                                                                         \
    VkCommandBuffer cb = {0};                                                                     \
    uint32_t i = idx;                                                                             \
    cb = cmds->cmds[i];


#define CMD_START_CLIP(cnt)                                                                       \
    ASSERT(cmds != NULL);                                                                         \
    ASSERT((cnt) == 1 || (cnt) == cmds->count);                                                   \
    VkCommandBuffer cb = {0};                                                                     \
    uint32_t iclip = 0;                                                                           \
    uint32_t i = idx;                                                                             \
    iclip = (cnt) == 1 ? 0 : (MIN(i, (cnt)-1));                                                   \
    ASSERT(iclip < (cnt));                                                                        \
    cb = cmds->cmds[i];


#define CMD_END //



/*************************************************************************************************/
/*  Common                                                                                       */
/*************************************************************************************************/

struct VklObject
{
    VklObjectType type;
    VklObjectStatus status;
};



static void obj_init(VklObject* obj) { obj->status = VKL_OBJECT_STATUS_INIT; }

static void obj_created(VklObject* obj) { obj->status = VKL_OBJECT_STATUS_CREATED; }

static void obj_destroyed(VklObject* obj) { obj->status = VKL_OBJECT_STATUS_DESTROYED; }



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VklApp
{
    VklObject obj;
    uint32_t n_errors;

    // Backend
    VklBackend backend;

    // Vulkan objects.
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;

    // GPUs.
    uint32_t max_gpus;
    uint32_t gpu_count;
    VklGpu* gpus;

    // Windows.
    uint32_t max_windows;
    VklWindow* windows;

    // Canvas.
    uint32_t max_canvases;
    VklCanvas* canvases;
};



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
    VkCommandPool cmd_pools[VKL_MAX_QUEUES];
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

    uint32_t max_swapchains;
    VklSwapchain* swapchains;

    uint32_t max_commands;
    VklCommands* commands;

    uint32_t max_bindings;
    VklBindings* bindings;

    uint32_t max_graphics;
    VklGraphics* graphics;

    uint32_t max_renderpasses;
    VklRenderpass* renderpasses;

    uint32_t max_framebuffers;
    VklFramebuffers* framebuffers;

    uint32_t max_semaphores;
    VklSemaphores* semaphores;

    uint32_t max_fences;
    VklFences* fences;
};



struct VklWindow
{
    VklObject obj;
    VklApp* app;

    void* backend_window;
    uint32_t width, height; // in screen coordinates

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

    VkBuffer buffer;
    VkDeviceMemory device_memory;

    // Queues that need access to the buffer.
    uint32_t queue_count;
    uint32_t queues[VKL_MAX_QUEUES];

    VkDeviceSize size;
    VkDeviceSize item_size; // stride, must be aligned, used in dynamic uniform buffer objects
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags memory;
};



struct VklBufferRegions
{
    VklBuffer* buffer;
    uint32_t count;
    VkDeviceSize size;
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



struct VklBindings
{
    VklObject obj;
    VklGpu* gpu;

    uint32_t bindings_count;
    VkDescriptorType types[VKL_MAX_BINDINGS_SIZE];
    VkDeviceSize alignments[VKL_MAX_BINDINGS_SIZE]; // dynamic uniform alignments

    VkPipelineLayout pipeline_layout;
    VkDescriptorSetLayout dset_layout;

    // a Bindings struct holds multiple almost-identical copies of descriptor sets
    // with the same layout, but possibly with the different idx in the VklBuffer
    uint32_t dset_count;
    VkDescriptorSet dsets[VKL_MAX_SWAPCHAIN_IMAGES];

    VklBufferRegions buffer_regions[VKL_MAX_BINDINGS_SIZE];
    VklImages* images[VKL_MAX_BINDINGS_SIZE];
    VklSampler* samplers[VKL_MAX_BINDINGS_SIZE];
};



struct VklCompute
{
    VklObject obj;
    VklGpu* gpu;
    VklContext* context;

    char shader_path[1024];

    VkPipeline pipeline;
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

    VklRenderpass* renderpass;
    uint32_t subpass;

    VkPrimitiveTopology topology;
    VklBlendType blend_type;
    VklDepthTest depth_test;
    VkPolygonMode polygon_mode;
    VkCullModeFlags cull_mode;
    VkFrontFace front_face;

    VkPipeline pipeline;
    VklBindings* bindings;

    uint32_t vertex_binding_count;
    VklVertexBinding vertex_bindings[VKL_MAX_VERTEX_BINDINGS];

    uint32_t vertex_attr_count;
    VklVertexAttr vertex_attrs[VKL_MAX_VERTEX_ATTRS];

    uint32_t shader_count;
    VkShaderStageFlagBits shader_stages[VKL_MAX_SHADERS_PER_GRAPHICS];
    VkShaderModule shader_modules[VKL_MAX_SHADERS_PER_GRAPHICS];
};



struct VklBarrierBuffer
{
    VklBufferRegions buffer_regions;

    VkAccessFlags src_access;
    uint32_t src_queue;

    VkAccessFlags dst_access;
    uint32_t dst_queue;
};



struct VklBarrierImage
{
    VklImages* images;

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

VKY_EXPORT void vkl_gpu_queue_wait(VklGpu* gpu, uint32_t queue_idx);

VKY_EXPORT void vkl_gpu_wait(VklGpu* gpu);

VKY_EXPORT void vkl_gpu_destroy(VklGpu* gpu);



/*************************************************************************************************/
/*  Window                                                                                       */
/*************************************************************************************************/

VKY_EXPORT VklWindow* vkl_window(VklApp* app, uint32_t width, uint32_t height);

VKY_EXPORT void
vkl_window_get_size(VklWindow* window, uint32_t* framebuffer_width, uint32_t* framebuffer_height);

// NOTE: to be called AFTER vkl_swapchain_destroy()
VKY_EXPORT void vkl_window_destroy(VklWindow* window);

VKY_EXPORT void vkl_canvas_destroy(VklCanvas* canvas);

VKY_EXPORT void vkl_canvases_destroy(uint32_t canvas_count, VklCanvas* canvases);



/*************************************************************************************************/
/*  Swapchain                                                                                    */
/*************************************************************************************************/

VKY_EXPORT VklSwapchain* vkl_swapchain(VklGpu* gpu, VklWindow* window, uint32_t min_img_count);

VKY_EXPORT void vkl_swapchain_format(VklSwapchain* swapchain, VkFormat format);

VKY_EXPORT void vkl_swapchain_present_mode(VklSwapchain* swapchain, VkPresentModeKHR present_mode);

VKY_EXPORT void
vkl_swapchain_requested_size(VklSwapchain* swapchain, uint32_t width, uint32_t height);

VKY_EXPORT void vkl_swapchain_create(VklSwapchain* swapchain);

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

VKY_EXPORT VklCommands* vkl_commands(VklGpu* gpu, uint32_t queue, uint32_t count);

VKY_EXPORT void vkl_cmd_begin(VklCommands* cmds, uint32_t idx);

VKY_EXPORT void vkl_cmd_end(VklCommands* cmds, uint32_t idx);

VKY_EXPORT void vkl_cmd_reset(VklCommands* cmds);

VKY_EXPORT void vkl_cmd_free(VklCommands* cmds);

VKY_EXPORT void vkl_cmd_submit_sync(VklCommands* cmds, uint32_t idx);

VKY_EXPORT void vkl_commands_destroy(VklCommands* cmds);



/*************************************************************************************************/
/*  Buffers                                                                                      */
/*************************************************************************************************/

VKY_EXPORT VklBuffer vkl_buffer(VklGpu* gpu);

VKY_EXPORT void vkl_buffer_size(VklBuffer* buffer, VkDeviceSize size, VkDeviceSize item_size);

VKY_EXPORT void vkl_buffer_usage(VklBuffer* buffer, VkBufferUsageFlags usage);

VKY_EXPORT void vkl_buffer_memory(VklBuffer* buffer, VkMemoryPropertyFlags memory);

VKY_EXPORT void vkl_buffer_queue_access(VklBuffer* buffer, uint32_t queues);

VKY_EXPORT void vkl_buffer_create(VklBuffer* buffer);

VKY_EXPORT void
vkl_buffer_resize(VklBuffer* buffer, VkDeviceSize size, uint32_t queue_idx, VklCommands* cmds);

VKY_EXPORT VklBufferRegions
vkl_buffer_regions(VklBuffer* buffer, uint32_t count, VkDeviceSize size, VkDeviceSize* offsets);

VKY_EXPORT void* vkl_buffer_regions_map(VklBufferRegions* buffer_regions, uint32_t idx);

VKY_EXPORT void vkl_buffer_regions_unmap(VklBufferRegions* buffer_regions, uint32_t idx);

VKY_EXPORT void
vkl_buffer_download(VklBuffer* buffer, VkDeviceSize offset, VkDeviceSize size, void* data);

VKY_EXPORT void
vkl_buffer_upload(VklBuffer* buffer, VkDeviceSize offset, VkDeviceSize size, const void* data);

VKY_EXPORT void vkl_bindings_update(VklBindings* bindings);

VKY_EXPORT void vkl_buffer_destroy(VklBuffer* buffer);



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
/*  Bindings                                                                                     */
/*************************************************************************************************/

VKY_EXPORT VklBindings* vkl_bindings(VklGpu* gpu);

VKY_EXPORT void vkl_bindings_slot(
    VklBindings* bindings, uint32_t idx, VkDescriptorType type, VkDeviceSize item_size);

VKY_EXPORT void*
vkl_bindings_dynamic_allocate(VklBindings* bindings, uint32_t idx, VkDeviceSize size);

VKY_EXPORT void* vkl_bindings_dynamic_pointer(
    VklBindings* bindings, uint32_t idx, uint32_t item_idx, const void* data);

VKY_EXPORT void vkl_bindings_create(VklBindings* bindings, uint32_t dset_count);

VKY_EXPORT void
vkl_bindings_buffer(VklBindings* bindings, uint32_t idx, VklBufferRegions* buffer_regions);

VKY_EXPORT void
vkl_bindings_texture(VklBindings* bindings, uint32_t idx, VklImages* imagess, VklSampler* sampler);

VKY_EXPORT void vkl_bindings_destroy(VklBindings* bindings);



/*************************************************************************************************/
/*  Compute                                                                                      */
/*************************************************************************************************/

VKY_EXPORT VklCompute vkl_compute(VklGpu* gpu, const char* shader_path);

VKY_EXPORT void vkl_compute_create(VklCompute* compute);

VKY_EXPORT void vkl_compute_bindings(VklCompute* compute, VklBindings* bindings);

VKY_EXPORT void vkl_compute_destroy(VklCompute* compute);



/*************************************************************************************************/
/*  Pipeline                                                                                     */
/*************************************************************************************************/

VKY_EXPORT VklGraphics* vkl_graphics(VklGpu* gpu);

VKY_EXPORT void
vkl_graphics_renderpass(VklGraphics* graphics, VklRenderpass* renderpass, uint32_t subpass);

VKY_EXPORT void vkl_graphics_topology(VklGraphics* graphics, VkPrimitiveTopology topology);

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

VKY_EXPORT void vkl_graphics_bindings(VklGraphics* graphics, VklBindings* bindings);

VKY_EXPORT void vkl_graphics_destroy(VklGraphics* graphics);



/*************************************************************************************************/
/*  Barrier                                                                                      */
/*************************************************************************************************/

VKY_EXPORT VklBarrier vkl_barrier(VklGpu* gpu);

VKY_EXPORT void vkl_barrier_stages(
    VklBarrier* barrier, VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage);

VKY_EXPORT void vkl_barrier_buffer(VklBarrier* barrier, VklBufferRegions* buffer_regions);

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

VKY_EXPORT VklSemaphores* vkl_semaphores(VklGpu* gpu, uint32_t count);

VKY_EXPORT void vkl_semaphores_destroy(VklSemaphores* semaphores);



/*************************************************************************************************/
/*  Fences                                                                                       */
/*************************************************************************************************/

VKY_EXPORT VklFences* vkl_fences(VklGpu* gpu, uint32_t count);

VKY_EXPORT void vkl_fences_create(VklFences* fences);

VKY_EXPORT void
vkl_fences_copy(VklFences* src_fences, uint32_t src_idx, VklFences* dst_fences, uint32_t dst_idx);

VKY_EXPORT void vkl_fences_wait(VklFences* fences, uint32_t idx);

VKY_EXPORT void vkl_fences_reset(VklFences* fences, uint32_t idx);

VKY_EXPORT void vkl_fences_destroy(VklFences* fences);



/*************************************************************************************************/
/*  Renderpass                                                                                   */
/*************************************************************************************************/

VKY_EXPORT VklRenderpass* vkl_renderpass(VklGpu* gpu);

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

VKY_EXPORT VklFramebuffers* vkl_framebuffers(VklGpu* gpu);

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
vkl_submit_send(VklSubmit* submit, uint32_t img_idx, VklFences* fence, uint32_t fence_idx);



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

VKY_EXPORT void
vkl_cmd_copy_image(VklCommands* cmds, uint32_t idx, VklImages* src_img, VklImages* dst_img);

VKY_EXPORT void vkl_cmd_viewport(VklCommands* cmds, uint32_t idx, VkViewport viewport);

VKY_EXPORT void vkl_cmd_bind_graphics(
    VklCommands* cmds, uint32_t idx, VklGraphics* graphics, uint32_t dynamic_idx);

VKY_EXPORT void vkl_cmd_bind_vertex_buffer(
    VklCommands* cmds, uint32_t idx, VklBufferRegions* buffer_regions, VkDeviceSize offset);

VKY_EXPORT void vkl_cmd_bind_index_buffer(
    VklCommands* cmds, uint32_t idx, VklBufferRegions* buffer_regions, VkDeviceSize offset);

VKY_EXPORT void
vkl_cmd_draw(VklCommands* cmds, uint32_t idx, uint32_t first_vertex, uint32_t vertex_count);

VKY_EXPORT void vkl_cmd_draw_indexed(
    VklCommands* cmds, uint32_t idx, uint32_t first_index, uint32_t vertex_offset,
    uint32_t index_count);

VKY_EXPORT void vkl_cmd_draw_indirect(VklCommands* cmds, uint32_t idx, VklBufferRegions* indirect);

VKY_EXPORT void
vkl_cmd_draw_indexed_indirect(VklCommands* cmds, uint32_t idx, VklBufferRegions* indirect);

VKY_EXPORT void vkl_cmd_copy_buffer(
    VklCommands* cmds, uint32_t idx,             //
    VklBuffer* src_buf, VkDeviceSize src_offset, //
    VklBuffer* dst_buf, VkDeviceSize dst_offset, //
    VkDeviceSize size);

VKY_EXPORT void vkl_cmd_push_constants(
    VklCommands* cmds, uint32_t idx, VklBindings* bindings, VkDeviceSize size, const void* data);



/*************************************************************************************************/
/*  Context                                                                                      */
/*************************************************************************************************/

VKY_EXPORT void vkl_context_destroy(VklContext* context);



#endif
