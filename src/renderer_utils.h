/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Renderer                                                                                     */
/*************************************************************************************************/

#ifndef DVZ_HEADER_RENDERER_UTILS
#define DVZ_HEADER_RENDERER_UTILS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <map>

#include "_log.h"
#include "_map.h"
#include "board.h"
#include "canvas.h"
#include "context.h"
#include "datoviz_enums.h"
#include "datoviz_macros.h"
#include "pipe.h"
#include "pipelib.h"
#include "recorder.h"
#include "renderer.h"
#include "resources_utils.h"
#include "scene/graphics.h"
#include "shader.h"
#include "vklite.h"
#include "workspace.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define SET_ID(x)                                                                                 \
    ASSERT(req.id != DVZ_ID_NONE);                                                                \
    (x)->obj.id = req.id;

#define GET_ID(t, n, i)                                                                           \
    t* n = (t*)dvz_map_get(rd->map, i);                                                           \
    if (n == NULL)                                                                                \
    {                                                                                             \
        log_error("%s Ox%" PRIx64 " doesn't exist", #n, i);                                       \
        return NULL;                                                                              \
    }                                                                                             \
    ANN(n);



EXTERN_C_ON

/*************************************************************************************************/
/*  Canvas                                                                                       */
/*************************************************************************************************/

static void* _canvas_create(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    ANN(rd);
    ANN(rd->gpu);
    ANN(rd->gpu->host);
    log_trace("create canvas");

    if (rd->workspace == NULL)
    {
        return NULL;
    }

    DvzCanvas* canvas = NULL;

    // NOTE: when creating a desktop canvas, we know the requested screen size, but not the
    // framebuffer size yet. This will be determined *after* the window has been created, which
    // will occur in the presenter (client-side), not on the renderer (server-side).

    // NOTE: force offscreen rendering for all canvases with offscreen backend.

    if ((rd->flags & DVZ_RENDERER_FLAGS_OFFSCREEN) != 0)
    {
        log_trace("forcing created canvas to be offscreen as the backend is offscreen");
        req.content.canvas.is_offscreen = true;
    }

    if (req.content.canvas.is_offscreen)
    {
        log_trace("renderer create board");

        if (req.content.canvas.framebuffer_width == 0)
        {
            log_debug("offscreen canvas creation request has framebuffer_width==0, using "
                      "screen_width instead");
            req.content.canvas.framebuffer_width = req.content.canvas.screen_width;
        }
        if (req.content.canvas.framebuffer_height == 0)
        {
            log_debug("offscreen canvas creation request has framebuffer_height==0, using "
                      "screen_height instead");
            req.content.canvas.framebuffer_height = req.content.canvas.screen_height;
        }

        ASSERT(req.content.canvas.framebuffer_width > 0);
        ASSERT(req.content.canvas.framebuffer_height > 0);

        canvas = dvz_workspace_board(
            rd->workspace,                         //
            req.content.canvas.framebuffer_width,  //
            req.content.canvas.framebuffer_height, //
            req.flags);
        ANN(canvas);
        SET_ID(canvas)
        canvas->rgb = dvz_board_alloc(canvas);
    }
    else
    {
        log_trace("renderer create canvas");
        canvas = dvz_workspace_canvas(
            rd->workspace,                         //
            req.content.canvas.framebuffer_width,  //
            req.content.canvas.framebuffer_height, //
            req.flags);
        ANN(canvas);
        SET_ID(canvas)
    }

    // NOTE: we cannot create the canvas recorder yet, as we need the swapchain image count, and
    // this requires the canvas to be actually created. This is done by the presenter, after a
    // window and surface have been created.

    return (void*)canvas;
}



static void* _canvas_background(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    ANN(rd);
    ASSERT(req.id != 0);

    GET_ID(DvzCanvas, canvas, req.id)

    // TODO
    // dvz_board_clear_color(board, req.content.board.background);
    dvz_board_recreate(canvas);

    return NULL;
}

static void* _canvas_update(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    ANN(rd);
    ASSERT(req.id != 0);
    log_trace("update canvas");

    GET_ID(DvzCanvas, canvas, req.id)

    dvz_cmd_submit_sync(&canvas->cmds, DVZ_DEFAULT_QUEUE_RENDER);

    return NULL;
}



static void* _canvas_resize(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    ANN(rd);
    ASSERT(req.id != 0);

    uint32_t w = req.content.canvas.framebuffer_width;
    uint32_t h = req.content.canvas.framebuffer_height;
    ASSERT(w > 0);
    ASSERT(h > 0);

    log_debug("resize canvas to %dx%d", w, h);

    GET_ID(DvzCanvas, canvas, req.id)
    if (canvas->obj.type == DVZ_OBJECT_TYPE_BOARD)
    {
        dvz_board_resize(canvas, w, h);
    }

    return NULL;
}



static void* _canvas_delete(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    ANN(rd);
    ASSERT(req.id != 0);
    log_trace("delete canvas");

    GET_ID(DvzCanvas, canvas, req.id)
    ANN(canvas);

    if (canvas->recorder != NULL)
    {
        dvz_recorder_destroy(canvas->recorder);
        canvas->recorder = NULL;
    }

    if (canvas->obj.type == DVZ_OBJECT_TYPE_CANVAS)
    {
        dvz_canvas_destroy(canvas);
    }
    else if (canvas->obj.type == DVZ_OBJECT_TYPE_BOARD)
    {
        dvz_board_destroy(canvas);
    }

    return NULL;
}



/*************************************************************************************************/
/*  Shaders                                                                                      */
/*************************************************************************************************/

static void* _shader_create(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    ANN(rd);

    // Create the pipe.
    log_trace("create shader");

    // NOTE: the passed code and buffer will be copied by the shader object.
    DvzShader* shader = dvz_pipelib_shader(
        rd->pipelib, req.content.shader.format, req.content.shader.type, //
        req.content.shader.size, req.content.shader.code, req.content.shader.buffer);
    ANN(shader);

    // Now we can free code and buffer as they've been copied by the shader in pipelib.
    FREE(req.content.shader.code);
    FREE(req.content.shader.buffer);

    SET_ID(shader)
    return (void*)shader;
}



/*************************************************************************************************/
/*  Graphics                                                                                     */
/*************************************************************************************************/

static void* _graphics_create(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    ANN(rd);

    DvzGpu* gpu = rd->gpu;
    ANN(gpu);
    ANN(gpu->host);

    DvzPipe* pipe = NULL;

    bool is_offscreen = (req.flags & DVZ_GRAPHICS_REQUEST_FLAGS_OFFSCREEN) != 0;

    // TODO: backend
    // if (gpu->host->backend && !is_offscreen)
    // {
    //     log_debug("non-offscreen graphics pipeline creation was requested with an offscreen "
    //               "backend, forcing offscreen pipepline");
    //     is_offscreen = true;
    // }
    DvzRenderpass* renderpass =
        is_offscreen ? &rd->workspace->renderpass_offscreen : &rd->workspace->renderpass_desktop;

    // Create the pipe.
    log_trace("create pipelib graphics, offscreen=%d", is_offscreen);
    pipe = dvz_pipelib_graphics(
        rd->pipelib, rd->ctx, renderpass, req.content.graphics.type, req.flags);
    ANN(pipe);
    SET_ID(pipe)

    return (void*)pipe;
}



// Helper function to retrieve the DvzGraphics* pointer of graphics creation request.
static inline DvzGraphics* _get_graphics(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    ANN(rd);
    ASSERT(req.id != 0);

    // Get the graphics pipe.
    GET_ID(DvzPipe, pipe, req.id)

    // If calling a graphics modification function while the pipe has already been created, mark it
    // has needing to be recreated.
    if (dvz_obj_is_created(&pipe->obj))
    {
        pipe->obj.status = DVZ_OBJECT_STATUS_NEED_RECREATE;
    }

    // Get the graphics object.
    ASSERT(pipe->type == DVZ_PIPE_GRAPHICS);
    DvzGraphics* graphics = &pipe->u.graphics;

    return graphics;
}



static void* _graphics_primitive(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    DvzGraphics* graphics = _get_graphics(rd, req, user_data);
    ASSERT(req.type == DVZ_REQUEST_OBJECT_PRIMITIVE);

    // NOTE: we assume VkPrimitiveTopology and DvzPrimitiveTopology match.
    dvz_graphics_primitive(graphics, (VkPrimitiveTopology)req.content.set_primitive.primitive);

    return NULL;
}



static void* _graphics_depth(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    DvzGraphics* graphics = _get_graphics(rd, req, user_data);
    ASSERT(req.type == DVZ_REQUEST_OBJECT_DEPTH);

    dvz_graphics_depth_test(graphics, req.content.set_depth.depth);

    return NULL;
}



static void* _graphics_blend(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    DvzGraphics* graphics = _get_graphics(rd, req, user_data);
    ASSERT(req.type == DVZ_REQUEST_OBJECT_BLEND);

    dvz_graphics_blend(graphics, req.content.set_blend.blend);

    return NULL;
}



static void* _graphics_mask(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    DvzGraphics* graphics = _get_graphics(rd, req, user_data);
    ASSERT(req.type == DVZ_REQUEST_OBJECT_MASK);

    dvz_graphics_mask(graphics, req.content.set_mask.mask);

    return NULL;
}



static void* _graphics_polygon(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    DvzGraphics* graphics = _get_graphics(rd, req, user_data);
    ASSERT(req.type == DVZ_REQUEST_OBJECT_POLYGON);

    // NOTE: we assume VkPolygonMode and DvzPolygonMode match.
    dvz_graphics_polygon_mode(graphics, (VkPolygonMode)req.content.set_polygon.polygon);

    return NULL;
}



static void* _graphics_cull(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    DvzGraphics* graphics = _get_graphics(rd, req, user_data);
    ASSERT(req.type == DVZ_REQUEST_OBJECT_CULL);

    // NOTE: we assume VkCullModeFlags and DvzCullMode match.
    dvz_graphics_cull_mode(graphics, (VkCullModeFlags)req.content.set_cull.cull);

    return NULL;
}



static void* _graphics_front(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    DvzGraphics* graphics = _get_graphics(rd, req, user_data);
    ASSERT(req.type == DVZ_REQUEST_OBJECT_FRONT);

    // NOTE: we assume VkFrontFace and DvzFrontFace match.
    dvz_graphics_front_face(graphics, (VkFrontFace)req.content.set_front.front);

    return NULL;
}



static void* _graphics_shader(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    DvzGraphics* graphics = _get_graphics(rd, req, user_data);
    ASSERT(req.type == DVZ_REQUEST_OBJECT_SHADER);

    // Get the shader object.
    GET_ID(DvzShader, shader, req.content.set_shader.shader)

    DvzShaderFormat format = shader->format;
    ASSERT(format != DVZ_SHADER_NONE);

    if (format == DVZ_SHADER_GLSL)
    {
        // NOTE: we assume VkShaderStageFlagBits and DvzShaderType match.
        dvz_graphics_shader_glsl(graphics, (VkShaderStageFlagBits)shader->type, shader->code);

        // NOTE: the code has been copied by the requester, we can free it now.
        // FREE(shader->code);
    }
    else if (format == DVZ_SHADER_SPIRV)
    {
        // NOTE: we assume VkShaderStageFlagBits and DvzShaderType match.
        dvz_graphics_shader_spirv(
            graphics, (VkShaderStageFlagBits)shader->type, shader->size, shader->buffer);

        // NOTE: the buffer has been copied by the requester, we can free it now.
        // FREE(shader->buffer);
    }

    return NULL;
}



static void* _graphics_vertex(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    DvzGraphics* graphics = _get_graphics(rd, req, user_data);
    ASSERT(req.type == DVZ_REQUEST_OBJECT_VERTEX);

    // NOTE: we assume VkVertexInputRate and DvzVertexInputRate match.
    dvz_graphics_vertex_binding(
        graphics, req.content.set_vertex.binding_idx, req.content.set_vertex.stride,
        (VkVertexInputRate)req.content.set_vertex.input_rate);

    return NULL;
}



static void* _graphics_vertex_attr(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    DvzGraphics* graphics = _get_graphics(rd, req, user_data);
    ASSERT(req.type == DVZ_REQUEST_OBJECT_VERTEX_ATTR);

    // NOTE: we assume VkFormat and DvzFormat match.
    dvz_graphics_vertex_attr(
        graphics, req.content.set_attr.binding_idx, req.content.set_attr.location,
        (VkFormat)req.content.set_attr.format, req.content.set_attr.offset);

    return NULL;
}



static void* _graphics_slot(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    DvzGraphics* graphics = _get_graphics(rd, req, user_data);
    ASSERT(req.type == DVZ_REQUEST_OBJECT_SLOT);

    // NOTE: we assume VkDescriptorType and DvzDescriptorType match.
    dvz_graphics_slot(
        graphics, req.content.set_slot.slot_idx, (VkDescriptorType)req.content.set_slot.type);

    return NULL;
}



static void* _graphics_push(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    DvzGraphics* graphics = _get_graphics(rd, req, user_data);
    ASSERT(req.type == DVZ_REQUEST_OBJECT_PUSH);

    VkShaderStageFlagBits stages = (VkShaderStageFlagBits)req.content.set_push.shader_stages;
    dvz_graphics_push(graphics, stages, req.content.set_push.offset, req.content.set_push.size);

    return NULL;
}



static void* _graphics_specialization(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    DvzGraphics* graphics = _get_graphics(rd, req, user_data);
    ASSERT(req.type == DVZ_REQUEST_OBJECT_SPECIALIZATION);

    // HACK: from DvzShaderType to VkShaderStageFlagBits.
    VkShaderStageFlagBits stage = req.content.set_specialization.shader == DVZ_SHADER_VERTEX
                                      ? VK_SHADER_STAGE_VERTEX_BIT
                                      : VK_SHADER_STAGE_FRAGMENT_BIT;
    dvz_graphics_specialization(
        graphics, stage, req.content.set_specialization.idx, //
        req.content.set_specialization.size, req.content.set_specialization.value);
    // NOTE: we can safely FREE the data now.
    FREE(req.content.set_specialization.value);

    return NULL;
}



static void* _graphics_delete(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    ANN(rd);
    ASSERT(req.id != 0);
    log_trace("delete pipe");

    GET_ID(DvzPipe, pipe, req.id)

    dvz_pipe_destroy(pipe);
    return NULL;
}



/*************************************************************************************************/
/*  Computes                                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Bindings                                                                                     */
/*************************************************************************************************/

static void* _graphics_bind_vertex(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    ANN(rd);
    ASSERT(req.id != 0);

    // Get the graphics pipe.
    GET_ID(DvzPipe, pipe, req.id)

    // Get the dat with the vertex data.
    GET_ID(DvzDat, dat, req.content.bind_vertex.dat);

    if (_is_dat_valid(dat))
    {
        // Link the two.
        dvz_pipe_vertex(
            pipe, req.content.bind_vertex.binding_idx, dat, req.content.bind_vertex.offset);
    }

    return NULL;
}



static void* _graphics_bind_index(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    ANN(rd);
    ASSERT(req.id != 0);

    // Get the graphics pipe.
    GET_ID(DvzPipe, pipe, req.id)

    // Get the dat with the index data.
    GET_ID(DvzDat, dat, req.content.bind_index.dat);

    if (_is_dat_valid(dat))
    {
        // Link the two.
        dvz_pipe_index(pipe, dat, req.content.bind_index.offset);
    }

    return NULL;
}



static void* _graphics_bind_dat(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    ANN(rd);
    ASSERT(req.id != 0);

    // Get the graphics pipe.
    GET_ID(DvzPipe, pipe, req.id)

    // Get the dat data.
    GET_ID(DvzDat, dat, req.content.bind_dat.dat);


    if (_is_dat_valid(dat))
    {
        dvz_pipe_dat(pipe, req.content.bind_dat.slot_idx, dat);
        if (dvz_pipe_complete(pipe))
            dvz_descriptors_update(&pipe->descriptors);
    }

    return NULL;
}



static void* _graphics_bind_tex(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    ANN(rd);
    ASSERT(req.id != 0);

    // Get the graphics pipe.
    GET_ID(DvzPipe, pipe, req.id)

    // Get the tex.
    GET_ID(DvzTex, tex, req.content.bind_tex.tex);

    // Get the sampler.
    GET_ID(DvzSampler, sampler, req.content.bind_tex.sampler);
    ANN(tex);

    // Link the tex.
    // pipe->texs[req.content.set_binding.slot_idx] = tex;

    dvz_pipe_tex(pipe, req.content.bind_tex.slot_idx, tex, sampler);
    if (dvz_pipe_complete(pipe))
        dvz_descriptors_update(&pipe->descriptors);

    return NULL;
}



/*************************************************************************************************/
/*  Dat                                                                                          */
/*************************************************************************************************/

static void* _dat_create(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    ANN(rd);
    log_trace("create dat");

    DvzDat* dat = dvz_dat(rd->ctx, req.content.dat.type, req.content.dat.size, req.flags);
    ANN(dat);
    SET_ID(dat)

    return (void*)dat;
}



static void* _dat_upload(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    ANN(rd);
    ASSERT(req.id != 0);

    GET_ID(DvzDat, dat, req.id)

    if (!_is_dat_valid(dat))
    {
        return NULL;
    }

    ANN(dat->br.buffer);
    ASSERT(dat->br.size > 0);
    ANN(req.content.dat_upload.data);
    ASSERT(req.content.dat_upload.size > 0);

    // Make sure the target dat is large enough to hold the uploaded data.
    if (req.content.dat_upload.size > dat->br.aligned_size)
    {
        log_debug(
            "data to upload is larger (%s) than the dat size (%s), resizing it",
            pretty_size(req.content.dat_upload.size), pretty_size(dat->br.aligned_size));
        dvz_dat_resize(dat, req.content.dat_upload.size);
        ASSERT(req.content.dat_upload.size <= dat->br.aligned_size);
    }

    log_trace(
        "uploading %s to dat (buffer type %d region offset %d)",
        pretty_size(req.content.dat_upload.size), dat->br.buffer->type, dat->br.offsets[0]);

    if ((dat->flags & DVZ_DAT_FLAGS_MAPPABLE) != 0)
    {
        dvz_buffer_regions_upload(
            &dat->br, 0,
            req.content.dat_upload.offset, //
            req.content.dat_upload.size,   //
            req.content.dat_upload.data    //
        );
    }
    else
    {
        dvz_dat_upload(
            dat,                           //
            req.content.dat_upload.offset, //
            req.content.dat_upload.size,   //
            req.content.dat_upload.data,   //
            true);                         // TODO: do not wait? try false
    }

    // We free the copy of the data that had been done by the requester in dvz_upload_dat().
    if ((req.flags & DVZ_UPLOAD_FLAGS_NOCOPY) == 0)
    {
        FREE(req.content.dat_upload.data);
    }
    // TODO: if we do not wait, the freeing must wait until the transfer is done.

    return NULL;
}



static void* _dat_resize(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    ANN(rd);
    ASSERT(req.id != 0);
    log_trace("resize dat");

    GET_ID(DvzDat, dat, req.id)

    if (_is_dat_valid(dat))
    {
        dvz_dat_resize(dat, req.content.dat.size);
    }

    return NULL;
}



static void* _dat_delete(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    ANN(rd);
    ASSERT(req.id != 0);
    log_trace("delete dat");

    GET_ID(DvzDat, dat, req.id)

    dvz_dat_destroy(dat);
    return NULL;
}



/*************************************************************************************************/
/*  Tex                                                                                          */
/*************************************************************************************************/

static void* _tex_create(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    ANN(rd);
    log_trace("create tex");

    DvzTex* tex = dvz_tex(
        rd->ctx, req.content.tex.dims, req.content.tex.shape, req.content.tex.format, req.flags);
    ANN(tex);
    SET_ID(tex)

    return (void*)tex;
}



static void* _tex_upload(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    ANN(rd);
    ASSERT(req.id != 0);

    GET_ID(DvzTex, tex, req.id)
    ANN(tex->img);
    ASSERT(req.content.tex_upload.size > 0);

    if ( //
        (req.content.tex_upload.offset[0] + req.content.tex_upload.shape[0] > tex->shape[0]) ||
        (req.content.tex_upload.offset[1] + req.content.tex_upload.shape[1] > tex->shape[1]) ||
        (req.content.tex_upload.offset[2] + req.content.tex_upload.shape[2] > tex->shape[2]))
    {
        log_error("tex to upload is larger than the tex shape");
        return NULL;
    }

    log_trace("uploading %s to tex", pretty_size(req.content.tex_upload.size));

    dvz_tex_upload(
        tex,                           //
        req.content.tex_upload.offset, //
        req.content.tex_upload.shape,  //
        req.content.tex_upload.size,   //
        req.content.tex_upload.data,   //
        true);                         // TODO: do not wait? try false

    // We free the copy of the data that had been done by the requester in dvz_upload_dat().
    FREE(req.content.tex_upload.data);
    // TODO: if we do not wait, the freeing must wait until the transfer is done.

    return NULL;
}



static void* _tex_resize(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    ANN(rd);
    ASSERT(req.id != 0);
    log_trace("resize tex");

    GET_ID(DvzTex, tex, req.id)

    dvz_tex_resize(tex, req.content.tex.shape);

    return NULL;
}



static void* _tex_delete(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    ANN(rd);
    ASSERT(req.id != 0);
    log_trace("delete tex");

    GET_ID(DvzTex, tex, req.id)

    dvz_tex_destroy(tex);
    return NULL;
}



/*************************************************************************************************/
/*  Sampler                                                                                      */
/*************************************************************************************************/

static void* _sampler_create(DvzRenderer* rd, DvzRequest req, void* user_data)
{

    ANN(rd);
    log_trace("create sampler");

    DvzSampler* sampler =
        dvz_resources_sampler(&rd->ctx->res, req.content.sampler.filter, req.content.sampler.mode);
    ANN(sampler);
    SET_ID(sampler)

    return (void*)sampler;
}



static void* _sampler_delete(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    ANN(rd);
    ASSERT(req.id != 0);
    log_trace("delete sampler");

    GET_ID(DvzSampler, sampler, req.id)

    dvz_sampler_destroy(sampler);
    return NULL;
}



/*************************************************************************************************/
/*  Command buffer recording                                                                     */
/*************************************************************************************************/

static DvzRecorder* _get_or_create_recorder(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);
    ANN(rd->map);
    ASSERT(req.id != 0);

    // Get the canvas.
    GET_ID(DvzCanvas, canvas, req.id)

    // Ensure the canvas Recorder exists.
    if (!canvas->recorder)
    {
        log_debug("renderer automatically creates recorder for canvas 0x%" PRIx64, req.id);
        canvas->recorder = dvz_recorder(0);
    }
    DvzRecorder* recorder = canvas->recorder;

    ANN(recorder);
    return recorder;
}



static void* _record_append(DvzRenderer* rd, DvzRequest req, void* user_data)
{
    // NOTE: this function is called whenever a RECORD request is processed by the renderer.
    ANN(rd);
    ANN(rd->map);
    if (req.id == DVZ_ID_NONE)
    {
        log_error("invalid record command on unspecified canvas #0");
        return NULL;
    }

    // Get the recorder command.
    DvzRecorderCommand* cmd = &req.content.record.command;
    ANN(cmd);

    // Keep track in DvzRecorderCommand of whether we're in a canvas.
    cmd->object_type = (DvzRequestObject)dvz_map_type(rd->map, req.id);
    cmd->canvas_id = req.id;

    DvzRecorder* recorder = _get_or_create_recorder(rd, req);
    ANN(recorder);

    // Reset the buffer when beginning a new record.
    if (cmd->type == DVZ_RECORDER_BEGIN)
    {
        dvz_recorder_clear(recorder);
    }

    // Directly append the record command, set in the request, into the recorder.
    dvz_recorder_append(recorder, *cmd);

    // If canvas, the presenter will take care of calling dvz_recorder_set() in the event loop.
    // If board, the recorder needs to be applied directly once the recording has finished.
    if (req.content.record.command.type == DVZ_RECORDER_END)
    {
        DvzCanvas* canvas = dvz_renderer_canvas(rd, req.id);
        ANN(canvas);

        if (canvas->obj.type == DVZ_OBJECT_TYPE_BOARD)
        {
            log_debug("applying the recorder to canvas 0x%" PRIx64, req.id);
            dvz_recorder_set(recorder, rd, &canvas->cmds, 0);
        }
    }

    return NULL;
}



EXTERN_C_OFF

#endif
