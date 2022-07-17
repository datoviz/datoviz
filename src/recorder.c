/*************************************************************************************************/
/*  Recorder                                                                                     */
/*************************************************************************************************/

#include "recorder.h"
#include "canvas.h"
#include "common.h"
#include "pipe.h"
#include "renderer.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void
_process_command(DvzRecorderCommand* record, DvzRenderer* rnd, DvzCommands* cmds, uint32_t img_idx)
{
    ASSERT(record != NULL);
    ASSERT(rnd != NULL);
    ASSERT(cmds != NULL);
    ASSERT(img_idx < cmds->count);

    DvzCanvas* canvas = dvz_renderer_canvas(rnd, record->canvas_id);
    ASSERT(canvas != NULL);

    DvzPipe* pipe = NULL;
    DvzDat* dat_indirect = NULL;

    switch (record->type)
    {

    case DVZ_RECORDER_BEGIN:
        log_debug("recorder: begin (#%d)", img_idx);
        dvz_cmd_reset(cmds, img_idx);
        dvz_canvas_begin(canvas, cmds, img_idx);
        break;

    case DVZ_RECORDER_VIEWPORT:;
        float x = record->contents.v.offset[0];
        float y = record->contents.v.offset[1];
        float w = record->contents.v.shape[0];
        float h = record->contents.v.shape[1];
        log_debug("recorder: viewport %0.0fx%0.0f -> %0.0fx%0.0f (#%d)", x, y, w, h, img_idx);
        dvz_canvas_viewport(
            canvas, cmds, img_idx, record->contents.v.offset, record->contents.v.shape);
        break;

    case DVZ_RECORDER_DRAW_DIRECT:;
        uint32_t first_vertex = record->contents.draw_direct.first_vertex;
        uint32_t vertex_count = record->contents.draw_direct.vertex_count;
        log_debug(
            "recorder: draw direct from vertex #%d for %d vertices (#%d)", //
            first_vertex, vertex_count, img_idx);
        pipe = dvz_renderer_pipe(rnd, record->contents.draw_direct.pipe_id);
        ASSERT(pipe != NULL);
        dvz_pipe_draw(pipe, cmds, img_idx, first_vertex, vertex_count);
        break;

    case DVZ_RECORDER_DRAW_DIRECT_INDEXED:;
        pipe = dvz_renderer_pipe(rnd, record->contents.draw_direct_indexed.pipe_id);
        ASSERT(pipe != NULL);
        dvz_pipe_draw_indexed(
            pipe, cmds, img_idx,                                //
            record->contents.draw_direct_indexed.first_index,   //
            record->contents.draw_direct_indexed.vertex_offset, //
            record->contents.draw_direct_indexed.index_count);
        break;

    case DVZ_RECORDER_DRAW_INDIRECT:
        pipe = dvz_renderer_pipe(rnd, record->contents.draw_indirect.pipe_id);
        ASSERT(pipe != NULL);

        dat_indirect = dvz_renderer_dat(rnd, record->contents.draw_indirect.dat_indirect_id);
        ASSERT(dat_indirect != NULL);

        dvz_pipe_draw_indirect(pipe, cmds, img_idx, dat_indirect);
        break;

    case DVZ_RECORDER_DRAW_INDIRECT_INDEXED:
        pipe = dvz_renderer_pipe(rnd, record->contents.draw_indirect.pipe_id);
        ASSERT(pipe != NULL);

        dat_indirect = dvz_renderer_dat(rnd, record->contents.draw_indirect.dat_indirect_id);
        ASSERT(dat_indirect != NULL);

        dvz_pipe_draw_indexed_indirect(pipe, cmds, img_idx, dat_indirect);
        break;

    case DVZ_RECORDER_END:
        log_debug("recorder: end (#%d)", img_idx);
        dvz_canvas_end(canvas, cmds, img_idx);
        break;

    default:
        log_error("unknown record command with type %d", record->type);
        break;
    }
}



/*************************************************************************************************/
/*  Recorder functions                                                                           */
/*************************************************************************************************/

DvzRecorder* dvz_recorder(uint32_t img_count)
{
    ASSERT(img_count > 0);
    DvzRecorder* recorder = calloc(1, sizeof(DvzRecorder));
    recorder->img_count = img_count;
    recorder->capacity = DVZ_RECORDER_COMMAND_COUNT;
    recorder->commands = calloc(recorder->capacity, sizeof(DvzRecorderCommand));
    return recorder;
}



void dvz_recorder_clear(DvzRecorder* recorder)
{
    ASSERT(recorder != NULL);
    log_debug("clear recorder commands");
    recorder->count = 0;
    dvz_recorder_need_refill(recorder);
}



void dvz_recorder_append(DvzRecorder* recorder, DvzRecorderCommand rc)
{
    ASSERT(recorder != NULL);
    ASSERT(rc.canvas_id != 0);
    log_debug("append recorder command");

    if (recorder->count >= recorder->capacity)
    {
        recorder->capacity *= 2;
        REALLOC(recorder->commands, recorder->capacity * sizeof(DvzRecorderCommand))
    }
    ASSERT(recorder->count < recorder->capacity);
    recorder->commands[recorder->count++] = rc;
}



void dvz_recorder_set(DvzRecorder* recorder, DvzRenderer* rnd, DvzCommands* cmds, uint32_t img_idx)
{
    ASSERT(recorder != NULL);

    // this function updates the command buffer for the given swapchain image index, only if needed
    if (!recorder->dirty[img_idx])
        return;

    // Go through all record commands and update the command buffer
    for (uint32_t i = 0; i < recorder->count; i++)
    {
        _process_command(&recorder->commands[i], rnd, cmds, img_idx);
    }

    recorder->dirty[img_idx] = false;
}



bool dvz_recorder_is_dirty(DvzRecorder* recorder, uint32_t img_idx)
{
    ASSERT(recorder != NULL);
    return recorder->dirty[img_idx];
}



void dvz_recorder_need_refill(DvzRecorder* recorder)
{
    ASSERT(recorder != NULL);

    // Reset dirty to true for all swapchain image indices.
    memset(recorder->dirty, 1, sizeof(recorder->dirty));
}



uint32_t dvz_recorder_size(DvzRecorder* recorder)
{
    ASSERT(recorder != NULL);
    return (recorder->count);
}



void dvz_recorder_destroy(DvzRecorder* recorder)
{
    ASSERT(recorder != NULL);
    FREE(recorder->commands);
    FREE(recorder);
}
