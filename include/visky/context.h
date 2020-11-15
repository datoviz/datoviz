#ifndef VKL_CONTEXT_HEADER
#define VKL_CONTEXT_HEADER

#include "vklite2.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Type definitions                                                                             */
/*************************************************************************************************/

typedef struct VklTexture VklTexture;



/*************************************************************************************************/
/*  Enums                                                                                      */
/*************************************************************************************************/

typedef enum
{
    VKL_BUFFER_STAGING,
    VKL_BUFFER_VERTEX,
    VKL_BUFFER_INDEX,
    VKL_BUFFER_STORAGE,
    VKL_BUFFER_UNIFORM,
} VklBufferType;



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


    uint32_t max_buffers;
    VklBuffer* buffers;

    uint32_t max_images;
    VklImages* images;

    uint32_t max_samplers;
    VklSampler* samplers;

    uint32_t max_textures;
    VklTexture* textures;

    uint32_t max_computes;
    VklCompute* computes;
};



struct VklTexture
{
    // VklObject obj;
    // VklGpu* gpu;
    VklContext* context;

    VklImages* image;
    VklSampler* sampler;
};



/*************************************************************************************************/
/*  Context                                                                                      */
/*************************************************************************************************/

VKY_EXPORT VklContext* vkl_context(VklGpu* gpu);

VKY_EXPORT void vkl_context_destroy(VklContext* context);

VKY_EXPORT VklBufferRegions
vkl_alloc_buffer(VklContext* context, VklBufferType type, VkDeviceSize size);

VKY_EXPORT VklCompute* vkl_new_compute(VklContext* context, const char* shader_path);

VKY_EXPORT VklTexture*
vkl_new_texture(VklContext* context, uint32_t dims, uvec3 size, VkFormat format);

VKY_EXPORT void vkl_texture_resize(VklTexture* texture, uvec3 size);

VKY_EXPORT void vkl_texture_filter(VklTexture* texture, VklFilterType type, VkFilter filter);

VKY_EXPORT void vkl_texture_address_mode(
    VklTexture* texture, VklTextureAxis axis, VkSamplerAddressMode address_mode);

VKY_EXPORT void vkl_texture_destroy(VklTexture* texture);


#endif
