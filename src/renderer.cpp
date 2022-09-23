/*************************************************************************************************/
/*  Renderer                                                                                     */
/*************************************************************************************************/

#include <map>

#include "_log.h"
#include "_map.h"
#include "board.h"
#include "canvas.h"
#include "context.h"
#include "graphics.h"
#include "pipe.h"
#include "pipelib.h"
#include "recorder.h"
#include "renderer.h"
#include "workspace.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define ROUTE(action, type, function)                                                             \
    rd->router->router[std::make_pair(DVZ_REQUEST_ACTION_##action, DVZ_REQUEST_OBJECT_##type)] =  \
        function;

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



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef void* (*DvzRouterCallback)(DvzRenderer*, DvzRequest);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

extern "C" struct DvzRouter
{
    std::map<std::pair<DvzRequestAction, DvzRequestObject>, DvzRouterCallback> router;
};



/*************************************************************************************************/
/*  Board                                                                                        */
/*************************************************************************************************/

static void* _board_create(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);
    log_trace("create board");

    ASSERT(req.content.board.width > 0);
    ASSERT(req.content.board.height > 0);

    DvzBoard* board = dvz_workspace_board(
        rd->workspace, req.content.board.width, req.content.board.height, req.flags);
    ANN(board);
    SET_ID(board)
    board->rgb = dvz_board_alloc(board);
    return (void*)board;
}



static void* _board_update(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);
    ASSERT(req.id != 0);
    log_trace("update board");

    GET_ID(DvzBoard, board, req.id)

    dvz_cmd_submit_sync(&board->cmds, DVZ_DEFAULT_QUEUE_RENDER);

    return NULL;
}



static void* _board_resize(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);
    ASSERT(req.id != 0);
    log_trace("resize board");

    ASSERT(req.content.board.width > 0);
    ASSERT(req.content.board.height > 0);

    GET_ID(DvzBoard, board, req.id)

    dvz_board_resize(board, req.content.board.width, req.content.board.height);

    return NULL;
}



static void* _board_background(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);
    ASSERT(req.id != 0);

    GET_ID(DvzBoard, board, req.id)

    // dvz_board_clear_color(board, req.content.board.background);
    dvz_board_recreate(board);

    return NULL;
}



static void* _board_delete(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);
    ASSERT(req.id != 0);
    log_trace("delete board");

    GET_ID(DvzBoard, board, req.id)

    dvz_board_free(board);
    dvz_board_destroy(board);
    return NULL;
}



/*************************************************************************************************/
/*  Canvas                                                                                       */
/*************************************************************************************************/

static void* _canvas_create(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);
    log_trace("create canvas");

    // NOTE: when creating a desktop canvas, we know the requested screen size, but not the
    // framebuffer size yet. This will be determined *after* the window has been created, which
    // will occur in the presenter (client-side), not on the renderer (server-side).

    DvzCanvas* canvas = dvz_workspace_canvas(
        rd->workspace, req.content.canvas.framebuffer_width, req.content.canvas.framebuffer_height,
        req.flags);
    ANN(canvas);
    SET_ID(canvas)

    // NOTE: we cannot create the canvas recorder yet, as we need the swapchain image count, and
    // this requires the canvas to be actually created. This is done by the presenter, after a
    // window and surface have been created.

    return (void*)canvas;
}



static void* _canvas_delete(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);
    ASSERT(req.id != 0);
    log_trace("delete canvas");

    GET_ID(DvzCanvas, canvas, req.id)

    ANN(canvas);
    if (canvas->recorder != NULL)
        dvz_recorder_destroy(canvas->recorder);
    dvz_canvas_destroy(canvas);
    return NULL;
}



/*************************************************************************************************/
/*  Graphics                                                                                     */
/*************************************************************************************************/

static void* _graphics_create(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);

    DvzGpu* gpu = rd->gpu;
    ANN(gpu);

    DvzPipe* pipe = NULL;

    bool is_offscreen = (req.flags & DVZ_REQUEST_FLAGS_OFFSCREEN) != 0;
    DvzRenderpass* renderpass =
        is_offscreen ? &rd->workspace->renderpass_offscreen : &rd->workspace->renderpass_desktop;

    // Create the pipe.
    log_trace("create pipelib graphics");
    pipe = dvz_pipelib_graphics(
        rd->pipelib, rd->ctx, renderpass, req.content.graphics.type, req.flags);
    ANN(pipe);
    SET_ID(pipe)

    return (void*)pipe;
}



static void* _graphics_vertex(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);
    ASSERT(req.id != 0);

    // Get the graphics pipe.
    GET_ID(DvzPipe, pipe, req.id)

    // Get the dat with the vertex data.
    GET_ID(DvzDat, dat, req.content.set_vertex.dat);

    // Link the two.
    pipe->dat_vertex = dat;

    return NULL;
}



/*************************************************************************************************/
/*  Computes                                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Pipes                                                                                        */
/*************************************************************************************************/

static void* _pipe_dat(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);
    ASSERT(req.id != 0);

    // Get the graphics pipe.
    GET_ID(DvzPipe, pipe, req.id)

    // Get the dat data.
    GET_ID(DvzDat, dat, req.content.set_dat.dat);

    // Link the dat.
    // pipe->dats[req.content.set_dat.slot_idx] = dat;

    dvz_pipe_dat(pipe, req.content.set_dat.slot_idx, dat);
    if (dvz_pipe_complete(pipe))
        dvz_bindings_update(&pipe->bindings);

    return NULL;
}



static void* _pipe_tex(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);
    ASSERT(req.id != 0);

    // Get the graphics pipe.
    GET_ID(DvzPipe, pipe, req.id)

    // Get the tex.
    GET_ID(DvzTex, tex, req.content.set_tex.tex);

    // Get the sampler.
    GET_ID(DvzSampler, sampler, req.content.set_tex.sampler);
    ANN(tex);

    // Link the tex.
    // pipe->texs[req.content.set_binding.slot_idx] = tex;

    dvz_pipe_tex(pipe, req.content.set_tex.slot_idx, tex, sampler);
    if (dvz_pipe_complete(pipe))
        dvz_bindings_update(&pipe->bindings);

    return NULL;
}



static void* _pipe_delete(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);
    ASSERT(req.id != 0);
    log_trace("delete pipe");

    GET_ID(DvzPipe, pipe, req.id)

    dvz_pipe_destroy(pipe);
    return NULL;
}



/*************************************************************************************************/
/*  Dat                                                                                          */
/*************************************************************************************************/

static void* _dat_create(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);
    log_trace("create dat");

    DvzDat* dat = dvz_dat(rd->ctx, req.content.dat.type, req.content.dat.size, req.flags);
    ANN(dat);
    SET_ID(dat)

    return (void*)dat;
}



static void* _dat_upload(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);
    ASSERT(req.id != 0);

    GET_ID(DvzDat, dat, req.id)
    ANN(dat->br.buffer);
    ASSERT(dat->br.size > 0);
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

    return NULL;
}



static void* _dat_resize(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);
    ASSERT(req.id != 0);
    log_trace("resize dat");

    GET_ID(DvzDat, dat, req.id)

    dvz_dat_resize(dat, req.content.dat.size);

    return NULL;
}



static void* _dat_delete(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);
    ASSERT(req.id != 0);
    log_trace("delete dat");

    GET_ID(DvzDat, dat, req.id)

    dvz_dat_destroy(dat);
    return NULL;
}



/*************************************************************************************************/
/*  Dat                                                                                          */
/*************************************************************************************************/

static void* _tex_create(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);
    log_trace("create tex");

    DvzTex* tex = dvz_tex(
        rd->ctx, req.content.tex.dims, req.content.tex.shape, req.content.tex.format, req.flags);
    ANN(tex);
    SET_ID(tex)

    return (void*)tex;
}



static void* _tex_upload(DvzRenderer* rd, DvzRequest req)
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

    return NULL;
}



static void* _tex_resize(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);
    ASSERT(req.id != 0);
    log_trace("resize tex");

    GET_ID(DvzTex, tex, req.id)

    dvz_tex_resize(tex, req.content.tex.shape);

    return NULL;
}



static void* _tex_delete(DvzRenderer* rd, DvzRequest req)
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

static void* _sampler_create(DvzRenderer* rd, DvzRequest req)
{

    ANN(rd);
    log_trace("create sampler");

    DvzSampler* sampler =
        dvz_resources_sampler(&rd->ctx->res, req.content.sampler.filter, req.content.sampler.mode);
    ANN(sampler);
    SET_ID(sampler)

    return (void*)sampler;
}



static void* _sampler_delete(DvzRenderer* rd, DvzRequest req)
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

static void* _record_append(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);
    ASSERT(req.id != 0);

    // Get the canvas.
    GET_ID(DvzCanvas, canvas, req.id)

    // Ensure the canvas Recorder exists.
    if (!canvas->recorder)
    {
        log_debug("renderer automatically creates recorder for canvas 0x%" PRIx64, req.id);
        canvas->recorder = dvz_recorder(canvas->render.swapchain.img_count, 0);
    }

    // Get the recorder command.
    DvzRecorderCommand* cmd = &req.content.record.command;
    cmd->canvas_id = req.id;

    // Reset the buffer when beginning a new record.
    if (cmd->type == DVZ_RECORDER_BEGIN)
        dvz_recorder_clear(canvas->recorder);

    // Directly append the record command, set in the request, into the recorder.
    dvz_recorder_append(canvas->recorder, *cmd);

    return NULL;
}



static void* _record_begin(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);

    // DvzRequestObject type = (DvzRequestObject)dvz_map_type(rd->map, req.id);

    GET_ID(DvzBoard, board, req.id)
    dvz_cmd_reset(&board->cmds, 0);
    dvz_board_begin(board, &board->cmds, 0);

    return NULL;
}



static void* _record_viewport(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);

    // DvzRequestObject type = (DvzRequestObject)dvz_map_type(rd->map, req.id);

    GET_ID(DvzBoard, board, req.id)
    dvz_board_viewport(
        board, &board->cmds, 0, //
        req.content.record.command.contents.v.offset, req.content.record.command.contents.v.shape);

    return NULL;
}



static void* _record_draw(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);
    GET_ID(DvzPipe, pipe, req.content.record.command.contents.draw_direct.pipe_id);

    // DvzRequestObject type = (DvzRequestObject)dvz_map_type(rd->map, req.id);

    GET_ID(DvzBoard, board, req.id)
    dvz_pipe_draw(
        pipe, &board->cmds, 0, //
        req.content.record.command.contents.draw_direct.first_vertex,
        req.content.record.command.contents.draw_direct.vertex_count);

    return NULL;
}



static void* _record_end(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);

    // DvzRequestObject type = (DvzRequestObject)dvz_map_type(rd->map, req.id);

    GET_ID(DvzBoard, board, req.id)
    dvz_board_end(board, &board->cmds, 0);

    return NULL;
}



static void* _record(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);

    DvzRequestObject type = (DvzRequestObject)dvz_map_type(rd->map, req.id);

    if (type == DVZ_REQUEST_OBJECT_BOARD)
    {
        switch (req.content.record.command.type)
        {
        case DVZ_RECORDER_BEGIN:
            _record_begin(rd, req);
            break;

        case DVZ_RECORDER_VIEWPORT:
            _record_viewport(rd, req);
            break;

        case DVZ_RECORDER_DRAW_DIRECT:
            _record_draw(rd, req);
            break;

        case DVZ_RECORDER_END:
            _record_end(rd, req);
            break;

        default:
            break;
        }
    }

    else if (type == DVZ_REQUEST_OBJECT_CANVAS)
    {
        _record_append(rd, req);
    }

    return NULL;
}



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _init_renderer(DvzRenderer* rd)
{
    ANN(rd);
    ANN(rd->gpu);
    rd->ctx = dvz_context(rd->gpu);
    rd->pipelib = dvz_pipelib(rd->ctx);
    // NOTE: the renderer flags are passed directly to the workspace flags for now
    rd->workspace = dvz_workspace(rd->gpu, rd->flags);
    rd->map = dvz_map();

    dvz_obj_init(&rd->obj);
}



static void _setup_router(DvzRenderer* rd)
{
    ANN(rd);

    rd->router = new DvzRouter();
    rd->router->router =
        std::map<std::pair<DvzRequestAction, DvzRequestObject>, DvzRouterCallback>();

    // Board.
    ROUTE(CREATE, BOARD, _board_create)
    ROUTE(UPDATE, BOARD, _board_update)
    ROUTE(RESIZE, BOARD, _board_resize)
    ROUTE(SET, BACKGROUND, _board_background)
    ROUTE(DELETE, BOARD, _board_delete)

    // Canvas.
    ROUTE(CREATE, CANVAS, _canvas_create)
    ROUTE(DELETE, CANVAS, _canvas_delete)

    // Graphics.
    ROUTE(CREATE, GRAPHICS, _graphics_create)
    ROUTE(SET, VERTEX, _graphics_vertex)
    ROUTE(BIND, DAT, _pipe_dat)
    ROUTE(BIND, TEX, _pipe_tex)
    ROUTE(DELETE, GRAPHICS, _pipe_delete)

    // TODO: computes.

    // Dat.
    ROUTE(CREATE, DAT, _dat_create)
    ROUTE(UPLOAD, DAT, _dat_upload)
    ROUTE(RESIZE, DAT, _dat_resize)
    ROUTE(DELETE, DAT, _dat_delete)

    // Tex.
    ROUTE(CREATE, TEX, _tex_create)
    ROUTE(UPLOAD, TEX, _tex_upload)
    ROUTE(RESIZE, TEX, _tex_resize)
    ROUTE(DELETE, TEX, _tex_delete)

    // Sampler.
    ROUTE(CREATE, SAMPLER, _sampler_create)
    ROUTE(DELETE, SAMPLER, _sampler_delete)

    // Command buffer recording.
    ROUTE(RECORD, RECORD, _record)
}



static void _update_mapping(DvzRenderer* rd, DvzRequest req, void* obj)
{
    ANN(rd);

    // Handle the id-object mapping.
    switch (req.action)
    {
        // Creation.
    case DVZ_REQUEST_ACTION_CREATE:
        ANN(obj);
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



void dvz_renderer_request(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);

    DvzRouterCallback cb = rd->router->router[std::make_pair(req.action, req.type)];
    if (cb == NULL)
    {
        log_error("no router function registered for action %d and type %d", req.action, req.type);
        return;
    }
    log_trace("processing renderer request action %d and type %d", req.action, req.type);

    // Call the router callback.
    void* obj = cb(rd, req);

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



DvzBoard* dvz_renderer_board(DvzRenderer* rd, DvzId id)
{
    ANN(rd);

    DvzBoard* board = (DvzBoard*)dvz_map_get(rd->map, id);
    ANN(board);
    return board;
}



DvzCanvas* dvz_renderer_canvas(DvzRenderer* rd, DvzId id)
{
    ANN(rd);

    DvzCanvas* canvas = (DvzCanvas*)dvz_map_get(rd->map, id);
    ANN(canvas);
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
    return pipe;
}



uint8_t* dvz_renderer_image(DvzRenderer* rd, DvzId board_id, DvzSize* size, uint8_t* rgb)
{
    ANN(rd);

    DvzBoard* board = (DvzBoard*)dvz_map_get(rd->map, board_id);
    ANN(board);

    // Find the pointer: either passed here, or the board-owned pointer.
    rgb = rgb != NULL ? rgb : board->rgb;
    ANN(rgb);

    // Download the image to the buffer.
    dvz_board_download(board, board->size, rgb);

    // Set the size.
    ANN(size);
    *size = board->size;

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

    dvz_map_destroy(rd->map);
    delete rd->router;

    dvz_obj_destroyed(&rd->obj);
    FREE(rd);
}
