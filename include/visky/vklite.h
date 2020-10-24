#ifndef VKY_VKLITE_HEADER
#define VKY_VKLITE_HEADER

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <vulkan/vulkan.h>

#include "common.h"

BEGIN_INCL_NO_WARN
#include <cglm/struct.h>
END_INCL_NO_WARN



/*************************************************************************************************/
/*  Type definitions                                                                             */
/*************************************************************************************************/

typedef struct VkyScene VkyScene;
typedef struct VkyGui VkyGui;
typedef struct VkyGpu VkyGpu;
typedef struct VkyPrompt VkyPrompt;
typedef struct VkyCanvas VkyCanvas;
typedef struct VkyApp VkyApp;
typedef struct VkyQueueFamilyIndices VkyQueueFamilyIndices;
typedef struct VkyDrawSync VkyDrawSync;
typedef struct VkyViewport VkyViewport;
typedef struct VkyDefaultVertex VkyDefaultVertex;
typedef struct VkyVertexLayout VkyVertexLayout;
typedef struct VkyResourceLayout VkyResourceLayout;
typedef struct VkyShaders VkyShaders;
typedef struct VkyGraphicsPipelineParams VkyGraphicsPipelineParams;
typedef struct VkyGraphicsPipeline VkyGraphicsPipeline;
typedef struct VkyBuffer VkyBuffer;
typedef struct VkyBufferRegion VkyBufferRegion;
typedef struct VkyTextureParams VkyTextureParams;
typedef struct VkyTexture VkyTexture;
typedef struct VkyUniformBuffer VkyUniformBuffer;
typedef struct VkyWindowSize VkyWindowSize;
typedef struct VkyEventController VkyEventController;
typedef struct VkyComputePipeline VkyComputePipeline;

typedef void (*VkyWaitCallback)(VkyCanvas*);
typedef void (*VkyCommandBufferCallback)(VkyCanvas*, VkCommandBuffer);
typedef void (*VkyResizeCallback)(VkyCanvas*);

typedef struct VkyCloseCallbackStruct VkyCloseCallbackStruct;
typedef void (*VkyCloseCallback)(VkyCanvas*, void*);



/*************************************************************************************************/
/*  Misc structs                                                                                 */
/*************************************************************************************************/

struct VkyCloseCallbackStruct
{
    VkyCloseCallback callback;
    void* data;
};


struct VkyQueueFamilyIndices
{
    uint32_t graphics_family;
    uint32_t present_family;
    uint32_t compute_family;
};



struct VkyDrawSync
{
    // Synchronization primitives, initialized with calloc.
    VkSemaphore* image_available_semaphores;
    VkSemaphore* render_finished_semaphores;
    VkFence* in_flight_fences;
    VkFence* images_in_flight;
};



struct VkyWindowSize
{
    // uint32_t w, h, lw, lh;
    uint32_t window_width;
    uint32_t window_height;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    double content_scale;
    bool resized;
};



struct VkyViewport
{
    VkyCanvas* canvas;
    float x, y, w, h;
};



struct VkyDefaultVertex
{
    vec3 pos;
    vec4 color;
};



/*************************************************************************************************/
/*  GPU                                                                                          */
/*************************************************************************************************/

struct VkyGpu
{
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkPhysicalDevice physical_device;

    // Created after the creation of a window:
    VkDevice device;
    VkPhysicalDeviceProperties device_properties;
    VkPhysicalDeviceFeatures device_features;
    VkPhysicalDeviceMemoryProperties memory_properties;

    VkyQueueFamilyIndices queue_indices;

    // Queue.
    VkQueue graphics_queue;
    VkQueue present_queue;
    VkQueue compute_queue;

    // Used for graphics/compute interactions.
    // NOTE: should this move to VkyCanvas in order to support multiple canvases??
    VkSemaphore graphics_semaphore;
    VkSemaphore compute_semaphore;
    VkCommandBuffer compute_command_buffer;

    bool has_validation;
    bool has_compute;
    bool has_graphics;

    VkCommandPool command_pool;
    VkCommandPool compute_command_pool;
    VkDescriptorPool descriptor_pool;

    VkPipelineCache pipeline_cache;
    VkAllocationCallbacks* allocator;

    uint32_t image_count;
    VkFormat image_format;

    // By convention, the first buffer is the indirect draw buffer.
    uint32_t buffer_count;
    VkyBuffer* buffers;

    uint32_t texture_count;
    VkyTexture* textures; // By convention, the first texture is the colormap texture
};



/*************************************************************************************************/
/*  Canvas                                                                                       */
/*************************************************************************************************/

struct VkyCanvas
{
    VkyApp* app;
    void* window; // backend-dependent
    VkyGpu* gpu;
    VkyScene* scene;

    VkyEventController* event_controller;

    double dpi_factor;
    double local_time; // local time
    double dt;         // time since last frame

    uint32_t current_frame;
    uint32_t image_index;
    uint32_t image_count;

    uint64_t frame_count;
    uint64_t fps;

    VkSurfaceKHR surface;
    // VkExtent2D extent;
    // VkyWindowSize window_size;
    VkyWindowSize size;

    VkFormat depth_format;
    VkFormat image_format;
    VkImage depth_image;
    VkImageView depth_image_view;
    VkDeviceMemory depth_image_memory;

    VkRenderPass render_pass;
    VkRenderPass live_render_pass;

    VkSwapchainKHR swapchain;

    VkImage* images;             // One per swap image.
    VkImageView* image_views;    // One per swap image.
    VkDeviceMemory image_memory; // Only used with offscreen rendering.
    VkFramebuffer* framebuffers;

    // Command buffers.
    VkCommandBuffer* command_buffers; // 1 per swap chain
    uint32_t current_command_buffer_index;
    // Used by Dear ImGui which updates the "live" command buffer at every frame
    VkCommandBuffer* live_command_buffers;

    VkyComputePipeline* compute_pipeline;

    VkyDrawSync draw_sync;

    VkyGui** guis;
    uint32_t gui_count;
    VkyPrompt* prompt;

    VkyCommandBufferCallback cb_fill_command_buffer;
    VkyCommandBufferCallback cb_fill_live_command_buffer;
    VkyResizeCallback cb_resize;

    VkyCloseCallbackStruct cb_close;

    bool is_offscreen;
    bool need_recreation;
    bool need_refill;
    bool need_data_upload;
    bool to_close;
};



/*************************************************************************************************/
/*  Pipeline                                                                                     */
/*************************************************************************************************/

struct VkyVertexLayout
{
    VkyGpu* gpu;
    uint32_t attribute_count;
    VkFormat* attribute_formats;
    uint32_t* attribute_offsets;
    uint32_t binding;
    uint32_t stride;
};



struct VkyResourceLayout
{
    VkyGpu* gpu;
    uint32_t binding_count;         // number of bindings
    uint32_t dynamic_binding_count; // number of dynamic uniform bindings
    VkDescriptorType* binding_types;
    uint32_t image_count;
};



struct VkyShaders
{
    VkyGpu* gpu;
    uint32_t shader_count;
    VkShaderModule* modules;
    VkShaderStageFlagBits* stages;
    uint32_t _index;
};



struct VkyGraphicsPipelineParams
{
    VkBool32 depth_test_enable;
    uint32_t push_constant_size;
};



struct VkyGraphicsPipeline
{
    VkyGpu* gpu;

    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
    VkPrimitiveTopology topology;

    VkyVertexLayout vertex_layout;
    VkyResourceLayout resource_layout;
    VkyShaders shaders;

    VkDescriptorPool descriptor_pool;
    VkDescriptorSetLayout descriptor_set_layout;
    uint32_t descriptor_set_count;
    VkDescriptorSet* descriptor_sets;
};



struct VkyComputePipeline
{
    VkyGpu* gpu;

    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;

    VkyResourceLayout resource_layout;
    VkShaderModule shader;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSetLayout descriptor_set_layout;
    uint32_t descriptor_set_count;
    VkDescriptorSet* descriptor_sets;
};



/*************************************************************************************************/
/*  Buffers                                                                                     */
/*************************************************************************************************/

struct VkyBuffer
{
    VkyGpu* gpu;
    VkDeviceSize size;
    VkDeviceSize allocated_size;
    VkBufferUsageFlagBits flags;

    VkBuffer raw_buffer;
    VkDeviceMemory memory;
};



struct VkyBufferRegion
{
    VkyBuffer* buffer;
    VkDeviceSize offset;
    VkDeviceSize size;
};



struct VkyTextureParams
{
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint8_t format_bytes;
    VkFormat format;
    VkFilter filter;
    VkSamplerAddressMode address_mode;
    VkImageLayout layout;
    bool enable_compute; // if true, set the VK_IMAGE_USAGE_STORAGE_BIT flag
};



struct VkyTexture
{
    VkyGpu* gpu;

    VkyTextureParams params;
    VkImage image;
    VkImageView image_view;
    VkDeviceMemory image_memory;

    VkSampler sampler;
};



struct VkyUniformBuffer
{
    VkyGpu* gpu;

    uint32_t image_count;

    uint32_t item_count;
    VkDeviceSize type_size;
    VkDeviceSize alignment;
    VkDeviceSize buffer_size;

    VkBuffer* buffers;
    VkDeviceMemory* memories;
    void** cdata; // used for pre-mapping dynamic uniform buffer arrays
    void* data;
};



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

void vky_show_fps(VkyCanvas*);

VkyQueueFamilyIndices vky_find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface);

VkCommandBuffer begin_single_time_commands(VkDevice device, VkCommandPool command_pool);
void end_single_time_commands(
    VkDevice device, VkCommandPool command_pool, VkCommandBuffer* command_buffer,
    VkQueue graphics_queue);
VkSampler create_texture_sampler(
    VkDevice device, VkFilter mag_filter, VkFilter min_filter, VkSamplerAddressMode address_mode);

void create_image(
    VkDevice device, uint32_t width, uint32_t height, uint32_t depth, VkFormat format,
    VkImageTiling tiling, VkImageUsageFlags usage, VkImage* image, VkDeviceMemory* imageMemory,
    VkMemoryPropertyFlags properties, VkPhysicalDeviceMemoryProperties memory_properties);

void create_buffer(
    VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer* buffer,
    VkDeviceMemory* bufferMemory, VkMemoryPropertyFlags properties,
    VkPhysicalDeviceMemoryProperties memory_properties);

VkImageView create_image_view(
    VkDevice device, VkImage image, VkImageViewType image_type, VkFormat format,
    VkImageAspectFlags aspect_flags);

void add_image_transition(
    VkCommandBuffer copyCmd, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout,
    VkAccessFlags src_mask, VkAccessFlags dst_mask);



/*************************************************************************************************/
/*  Uniform buffers                                                                              */
/*************************************************************************************************/

VKY_EXPORT VkyUniformBuffer
vky_create_uniform_buffer(VkyGpu* gpu, uint32_t image_count, VkDeviceSize type_size);

VKY_EXPORT VkyUniformBuffer vky_create_dynamic_uniform_buffer(
    VkyGpu* gpu, uint32_t image_count, uint32_t item_count, VkDeviceSize type_size);

VKY_EXPORT void vky_destroy_dynamic_uniform_buffer(VkyUniformBuffer* dubo);

VKY_EXPORT void vky_bind_dynamic_uniform(
    VkCommandBuffer command_buffer, VkyGraphicsPipeline* pipeline, VkyUniformBuffer* dubo,
    uint32_t current_image, uint32_t item_index);

VKY_EXPORT void* vky_get_dynamic_uniform_pointer(VkyUniformBuffer* dubo, uint32_t item_index);

VKY_EXPORT void
vky_upload_uniform_buffer(VkyUniformBuffer* ubo, uint32_t current_image, const void* data);

VKY_EXPORT void vky_upload_dynamic_uniform_buffer(VkyUniformBuffer* dubo, uint32_t current_image);

VKY_EXPORT void vky_destroy_uniform_buffer(VkyUniformBuffer* ubo);



/*************************************************************************************************/
/*  Textures                                                                                     */
/*************************************************************************************************/

VKY_EXPORT VkyTexture vky_create_texture(VkyGpu* gpu, const VkyTextureParams* params);

VKY_EXPORT void vky_upload_texture(VkyTexture* texture, const void* pixels);

VKY_EXPORT void vky_destroy_texture(VkyTexture* texture);



/*************************************************************************************************/
/*  Buffers                                                                                      */
/*************************************************************************************************/

VKY_EXPORT VkyBuffer vky_create_buffer(
    VkyGpu* gpu, VkDeviceSize size, VkBufferUsageFlagBits flags, VkMemoryPropertyFlagBits memory);

VKY_EXPORT void vky_clear_buffer(VkyBuffer*);

VKY_EXPORT void vky_clear_all_buffers(VkyGpu*);

VKY_EXPORT VkyBufferRegion vky_allocate_buffer(VkyBuffer* buffer, VkDeviceSize size);

VKY_EXPORT void
vky_bind_index_buffer(VkCommandBuffer command_buffer, VkyBufferRegion buffer, VkDeviceSize offset);

VKY_EXPORT void vky_bind_vertex_buffer(
    VkCommandBuffer command_buffer, VkyBufferRegion buffer, VkDeviceSize offset);

VKY_EXPORT void vky_upload_buffer(
    VkyBufferRegion buffer, VkDeviceSize offset, VkDeviceSize size, const void* data);

VKY_EXPORT void vky_download_buffer(VkyBufferRegion* buffer, void* out);

VKY_EXPORT void vky_destroy_buffer(VkyBuffer* buffer);



/*************************************************************************************************/
/*  Data                                                                                         */
/*************************************************************************************************/

VKY_EXPORT VkyBuffer* vky_add_buffer(VkyGpu*, VkDeviceSize size, VkBufferUsageFlagBits flags);
VKY_EXPORT VkyBuffer* vky_add_vertex_buffer(VkyGpu*, VkDeviceSize size);
VKY_EXPORT VkyBuffer* vky_add_index_buffer(VkyGpu*, VkDeviceSize size);
VKY_EXPORT VkyBuffer* vky_find_buffer(VkyGpu*, VkDeviceSize size, VkBufferUsageFlagBits flags);
VKY_EXPORT VkyTexture* vky_add_texture(VkyGpu*, const VkyTextureParams* params);

VKY_EXPORT VkyTextureParams
vky_default_texture_params(uint32_t width, uint32_t height, uint32_t depth);


/*************************************************************************************************/
/* Common textures                                                                               */
/*************************************************************************************************/

VKY_EXPORT VkyTexture* vky_get_color_texture(VkyGpu* gpu);

VKY_EXPORT VkyTexture* vky_get_font_texture(VkyGpu* gpu);



/*************************************************************************************************/
/*  Pipeline                                                                                     */
/*************************************************************************************************/

VKY_EXPORT VkyGraphicsPipeline vky_create_graphics_pipeline(
    VkyCanvas* canvas, VkPrimitiveTopology topology, VkyShaders shaders,
    VkyVertexLayout vertex_layout, VkyResourceLayout resource_layout,
    VkyGraphicsPipelineParams params);

VKY_EXPORT void
vky_bind_graphics_pipeline(VkCommandBuffer command_buffer, VkyGraphicsPipeline* pipeline);

VKY_EXPORT void vky_destroy_graphics_pipeline(VkyGraphicsPipeline* gp);

VKY_EXPORT VkShaderModule vky_create_shader_module(VkyGpu* gpu, uint32_t, const uint32_t*);
VKY_EXPORT VkShaderModule vky_create_shader_module_from_file(VkyGpu* gpu, char* filename);

VKY_EXPORT VkyShaders vky_create_shaders(VkyGpu* gpu);

VKY_EXPORT void
vky_add_shader(VkyShaders* shaders, VkShaderStageFlagBits stage, const char* filename);

VKY_EXPORT void vky_destroy_shaders(VkyShaders* shaders);

VKY_EXPORT VkyResourceLayout vky_create_resource_layout(VkyGpu* gpu, uint32_t image_count);

VKY_EXPORT void vky_add_vertex_attribute(
    VkyVertexLayout* layout, uint32_t location, VkFormat format, uint32_t offset);

VKY_EXPORT void
vky_add_resource_binding(VkyResourceLayout* layout, uint32_t binding, VkDescriptorType type);

VKY_EXPORT void vky_bind_resources(
    VkyResourceLayout resource_layout, VkDescriptorSet* descriptor_sets, void** resources);

VKY_EXPORT void vky_destroy_resource_layout(VkyResourceLayout* layout);

VKY_EXPORT VkyVertexLayout
vky_create_vertex_layout(VkyGpu* gpu, uint32_t binding, uint32_t stride);

VKY_EXPORT void vky_destroy_vertex_layout(VkyVertexLayout* layout);



/*************************************************************************************************/
/*  Core                                                                                         */
/*************************************************************************************************/

VKY_EXPORT void vky_create_sync_resources(VkyCanvas*);

VKY_EXPORT void vky_wait_canvas_ready(VkyCanvas* canvas);

VKY_EXPORT void vky_create_descriptor_pool(VkyGpu* gpu);

VKY_EXPORT void vky_create_command_pool(VkyGpu* gpu);

VKY_EXPORT VkyGpu vky_create_device(uint32_t, const char**);

VKY_EXPORT VkyCanvas*
vky_create_canvas_from_surface(VkyApp* app, void* window, VkSurfaceKHR* surface);

VKY_EXPORT void vky_reset_canvas(VkyCanvas*);

VKY_EXPORT void vky_create_shared_objects(VkyGpu* gpu);

VKY_EXPORT void vky_prepare_gpu(VkyGpu* gpu, VkSurfaceKHR* surface);

VKY_EXPORT void vky_create_swapchain_resources(VkyCanvas* canvas);

VKY_EXPORT void vky_destroy_device(VkyGpu* gpu);

VKY_EXPORT void vky_destroy_scene(VkyScene* scene);

VKY_EXPORT void vky_destroy_canvas(VkyCanvas* canvas);

VKY_EXPORT void vky_destroy_swapchain_resources(VkyCanvas* canvas);

VKY_EXPORT void vky_destroy_command_buffers(VkyCanvas*);

VKY_EXPORT void vky_push_constants(
    VkCommandBuffer command_buffer, VkyGraphicsPipeline* pipeline, uint32_t size,
    const void* data);

VKY_EXPORT VkDeviceSize vky_compute_dynamic_alignment(VkyGpu* gpu, size_t size);



/*************************************************************************************************/
/*  Render pass                                                                                  */
/*************************************************************************************************/

VKY_EXPORT void
vky_begin_render_pass(VkCommandBuffer command_buffer, VkyCanvas* canvas, VkyColor clear_color);

VKY_EXPORT void vky_begin_live_render_pass(VkCommandBuffer command_buffer, VkyCanvas* canvas);

VKY_EXPORT void vky_begin_command_buffer(VkCommandBuffer command_buffer, VkyGpu* gpu);

VKY_EXPORT void vky_fill_command_buffers(VkyCanvas* canvas);

VKY_EXPORT void vky_fill_live_command_buffers(VkyCanvas* canvas);

VKY_EXPORT void
vky_create_command_buffers(VkyGpu* gpu, uint32_t image_count, VkCommandBuffer* command_buffers);

VKY_EXPORT void
vky_set_viewport(VkCommandBuffer command_buffer, int32_t x, int32_t y, uint32_t w, uint32_t h);

VKY_EXPORT void vky_submit_command_buffers(
    VkyCanvas* canvas, uint32_t commandBufferCount, VkCommandBuffer* command_buffers);

VKY_EXPORT void vky_end_command_buffer(VkCommandBuffer command_buffer, VkyGpu* gpu);

VKY_EXPORT void vky_end_render_pass(VkCommandBuffer command_buffer, VkyCanvas* canvas);

VKY_EXPORT void
vky_draw(VkCommandBuffer command_buffer, uint32_t first_vertex, uint32_t vertex_count);

VKY_EXPORT void vky_draw_indirect(VkCommandBuffer command_buffer, VkyBufferRegion indirect_buffer);

VKY_EXPORT void vky_draw_indexed(
    VkCommandBuffer command_buffer, uint32_t first_index, uint32_t vertex_offset,
    uint32_t index_count);

VKY_EXPORT void
vky_draw_indexed_indirect(VkCommandBuffer command_buffer, VkyBufferRegion indirect_buffer);



/*************************************************************************************************/
/*  Compute                                                                                      */
/*************************************************************************************************/

VKY_EXPORT VkyComputePipeline
vky_create_compute_pipeline(VkyGpu* gpu, const char* filename, VkyResourceLayout resource_layout);

VKY_EXPORT void vky_begin_compute(VkyGpu* gpu);

VKY_EXPORT void vky_compute_acquire(
    VkyComputePipeline* pipeline, VkDescriptorType descriptor_type, void* resource,
    VkPipelineStageFlagBits stage);

VKY_EXPORT void vky_compute(VkyComputePipeline* pipeline, uint32_t nx, uint32_t ny, uint32_t nz);

VKY_EXPORT void vky_compute_release(
    VkyComputePipeline* pipeline, VkDescriptorType descriptor_type, void* resource,
    VkPipelineStageFlagBits stage);

VKY_EXPORT void vky_end_compute(
    VkyGpu* gpu, uint32_t resource_count, VkDescriptorType* descriptor_types, void** resources);

VKY_EXPORT void vky_compute_submit(VkyGpu* gpu);

VKY_EXPORT void vky_compute_wait(VkyGpu* gpu);

VKY_EXPORT void vky_destroy_compute_pipeline(VkyComputePipeline* pipeline);



#endif
