/*************************************************************************************************/
/*  GPU context holding the Resources, DatAlloc, Transfers instances                             */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "context.h"
#include "datalloc_utils.h"
#include "host.h"
#include "resources_utils.h"
#include "transfers_utils.h"
#include "vklite_utils.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _buffer_upload_done(DvzDeq* deq, void* item, void* user_data)
{
    DvzTransferUploadDone* up = (DvzTransferUploadDone*)item;
    ASSERT(up != NULL);
    DvzDat* dat = (DvzDat*)up->user_data;
    if (dat == NULL)
        return;

    // Only for staging buffers.
    ASSERT(dat->br.buffer != NULL);
    ASSERT(dat->br.buffer->type == DVZ_BUFFER_TYPE_STAGING);
    log_info("deallocate temporary staging dat with size %s", pretty_size(dat->br.size));
    dvz_dat_destroy(dat);
}



/*************************************************************************************************/
/*  Context                                                                                      */
/*************************************************************************************************/

DvzContext* dvz_context(DvzGpu* gpu)
{
    ASSERT(gpu != NULL);
    ASSERT(dvz_obj_is_created(&gpu->obj));
    log_trace("creating context");

    DvzContext* ctx = calloc(1, sizeof(DvzContext));
    ASSERT(ctx != NULL);
    ctx->gpu = gpu;

    // Create the transfers.
    dvz_transfers(gpu, &ctx->transfers);

    // Called when a transfer upload is finished, if the temporary staging buffer needs to be
    // deallocated.
    dvz_deq_callback(
        &ctx->transfers.deq, DVZ_TRANSFER_DEQ_EV, //
        DVZ_TRANSFER_UPLOAD_DONE, _buffer_upload_done, NULL);


    // Create the resources.
    dvz_resources(gpu, &ctx->res);

    // Create the datalloc.
    dvz_datalloc(gpu, &ctx->res, &ctx->datalloc);

    // HACK: the vklite module makes the assumption that the queue #0 supports transfers.
    // Here, in the context, we make the same assumption. The first queue is reserved to transfers.
    ASSERT(DVZ_DEFAULT_QUEUE_TRANSFER == 0);

    // Create the context.
    // gpu->context = ctx;
    dvz_obj_created(&ctx->obj);

    return ctx;
}



void dvz_context_wait(DvzContext* ctx)
{
    ASSERT(ctx != NULL);
    for (uint32_t i = 0; i < 4; i++)
        dvz_deq_wait(&ctx->transfers.deq, i);
    dvz_queue_wait(ctx->transfers.gpu, DVZ_DEFAULT_QUEUE_TRANSFER);
}



void dvz_context_destroy(DvzContext* ctx)
{
    if (ctx == NULL)
    {
        log_error("skip destruction of null context");
        return;
    }
    log_trace("destroying context");
    ASSERT(ctx != NULL);

    DvzGpu* gpu = ctx->gpu;
    ASSERT(gpu != NULL);
    // ASSERT(gpu->context == ctx);

    // Destroy the companion objects.
    dvz_transfers_destroy(&ctx->transfers);
    dvz_resources_destroy(&ctx->res);
    dvz_datalloc_destroy(&ctx->datalloc);

    // Free the context.
    FREE(ctx);
    // gpu->context = NULL;
}



/*************************************************************************************************/
/*  Default initializers                                                                         */
/*************************************************************************************************/

DvzGpu* dvz_init_offscreen(void)
{
    DvzHost* host = dvz_host(DVZ_BACKEND_OFFSCREEN);

    DvzGpu* gpu = dvz_gpu_best(host);
    _default_queues(gpu, false);
    dvz_gpu_create(gpu, 0);

    return gpu;
}



DvzGpu* dvz_init_glfw(void)
{
    DvzHost* host = dvz_host(DVZ_BACKEND_GLFW);

    DvzGpu* gpu = dvz_gpu_best(host);
    _default_queues(gpu, false);
    dvz_gpu_request_features(gpu, (VkPhysicalDeviceFeatures){.independentBlend = true});
    dvz_gpu_create(gpu, 0);

    return gpu;
}
