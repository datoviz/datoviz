/*************************************************************************************************/
/*  Holds all GPU data resources (buffers, images, dats, texs)                                   */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/


#include "resources.h"
#include "fifo.h"
#include "resources_utils.h"
#include "vklite_utils.h"
#include <stdlib.h>



/*************************************************************************************************/
/*  Resources                                                                                    */
/*************************************************************************************************/

void dvz_resources(DvzGpu* gpu, DvzResources* res)
{
    ASSERT(gpu != NULL);
    ASSERT(dvz_obj_is_created(&gpu->obj));
    ASSERT(res != NULL);
    ASSERT(!dvz_obj_is_created(&res->obj));
    // NOTE: this function should only be called once, at context creation.

    log_trace("creating resources");

    // Create the resources.
    res->gpu = gpu;

    // Allocate memory for buffers, textures, and computes.
    _create_resources(res);

    dvz_obj_created(&res->obj);
}



DvzImages* dvz_resources_image(DvzResources* res, DvzTexDims dims, uvec3 shape, VkFormat format)
{
    ASSERT(res != NULL);
    ASSERT(res->gpu != NULL);
    DvzImages* img = (DvzImages*)dvz_container_alloc(&res->images);
    _make_image(res->gpu, img, dims, shape, format);
    return img;
}



DvzBuffer*
dvz_resources_buffer(DvzResources* res, DvzBufferType type, bool mappable, VkDeviceSize size)
{
    ASSERT(res != NULL);
    DvzBuffer* buffer = _make_standalone_buffer(res, type, mappable, size);
    return buffer;
}



DvzSampler* dvz_resources_sampler(DvzResources* res, VkFilter filter, VkSamplerAddressMode mode)
{
    ASSERT(res != NULL);
    DvzSampler* sampler = (DvzSampler*)dvz_container_alloc(&res->samplers);
    *sampler = dvz_sampler(res->gpu);
    dvz_sampler_min_filter(sampler, VK_FILTER_NEAREST);
    dvz_sampler_mag_filter(sampler, VK_FILTER_NEAREST);
    dvz_sampler_address_mode(sampler, DVZ_SAMPLER_AXIS_U, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    dvz_sampler_address_mode(sampler, DVZ_SAMPLER_AXIS_V, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    dvz_sampler_address_mode(sampler, DVZ_SAMPLER_AXIS_V, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    dvz_sampler_create(sampler);
    return sampler;
}



DvzCompute* dvz_resources_compute(DvzResources* res, const char* shader_path)
{
    ASSERT(res != NULL);
    ASSERT(shader_path != NULL);
    DvzCompute* compute = (DvzCompute*)dvz_container_alloc(&res->computes);
    *compute = dvz_compute(res->gpu, shader_path);
    return compute;
}



void dvz_resources_destroy(DvzResources* res)
{
    if (res == NULL)
    {
        log_error("skip destruction of null resources");
        return;
    }
    log_trace("destroying resources");
    ASSERT(res != NULL);
    ASSERT(res->gpu != NULL);

    // Destroy the resources.
    _destroy_resources(res);

    // Free the allocated memory.
    dvz_container_destroy(&res->buffers);
    dvz_container_destroy(&res->images);
    dvz_container_destroy(&res->dats);
    dvz_container_destroy(&res->texs);
    dvz_container_destroy(&res->samplers);
    dvz_container_destroy(&res->computes);

    dvz_obj_destroyed(&res->obj);
}



/*************************************************************************************************/
/*  Dats                                                                                         */
/*************************************************************************************************/

DvzDat*
dvz_dat(DvzResources* res, DvzDatAlloc* datalloc, DvzBufferType type, VkDeviceSize size, int flags)
{
    ASSERT(res != NULL);
    ASSERT(datalloc != NULL);
    ASSERT(size > 0);

    DvzDat* dat = (DvzDat*)dvz_container_alloc(&res->dats);
    dat->res = res;
    dat->datalloc = datalloc;
    dat->flags = flags;
    log_debug("create dat with size %s", pretty_size(size));

    // Find the number of copies.
    uint32_t count = _dat_is_dup(dat) ? res->img_count : 1;
    if (count == 0)
    {
        log_warn("DvzContext.img_count is not set");
        count = DVZ_MAX_SWAPCHAIN_IMAGES;
    }
    ASSERT(count > 0);
    ASSERT(count <= DVZ_MAX_SWAPCHAIN_IMAGES);
    _dat_alloc(res, dat, type, count, size);

    // Allocate a permanent staging dat.
    // TODO: staging standalone or not?
    if (_dat_persistent_staging(dat))
    {
        log_debug("allocate persistent staging for dat with size %s", pretty_size(size));
        dat->stg = _alloc_staging(res, datalloc, size);
    }

    dvz_obj_created(&dat->obj);
    return dat;
}



void dvz_dat_resize(DvzDat* dat, VkDeviceSize new_size)
{
    ASSERT(dat != NULL);
    ASSERT(dat->br.buffer != NULL);

    if (new_size == dat->br.size)
    {
        return;
    }

    log_debug("resize dat to size %s", pretty_size(new_size));
    _dat_dealloc(dat);

    // Resize the persistent staging dat if there is one.
    if (dat->stg != NULL)
        dvz_dat_resize(dat->stg, new_size);

    _dat_alloc(dat->res, dat, dat->br.buffer->type, dat->br.count, new_size);
}



void dvz_dat_destroy(DvzDat* dat)
{
    ASSERT(dat != NULL);
    _dat_dealloc(dat);

    // Destroy the persistent staging dat if there is one.
    if (dat->stg != NULL)
        dvz_dat_destroy(dat->stg);

    dvz_obj_destroyed(&dat->obj);
}



/*************************************************************************************************/
/*  Texs                                                                                         */
/*************************************************************************************************/


DvzTex* dvz_tex(DvzResources* res, DvzTexDims dims, uvec3 shape, VkFormat format, int flags)
{
    ASSERT(res != NULL);

    DvzTex* tex = (DvzTex*)dvz_container_alloc(&res->texs);
    tex->res = res;
    tex->flags = flags;
    tex->dims = dims;
    _copy_shape(shape, tex->shape);

    // Allocate the tex.
    // TODO: GPU sync before?
    _tex_alloc(res, tex, dims, shape, format);

    dvz_obj_created(&tex->obj);
    return tex;
}



void dvz_tex_resize(DvzTex* tex, uvec3 new_shape, VkDeviceSize new_size)
{
    ASSERT(tex != NULL);
    ASSERT(tex->img != NULL);

    // TODO: GPU sync before?
    dvz_images_resize(tex->img, new_shape);

    // Resize the persistent staging tex if there is one.
    if (tex->stg != NULL)
        dvz_dat_resize(tex->stg, new_size);
}



void dvz_tex_destroy(DvzTex* tex)
{
    ASSERT(tex != NULL);

    // Deallocate the tex.
    _tex_dealloc(tex);

    // Destroy the persistent staging tex if there is one.
    if (tex->stg != NULL)
        dvz_dat_destroy(tex->stg);

    dvz_obj_destroyed(&tex->obj);
}
