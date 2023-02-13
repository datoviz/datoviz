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

static void _ensure_bindings_created(DvzPipe* pipe, uint32_t count)
{
    ANN(pipe);

    if (pipe->bindings.obj.status != DVZ_OBJECT_STATUS_NONE)
        return;

    ASSERT(count > 0);

    log_trace("create bindings with %d descriptors", count);
    if (pipe->type == DVZ_PIPE_GRAPHICS)
        pipe->bindings = dvz_bindings(&pipe->u.graphics.slots, count);
    else if (pipe->type == DVZ_PIPE_COMPUTE)
        pipe->bindings = dvz_bindings(&pipe->u.compute.slots, count);
    else
        log_error("unknown pipe type %d", pipe->type);
}



static bool _all_set(uint32_t count, bool* vars)
{
    if (count == 0)
        return true;
    ASSERT(count > 0);
    bool res = true;
    for (uint32_t i = 0; i < count; i++)
        res &= vars[i];
    return res;
}



/*************************************************************************************************/
/*  Pipe                                                                                         */
/*************************************************************************************************/

DvzPipe dvz_pipe(DvzGpu* gpu)
{
    ANN(gpu);
    DvzPipe pipe = {0};
    pipe.obj.type = DVZ_OBJECT_TYPE_PIPE;
    pipe.gpu = gpu;
    dvz_obj_init(&pipe.obj);
    return pipe;
}



DvzGraphics* dvz_pipe_graphics(DvzPipe* pipe)
{
    ANN(pipe);

    pipe->type = DVZ_PIPE_GRAPHICS;
    DvzGraphics* graphics = &pipe->u.graphics;
    *graphics = dvz_graphics(pipe->gpu);

    return graphics;
}



DvzCompute* dvz_pipe_compute(DvzPipe* pipe, const char* shader_path)
{
    ANN(pipe);
    ANN(shader_path);

    pipe->type = DVZ_PIPE_COMPUTE;

    DvzCompute* compute = &pipe->u.compute;
    *compute = dvz_compute(pipe->gpu, shader_path);

    return compute;
}



void dvz_pipe_vertex(DvzPipe* pipe, uint32_t binding_idx, DvzDat* dat_vertex, DvzSize offset)
{
    ANN(pipe);
    ANN(dat_vertex);
    ASSERT(binding_idx < DVZ_MAX_VERTEX_BINDINGS);
    pipe->vertex_bindings[binding_idx].binding_idx = binding_idx;
    pipe->vertex_bindings[binding_idx].dat = dat_vertex;
    pipe->vertex_bindings[binding_idx].offset = offset;

    // Update the number of vertex bindings.
    pipe->vertex_bindings_count = MAX(pipe->vertex_bindings_count, binding_idx + 1);
}



void dvz_pipe_index(DvzPipe* pipe, DvzDat* dat_index, DvzSize offset)
{
    ANN(pipe);
    ANN(dat_index);
    pipe->index_binding.dat = dat_index;
    pipe->index_binding.offset = offset;
}



void dvz_pipe_dat(DvzPipe* pipe, uint32_t idx, DvzDat* dat)
{
    ANN(pipe);
    ASSERT(idx < DVZ_MAX_BINDINGS);

    ANN(dat);
    ANN(dat->br.buffer);
    ASSERT(dat->br.size > 0);

    pipe->bindings_set[idx] = true;
    // pipe->dats[idx] = dat;

    // Create the bindings if needed.
    _ensure_bindings_created(pipe, dat->br.count);

    dvz_bindings_buffer(&pipe->bindings, idx, dat->br);
}



void dvz_pipe_tex(DvzPipe* pipe, uint32_t idx, DvzTex* tex, DvzSampler* sampler)
{
    ANN(pipe);
    ASSERT(idx < DVZ_MAX_BINDINGS);

    ANN(tex);
    ANN(sampler);
    // pipe->texs[idx] = tex;
    // pipe->samplers[idx] = sampler;

    pipe->bindings_set[idx] = true;

    // Create the bindings if needed.
    _ensure_bindings_created(pipe, tex->img->count);

    dvz_bindings_texture(&pipe->bindings, idx, tex->img, sampler);
}



bool dvz_pipe_complete(DvzPipe* pipe)
{
    ANN(pipe);
    if (pipe->bindings.slots == NULL)
        return false;
    return _all_set(pipe->bindings.slots->slot_count, pipe->bindings_set);
}



void dvz_pipe_create(DvzPipe* pipe)
{
    ANN(pipe);
    log_trace("creating pipe");

    // Create the bindings if needed.
    if (pipe->bindings.dset_count == 0)
    {
        log_debug("by default, create bindings with dset count=1");
        _ensure_bindings_created(pipe, 1);
    }

    // Graphics pipe.
    if (pipe->type == DVZ_PIPE_GRAPHICS)
    {
        if (dvz_obj_is_created(&pipe->u.graphics.obj))
        {
            log_debug(
                "requesting pipe creation for an already-existing pipe, destroying it first");
            dvz_graphics_destroy(&pipe->u.graphics);
        }
        dvz_graphics_create(&pipe->u.graphics);
    }

    // Compute pipe.
    else if (pipe->type == DVZ_PIPE_COMPUTE)
    {
        if (dvz_obj_is_created(&pipe->u.graphics.obj))
        {
            log_debug(
                "requesting pipe creation for an already-existing pipe, destroying it first");
            dvz_graphics_destroy(&pipe->u.graphics);
        }
        dvz_compute_create(&pipe->u.compute);
    }

    // if (dvz_obj_is_created(&pipe->bindings.obj))
    if (dvz_pipe_complete(pipe))
    {
        log_trace("update bindings upon pipe creation");
        dvz_bindings_update(&pipe->bindings);
    }

    dvz_obj_created(&pipe->obj);
}



void dvz_pipe_destroy(DvzPipe* pipe)
{
    ANN(pipe);

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
    ANN(pipe);
    ANN(cmds);

    ASSERT(pipe->type == DVZ_PIPE_GRAPHICS);
    DvzGraphics* graphics = &pipe->u.graphics;
    ANN(graphics);

    // Vertex bindings.
    uint32_t count = pipe->vertex_bindings_count;
    DvzBufferRegions brs[DVZ_MAX_VERTEX_BINDINGS] = {0};
    DvzSize offsets[DVZ_MAX_VERTEX_BINDINGS] = {0};
    DvzDat* dat = NULL;
    for (uint32_t i = 0; i < count; i++)
    {
        dat = pipe->vertex_bindings[i].dat;
        ANN(dat);
        ASSERT(pipe->vertex_bindings[i].binding_idx == i);
        brs[i] = dat->br;
        offsets[i] = pipe->vertex_bindings[i].offset;
    }
    dvz_cmd_bind_vertex_buffer(cmds, idx, count, brs, offsets);

    // Index buffer.
    if (pipe->index_binding.dat != NULL)
    {
        dvz_cmd_bind_index_buffer(
            cmds, idx, pipe->index_binding.dat->br, pipe->index_binding.offset);
    }

    // TODO: dynamic uniform buffer index
    dvz_cmd_bind_graphics(cmds, idx, graphics, &pipe->bindings, 0);

    return graphics;
}



void dvz_pipe_draw(
    DvzPipe* pipe, DvzCommands* cmds, uint32_t idx, uint32_t first_vertex, uint32_t vertex_count,
    uint32_t first_instance, uint32_t instance_count)
{
    // NOTE: this function is called by the recorder, in _process_command().
    DvzGraphics* graphics = _pre_draw(pipe, cmds, idx);
    ANN(graphics);
    dvz_cmd_draw(cmds, idx, first_vertex, vertex_count, first_instance, instance_count);
}



void dvz_pipe_draw_indexed(
    DvzPipe* pipe, DvzCommands* cmds, uint32_t idx, uint32_t first_index, uint32_t vertex_offset,
    uint32_t index_count, uint32_t first_instance, uint32_t instance_count)
{
    // NOTE: this function is called by the recorder, in _process_command().
    DvzGraphics* graphics = _pre_draw(pipe, cmds, idx);
    ANN(graphics);
    dvz_cmd_draw_indexed(
        cmds, idx, first_index, vertex_offset, index_count, first_instance, instance_count);
}



void dvz_pipe_draw_indirect(
    DvzPipe* pipe, DvzCommands* cmds, uint32_t idx, DvzDat* dat_indirect, uint32_t draw_count)
{
    // NOTE: this function is called by the recorder, in _process_command().
    ANN(dat_indirect);
    DvzGraphics* graphics = _pre_draw(pipe, cmds, idx);
    ANN(graphics);
    dvz_cmd_draw_indirect(cmds, idx, dat_indirect->br, draw_count);
}



void dvz_pipe_draw_indexed_indirect(
    DvzPipe* pipe, DvzCommands* cmds, uint32_t idx, DvzDat* dat_indirect, uint32_t draw_count)
{
    // NOTE: this function is called by the recorder, in _process_command().
    ANN(dat_indirect);
    DvzGraphics* graphics = _pre_draw(pipe, cmds, idx);
    ANN(graphics);
    dvz_cmd_draw_indexed_indirect(cmds, idx, dat_indirect->br, draw_count);
}



/*************************************************************************************************/
/*  Pipe compute commands                                                                        */
/*************************************************************************************************/

void dvz_pipe_run(DvzPipe* pipe, DvzCommands* cmds, uint32_t idx, uvec3 size)
{
    ANN(pipe);
    ANN(cmds);

    ASSERT(pipe->type == DVZ_PIPE_COMPUTE);
    DvzCompute* compute = &pipe->u.compute;
    ANN(compute);

    dvz_cmd_compute(cmds, idx, compute, size);
}
