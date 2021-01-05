#ifndef VKL_CONTEXT_HEADER
#define VKL_CONTEXT_HEADER

#include "common.h"
#include "vklite.h"

#ifdef __cplusplus
extern "C" {
#endif



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define VKL_DEFAULT_WIDTH  800
#define VKL_DEFAULT_HEIGHT 600

#define VKL_BUFFER_TYPE_STAGING_SIZE (16 * 1024 * 1024)
#define VKL_BUFFER_TYPE_VERTEX_SIZE  (16 * 1024 * 1024)
#define VKL_BUFFER_TYPE_INDEX_SIZE   (16 * 1024 * 1024)
#define VKL_BUFFER_TYPE_STORAGE_SIZE (16 * 1024 * 1024)
#define VKL_BUFFER_TYPE_UNIFORM_SIZE (4 * 1024 * 1024)

#define VKL_ZERO_OFFSET                                                                           \
    (uvec3) { 0, 0, 0 }



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Default queue.
typedef enum
{
    VKL_DEFAULT_QUEUE_TRANSFER,
    VKL_DEFAULT_QUEUE_COMPUTE,
    VKL_DEFAULT_QUEUE_RENDER,
    VKL_DEFAULT_QUEUE_PRESENT,
    VKL_DEFAULT_QUEUE_COUNT,
} VklDefaultQueue;



// Filter type.
typedef enum
{
    VKL_FILTER_MIN,
    VKL_FILTER_MAX,
} VklFilterType;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VklContext
{
    VklObject obj;
    VklGpu* gpu;

    VklCommands transfer_cmd;

    VklContainer buffers;
    VklContainer images;
    VklContainer samplers;
    VklContainer textures;
    VklContainer computes;
};



/*************************************************************************************************/
/*  Context                                                                                      */
/*************************************************************************************************/

VKY_EXPORT VklContext* vkl_context(VklGpu* gpu, VklWindow* window);

VKY_EXPORT void vkl_context_reset(VklContext* context);



/*************************************************************************************************/
/*  Buffer allocation                                                                            */
/*************************************************************************************************/

VKY_EXPORT VklBufferRegions vkl_ctx_buffers(
    VklContext* context, VklBufferType buffer_type, uint32_t buffer_count, VkDeviceSize size);

VKY_EXPORT void
vkl_ctx_buffers_resize(VklContext* context, VklBufferRegions* br, VkDeviceSize new_size);



/*************************************************************************************************/
/*  Compute                                                                                      */
/*************************************************************************************************/

VKY_EXPORT VklCompute* vkl_ctx_compute(VklContext* context, const char* shader_path);



/*************************************************************************************************/
/*  Texture                                                                                      */
/*************************************************************************************************/

VKY_EXPORT VklTexture*
vkl_ctx_texture(VklContext* context, uint32_t dims, uvec3 size, VkFormat format);

VKY_EXPORT void vkl_texture_resize(VklTexture* texture, uvec3 size);

VKY_EXPORT void vkl_texture_filter(VklTexture* texture, VklFilterType type, VkFilter filter);

VKY_EXPORT void vkl_texture_address_mode(
    VklTexture* texture, VklTextureAxis axis, VkSamplerAddressMode address_mode);

VKY_EXPORT void vkl_texture_destroy(VklTexture* texture);



#ifdef __cplusplus
}
#endif

#endif
