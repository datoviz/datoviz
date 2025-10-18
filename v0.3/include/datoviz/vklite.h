/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Vklite                                                                                       */
/*************************************************************************************************/

#ifndef DVZ_HEADER_VKLITE
#define DVZ_HEADER_VKLITE



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_enums.h"
#include "common.h"
#include "datoviz_types.h"

#include <vulkan/vulkan.h>

MUTE_ON
#define VMA_EXTERNAL_MEMORY 1
#include "vk_mem_alloc.h"
MUTE_OFF



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_VULKAN_API VK_API_VERSION_1_3

#define DVZ_MAX_DEVICE_EXTENSIONS           16
#define DVZ_MAX_DESCRIPTOR_SETS             1024
#define DVZ_MAX_PRESENT_MODES               16
#define DVZ_MAX_PUSH_CONSTANTS              16
#define DVZ_MAX_QUEUE_FAMILIES              16
#define DVZ_MAX_QUEUES                      16
#define DVZ_MAX_SWAPCHAIN_IMAGES            4
#define DVZ_MAX_COMMAND_BUFFERS_PER_SET     DVZ_MAX_SWAPCHAIN_IMAGES
#define DVZ_MAX_BUFFER_REGIONS_PER_SET      DVZ_MAX_SWAPCHAIN_IMAGES
#define DVZ_MAX_IMAGES_PER_SET              DVZ_MAX_SWAPCHAIN_IMAGES
#define DVZ_MAX_SEMAPHORES_PER_SET          DVZ_MAX_SWAPCHAIN_IMAGES
#define DVZ_MAX_FENCES_PER_SET              DVZ_MAX_SWAPCHAIN_IMAGES
#define DVZ_MAX_COMMANDS_PER_SUBMIT         8
#define DVZ_MAX_BARRIERS_PER_SET            8
#define DVZ_MAX_SEMAPHORES_PER_SUBMIT       8
#define DVZ_MAX_SHADERS_PER_GRAPHICS        6
#define DVZ_MAX_ATTACHMENTS_PER_RENDERPASS  8
#define DVZ_MAX_SUBPASSES_PER_RENDERPASS    8
#define DVZ_MAX_DEPENDENCIES_PER_RENDERPASS 8
#define DVZ_MAX_FRAMES_IN_FLIGHT            2
#define DVZ_MAX_SPECIALIZATION_CONSTANTS    8



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzQueues DvzQueues;
typedef struct DvzGpu DvzGpu;
typedef struct DvzVma DvzVma;
typedef struct DvzSwapchain DvzSwapchain;
typedef struct DvzCommands DvzCommands;
typedef struct DvzBuffer DvzBuffer;
typedef struct DvzBufferRegions DvzBufferRegions;
typedef struct DvzImages DvzImages;
typedef struct DvzSampler DvzSampler;
typedef struct DvzSlots DvzSlots;
typedef struct DvzDescriptors DvzDescriptors;

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
typedef struct DvzSpecializationConstants DvzSpecializationConstants;
typedef struct DvzFramebuffers DvzFramebuffers;
typedef struct DvzSubmit DvzSubmit;

// Forward declarations.
typedef struct DvzHost DvzHost;



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



// Render pass attachment type.
typedef enum
{
    DVZ_RENDERPASS_ATTACHMENT_COLOR,
    DVZ_RENDERPASS_ATTACHMENT_DEPTH,
    DVZ_RENDERPASS_ATTACHMENT_PICK,
} DvzRenderpassAttachmentType;



/*************************************************************************************************/
/*  Renderpass structs                                                                           */
/*************************************************************************************************/

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



/*************************************************************************************************/
/*  Specialization constants structs                                                             */
/*************************************************************************************************/

struct DvzSpecializationConstants
{
    VkShaderStageFlagBits stage;
    uint32_t count;
    uint32_t ids[DVZ_MAX_SPECIALIZATION_CONSTANTS];         // specialization constant IDs
    VkDeviceSize offsets[DVZ_MAX_SPECIALIZATION_CONSTANTS]; // NOTE: will be computed
    VkDeviceSize sizes[DVZ_MAX_SPECIALIZATION_CONSTANTS];
    void* data[DVZ_MAX_SPECIALIZATION_CONSTANTS];
    void* concatenated_constants;
};



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



struct DvzCommands
{
    DvzObject obj;
    DvzGpu* gpu;

    uint32_t queue_idx;
    uint32_t count;
    VkCommandBuffer cmds[DVZ_MAX_COMMAND_BUFFERS_PER_SET];
    bool blocked[DVZ_MAX_COMMAND_BUFFERS_PER_SET]; // if true, no need to refill it in the FRAME
};



struct DvzGpu
{
    DvzObject obj;
    DvzHost* host;

    uint32_t idx; // GPU index within the host
    const char* name;

    uint32_t extension_count;
    const char* extensions[DVZ_MAX_DEVICE_EXTENSIONS];

    VkExternalMemoryHandleTypeFlags external_memory_handle_type;

    VkPhysicalDevice physical_device;
    VkPhysicalDeviceProperties device_properties;
    VkPhysicalDeviceFeatures device_features;
    VkPhysicalDeviceMemoryProperties memory_properties;
    VkDeviceSize vram; // amount of VRAM

    uint32_t present_mode_count;
    VkPresentModeKHR present_modes[DVZ_MAX_PRESENT_MODES];

    DvzQueues queues;
    VkDescriptorPool dset_pool;

    VkPhysicalDeviceFeatures requested_features;
    VkDevice device;

    VmaAllocator allocator;

    DvzCommands cmd; // Command buffer for transfers.

    // Renderpasses.
    // DvzRenderpass renderpass; // default renderpass
};



struct DvzSwapchain
{
    DvzObject obj;
    DvzGpu* gpu;
    VkSurfaceKHR surface;

    VkFormat format;
    VkPresentModeKHR present_mode;
    bool support_transfer; // whether the swapchain supports copying the image to another

    // extent in pixel coordinates if caps.currentExtent is not available
    uint32_t requested_width, requested_height;

    uint32_t img_count;
    uint32_t img_idx;
    VkSwapchainKHR swapchain;
    VkSurfaceCapabilitiesKHR caps;

    // The actual framebuffer size in pixels is found in the images size
    DvzImages* images;
};



struct DvzVma
{
    VmaMemoryUsage usage;
    VmaAllocationCreateFlags flags;
    VmaAllocationInfo info;
    VmaAllocation alloc;
    VkDeviceSize alignment; // alignment required by Vulkan
};



struct DvzBuffer
{
    DvzObject obj;
    DvzGpu* gpu;

    DvzBufferType type;
    VkBuffer buffer;

    // Queues that need access to the buffer.
    uint32_t queue_count;
    uint32_t queues[DVZ_MAX_QUEUES];

    VkDeviceSize size;
    VkMemoryPropertyFlags memory;
    VkBufferUsageFlags usage;
    bool mappable_intended;

    // VMA
    DvzVma vma;

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
    // uint32_t width, height, depth;
    uvec3 shape; // width, height, depth
    VkFormat format;
    VkImageLayout layout;
    VkImageTiling tiling;
    VkImageUsageFlags usage;
    VkMemoryPropertyFlags memory;
    VkImageAspectFlags aspect;
    VkDeviceSize size;

    // VMA
    DvzVma vma[DVZ_MAX_IMAGES_PER_SET];

    VkImage images[DVZ_MAX_IMAGES_PER_SET];
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
    VkDescriptorType types[DVZ_MAX_BINDINGS];

    uint32_t push_count;
    VkDeviceSize push_offsets[DVZ_MAX_PUSH_CONSTANTS];
    VkDeviceSize push_sizes[DVZ_MAX_PUSH_CONSTANTS];
    VkShaderStageFlagBits push_stages[DVZ_MAX_PUSH_CONSTANTS];

    VkPipelineLayout pipeline_layout;
    VkDescriptorSetLayout dset_layout;
};



struct DvzDescriptors
{
    DvzObject obj;
    DvzGpu* gpu;

    DvzSlots* dslots;

    // a Bindings struct holds multiple almost-identical copies of descriptor sets
    // with the same layout, but possibly with the different idx in the DvzBuffer
    uint32_t dset_count;
    VkDescriptorSet dsets[DVZ_MAX_SWAPCHAIN_IMAGES];

    DvzBufferRegions br[DVZ_MAX_BINDINGS];
    DvzImages* images[DVZ_MAX_BINDINGS];
    DvzSampler* samplers[DVZ_MAX_BINDINGS];
};



struct DvzCompute
{
    DvzObject obj;
    DvzGpu* gpu;

    char shader_path[1024];
    const char* shader_code;

    VkPipeline pipeline;
    DvzSlots dslots;
    DvzDescriptors* descriptors;
    VkShaderModule shader_module;
};



struct DvzVertexBinding
{
    uint32_t binding;
    VkDeviceSize stride;
    VkVertexInputRate input_rate;
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

    uint32_t type;
    int flags;
    bool support_pick;
    void* user_data;

    DvzRenderpass* renderpass;
    uint32_t subpass;

    VkPrimitiveTopology topology;
    DvzBlendType blend_type;
    int32_t color_mask; // flags DvzColorMask
    DvzDepthTest depth_test;
    VkPolygonMode polygon_mode;
    VkCullModeFlags cull_mode;
    VkFrontFace front_face;
    int drawing;

    VkPipeline pipeline;
    DvzSlots dslots;

    uint32_t vertex_binding_count;
    DvzVertexBinding vertex_bindings[DVZ_MAX_VERTEX_BINDINGS];

    uint32_t vertex_attr_count;
    DvzVertexAttr vertex_attrs[DVZ_MAX_VERTEX_ATTRS];

    uint32_t shader_count;
    VkShaderStageFlagBits shader_stages[DVZ_MAX_SHADERS_PER_GRAPHICS];
    VkShaderModule shader_modules[DVZ_MAX_SHADERS_PER_GRAPHICS];

    uint32_t spec_const_count;
    DvzSpecializationConstants spec_consts[DVZ_MAX_SHADERS_PER_GRAPHICS];
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
    VkImageAspectFlags aspect;

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



EXTERN_C_ON

/*************************************************************************************************/
/*  Swapchain                                                                                    */
/*************************************************************************************************/

/**
 * Initialize a swapchain.
 *
 * @param gpu the GPU
 * @param surface the surface
 * @param min_img_count the minimum acceptable number of images in the swapchain
 * @returns the swapchain
 */
DvzSwapchain dvz_swapchain(DvzGpu* gpu, VkSurfaceKHR surface, uint32_t min_img_count);

/**
 * Set the swapchain image format.
 *
 * @param swapchain the swapchain
 * @param format the format
 */
void dvz_swapchain_format(DvzSwapchain* swapchain, VkFormat format);

/**
 * Set the swapchain present mode.
 *
 * @param swapchain the swapchain
 * @param present_mode the present mode
 */
void dvz_swapchain_present_mode(DvzSwapchain* swapchain, VkPresentModeKHR present_mode);

/**
 * Set the swapchain requested image size.
 *
 * @param swapchain the swapchain
 * @param width the requested width
 * @param height the requested height
 */
void dvz_swapchain_requested_size(DvzSwapchain* swapchain, uint32_t width, uint32_t height);

/**
 * Create the swapchain once it has been set up.
 *
 * The swapchain's size is automatically determined from the surface's size, which is queried via
 * Vulkan.
 *
 * @param swapchain the swapchain
 */
void dvz_swapchain_create(DvzSwapchain* swapchain);

/**
 * Recreate a swapchain (for example after a window resize).
 *
 * @param swapchain the swapchain
 */
void dvz_swapchain_recreate(DvzSwapchain* swapchain);

/**
 * Acquire a swapchain image.
 *
 * @param swapchain the swapchain
 * @param semaphores the set of signal semaphores
 * @param semaphore_idx the index of the semaphore to signal after image acquisition
 * @param fences the set of signal fences
 * @param fence_idx the index of the fence to signal after image acquisition
 */
void dvz_swapchain_acquire(
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
void dvz_swapchain_present(
    DvzSwapchain* swapchain, uint32_t queue_idx, DvzSemaphores* semaphores,
    uint32_t semaphore_idx);

/**
 * Destroy a swapchain
 *
 * !!! warning
 *     This function must imperatively be called *before* destroying the swapchain's surface.
 *
 * @param swapchain the swapchain
 */
void dvz_swapchain_destroy(DvzSwapchain* swapchain);



/*************************************************************************************************/
/*  Commands                                                                                     */
/*************************************************************************************************/

/**
 * Create a set of command buffers.
 *
 * The status is INIT when the command buffers are initialized, and CREATED when they are filled.
 *
 * !!! note
 *     We use the following convention in vklite and elsewhere in datoviz: the queue #0 **must**
 *     support transfer tasks. This convention makes the implementation a bit simpler.
 *     This convention is respected by the context module, where the first default queue
 *     is the transfer queue, dedicated to transfer tasks.
 *
 * @param gpu the GPU
 * @param queue the queue index within the GPU
 * @param count the number of command buffers to create
 * @returns the set of command buffers
 */
DvzCommands dvz_commands(DvzGpu* gpu, uint32_t queue, uint32_t count);

/**
 * Start recording a command buffer.
 *
 * @param cmds the set of command buffers
 * @param idx the index of the command buffer to begin recording on
 */
void dvz_cmd_begin(DvzCommands* cmds, uint32_t idx);

/**
 * Stop recording a command buffer.
 *
 * @param cmds the set of command buffers
 * @param idx the index of the command buffer to stop the recording on
 */
void dvz_cmd_end(DvzCommands* cmds, uint32_t idx);

/**
 * Reset a command buffer.
 *
 * @param cmds the set of command buffers
 * @param idx the index of the command buffer to reset
 */
void dvz_cmd_reset(DvzCommands* cmds, uint32_t idx);

/**
 * Free a set of command buffers.
 *
 * @param cmds the set of command buffers
 */
void dvz_cmd_free(DvzCommands* cmds);

/**
 * Submit a command buffer on its queue with inefficient full synchronization.
 *
 * This function is relatively inefficient because it calls `dvz_queue_wait()`.
 *
 * @param cmds the set of command buffers
 * @param idx the index of the command buffer to submit
 */
void dvz_cmd_submit_sync(DvzCommands* cmds, uint32_t idx);

/**
 * Destroy a set of command buffers.
 *
 * @param cmds the set of command buffers
 */
void dvz_commands_destroy(DvzCommands* cmds);



/*************************************************************************************************/
/*  Buffers                                                                                      */
/*************************************************************************************************/

/**
 * Initialize a GPU buffer.
 *
 * @param gpu the GPU
 * @returns the buffer
 */
DvzBuffer dvz_buffer(DvzGpu* gpu);

/**
 * Set the buffer size.
 *
 * @param buffer the buffer
 * @param size the buffer size, in bytes
 */
void dvz_buffer_size(DvzBuffer* buffer, VkDeviceSize size);

/**
 * Set the buffer type.
 *
 * @param buffer the buffer
 * @param type the buffer type
 */
void dvz_buffer_type(DvzBuffer* buffer, DvzBufferType type);

/**
 * Set the buffer usage.
 *
 * @param buffer the buffer
 * @param usage the buffer usage
 */
void dvz_buffer_usage(DvzBuffer* buffer, VkBufferUsageFlags usage);

/**
 * Set the buffer VMA usage.
 *
 * @param buffer the buffer
 * @param usage the buffer usage
 */
void dvz_buffer_vma_usage(DvzBuffer* buffer, VmaMemoryUsage vma_usage);

/**
 * Set the buffer memory properties.
 *
 * @param buffer the buffer
 * @param memory the memory properties
 */
void dvz_buffer_memory(DvzBuffer* buffer, VkMemoryPropertyFlags memory);

/**
 * Set the buffer queue access.
 *
 * @param buffer the buffer
 * @param queue_idx the queue index
 */
void dvz_buffer_queue_access(DvzBuffer* buffer, uint32_t queue_idx);

/**
 * Create the buffer after it has been set.
 *
 * @param buffer the buffer
 */
void dvz_buffer_create(DvzBuffer* buffer);

/**
 * Resize a buffer.
 *
 * @param buffer the buffer
 * @param size the new buffer size, in bytes
 */
void dvz_buffer_resize(DvzBuffer* buffer, VkDeviceSize size);

/**
 * Memory-map a buffer.
 *
 * @param buffer the buffer
 * @param offset the offset within the buffer, in bytes
 * @param size the size to map, in bytes
 */
void* dvz_buffer_map(DvzBuffer* buffer, VkDeviceSize offset, VkDeviceSize size);

/**
 * Unmap a buffer.
 *
 * @param buffer the buffer
 */
void dvz_buffer_unmap(DvzBuffer* buffer);

/**
 * Download a buffer data to the CPU.
 *
 * !!! important
 *     This function does **not** use any GPU synchronization primitive: this is the responsibility
 *     of the caller. A simple (but not optimal) method is just to call the following function
 *     after every call to this function:
 *     `dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_TRANSFER);`
 *
 * @param buffer the buffer
 * @param offset the offset within the buffer, in bytes
 * @param size the size of the region to download, in bytes
 * @param[out] data (array) the buffer to download on (must be allocated with the appropriate size)
 */
void dvz_buffer_download(DvzBuffer* buffer, VkDeviceSize offset, VkDeviceSize size, void* data);

/**
 * Upload data to a GPU buffer.
 *
 * !!! important
 *     This function does **not** use any GPU synchronization primitive: this is the responsibility
 *     of the caller. A simple (but not optimal) method is just to call the following function
 *     after every call to this function:
 *     `dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_TRANSFER);`
 *
 * @param buffer the buffer
 * @param offset the offset within the buffer, in bytes
 * @param size the buffer size, in bytes
 * @param data the data to upload
 */
void dvz_buffer_upload(
    DvzBuffer* buffer, VkDeviceSize offset, VkDeviceSize size, const void* data);

/**
 * Destroy a buffer
 *
 * @param buffer the buffer
 */
void dvz_buffer_destroy(DvzBuffer* buffer);



/**
 * Create buffer regions on an existing GPU buffer.
 *
 * @param buffer the buffer
 * @param count the number of successive regions
 * @param offset the offset within the buffer
 * @param size the size of each region, in bytes
 * @param alignment the alignment requirement for the region offsets
 */
DvzBufferRegions dvz_buffer_regions(
    DvzBuffer* buffer, uint32_t count, //
    VkDeviceSize offset, VkDeviceSize size, VkDeviceSize alignment);

/**
 * Map a buffer region.
 *
 * @param br the buffer regions
 * @param idx the index of the buffer region to map
 * @param offset the offset
 * @param size the size
 */
void* dvz_buffer_regions_map(
    DvzBufferRegions* br, uint32_t idx, VkDeviceSize offset, VkDeviceSize size);

/**
 * Unmap a set of buffer regions.
 *
 * @param br the buffer regions
 */
void dvz_buffer_regions_unmap(DvzBufferRegions* br);

/**
 * Upload data to a mappable buffer region.
 *
 * @param br the set of buffer regions
 * @param idx the index of the buffer region to upload data to
 * @param offset the offset
 * @param size the size
 * @param data the data to upload
 */
void dvz_buffer_regions_upload(
    DvzBufferRegions* br, uint32_t idx, VkDeviceSize offset, VkDeviceSize size, const void* data);

/**
 * Download data to a mappable buffer region.
 *
 * @param br the set of buffer regions
 * @param idx the index of the buffer region to download data from
 * @param offset the offset
 * @param size the size
 * @param data pointer to the buffer where to download to
 */
void dvz_buffer_regions_download(
    DvzBufferRegions* br, uint32_t idx, VkDeviceSize offset, VkDeviceSize size, void* data);

/**
 * Copy data between two buffer region.
 *
 * @param src the source buffer regions
 * @param src_offset, the offset, in bytes
 * @param dst the destination buffer regions
 * @param dst_offset, the offset, in bytes
 * @param size the size, in bytes
 * @param the region idx to copy, or -1 if all regions must be copied
 */
void dvz_buffer_regions_copy(
    DvzBufferRegions* src, uint32_t src_idx, VkDeviceSize src_offset, //
    DvzBufferRegions* dst, uint32_t dst_idx, VkDeviceSize dst_offset, VkDeviceSize size);



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
DvzImages dvz_images(DvzGpu* gpu, VkImageType type, uint32_t count);

/**
 * Set the images format.
 *
 * @param images the images
 * @param format the image format
 */
void dvz_images_format(DvzImages* img, VkFormat format);

/**
 * Set the images layout.
 *
 * @param images the images
 * @param layout the image layout
 */
void dvz_images_layout(DvzImages* img, VkImageLayout layout);

/**
 * Set the images shape.
 *
 * @param images the images
 * @param shape the image shape (width, height, depth)
 */
void dvz_images_size(DvzImages* img, uvec3 shape);

/**
 * Set the images tiling.
 *
 * @param images the images
 * @param tiling the image tiling
 */
void dvz_images_tiling(DvzImages* img, VkImageTiling tiling);

/**
 * Set the images usage.
 *
 * @param images the images
 * @param usage the image usage
 */
void dvz_images_usage(DvzImages* img, VkImageUsageFlags usage);

/**
 * Set the images VMA usage.
 *
 * @param images the images
 * @param usage the memory usage
 */
void dvz_images_vma_usage(DvzImages* img, VmaMemoryUsage vma_usage);

/**
 * Set the images memory properties.
 *
 * @param images the images
 * @param memory the memory properties
 */
void dvz_images_memory(DvzImages* img, VkMemoryPropertyFlags memory);

/**
 * Set the images aspect.
 *
 * @param images the images
 * @param aspect the image aspect
 */
void dvz_images_aspect(DvzImages* img, VkImageAspectFlags aspect);

/**
 * Set the images queue access.
 *
 * This parameter specifies which queues may access the image from command buffers submitted to
 * them.
 *
 * @param images the images
 * @param queue_idx the queue index
 */
void dvz_images_queue_access(DvzImages* img, uint32_t queue_idx);

/**
 * Create the images after they have been set up.
 *
 * @param images the images
 */
void dvz_images_create(DvzImages* img);

/**
 * Resize images.
 *
 * !!! warning
 *     This function deletes the images contents when resizing.
 *
 * @param images the images
 * @param new_shape the new shape
 */
void dvz_images_resize(DvzImages* img, uvec3 shape);

/**
 * Transition the images to their layout after creation.
 *
 * This function performs a hard synchronization on the queue and submits a command buffer with the
 * image transition.
 *
 * @param images the images
 */
void dvz_images_transition(DvzImages* img);

/**
 * Download the data from a staging GPU image.
 *
 * @param staging the images to download the data from
 * @param idx the index of the image
 * @param bytes_per_component number of bytes per component
 * @param swizzle whether the RGB(A) values need to be transposed
 * @param has_alpha whether there is an Alpha component in the output buffer
 * @param[out] out (array) the buffer that will be filled with the image data (must be already
 * allocated)
 */
void dvz_images_download(
    DvzImages* staging, uint32_t idx, VkDeviceSize bytes_per_component, bool swizzle,
    bool has_alpha, void* out);

/**
 * Copy part of an image to another image.
 *
 * This function does not involve CPU-GPU data transfers.
 *
 * @param src the source texture
 * @param src_offset offset within the source texture
 * @param dst the target texture
 * @param dst_offset offset within the target texture
 * @param shape shape of the part of the texture to copy
 */
void dvz_images_copy(
    DvzImages* src, uvec3 src_offset, DvzImages* dst, uvec3 dst_offset, uvec3 shape);

/**
 * Copy a buffer to an image.
 *
 * @param img the source image
 * @param tex_offset offset within the source texture
 * @param shape shape of the part of the texture to copy
 * @param br the buffer regions
 * @param buf_offset the offset within the buffer region
 * @param size the size of the data to copy
 */
void dvz_images_copy_from_buffer(
    DvzImages* img, uvec3 tex_offset, uvec3 shape, //
    DvzBufferRegions br, VkDeviceSize buf_offset, VkDeviceSize size);

/**
 * Copy an image to a buffer.
 *
 * @param img the source image
 * @param tex_offset offset within the source texture
 * @param shape shape of the part of the texture to copy
 * @param br the buffer regions
 * @param buf_offset the offset within the buffer region
 * @param size the size of the data to copy
 */
void dvz_images_copy_to_buffer(
    DvzImages* img, uvec3 tex_offset, uvec3 shape, //
    DvzBufferRegions br, VkDeviceSize buf_offset, VkDeviceSize size);

/**
 * Destroy images.
 *
 * @param images the images
 */
void dvz_images_destroy(DvzImages* img);



/*************************************************************************************************/
/*  Sampler                                                                                      */
/*************************************************************************************************/

/**
 * Initialize a texture sampler.
 *
 * @param gpu the GPU
 * @returns the sampler object
 */
DvzSampler dvz_sampler(DvzGpu* gpu);

/**
 * Set the sampler min filter.
 *
 * @param sampler the sampler
 * @param filter the filter
 */
void dvz_sampler_min_filter(DvzSampler* sampler, VkFilter filter);

/**
 * Set the sampler mag filter.
 *
 * @param sampler the sampler
 * @param filter the filter
 */
void dvz_sampler_mag_filter(DvzSampler* sampler, VkFilter filter);

/**
 * Set the sampler address mode
 *
 * @param sampler the sampler
 * @param axis the sampler axis
 * @param address_mode the address mode
 */
void dvz_sampler_address_mode(
    DvzSampler* sampler, DvzSamplerAxis axis, VkSamplerAddressMode address_mode);

/**
 * Create the sampler after it has been set up.
 *
 * @param sampler the sampler
 */
void dvz_sampler_create(DvzSampler* sampler);

/**
 * Destroy a sampler
 *
 * @param sampler the sampler
 */
void dvz_sampler_destroy(DvzSampler* sampler);



/*************************************************************************************************/
/*  Slots                                                                                        */
/*************************************************************************************************/

/**
 * Initialize pipeline dslots (aka Vulkan descriptor set layout).
 *
 * @param gpu the GPU
 * @returns the dslots
 */
DvzSlots dvz_slots(DvzGpu* gpu);

/**
 * Set the dslots descriptor.
 *
 * @param dslots the dslots
 * @param idx the slot index to set up
 * @param type the descriptor type for that slot
 */
void dvz_slots_binding(DvzSlots* dslots, uint32_t idx, VkDescriptorType type);

/**
 * Set up push constants.
 *
 * @param dslots the dslots
 * @param stages the shader stages that will access the push constant
 * @param offset the push constant offset, in bytes
 * @param size the push constant size, in bytes
 */
void dvz_slots_push(
    DvzSlots* dslots, VkShaderStageFlagBits stages, VkDeviceSize offset, VkDeviceSize size);

/**
 * Create the dslots after they have been set up.
 *
 * @param dslots the dslots
 */
void dvz_slots_create(DvzSlots* dslots);

/**
 * Destroy the dslots
 *
 * @param dslots the dslots
 */
void dvz_slots_destroy(DvzSlots* dslots);



/*************************************************************************************************/
/*  Bindings                                                                                     */
/*************************************************************************************************/

/**
 * Initialize descriptors corresponding to dslots.
 *
 * @param dslots the dslots
 * @param dset_count the number of descriptor sets (number of swapchain images)
 */
DvzDescriptors dvz_descriptors(DvzSlots* dslots, uint32_t dset_count);

/**
 * Bind a buffer to a slot.
 *
 * @param descriptors the descriptors
 * @param idx the slot index
 * @param br the buffer regions to bind to that slot
 */
void dvz_descriptors_buffer(DvzDescriptors* descriptors, uint32_t idx, DvzBufferRegions br);

/**
 * Bind a texture to a slot.
 *
 * @param descriptors the descriptors
 * @param idx the slot index
 * @param br the texture to bind to that slot
 */
void dvz_descriptors_texture(
    DvzDescriptors* descriptors, uint32_t idx, DvzImages* img, DvzSampler* sampler);

/**
 * Update the descriptors after the buffers/textures have been set up.
 *
 * @param descriptors the descriptors
 */
void dvz_descriptors_update(DvzDescriptors* descriptors);

/**
 * Destroy descriptors.
 *
 * @param descriptors the descriptors
 */
void dvz_descriptors_destroy(DvzDescriptors* descriptors);



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
DvzCompute dvz_compute(DvzGpu* gpu, const char* shader_path);

/**
 * Create a compute pipeline after it has been set up.
 *
 * @param compute the compute pipeline
 */
void dvz_compute_create(DvzCompute* compute);

/**
 * Set the GLSL code directly (the library will compile it automatically to SPIRV).
 *
 * @param compute the compute pipeline
 * @param code the GLSL code defining the compute shader
 */
void dvz_compute_code(DvzCompute* compute, const char* code);

/**
 * Declare a slot for the compute pipeline.
 *
 * @param compute the compute pipeline
 * @param idx the slot index
 * @param type the descriptor type
 */
void dvz_compute_slot(DvzCompute* compute, uint32_t idx, VkDescriptorType type);

/**
 * Set up push constant.
 *
 * @param compute the compute pipeline
 * @param stages the stages that will need to access the push constant
 * @param offset the push constant offset, in bytes
 * @param size the push constant size, in bytes
 */
void dvz_compute_push(
    DvzCompute* compute, VkShaderStageFlagBits stages, VkDeviceSize offset, VkDeviceSize size);

/**
 * Associate a descriptors object to a compute pipeline.
 *
 * @param compute the compute pipeline
 * @param descriptors the descriptors
 */
void dvz_compute_descriptors(DvzCompute* compute, DvzDescriptors* descriptors);

/**
 * Destroy a compute pipeline.
 *
 * @param compute the compute pipeline
 */
void dvz_compute_destroy(DvzCompute* compute);



/*************************************************************************************************/
/*  Pipeline                                                                                     */
/*************************************************************************************************/

/**
 * Initialize a graphics pipeline.
 *
 * @param gpu the GPU
 * @returns the graphics pipeline
 */
DvzGraphics dvz_graphics(DvzGpu* gpu);

/**
 * Set the renderpass of a graphics pipeline.
 *
 * @param graphics the graphics pipeline
 * @param renderpass the render pass
 * @param subpass the subpass index
 */
void dvz_graphics_renderpass(DvzGraphics* graphics, DvzRenderpass* renderpass, uint32_t subpass);

/**
 * NOTE: useless, to delete?
 * Set the graphics drawing mode.
 *
 * @param graphics the graphics pipeline
 * @param drawing the drawing flags: direct/indirect, flat/indexed
 */
void dvz_graphics_drawing(DvzGraphics* graphics, int drawing);

/**
 * Set the graphics pipeline primitive topology
 *
 * @param graphics the graphics pipeline
 * @param topology the primitive topology
 */
void dvz_graphics_primitive(DvzGraphics* graphics, VkPrimitiveTopology topology);

/**
 * Set the GLSL code of a graphics pipeline.
 *
 * @param graphics the graphics pipeline
 * @param stage the shader stage
 * @param code the GLSL code of the shader
 */
void dvz_graphics_shader_glsl(
    DvzGraphics* graphics, VkShaderStageFlagBits stage, const char* code);

/**
 * Set the SPIRV code of a graphics pipeline.
 *
 * @param graphics the graphics pipeline
 * @param stage the shader stage
 * @param size the size of the SPIRV buffer, in bytes
 * @param buffer the binary buffer with the SPIRV code
 */
void dvz_graphics_shader_spirv(
    DvzGraphics* graphics, VkShaderStageFlagBits stage, VkDeviceSize size, const uint32_t* buffer);

/**
 * Set the path to a shader for a graphics pipeline.
 *
 * @param graphics the graphics pipeline
 * @param stage the shader stage
 * @param shader_path the path to the `.spirv` shader file
 */
void dvz_graphics_shader(
    DvzGraphics* graphics, VkShaderStageFlagBits stage, const char* shader_path);

/**
 * Set the vertex binding.
 *
 * @param graphics the graphics pipeline
 * @param binding the binding index
 * @param stride the stride in the vertex buffer, in bytes
 * @param input_rate the vertex input rate, VK_VERTEX_INPUT_RATE_VERTEX|INSTANCE
 */
void dvz_graphics_vertex_binding(
    DvzGraphics* graphics, uint32_t binding, VkDeviceSize stride, VkVertexInputRate input_rate);

/**
 * Add a vertex attribute.
 *
 * @param graphics the graphics pipeline
 * @param binding the binding index (as specified in the vertex shader)
 * @param location the location index (as specified in the vertex shader)
 * @param format the format
 * @param offset the offset, in bytes
 */
void dvz_graphics_vertex_attr(
    DvzGraphics* graphics, uint32_t binding, uint32_t location, VkFormat format,
    VkDeviceSize offset);

/**
 * Set the graphics blend type.
 *
 * @param graphics the graphics pipeline
 * @param blend_type the blend type
 */
void dvz_graphics_blend(DvzGraphics* graphics, DvzBlendType blend_type);

/**
 * Set the graphics color mask.
 *
 * @param graphics the graphics pipeline
 * @param mask the color mask
 */
void dvz_graphics_mask(DvzGraphics* graphics, int32_t mask);

/**
 * Set the graphics depth test.
 *
 * @param graphics the graphics pipeline
 * @param depth_test the depth test
 */
void dvz_graphics_depth_test(DvzGraphics* graphics, DvzDepthTest depth_test);

/**
 * Set whether the graphics pipeline supports picking.
 *
 * !!! note
 *     Picking support is currently all or nothing: all graphics of a canvas must either support
 *     picking or not. In addition, the canvas must have been created with the
 *     DVZ_CANVAS_FLAGS_PICK flag.
 *
 * @param graphics the graphics pipeline
 * @param support_pick whether the graphics pipeline supports picking
 */
void dvz_graphics_pick(DvzGraphics* graphics, bool support_pick);

/**
 * Set the graphics polygon mode.
 *
 * @param graphics the graphics pipeline
 * @param polygon_mode the polygon mode
 */
void dvz_graphics_polygon_mode(DvzGraphics* graphics, VkPolygonMode polygon_mode);

/**
 * Set the graphics cull mode.
 *
 * @param graphics the graphics pipeline
 * @param cull_mode the cull mode
 */
void dvz_graphics_cull_mode(DvzGraphics* graphics, VkCullModeFlags cull_mode);

/**
 * Set the graphics front face.
 *
 * @param graphics the graphics pipeline
 * @param front_face the front face
 */
void dvz_graphics_front_face(DvzGraphics* graphics, VkFrontFace front_face);

/**
 * Create a graphics pipeline after it has been set up.
 *
 * @param graphics the graphics pipeline
 */
void dvz_graphics_create(DvzGraphics* graphics);

/**
 * Set a descriptor slot for a graphics pipeline.
 *
 * @param graphics the graphics pipeline
 * @param idx the slot index
 * @param type the descriptor type
 */
void dvz_graphics_slot(DvzGraphics* graphics, uint32_t idx, VkDescriptorType type);

/**
 * Set a graphics pipeline push constant.
 *
 * @param graphics the graphics pipeline
 * @param stages the shader stages that will access the push constant
 * @param offset the push constant offset, in bytes
 * @param offset the push size, in bytes
 */
void dvz_graphics_push(
    DvzGraphics* graphics, VkShaderStageFlagBits stages, VkDeviceSize offset, VkDeviceSize size);

/**
 * Declare a specialization constant for a graphics pipeline.
 *
 * @param graphics the graphics pipeline
 * @param stage the shader stage
 * @param constant_id the constant ID
 * @param size the size of the value within the specialization data buffer
 * @param data the specialization data buffer
 */
void dvz_graphics_specialization(
    DvzGraphics* graphics, VkShaderStageFlagBits stage, uint32_t constant_id, //
    VkDeviceSize size, void* data);

/**
 * Destroy a graphics pipeline.
 *
 * @param graphics the graphics pipeline
 */
void dvz_graphics_destroy(DvzGraphics* graphics);



/*************************************************************************************************/
/*  Barrier                                                                                      */
/*************************************************************************************************/

/**
 * Initialize a synchronization barrier (usedwithin a command buffer).
 *
 * @param gpu the GPU
 * @returns the barrier
 */
DvzBarrier dvz_barrier(DvzGpu* gpu);

/**
 * Set the barrier stages.
 *
 * @param barrier the barrier
 * @param src_stage the source stage
 * @param dst_stage the destination stage
 */
void dvz_barrier_stages(
    DvzBarrier* barrier, VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage);

/**
 * Set the barrier buffer.
 *
 * @param barrier the barrier
 * @param br the buffer regions
 */
void dvz_barrier_buffer(DvzBarrier* barrier, DvzBufferRegions br);

/**
 * Set the barrier buffer queue.
 *
 * @param barrier the barrier
 * @param src_queue the source queue index
 * @param dst_queue the destination queue index
 */
void dvz_barrier_buffer_queue(DvzBarrier* barrier, uint32_t src_queue, uint32_t dst_queue);

/**
 * Set the barrier buffer access.
 *
 * @param barrier the barrier
 * @param src_access the source access flags
 * @param dst_access the destination access flags
 */
void dvz_barrier_buffer_access(
    DvzBarrier* barrier, VkAccessFlags src_access, VkAccessFlags dst_access);

/**
 * Set the barrier images.
 *
 * @param barrier the barrier
 * @param images the images
 */
void dvz_barrier_images(DvzBarrier* barrier, DvzImages* img);

/**
 * Set the barrier images layout.
 *
 * @param barrier the barrier
 * @param src_layout the source layout
 * @param dst_layout the destination layout
 */
void dvz_barrier_images_layout(
    DvzBarrier* barrier, VkImageLayout src_layout, VkImageLayout dst_layout);

/**
 * Set the barrier images aspect.
 *
 * @param barrier the barrier
 * @param aspect the aspect
 */
void dvz_barrier_images_aspect(DvzBarrier* barrier, VkImageAspectFlags aspect);

/**
 * Set the barrier images queue.
 *
 * @param barrier the barrier
 * @param src_queue the source queue index
 * @param dst_queue the destination queue index
 */
void dvz_barrier_images_queue(DvzBarrier* barrier, uint32_t src_queue, uint32_t dst_queue);

/**
 * Set the barrier images access.
 *
 * @param barrier the barrier
 * @param src_access the source access flags
 * @param dst_access the destination access flags
 */
void dvz_barrier_images_access(
    DvzBarrier* barrier, VkAccessFlags src_access, VkAccessFlags dst_access);



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
DvzSemaphores dvz_semaphores(DvzGpu* gpu, uint32_t count);

/**
 * Recreate semaphores.
 *
 * @param semaphores the semaphores
 */
void dvz_semaphores_recreate(DvzSemaphores* semaphores);

/**
 * Destroy semaphores.
 *
 * @param semaphores the semaphores
 */
void dvz_semaphores_destroy(DvzSemaphores* semaphores);



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
DvzFences dvz_fences(DvzGpu* gpu, uint32_t count, bool signaled);

/**
 * Copy a fence from a set of fences to another.
 *
 * @param src_fences the source fences
 * @param src_idx the fence index within the source fences
 * @param dst_fences the destination fences
 * @param dst_idx the fence index within the destination fences
 */
void dvz_fences_copy(
    DvzFences* src_fences, uint32_t src_idx, DvzFences* dst_fences, uint32_t dst_idx);

/**
 * Wait on the GPU until a fence is signaled.
 *
 * @param fences the fences
 * @param idx the fence index
 */
void dvz_fences_wait(DvzFences* fences, uint32_t idx);

/**
 * Return whether a fence is ready.
 *
 * @param fences the fences
 * @param idx the fence index
 */
bool dvz_fences_ready(DvzFences* fences, uint32_t idx);

/**
 * Rset the state of a fence.
 *
 * @param fences the fences
 * @param idx the fence index
 */
void dvz_fences_reset(DvzFences* fences, uint32_t idx);

/**
 * Destroy fences.
 *
 * @param fences the fences
 */
void dvz_fences_destroy(DvzFences* fences);



/*************************************************************************************************/
/*  Renderpass                                                                                   */
/*************************************************************************************************/

/**
 * Initialize a render pass.
 *
 * @param gpu the GPU
 * @returns the render pass
 */
DvzRenderpass dvz_renderpass(DvzGpu* gpu);

/**
 * Set the clear value of a render pass.
 *
 * @param renderpass the render pass
 * @param value the clear value
 */
void dvz_renderpass_clear(DvzRenderpass* renderpass, VkClearValue value);

/**
 * Specify a render pass attachment.
 *
 * @param renderpass the render pass
 * @param idx the attachment index
 * @param type the attachment type
 * @param format the attachment image format
 * @param ref_layout the image layout
 */
void dvz_renderpass_attachment(
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
void dvz_renderpass_attachment_layout(
    DvzRenderpass* renderpass, uint32_t idx, VkImageLayout src_layout, VkImageLayout dst_layout);

/**
 * Set the attachment load and store operations.
 *
 * @param renderpass the render pass
 * @param idx the attachment index
 * @param load_op the load operation
 * @param store_op the store operation
 */
void dvz_renderpass_attachment_ops(
    DvzRenderpass* renderpass, uint32_t idx, //
    VkAttachmentLoadOp load_op, VkAttachmentStoreOp store_op);

/**
 * Set a subpass attachment.
 *
 * @param renderpass the render pass
 * @param subpass_idx the subpass index
 * @param attachment_idx the attachment index
 */
void dvz_renderpass_subpass_attachment(
    DvzRenderpass* renderpass, uint32_t subpass_idx, uint32_t attachment_idx);

/**
 * Set a subpass dependency.
 *
 * @param renderpass the render pass
 * @param dependency_idx the dependency index
 * @param src_subpass the source subpass index
 * @param dst_subpass the destination subpass index
 */
void dvz_renderpass_subpass_dependency(
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
void dvz_renderpass_subpass_dependency_access(
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
void dvz_renderpass_subpass_dependency_stage(
    DvzRenderpass* renderpass, uint32_t dependency_idx, //
    VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage);

/**
 * Create a render pass after it has been set up.
 *
 * @param renderpass the render pass
 */
void dvz_renderpass_create(DvzRenderpass* renderpass);

/**
 * Destroy a render pass.
 *
 * @param renderpass the render pass
 */
void dvz_renderpass_destroy(DvzRenderpass* renderpass);



/*************************************************************************************************/
/*  Framebuffers                                                                                 */
/*************************************************************************************************/

/**
 * Initialize a set of framebuffers.
 *
 * @param gpu the GPU
 * @returns the framebuffers
 */
DvzFramebuffers dvz_framebuffers(DvzGpu* gpu);

/**
 * Set framebuffers attachment.
 *
 * @param framebuffers the framebuffers
 * @param attachment_idx the attachment index
 * @param images the images
 */

void dvz_framebuffers_attachment(
    DvzFramebuffers* framebuffers, uint32_t attachment_idx, DvzImages* img);

/**
 * Create a set of framebuffers after it has been set up.
 *
 * @param framebuffers the framebuffers
 * @param renderpass the render pass
 */
void dvz_framebuffers_create(DvzFramebuffers* framebuffers, DvzRenderpass* renderpass);

/**
 * Destroy a set of framebuffers.
 *
 * @param framebuffers the framebuffers
 */
void dvz_framebuffers_destroy(DvzFramebuffers* framebuffers);



/*************************************************************************************************/
/*  Submit                                                                                       */
/*************************************************************************************************/

/**
 * Create a submit object, used to submit command buffers to a GPU queue.
 *
 * @param gpu the GPU
 * @returns the submit
 */
DvzSubmit dvz_submit(DvzGpu* gpu);

/**
 * Set the command buffers to submit.
 *
 * @param submit the submit object
 * @param cmds the set of command buffers
 */
void dvz_submit_commands(DvzSubmit* submit, DvzCommands* commands);

/**
 * Set the wait semaphores
 *
 * @param submit the submit object
 * @param stage the pipeline stage
 * @param semaphores the set of semaphores to wait on
 * @param idx the semaphore index to wait on
 */
void dvz_submit_wait_semaphores(
    DvzSubmit* submit, VkPipelineStageFlags stage, DvzSemaphores* semaphores, uint32_t idx);

/**
 * Set the signal semaphores
 *
 * @param submit the submit object
 * @param semaphores the set of semaphores to signal after the commands have completed
 * @param idx the semaphore index to signal
 */
void dvz_submit_signal_semaphores(DvzSubmit* submit, DvzSemaphores* semaphores, uint32_t idx);

/**
 * Submit the command buffers to their queue.
 *
 * @param submit the submit object
 * @param cmd_idx the command buffer index to submit
 * @param fences the fences to signal after completion
 * @param fence_idx the fence index to signal
 */
void dvz_submit_send(DvzSubmit* submit, uint32_t cmd_idx, DvzFences* fences, uint32_t fence_idx);

/**
 * Reset a submit object.
 *
 * @param submit the submit object
 */
void dvz_submit_reset(DvzSubmit* submit);



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
void dvz_cmd_begin_renderpass(
    DvzCommands* cmds, uint32_t idx, DvzRenderpass* renderpass, DvzFramebuffers* framebuffers);

/**
 * End a render pass.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 */
void dvz_cmd_end_renderpass(DvzCommands* cmds, uint32_t idx);

/**
 * Launch a compute task.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param compute the computer pipeline
 * @param size the task shape
 */
void dvz_cmd_compute(DvzCommands* cmds, uint32_t idx, DvzCompute* compute, uvec3 size);

/**
 * Register a barrier.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param barrier the barrier
 */
void dvz_cmd_barrier(DvzCommands* cmds, uint32_t idx, DvzBarrier* barrier);

/**
 * Copy a GPU buffer to a GPU image.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param buffer the buffer
 * @param buf_offset the buffer offset
 * @param images the image
 * @param tex_offset the texture offset
 * @param shape the texture shape
 */
void dvz_cmd_copy_buffer_to_image(
    DvzCommands* cmds, uint32_t idx,            //
    DvzBuffer* buffer, VkDeviceSize buf_offset, //
    DvzImages* img, uvec3 tex_offset, uvec3 shape);

/**
 * Copy a GPU image to a GPU buffer.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param tex_offset the texture offset
 * @param shape the texture shape
 * @param images the image
 * @param buffer the buffer
 * @param buf_offset the buffer offset
 */
void dvz_cmd_copy_image_to_buffer(
    DvzCommands* cmds, uint32_t idx,               //
    DvzImages* img, uvec3 tex_offset, uvec3 shape, //
    DvzBuffer* buffer, VkDeviceSize buf_offset);

/**
 * Copy a GPU image to another.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param src_img the source image
 * @param src_offset the offset in the source image
 * @param dst_img the destination image
 * @param dst_offset the offset in the target image
 * @param shape the shape of the region to copy
 */
void dvz_cmd_copy_image_region(
    DvzCommands* cmds, uint32_t idx,      //
    DvzImages* src_img, ivec3 src_offset, //
    DvzImages* dst_img, ivec3 dst_offset, //
    uvec3 shape);

/**
 * Copy a GPU image to another.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param src_img the source image
 * @param dst_img the destination image
 */
void dvz_cmd_copy_image(DvzCommands* cmds, uint32_t idx, DvzImages* src_img, DvzImages* dst_img);

/**
 * Set the viewport.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param viewport the viewport
 */
void dvz_cmd_viewport(DvzCommands* cmds, uint32_t idx, VkViewport viewport);

/**
 * Bind a graphics pipeline.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param graphics the graphics pipeline
 */
void dvz_cmd_bind_graphics(DvzCommands* cmds, uint32_t idx, DvzGraphics* graphics);

/**
 * Bind descriptors.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param descriptors the descriptors associated to the pipeline
 * @param dynamic_idx the dynamic uniform buffer index
 */
void dvz_cmd_bind_descriptors(
    DvzCommands* cmds, uint32_t idx, DvzDescriptors* descriptors, uint32_t dynamic_idx);

/**
 * Bind a vertex buffer.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param br the buffer regions
 * @param offset the offset within the buffer regions, in bytes
 */
void dvz_cmd_bind_vertex_buffer(
    DvzCommands* cmds, uint32_t idx, uint32_t binding_count, DvzBufferRegions* brs,
    VkDeviceSize* offsets);

/**
 * Bind an index buffer.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param br the buffer regions
 * @param offset the offset within the buffer regions, in bytes
 */
void dvz_cmd_bind_index_buffer(
    DvzCommands* cmds, uint32_t idx, DvzBufferRegions br, VkDeviceSize offset);

/**
 * Direct draw.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param first_vertex index of the first vertex
 * @param vertex_count number of vertices to draw
 */
void dvz_cmd_draw(
    DvzCommands* cmds, uint32_t idx, uint32_t first_vertex, uint32_t vertex_count,
    uint32_t first_instance, uint32_t instance_count);

/**
 * Direct indexed draw.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param first_index index of the first index
 * @param vertex_offset offset of the vertex
 * @param index_count number of indices to draw
 */
void dvz_cmd_draw_indexed(
    DvzCommands* cmds, uint32_t idx, uint32_t first_index, uint32_t vertex_offset,
    uint32_t index_count, uint32_t first_instance, uint32_t instance_count);

/**
 * Indirect draw.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param indirect buffer regions with the indirect draw info
 */
void dvz_cmd_draw_indirect(
    DvzCommands* cmds, uint32_t idx, DvzBufferRegions indirect, uint32_t draw_count);

/**
 * Indirect indexed draw.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param indirect buffer regions with the indirect draw info
 */
void dvz_cmd_draw_indexed_indirect(
    DvzCommands* cmds, uint32_t idx, DvzBufferRegions indirect, uint32_t draw_count);

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
void dvz_cmd_copy_buffer(
    DvzCommands* cmds, uint32_t idx,             //
    DvzBuffer* src_buf, VkDeviceSize src_offset, //
    DvzBuffer* dst_buf, VkDeviceSize dst_offset, //
    VkDeviceSize size);

/**
 * Push constants.
 *
 * @param cmds the set of command buffers to record
 * @param idx the index of the command buffer to record
 * @param dslots the dslots
 * @param stages the shader stages that have access to the push constant
 * @param offset the offset in the push constant, in bytes
 * @param size the size in the push constant, in bytes
 * @param data the data to send via the push constant
 */
void dvz_cmd_push(
    DvzCommands* cmds, uint32_t idx, DvzSlots* dslots, VkShaderStageFlagBits stages, //
    VkDeviceSize offset, VkDeviceSize size, const void* data);



EXTERN_C_OFF

#endif
