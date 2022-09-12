/*************************************************************************************************/
/*  Holds graphics and computes pipes                                                            */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "pipelib.h"
#include "context.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static DvzDat* _make_dat_mvp(DvzContext* ctx)
{
    ANN(ctx);

    DvzDat* dat_mvp = dvz_dat(ctx, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzMVP), 0);
    ANN(dat_mvp);

    DvzMVP mvp = dvz_mvp_default();
    // TODO OPTIM: not wait here but later at the end of the pipelib graphics creation
    // will need dvz_resources_wait()
    dvz_dat_upload(dat_mvp, 0, sizeof(mvp), &mvp, true);

    return dat_mvp;
}



static DvzDat* _make_dat_viewport(DvzContext* ctx, uvec2 size)
{
    ANN(ctx);

    DvzDat* dat_viewport = dvz_dat(ctx, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzViewport), 0);
    ANN(dat_viewport);

    DvzViewport viewport = dvz_viewport_default(size[0], size[1]);
    // TODO OPTIM: not wait here but later at the end of the pipelib graphics creation
    dvz_dat_upload(dat_viewport, 0, sizeof(viewport), &viewport, true);

    return dat_viewport;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzPipelib* dvz_pipelib(DvzContext* ctx)
{
    ANN(ctx);
    ANN(ctx->gpu);

    DvzPipelib* lib = calloc(1, sizeof(DvzPipelib));
    ANN(lib);

    lib->gpu = ctx->gpu;
    lib->graphics =
        dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzPipe), DVZ_OBJECT_TYPE_PIPE);
    lib->computes =
        dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzPipe), DVZ_OBJECT_TYPE_PIPE);
    dvz_obj_created(&lib->obj);
    log_trace("pipelib created");
    return lib;
}



DvzPipe* dvz_pipelib_graphics(
    DvzPipelib* lib, DvzContext* ctx, DvzRenderpass* renderpass, //
    uint32_t img_count, DvzGraphicsType type, int flags)
{
    ANN(lib);
    ANN(renderpass);

    DvzGpu* gpu = lib->gpu;
    ANN(gpu);
    ASSERT(dvz_obj_is_created(&gpu->obj));

    ANN(ctx);
    ASSERT(dvz_obj_is_created(&ctx->obj));

    // Allocate a DvzPipe pointer.
    DvzPipe* pipe = (DvzPipe*)dvz_container_alloc(&lib->graphics);

    // Initialize the pipe.
    *pipe = dvz_pipe(gpu);
    pipe->flags = flags;

    // Initialize the graphics pipeline.
    DvzGraphics* graphics = dvz_pipe_graphics(pipe, img_count);
    dvz_graphics_builtin(renderpass, graphics, type, flags);

    // Create the first common uniform dat: MVP.
    if (pipe->flags & DVZ_PIPELIB_FLAGS_CREATE_MVP)
    {
        dvz_pipe_dat(pipe, 0, _make_dat_mvp(ctx));
    }

    // Create the second common uniform dat: viewport.
    if (pipe->flags & DVZ_PIPELIB_FLAGS_CREATE_VIEWPORT)
    {
        // NOTE: null viewport size here as we don't know the viewport size yet, the caller will
        // need to upload a correct viewport size later.
        dvz_pipe_dat(pipe, 1, _make_dat_viewport(ctx, (uvec2){0, 0}));
    }

    dvz_pipe_create(pipe);

    return pipe;
}



DvzPipe* dvz_pipelib_compute_file(DvzPipelib* lib, const char* shader_path)
{
    ANN(lib);
    // TODO: compute shader
    return NULL;
}



void dvz_pipelib_destroy(DvzPipelib* lib)
{
    ANN(lib);

    CONTAINER_DESTROY_ITEMS(DvzPipe, lib->graphics, dvz_pipe_destroy)
    dvz_container_destroy(&lib->graphics);

    CONTAINER_DESTROY_ITEMS(DvzPipe, lib->computes, dvz_pipe_destroy)
    dvz_container_destroy(&lib->computes);

    dvz_obj_destroyed(&lib->obj);
    FREE(lib);
}
