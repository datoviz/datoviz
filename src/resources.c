/*************************************************************************************************/
/*  Holds all GPU data resources (buffers, images, dats, texs)                                   */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "resources.h"
#include "context.h"
#include "fifo.h"
#include "resources_utils.h"
#include "transfers.h"
#include "transfers_utils.h"
// #include "vklite_utils.h"
#include <stdlib.h>



/*************************************************************************************************/
/*  Wait utils                                                                                   */
/*************************************************************************************************/

static void _wait_dat_upload(DvzTransfers* transfers, bool staging, bool need_dealloc_stg)
{
    ASSERT(transfers != NULL);

    if (staging)
        dvz_deq_dequeue(&transfers->deq, DVZ_TRANSFER_PROC_CPY, true);
    else
    {
        // WARNING: for mappable buffers, the transfer is done on the main thread (using
        // the COPY queue, not the UD queue), not in the background thread, so we need to
        // dequeue the COPY queue manually!
        dvz_deq_dequeue(&transfers->deq, DVZ_TRANSFER_PROC_CPY, true);
        dvz_queue_wait(transfers->gpu, DVZ_DEFAULT_QUEUE_TRANSFER);
    }

    // Dequeue the upload_done event if needed.
    if (need_dealloc_stg)
        dvz_deq_dequeue(&transfers->deq, DVZ_TRANSFER_PROC_EV, true);
}



static void _wait_dat_download(DvzTransfers* transfers, bool staging)
{
    ASSERT(transfers != NULL);

    if (staging)
        dvz_deq_dequeue(&transfers->deq, DVZ_TRANSFER_PROC_CPY, true);
    else
        // dvz_deq_dequeue(&transfers->deq, DVZ_TRANSFER_PROC_UD, true);
        dvz_queue_wait(transfers->gpu, DVZ_DEFAULT_QUEUE_TRANSFER);

    // Wait until the download finished event has been raised.
    dvz_deq_dequeue(&transfers->deq, DVZ_TRANSFER_PROC_EV, true);
}



static void _wait_dup(DvzTransfers* transfers, DvzDat* dat)
{
    ASSERT(transfers != NULL);
    ASSERT(dat != NULL);

    // IMPORTANT: before calling the dvz_transfers_frame(), we must wait for the DUP task
    // to be in the queue. Here we dequeue it manually. The callback will add it to the
    // special Dups structure, and it will be correctly processed by dvz_transfer_frame().
    dvz_deq_dequeue(&transfers->deq, DVZ_TRANSFER_PROC_DUP, true);

    ASSERT(dat->br.count > 0);
    for (uint32_t i = 0; i < dat->br.count; i++)
        dvz_transfers_frame(transfers, i);
}



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

    // Allocate memory for buffers, textures, samplers.
    _create_resources(res);

    dvz_obj_created(&res->obj);
}



DvzImages* dvz_resources_image(DvzResources* res, DvzTexDims dims, uvec3 shape, DvzFormat format)
{
    ASSERT(res != NULL);
    ASSERT(res->gpu != NULL);
    DvzImages* img = (DvzImages*)dvz_container_alloc(&res->images);
    _make_image(res->gpu, img, dims, shape, format);
    return img;
}



DvzBuffer* dvz_resources_buffer(DvzResources* res, DvzBufferType type, bool mappable, DvzSize size)
{
    ASSERT(res != NULL);
    DvzBuffer* buffer = _make_standalone_buffer(res, type, mappable, size);
    return buffer;
}



DvzSampler* dvz_resources_sampler(DvzResources* res, DvzFilter filter, DvzSamplerAddressMode mode)
{
    ASSERT(res != NULL);
    DvzSampler* sampler = (DvzSampler*)dvz_container_alloc(&res->samplers);
    *sampler = dvz_sampler(res->gpu);
    dvz_sampler_min_filter(sampler, (VkFilter)filter);
    dvz_sampler_mag_filter(sampler, (VkFilter)filter);
    dvz_sampler_address_mode(sampler, DVZ_SAMPLER_AXIS_U, (VkSamplerAddressMode)mode);
    dvz_sampler_address_mode(sampler, DVZ_SAMPLER_AXIS_V, (VkSamplerAddressMode)mode);
    dvz_sampler_address_mode(sampler, DVZ_SAMPLER_AXIS_V, (VkSamplerAddressMode)mode);
    dvz_sampler_create(sampler);
    return sampler;
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

    dvz_obj_destroyed(&res->obj);
}



/*************************************************************************************************/
/*  Dats                                                                                         */
/*************************************************************************************************/

DvzDat* dvz_dat(DvzContext* ctx, DvzBufferType type, DvzSize size, int flags)
{
    ASSERT(ctx != NULL);
    ASSERT(size > 0);

    DvzResources* res = &ctx->res;
    ASSERT(res != NULL);

    DvzDat* dat = (DvzDat*)dvz_container_alloc(&res->dats);
    dat->ctx = ctx;
    dat->res = res;
    dat->datalloc = &ctx->datalloc;
    dat->transfers = &ctx->transfers;
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
        dat->stg = _alloc_staging(ctx, size);
    }

    dvz_obj_created(&dat->obj);
    return dat;
}



void dvz_dat_resize(DvzDat* dat, DvzSize new_size)
{
    ASSERT(dat != NULL);
    ASSERT(dat->br.buffer != NULL);

    if (new_size == dat->br.size)
    {
        return;
    }

    log_debug("resize dat with offset %d to size %s", dat->br.offsets[0], pretty_size(new_size));
    _dat_dealloc(dat);

    // Resize the persistent staging dat if there is one.
    if (dat->stg != NULL)
        dvz_dat_resize(dat->stg, new_size);

    _dat_alloc(dat->res, dat, dat->br.buffer->type, dat->br.count, new_size);
}



void dvz_dat_upload(DvzDat* dat, DvzSize offset, DvzSize size, void* data, bool wait)
{
    ASSERT(dat != NULL);

    DvzResources* res = dat->res;
    ASSERT(res != NULL);

    DvzDatAlloc* datalloc = dat->datalloc;
    ASSERT(datalloc != NULL);

    DvzTransfers* transfers = dat->transfers;
    ASSERT(transfers != NULL);

    DvzGpu* gpu = res->gpu;
    ASSERT(gpu != NULL);

    // Do we need a staging buffer?
    DvzDat* stg = dat->stg;
    bool need_dealloc_stg = false;
    if (_dat_has_staging(dat) && stg == NULL)
    {
        // Need to allocate a temporary staging buffer.
        ASSERT(!_dat_persistent_staging(dat));
        log_warn("allocate temporary staging dat, not efficient -- if this message is displayed "
                 "frequently, you should have a permanent staging dat");
        stg = _alloc_staging(dat->ctx, size);
        need_dealloc_stg = true;
    }

    // Enqueue the transfer task corresponding to the flags.
    bool dup = _dat_is_dup(dat);
    bool staging = stg != NULL;
    DvzBufferRegions stg_br = staging ? stg->br : (DvzBufferRegions){0};

    log_debug("upload %s to dat%s", pretty_size(size), staging ? " (with staging)" : "");

    if (!dup)
    {
        // Enqueue a standard upload task, with or without staging buffer.
        DvzDeqItem* done = need_dealloc_stg ? _create_upload_done(stg) : NULL;
        _enqueue_buffer_upload(&transfers->deq, dat->br, offset, stg_br, 0, size, data, done);
        if (wait)
            _wait_dat_upload(transfers, staging, need_dealloc_stg);
    }

    else
    {
        // Enqueue a dup transfer task, with or without staging buffer.
        _enqueue_dup_transfer(&transfers->deq, dat->br, offset, stg_br, 0, size, data);
        if (wait)
            _wait_dup(transfers, dat);
    }
}



void dvz_dat_download(DvzDat* dat, DvzSize offset, DvzSize size, void* data, bool wait)
{
    ASSERT(dat != NULL);

    DvzContext* ctx = dat->ctx;
    ASSERT(ctx != NULL);

    DvzResources* res = dat->res;
    ASSERT(res != NULL);

    DvzDatAlloc* datalloc = dat->datalloc;
    ASSERT(datalloc != NULL);

    DvzTransfers* transfers = dat->transfers;
    ASSERT(transfers != NULL);

    DvzGpu* gpu = res->gpu;
    ASSERT(gpu != NULL);

    // Do we need a staging buffer?
    DvzDat* stg = dat->stg;
    if (_dat_has_staging(dat) && stg == NULL)
    {
        // Need to allocate a temporary staging buffer.
        ASSERT(!_dat_persistent_staging(dat));
        log_debug("allocate temporary staging dat");
        stg = _alloc_staging(ctx, size);
    }

    // Enqueue the transfer task corresponding to the flags.
    bool staging = stg != NULL;
    DvzBufferRegions stg_br = staging ? stg->br : (DvzBufferRegions){0};

    log_debug("download %s from dat%s", pretty_size(size), staging ? " (with staging)" : "");

    // Enqueue a standard download task, with or without staging buffer.
    _enqueue_buffer_download(&transfers->deq, dat->br, offset, stg_br, 0, size, data);

    if (wait)
        _wait_dat_download(transfers, staging);
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

DvzTex* dvz_tex(DvzContext* ctx, DvzTexDims dims, uvec3 shape, DvzFormat format, int flags)
{
    ASSERT(ctx != NULL);
    DvzResources* res = &ctx->res;
    ASSERT(res != NULL);

    DvzTex* tex = (DvzTex*)dvz_container_alloc(&res->texs);
    tex->ctx = ctx;
    tex->res = res;
    tex->flags = flags;
    tex->dims = dims;
    memcpy(tex->shape, shape, sizeof(uvec3));

    // Allocate the tex.
    // TODO: GPU sync before?
    _tex_alloc(res, tex, dims, shape, format);

    dvz_obj_created(&tex->obj);
    return tex;
}



void dvz_tex_resize(DvzTex* tex, uvec3 new_shape, DvzSize new_size)
{
    ASSERT(tex != NULL);
    ASSERT(tex->img != NULL);

    // TODO: GPU sync before?
    dvz_images_resize(tex->img, new_shape);

    // Resize the persistent staging tex if there is one.
    if (tex->stg != NULL)
        dvz_dat_resize(tex->stg, new_size);

    memcpy(tex->shape, new_shape, sizeof(uvec3));
}



void dvz_tex_upload(DvzTex* tex, uvec3 offset, uvec3 shape, DvzSize size, void* data, bool wait)
{
    ASSERT(tex != NULL);
    ASSERT(tex->img != NULL);

    DvzContext* ctx = tex->ctx;
    ASSERT(ctx != NULL);

    DvzTransfers* transfers = &ctx->transfers;
    ASSERT(transfers != NULL);

    // Get the associated staging buffer.
    DvzDat* stg = _tex_staging(ctx, tex, size);
    ASSERT(stg != NULL);

    // May use shape[i] = 0 to indicate the full shape along that axis.
    for (uint32_t i = 0; i < 3; i++)
        shape[i] = shape[i] | tex->shape[i];
    _enqueue_image_upload(&transfers->deq, tex->img, offset, shape, stg->br, 0, size, data);

    if (wait)
    {
        dvz_deq_dequeue(&transfers->deq, DVZ_TRANSFER_PROC_CPY, true);
    }
}



void dvz_tex_download(DvzTex* tex, uvec3 offset, uvec3 shape, DvzSize size, void* data, bool wait)
{
    ASSERT(tex != NULL);
    ASSERT(tex->img != NULL);

    DvzContext* ctx = tex->ctx;
    ASSERT(ctx != NULL);

    DvzTransfers* transfers = &ctx->transfers;
    ASSERT(transfers != NULL);

    // Get the associated staging buffer.
    DvzDat* stg = _tex_staging(ctx, tex, size);
    ASSERT(stg != NULL);

    _enqueue_image_download(&transfers->deq, tex->img, offset, shape, stg->br, 0, size, data);

    if (wait)
    {
        dvz_deq_dequeue(&transfers->deq, DVZ_TRANSFER_PROC_CPY, true);
        dvz_deq_dequeue(&transfers->deq, DVZ_TRANSFER_PROC_EV, true);
    }
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
