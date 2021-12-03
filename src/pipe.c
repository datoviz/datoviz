/*************************************************************************************************/
/*  Pipe: wrap a graphics/compute pipeline with bindings and dat/tex resources                   */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "pipe.h"
#include "resources.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Pipe                                                                                         */
/*************************************************************************************************/

DvzPipe dvz_pipe(DvzGpu* gpu)
{
    ASSERT(gpu != NULL);
    DvzPipe pipe = {0};
    pipe.gpu = gpu;
    dvz_obj_init(&pipe.obj);
    return pipe;
}



DvzGraphics* dvz_pipe_graphics(DvzPipe* pipe, uint32_t count)
{
    ASSERT(pipe != NULL);
    ASSERT(count > 0);

    pipe->type = DVZ_PIPE_GRAPHICS;
    DvzGraphics* graphics = &pipe->u.graphics;
    *graphics = dvz_graphics(pipe->gpu);

    pipe->bindings = dvz_bindings(&graphics->slots, count);

    return graphics;
}



DvzCompute* dvz_pipe_compute(DvzPipe* pipe, const char* shader_path)
{
    ASSERT(pipe != NULL);

    pipe->type = DVZ_PIPE_COMPUTE;

    DvzCompute* compute = &pipe->u.compute;
    *compute = dvz_compute(pipe->gpu, shader_path);

    pipe->bindings = dvz_bindings(&compute->slots, 1);

    return compute;
}



void dvz_pipe_vertex(DvzPipe* pipe, DvzDat* dat_vertex)
{
    ASSERT(pipe != NULL);
    pipe->dat_vertex = dat_vertex;
}



void dvz_pipe_index(DvzPipe* pipe, DvzDat* dat_index)
{
    ASSERT(pipe != NULL);
    pipe->dat_index = dat_index;
}



void dvz_pipe_dat(DvzPipe* pipe, uint32_t idx, DvzDat* dat)
{
    ASSERT(pipe != NULL);
    ASSERT(idx < DVZ_MAX_BINDINGS_SIZE);
    pipe->dats[idx] = dat;
    dvz_bindings_buffer(&pipe->bindings, idx, dat->br);
}



void dvz_pipe_tex(DvzPipe* pipe, uint32_t idx, DvzTex* tex, DvzSampler* sampler)
{
    ASSERT(pipe != NULL);
    ASSERT(idx < DVZ_MAX_BINDINGS_SIZE);
    pipe->texs[idx] = tex;
    pipe->samplers[idx] = sampler;
    dvz_bindings_texture(&pipe->bindings, idx, tex->img, sampler);
}



void dvz_pipe_create(DvzPipe* pipe)
{
    ASSERT(pipe != NULL);
    log_trace("creating pipe");

    if (pipe->type == DVZ_PIPE_GRAPHICS)
        dvz_graphics_create(&pipe->u.graphics);
    else if (pipe->type == DVZ_PIPE_COMPUTE)
        dvz_compute_create(&pipe->u.compute);

    if (dvz_obj_is_created(&pipe->bindings.obj))
        dvz_bindings_update(&pipe->bindings);

    dvz_obj_created(&pipe->obj);
}



void dvz_pipe_destroy(DvzPipe* pipe)
{
    ASSERT(pipe != NULL);

    if (pipe->type == DVZ_PIPE_GRAPHICS)
        dvz_graphics_destroy(&pipe->u.graphics);
    else if (pipe->type == DVZ_PIPE_COMPUTE)
        dvz_compute_destroy(&pipe->u.compute);

    if (dvz_obj_is_created(&pipe->bindings.obj))
        dvz_bindings_destroy(&pipe->bindings);

    dvz_obj_destroyed(&pipe->obj);
    log_trace("pipe destroyed");
}



/*************************************************************************************************/
/*  Pipe draw commands                                                                           */
/*************************************************************************************************/

static DvzGraphics* _pre_draw(DvzPipe* pipe, DvzCommands* cmds, uint32_t idx)
{
    ASSERT(pipe != NULL);
    ASSERT(cmds != NULL);

    ASSERT(pipe->type == DVZ_PIPE_GRAPHICS);
    DvzGraphics* graphics = &pipe->u.graphics;
    ASSERT(graphics != NULL);

    // TODO: dat vertex byte offset?
    dvz_cmd_bind_vertex_buffer(cmds, idx, pipe->dat_vertex->br, 0);

    // TODO: dat index byte offset?
    if (pipe->dat_index != NULL)
        dvz_cmd_bind_index_buffer(cmds, idx, pipe->dat_index->br, 0);

    // TODO: dynamic uniform buffer index
    dvz_cmd_bind_graphics(cmds, idx, graphics, &pipe->bindings, 0);

    return graphics;
}



void dvz_pipe_draw(
    DvzPipe* pipe, DvzCommands* cmds, uint32_t idx, uint32_t first_vertex, uint32_t vertex_count)
{
    DvzGraphics* graphics = _pre_draw(pipe, cmds, idx);
    dvz_cmd_draw(cmds, idx, first_vertex, vertex_count);
}



void dvz_pipe_draw_indexed(
    DvzPipe* pipe, DvzCommands* cmds, uint32_t idx, uint32_t first_index, uint32_t vertex_offset,
    uint32_t index_count)
{
    DvzGraphics* graphics = _pre_draw(pipe, cmds, idx);
    dvz_cmd_draw_indexed(cmds, idx, first_index, vertex_offset, index_count);
}



void dvz_pipe_draw_indirect(DvzPipe* pipe, DvzCommands* cmds, uint32_t idx, DvzDat* dat_indirect)
{
    ASSERT(dat_indirect != NULL);
    DvzGraphics* graphics = _pre_draw(pipe, cmds, idx);
    dvz_cmd_draw_indirect(cmds, idx, dat_indirect->br);
}



void dvz_pipe_draw_indexed_indirect(
    DvzPipe* pipe, DvzCommands* cmds, uint32_t idx, DvzDat* dat_indirect)
{
    ASSERT(dat_indirect != NULL);
    DvzGraphics* graphics = _pre_draw(pipe, cmds, idx);
    dvz_cmd_draw_indexed_indirect(cmds, idx, dat_indirect->br);
}



/*************************************************************************************************/
/*  Pipe compute commands                                                                        */
/*************************************************************************************************/

void dvz_pipe_run(DvzPipe* pipe, DvzCommands* cmds, uint32_t idx, uvec3 size)
{
    ASSERT(pipe != NULL);
    ASSERT(cmds != NULL);

    ASSERT(pipe->type == DVZ_PIPE_COMPUTE);
    DvzCompute* compute = &pipe->u.compute;
    ASSERT(compute != NULL);

    dvz_cmd_compute(cmds, idx, compute, size);
}
