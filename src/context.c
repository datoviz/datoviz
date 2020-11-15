#include "../include/visky/context.h"
#include <stdlib.h>



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Context                                                                                      */
/*************************************************************************************************/

VklContext* vkl_context(VklGpu* gpu)
{
    log_trace("creating context");
    VklContext* context = NULL;
    context = gpu->context = calloc(1, sizeof(VklContext));

    gpu->context->gpu = gpu;

    INSTANCES_INIT(
        VklBuffer, context, buffers, max_buffers, VKL_MAX_BUFFERS, VKL_OBJECT_TYPE_BUFFER)

    INSTANCES_INIT(
        VklImages, context, images, max_images, VKL_MAX_TEXTURES, VKL_OBJECT_TYPE_IMAGES)

    INSTANCES_INIT(
        VklSampler, context, samplers, max_samplers, VKL_MAX_TEXTURES, VKL_OBJECT_TYPE_SAMPLER)

    INSTANCES_INIT(
        VklCompute, context, computes, max_computes, VKL_MAX_COMPUTES, VKL_OBJECT_TYPE_COMPUTE)

    return gpu->context;
}



void vkl_context_destroy(VklContext* context)
{
    if (context == NULL)
    {
        log_error("skip destruction of null context");
        return;
    }
    log_trace("destroying context");
    ASSERT(context != NULL);
    ASSERT(context->gpu != NULL);


    log_trace("context destroy buffers");
    for (uint32_t i = 0; i < context->max_buffers; i++)
    {
        if (context->buffers[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_buffer_destroy(&context->buffers[i]);
    }
    INSTANCES_DESTROY(context->buffers)


    log_trace("context destroy sets of images");
    for (uint32_t i = 0; i < context->max_images; i++)
    {
        if (context->images[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_images_destroy(&context->images[i]);
    }
    INSTANCES_DESTROY(context->images)


    log_trace("context destroy samplers");
    for (uint32_t i = 0; i < context->max_samplers; i++)
    {
        if (context->samplers[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_sampler_destroy(&context->samplers[i]);
    }
    INSTANCES_DESTROY(context->samplers)


    log_trace("context destroy computes");
    for (uint32_t i = 0; i < context->max_computes; i++)
    {
        if (context->computes[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_compute_destroy(&context->computes[i]);
    }
    INSTANCES_DESTROY(context->computes)


    context->gpu->context = NULL;
    FREE(context);
}



VklBufferRegions vkl_alloc_buffer(VklContext* context, VklBufferType type, VkDeviceSize size) {}



VklCompute* vkl_new_compute(VklContext* context, const char* shader_path) {}



VklTexture* vkl_new_texture(VklContext* context, uint32_t dims, uvec3 size, VkFormat format) {}



void vkl_texture_resize(VklTexture* texture, uvec3 size) {}



void vkl_texture_filter(VklTexture* texture, VklFilterType type, VkFilter filter) {}



void vkl_texture_address_mode(
    VklTexture* texture, VklTextureAxis axis, VkSamplerAddressMode address_mode)
{
}



void vkl_texture_destroy(VklTexture* texture)
{
    if (texture != NULL)
    {
        vkl_images_destroy(texture->image);
        vkl_sampler_destroy(texture->sampler);
    }
}
