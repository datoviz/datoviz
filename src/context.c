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
    gpu->context = calloc(1, sizeof(VklContext));

    gpu->context->gpu = gpu;
    // TODO

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

    // TODO

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
