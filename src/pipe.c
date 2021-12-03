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



void dvz_pipe_graphics(DvzPipe* pipe, DvzGraphics* graphics, uint32_t count)
{
    ASSERT(pipe != NULL);
    ASSERT(graphics != NULL);

    pipe->type = DVZ_PIPE_GRAPHICS;
    pipe->u.graphics = graphics;

    pipe->bindings = dvz_bindings(&graphics->slots, count);
}



void dvz_pipe_compute(DvzPipe* pipe, DvzCompute* compute)
{
    ASSERT(pipe != NULL);
    ASSERT(compute != NULL);

    pipe->type = DVZ_PIPE_COMPUTE;
    pipe->u.compute = compute;
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

    if (dvz_obj_is_created(&pipe->bindings.obj))
        dvz_bindings_update(&pipe->bindings);

    dvz_obj_created(&pipe->obj);
}



void dvz_pipe_run(DvzPipe* pipe, DvzCommands* cmds, uint32_t idx)
{
    ASSERT(pipe != NULL);
    ASSERT(cmds != NULL);

    if (pipe->type == DVZ_PIPE_GRAPHICS)
    {
        ASSERT(pipe->u.graphics != NULL);
        DvzGraphics* graphics = pipe->u.graphics;
        ASSERT(graphics != NULL);

        dvz_cmd_bind_vertex_buffer(cmds, idx, pipe->dat_vertex->br, 0);
        // TODO: dynamic uniform buffer index
        dvz_cmd_bind_graphics(cmds, idx, graphics, &pipe->bindings, 0);

        switch (graphics->drawing)
        {
        case DVZ_DRAWING_DIRECT | DVZ_DRAWING_FLAT:
            // dvz_cmd_draw(cmds, idx, 0, n_vertices);
            break;

        case DVZ_DRAWING_DIRECT | DVZ_DRAWING_INDEXED:
            break;

        case DVZ_DRAWING_INDIRECT | DVZ_DRAWING_FLAT:
            break;

        case DVZ_DRAWING_INDIRECT | DVZ_DRAWING_INDEXED:
            break;

        case DVZ_DRAWING_NONE:
        default:
            log_error("graphics drawing mode not set");
            break;
        }
    }
}



void dvz_pipe_destroy(DvzPipe* pipe)
{
    ASSERT(pipe != NULL);

    if (dvz_obj_is_created(&pipe->bindings.obj))
        dvz_bindings_destroy(&pipe->bindings);

    dvz_obj_destroyed(&pipe->obj);
    log_trace("pipe destroyed");
}
