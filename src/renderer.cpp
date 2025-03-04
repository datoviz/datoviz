/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Renderer                                                                                     */
/*************************************************************************************************/

#include <map>

#include "_log.h"
#include "_map.h"
#include "board.h"
#include "canvas.h"
#include "context.h"
#include "datoviz_enums.h"
#include "pipe.h"
#include "pipelib.h"
#include "recorder.h"
#include "renderer.h"
#include "renderer_utils.h"
#include "resources_utils.h"
#include "scene/graphics.h"
#include "shader.h"
#include "vklite.h"
#include "workspace.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

extern "C" struct DvzRouter
{
    std::map<std::pair<DvzRequestAction, DvzRequestObject>, DvzRendererCallback> router;
    std::map<std::pair<DvzRequestAction, DvzRequestObject>, void*> user_data;
};



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define REGISTER(action, object, cb)                                                              \
    dvz_renderer_register(rd, DVZ_REQUEST_ACTION_##action, DVZ_REQUEST_OBJECT_##object, cb, NULL);



/*************************************************************************************************/
/*  Router                                                                                       */
/*************************************************************************************************/

static void _init_renderer(DvzRenderer* rd)
{
    ANN(rd);
    ANN(rd->gpu);
    rd->ctx = dvz_context(rd->gpu);
    rd->pipelib = dvz_pipelib(rd->ctx);
    // NOTE: the renderer flags are passed directly to the workspace flags for now
    if ((rd->flags & DVZ_RENDERER_FLAGS_NO_WORKSPACE) == 0)
        rd->workspace = dvz_workspace(rd->gpu, rd->flags);
    rd->map = dvz_map();

    dvz_obj_init(&rd->obj);
}



static void _setup_router(DvzRenderer* rd)
{
    ANN(rd);

    rd->router = new DvzRouter();
    rd->router->router =
        std::map<std::pair<DvzRequestAction, DvzRequestObject>, DvzRendererCallback>();
    rd->router->user_data = std::map<std::pair<DvzRequestAction, DvzRequestObject>, void*>();

    // Canvas.
    REGISTER(CREATE, CANVAS, _canvas_create)
    REGISTER(UPDATE, CANVAS, _canvas_update)
    REGISTER(RESIZE, CANVAS, _canvas_resize)
    REGISTER(SET, BACKGROUND, _canvas_background)
    REGISTER(DELETE, CANVAS, _canvas_delete)

    // Graphics.
    REGISTER(CREATE, GRAPHICS, _graphics_create)
    REGISTER(SET, PRIMITIVE, _graphics_primitive)
    REGISTER(SET, DEPTH, _graphics_depth)
    REGISTER(SET, BLEND, _graphics_blend)
    REGISTER(SET, MASK, _graphics_mask)
    REGISTER(SET, POLYGON, _graphics_polygon)
    REGISTER(SET, CULL, _graphics_cull)
    REGISTER(SET, FRONT, _graphics_front)
    REGISTER(SET, SHADER, _graphics_shader)
    REGISTER(SET, VERTEX, _graphics_vertex)
    REGISTER(SET, VERTEX_ATTR, _graphics_vertex_attr)
    REGISTER(SET, SLOT, _graphics_slot)
    REGISTER(SET, PUSH, _graphics_push)
    REGISTER(SET, SPECIALIZATION, _graphics_specialization)
    REGISTER(DELETE, GRAPHICS, _graphics_delete)

    // Shaders.
    REGISTER(CREATE, SHADER, _shader_create)

    // Bindings.
    REGISTER(BIND, VERTEX, _graphics_bind_vertex)
    REGISTER(BIND, INDEX, _graphics_bind_index)
    REGISTER(BIND, DAT, _graphics_bind_dat)
    REGISTER(BIND, TEX, _graphics_bind_tex)

    // TODO: computes.

    // Dat.
    REGISTER(CREATE, DAT, _dat_create)
    REGISTER(UPLOAD, DAT, _dat_upload)
    REGISTER(RESIZE, DAT, _dat_resize)
    REGISTER(DELETE, DAT, _dat_delete)

    // Tex.
    REGISTER(CREATE, TEX, _tex_create)
    REGISTER(UPLOAD, TEX, _tex_upload)
    REGISTER(RESIZE, TEX, _tex_resize)
    REGISTER(DELETE, TEX, _tex_delete)

    // Sampler.
    REGISTER(CREATE, SAMPLER, _sampler_create)
    REGISTER(DELETE, SAMPLER, _sampler_delete)

    // Command buffer recording.
    REGISTER(RECORD, RECORD, _record_append)
}



static void _update_mapping(DvzRenderer* rd, DvzRequest req, void* obj)
{
    ANN(rd);

    // Handle the id-object mapping.
    switch (req.action)
    {
        // Creation.
    case DVZ_REQUEST_ACTION_CREATE:
        if (obj == NULL)
            break;
        ASSERT(req.id != DVZ_ID_NONE);

        log_trace("adding object type %d id 0x%" PRIx64 " to mapping", req.type, req.id);

        if (dvz_map_exists(rd->map, req.id))
        {
            log_error("error while creating the object, id Ox%" PRIx64 " already exists", req.id);
            break;
        }

        // Register the id with the created object
        dvz_map_add(rd->map, req.id, req.type, obj);

        break;

        // Deletion.
    case DVZ_REQUEST_ACTION_DELETE:

        ASSERT(req.id != DVZ_ID_NONE);

        log_trace("removing object type %d id 0x%" PRIx64 " from mapping", req.type, req.id);

        if (dvz_map_get(rd->map, req.id) == NULL)
        {
            log_error("error while deleting this object, this ID doesn't exist");
            break;
        }

        // Remove the id from the mapping.
        dvz_map_remove(rd->map, req.id);

        break;

    default:
        break;
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzRenderer* dvz_renderer(DvzGpu* gpu, int flags)
{
    ANN(gpu);
    DvzRenderer* rd = (DvzRenderer*)calloc(1, sizeof(DvzRenderer));
    ANN(rd);
    rd->gpu = gpu;
    rd->flags = flags;
    _init_renderer(rd);
    _setup_router(rd);
    return rd;
}



void dvz_renderer_register(
    DvzRenderer* rd, DvzRequestAction action, DvzRequestObject object_type, DvzRendererCallback cb,
    void* user_data)
{
    ANN(rd);
    auto key = std::make_pair(action, object_type);
    rd->router->router[key] = cb;
    rd->router->user_data[key] = user_data;
}



void dvz_renderer_request(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);

    auto key = std::make_pair(req.action, req.type);
    DvzRendererCallback cb = rd->router->router[key];
    if (cb == NULL)
    {
        log_error("no router function registered for action %d and type %d", req.action, req.type);
        return;
    }
    log_trace("processing renderer request action %d and type %d", req.action, req.type);
    // dvz_request_print(&req, 0);

    void* user_data = rd->router->user_data[key];

    // Call the renderer callback.
    void* obj = cb(rd, req, user_data);

    // Register the pointer in the map table, associated with its id.
    _update_mapping(rd, req, obj);
}



void dvz_renderer_requests(DvzRenderer* rd, uint32_t count, DvzRequest* reqs)
{
    ANN(rd);
    if (count == 0)
        return;
    ASSERT(count > 0);
    ANN(reqs);
    for (uint32_t i = 0; i < count; i++)
    {
        dvz_renderer_request(rd, reqs[i]);
    }
}



DvzCanvas* dvz_renderer_canvas(DvzRenderer* rd, DvzId id)
{
    ANN(rd);

    DvzCanvas* canvas = (DvzCanvas*)dvz_map_get(rd->map, id);
    if (canvas == NULL)
    {
        canvas = (DvzCanvas*)dvz_map_first(rd->map, DVZ_REQUEST_OBJECT_CANVAS);
    }

    return canvas;
}



DvzDat* dvz_renderer_dat(DvzRenderer* rd, DvzId id)
{
    ANN(rd);

    DvzDat* dat = (DvzDat*)dvz_map_get(rd->map, id);
    ANN(dat);
    return dat;
}



DvzTex* dvz_renderer_tex(DvzRenderer* rd, DvzId id)
{
    ANN(rd);

    DvzTex* tex = (DvzTex*)dvz_map_get(rd->map, id);
    ANN(tex);
    return tex;
}



DvzPipe* dvz_renderer_pipe(DvzRenderer* rd, DvzId id)
{
    ANN(rd);

    DvzPipe* pipe = (DvzPipe*)dvz_map_get(rd->map, id);
    ANN(pipe);

    // NOTE: if the status is NEED_RECREATE, this condition will be false and the
    // dvz_pipe_create() will be called. That function will ensure the pipeline is destroyed
    // before being recreated.
    if (!dvz_obj_is_created(&pipe->obj))
    {
        log_debug("lazily create pipe before using it for command buffer recording");
        dvz_pipe_create(pipe);
    }

    return pipe;
}



uint8_t* dvz_renderer_image(DvzRenderer* rd, DvzId canvas_id, DvzSize* size, uint8_t* rgb)
{
    ANN(rd);

    DvzCanvas* canvas = (DvzCanvas*)dvz_map_get(rd->map, canvas_id);
    ANN(canvas);

    if (canvas->obj.type == DVZ_OBJECT_TYPE_BOARD)
    {
        // Find the pointer: either passed here, or the board-owned pointer.
        rgb = rgb != NULL ? rgb : canvas->rgb;
        ANN(rgb);

        // Download the image to the buffer.
        dvz_board_download(canvas, canvas->size, rgb);

        // Set the size.
        ANN(size);
        *size = canvas->size;
    }

    // else if (bctype == DVZ_REQUEST_OBJECT_CANVAS)
    // {
    //     DvzCanvas* canvas = (DvzCanvas*)dvz_map_get(rd->map, canvas_id);
    //     ANN(canvas);


    //     // Start the image transition command buffers.
    //     DvzCommands cmds = dvz_commands(gpu, DVZ_DEFAULT_QUEUE_TRANSFER, 1);
    //     dvz_cmd_begin(&cmds, 0);

    //     DvzBarrier barrier = dvz_barrier(gpu);
    //     dvz_barrier_stages(
    //         &barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    //     dvz_barrier_images(&barrier, &canvas->render.staging);
    //     dvz_barrier_images_layout(
    //         &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    //     dvz_barrier_images_access(&barrier, 0, VK_ACCESS_TRANSFER_WRITE_BIT);
    //     dvz_cmd_barrier(&cmds, 0, &barrier);

    //     // Copy the image to the staging image.
    //     dvz_cmd_copy_image(&cmds, 0, &canvas->images, &canvas->render.staging);

    //     dvz_barrier_images_layout(
    //         &barrier, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
    //     dvz_barrier_images_access(
    //         &barrier, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT);
    //     dvz_cmd_barrier(&cmds, 0, &barrier);

    //     // End the cmds and submit them.
    //     dvz_cmd_end(&cmds, 0);
    //     dvz_cmd_submit_sync(&cmds, 0);

    //     // Now, copy the staging image into CPU memory.
    //     dvz_images_download(&canvas->render.staging, 0, 1, true, false, rgb);
    // }

    // Return the pointer.
    return rgb;
}



void dvz_renderer_destroy(DvzRenderer* rd)
{
    ANN(rd);
    log_trace("destroy the renderer");

    // This call destroys all canvases etc.
    dvz_workspace_destroy(rd->workspace);

    dvz_pipelib_destroy(rd->pipelib);
    dvz_context_destroy(rd->ctx);
    dvz_gpu_wait(rd->gpu);

    dvz_map_destroy(rd->map);
    delete rd->router;

    dvz_obj_destroyed(&rd->obj);
    FREE(rd);
    log_trace("renderer destroyed");
}
