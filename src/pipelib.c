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
    ASSERT(ctx != NULL);

    DvzDat* dat_mvp = dvz_dat(ctx, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzMVP), 0);
    ASSERT(dat_mvp != NULL);

    DvzMVP mvp = {0};
    glm_mat4_identity(mvp.model);
    glm_mat4_identity(mvp.view);
    glm_mat4_identity(mvp.proj);
    // TODO OPTIM: not wait here but later at the end of the pipelib graphics creation
    // will need dvz_resources_wait()
    dvz_dat_upload(dat_mvp, 0, sizeof(mvp), &mvp, true);

    return dat_mvp;
}



static DvzDat* _make_dat_viewport(DvzContext* ctx, vec2 size)
{
    ASSERT(ctx != NULL);
    ASSERT(size[0] > 0);
    ASSERT(size[1] > 0);

    DvzDat* dat_viewport = dvz_dat(ctx, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzViewport), 0);
    ASSERT(dat_viewport != NULL);

    DvzViewport viewport = dvz_viewport_default(size[0], size[1]);
    // TODO OPTIM: not wait here but later at the end of the pipelib graphics creation
    dvz_dat_upload(dat_viewport, 0, sizeof(viewport), &viewport, true);

    return dat_viewport;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzPipelib dvz_pipelib(DvzGpu* gpu)
{
    ASSERT(gpu != NULL);
    DvzPipelib lib = {0};
    lib.gpu = gpu;
    lib.graphics =
        dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzPipe), DVZ_OBJECT_TYPE_PIPE);
    lib.computes =
        dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzPipe), DVZ_OBJECT_TYPE_PIPE);
    dvz_obj_created(&lib.obj);
    log_trace("pipelib created");
    return lib;
}



DvzPipe* dvz_pipelib_graphics(
    DvzPipelib* lib, DvzRenderpass* renderpass, uint32_t img_count, //
    uvec2 size, DvzGraphicsType type, int flags)
{
    ASSERT(lib != NULL);
    ASSERT(renderpass != NULL);

    DvzGpu* gpu = lib->gpu;
    ASSERT(gpu != NULL);
    ASSERT(dvz_obj_is_created(&gpu->obj));

    DvzContext* ctx = gpu->context;
    ASSERT(ctx != NULL);
    ASSERT(dvz_obj_is_created(&ctx->obj));

    // Allocate a DvzPipe pointer.
    DvzPipe* pipe = dvz_container_alloc(&lib->graphics);

    // Initialize the pipe.
    *pipe = dvz_pipe(gpu);

    // Initialize the graphics pipeline.
    DvzGraphics* graphics = dvz_pipe_graphics(pipe, img_count);
    dvz_graphics_builtin(renderpass, graphics, type, flags);

    // Create the first common uniform dat: MVP.
    DvzDat* dat_mvp = _make_dat_mvp(ctx);

    // Create the second common uniform dat: viewport.
    DvzDat* dat_viewport = _make_dat_viewport(ctx, size);

    // Create the bindings.
    dvz_pipe_dat(pipe, 0, dat_mvp);
    dvz_pipe_dat(pipe, 1, dat_viewport);

    // TODO: graphics-specific dats and texs

    dvz_pipe_create(pipe);

    return pipe;
}



DvzPipe* dvz_pipelib_compute_file(DvzPipelib* lib, const char* shader_path)
{
    ASSERT(lib != NULL);
    // TODO
    return NULL;
}



void dvz_pipelib_pipe_destroy(DvzPipelib* lib, DvzPipe* pipe)
{
    ASSERT(lib != NULL);
    ASSERT(pipe != NULL);
    dvz_pipe_destroy(pipe);
}



void dvz_pipelib_destroy(DvzPipelib* lib)
{
    ASSERT(lib != NULL);

    CONTAINER_DESTROY_ITEMS(DvzGraphics, lib->graphics, dvz_pipe_destroy)

    dvz_container_destroy(&lib->graphics);
    dvz_container_destroy(&lib->computes);
    dvz_obj_destroyed(&lib->obj);
}
