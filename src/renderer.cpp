/*************************************************************************************************/
/*  Renderer                                                                                     */
/*************************************************************************************************/

#include <map>

#include "_log.h"
#include "_map.h"
#include "board.h"
#include "canvas.h"
#include "context.h"
#include "pipe.h"
#include "pipelib.h"
#include "recorder.h"
#include "renderer.h"
#include "scene/graphics.h"
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



// static void* _canvas_update(DvzRenderer* rd, DvzRequest req)
// {
//     ANN(rd);
//     ASSERT(req.id != 0);
//     log_trace("update canvas");

//     GET_ID(DvzCanvas, canvas, req.id)

//     dvz_cmd_submit_sync(&canvas->cmds, DVZ_DEFAULT_QUEUE_RENDER);

//     return NULL;
// }



static void* _canvas_delete(DvzRenderer* rd, DvzRequest req)
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



// Helper function to retrieve the DvzGraphics* pointer of graphics creation request.
static inline DvzGraphics* _get_graphics(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);
    ASSERT(req.id != 0);

    // Get the graphics pipe.
    GET_ID(DvzPipe, pipe, req.id)

    // Get the graphics object.
    ASSERT(pipe->type == DVZ_PIPE_GRAPHICS);
    DvzGraphics* graphics = &pipe->u.graphics;

    return graphics;
}



static void* _graphics_primitive(DvzRenderer* rd, DvzRequest req)
{
    DvzGraphics* graphics = _get_graphics(rd, req);
    ASSERT(req.type == DVZ_REQUEST_OBJECT_PRIMITIVE);

    // NOTE: we assume VkPrimitiveTopology and DvzPrimitiveTopology match.
    dvz_graphics_primitive(graphics, (VkPrimitiveTopology)req.content.set_primitive.primitive);

    return NULL;
}



static void* _graphics_depth(DvzRenderer* rd, DvzRequest req)
{
    DvzGraphics* graphics = _get_graphics(rd, req);
    ASSERT(req.type == DVZ_REQUEST_OBJECT_DEPTH);

    dvz_graphics_depth_test(graphics, req.content.set_depth.depth);

    return NULL;
}



static void* _graphics_blend(DvzRenderer* rd, DvzRequest req)
{
    DvzGraphics* graphics = _get_graphics(rd, req);
    ASSERT(req.type == DVZ_REQUEST_OBJECT_BLEND);

    dvz_graphics_blend(graphics, req.content.set_blend.blend);

    return NULL;
}



static void* _graphics_polygon(DvzRenderer* rd, DvzRequest req)
{
    DvzGraphics* graphics = _get_graphics(rd, req);
    ASSERT(req.type == DVZ_REQUEST_OBJECT_POLYGON);

    // NOTE: we assume VkPolygonMode and DvzPolygonMode match.
    dvz_graphics_polygon_mode(graphics, (VkPolygonMode)req.content.set_polygon.polygon);

    return NULL;
}



static void* _graphics_cull(DvzRenderer* rd, DvzRequest req)
{
    DvzGraphics* graphics = _get_graphics(rd, req);
    ASSERT(req.type == DVZ_REQUEST_OBJECT_CULL);

    // NOTE: we assume VkCullModeFlags and DvzCullMode match.
    dvz_graphics_cull_mode(graphics, (VkCullModeFlags)req.content.set_cull.cull);

    return NULL;
}



static void* _graphics_front(DvzRenderer* rd, DvzRequest req)
{
    DvzGraphics* graphics = _get_graphics(rd, req);
    ASSERT(req.type == DVZ_REQUEST_OBJECT_FRONT);

    // NOTE: we assume VkFrontFace and DvzFrontFace match.
    dvz_graphics_front_face(graphics, (VkFrontFace)req.content.set_front.front);

    return NULL;
}



static void* _graphics_glsl(DvzRenderer* rd, DvzRequest req)
{
    DvzGraphics* graphics = _get_graphics(rd, req);
    ASSERT(req.type == DVZ_REQUEST_OBJECT_GLSL);

    // NOTE: we assume VkShaderStageFlagBits and DvzShaderType match.
    dvz_graphics_shader_glsl(
        graphics, (VkShaderStageFlagBits)req.content.set_glsl.shader_type,
        req.content.set_glsl.code);

    return NULL;
}



static void* _graphics_spirv(DvzRenderer* rd, DvzRequest req)
{
    DvzGraphics* graphics = _get_graphics(rd, req);
    ASSERT(req.type == DVZ_REQUEST_OBJECT_SPIRV);

    // NOTE: we assume VkShaderStageFlagBits and DvzShaderType match.
    dvz_graphics_shader_spirv(
        graphics, (VkShaderStageFlagBits)req.content.set_spirv.shader_type,
        req.content.set_spirv.size, req.content.set_spirv.buffer);

    return NULL;
}



static void* _graphics_vertex(DvzRenderer* rd, DvzRequest req)
{
    DvzGraphics* graphics = _get_graphics(rd, req);
    ASSERT(req.type == DVZ_REQUEST_OBJECT_VERTEX);

    // NOTE: we assume VkVertexInputRate and DvzVertexInputRate match.
    dvz_graphics_vertex_binding(
        graphics, req.content.set_vertex.binding_idx, req.content.set_vertex.stride,
        (VkVertexInputRate)req.content.set_vertex.input_rate);

    return NULL;
}



static void* _graphics_vertex_attr(DvzRenderer* rd, DvzRequest req)
{
    DvzGraphics* graphics = _get_graphics(rd, req);
    ASSERT(req.type == DVZ_REQUEST_OBJECT_VERTEX_ATTR);

    // NOTE: we assume VkFormat and DvzFormat match.
    dvz_graphics_vertex_attr(
        graphics, req.content.set_attr.binding_idx, req.content.set_attr.location,
        (VkFormat)req.content.set_attr.format, req.content.set_attr.offset);

    return NULL;
}



static void* _graphics_slot(DvzRenderer* rd, DvzRequest req)
{
    DvzGraphics* graphics = _get_graphics(rd, req);
    ASSERT(req.type == DVZ_REQUEST_OBJECT_SLOT);

    // NOTE: we assume VkDescriptorType and DvzDescriptorType match.
    dvz_graphics_slot(
        graphics, req.content.set_slot.slot_idx, (VkDescriptorType)req.content.set_slot.type);

    return NULL;
}



// TODO: remove this function once all graphics are manual (no more builtin graphics outside of
// scene/)
static void* _graphics_bind_vertex(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);
    ASSERT(req.id != 0);

    // Get the graphics pipe.
    GET_ID(DvzPipe, pipe, req.id)

    // Get the dat with the vertex data.
    GET_ID(DvzDat, dat, req.content.bind_vertex.dat);

    // Link the two.
    pipe->dat_vertex = dat;

    return NULL;
}



static void* _graphics_bind_index(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);
    ASSERT(req.id != 0);

    // Get the graphics pipe.
    GET_ID(DvzPipe, pipe, req.id)

    // Get the dat with the index data.
    GET_ID(DvzDat, dat, req.content.bind_index.dat);

    // Link the two.
    pipe->dat_index = dat;

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

    // We free the copy of the data that had been done by the requester in dvz_upload_dat().
    FREE(req.content.dat_upload.data);
    // TODO: if we do not wait, the freeing must wait until the transfer is done.

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

    // We free the copy of the data that had been done by the requester in dvz_upload_dat().
    FREE(req.content.tex_upload.data);
    // TODO: if we do not wait, the freeing must wait until the transfer is done.

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

static inline bool _is_canvas(DvzRenderer* rd, DvzId canvas_or_board_id)
{
    ASSERT(canvas_or_board_id != 0);

    DvzRequestObject type = (DvzRequestObject)dvz_map_type(rd->map, canvas_or_board_id);

    if (type != DVZ_REQUEST_OBJECT_CANVAS && type != DVZ_REQUEST_OBJECT_BOARD)
    {
        log_error(
            "type %d not supported, should be either BOARD (%d) or CANVAS (%d)", //
            type, DVZ_REQUEST_OBJECT_BOARD, DVZ_REQUEST_OBJECT_CANVAS);
        return false;
    }

    return type == DVZ_REQUEST_OBJECT_CANVAS;
}



static DvzRecorder* _get_or_create_recorder(DvzRenderer* rd, DvzRequest req)
{
    ANN(rd);
    ANN(rd->map);
    ASSERT(req.id != 0);

    bool is_canvas = _is_canvas(rd, req.id);
    DvzRecorder* recorder = NULL;

    if (!is_canvas)
    {
        // Get the board.
        GET_ID(DvzBoard, board, req.id)

        // Ensure the board Recorder exists.
        if (!board->recorder)
        {
            log_debug("renderer automatically creates recorder for board 0x%" PRIx64, req.id);
            board->recorder = dvz_recorder(0);
        }
        recorder = board->recorder;
    }
    else
    {
        // Get the canvas.
        GET_ID(DvzCanvas, canvas, req.id)

        // Ensure the canvas Recorder exists.
        if (!canvas->recorder)
        {
            log_debug("renderer automatically creates recorder for canvas 0x%" PRIx64, req.id);
            canvas->recorder = dvz_recorder(0);
        }
        recorder = canvas->recorder;
    }

    ANN(recorder);
    return recorder;
}



static void* _record_append(DvzRenderer* rd, DvzRequest req)
{
    // NOTE: this function is called whenever a RECORD request is processed by the renderer.

    ANN(rd);
    ANN(rd->map);

    // Get the recorder command.
    DvzRecorderCommand* cmd = &req.content.record.command;
    ANN(cmd);

    // Keep track in DvzRecorderCommand of whether we're in a canvas or board.
    cmd->object_type = (DvzRequestObject)dvz_map_type(rd->map, req.id);
    cmd->canvas_or_board_id = req.id;

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
    if (cmd->object_type == DVZ_REQUEST_OBJECT_BOARD &&
        req.content.record.command.type == DVZ_RECORDER_END)
    {
        log_debug("applying the recorder to board 0x%" PRIx64, req.id);

        DvzBoard* board = dvz_renderer_board(rd, req.id);
        ANN(board);
        dvz_recorder_set(recorder, rd, &board->cmds, 0);
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
    // ROUTE(UPDATE, CANVAS, _canvas_update)
    ROUTE(DELETE, CANVAS, _canvas_delete)

    // Graphics.
    ROUTE(CREATE, GRAPHICS, _graphics_create)
    ROUTE(SET, PRIMITIVE, _graphics_primitive)
    ROUTE(SET, DEPTH, _graphics_depth)
    ROUTE(SET, BLEND, _graphics_blend)
    ROUTE(SET, POLYGON, _graphics_polygon)
    ROUTE(SET, CULL, _graphics_cull)
    ROUTE(SET, FRONT, _graphics_front)
    ROUTE(SET, GLSL, _graphics_glsl)
    ROUTE(SET, SPIRV, _graphics_spirv)
    ROUTE(SET, VERTEX, _graphics_vertex)
    ROUTE(SET, VERTEX_ATTR, _graphics_vertex_attr)
    ROUTE(SET, SLOT, _graphics_slot)
    ROUTE(BIND, VERTEX, _graphics_bind_vertex)
    ROUTE(BIND, INDEX, _graphics_bind_index)

    // Pipes.
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
    ROUTE(RECORD, RECORD, _record_append)
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

    if (!dvz_obj_is_created(&pipe->obj))
    {
        log_debug("lazily create pipe before using it for command buffer recording");
        dvz_pipe_create(pipe);
    }

    return pipe;
}



uint8_t* dvz_renderer_image(DvzRenderer* rd, DvzId bc_id, DvzSize* size, uint8_t* rgb)
{
    ANN(rd);

    int bctype = dvz_map_type(rd->map, bc_id);

    if (bctype == DVZ_REQUEST_OBJECT_BOARD)
    {
        DvzBoard* board = (DvzBoard*)dvz_map_get(rd->map, bc_id);
        ANN(board);

        // Find the pointer: either passed here, or the board-owned pointer.
        rgb = rgb != NULL ? rgb : board->rgb;
        ANN(rgb);

        // Download the image to the buffer.
        dvz_board_download(board, board->size, rgb);

        // Set the size.
        ANN(size);
        *size = board->size;
    }

    // else if (bctype == DVZ_REQUEST_OBJECT_CANVAS)
    // {
    //     DvzCanvas* canvas = (DvzCanvas*)dvz_map_get(rd->map, bc_id);
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

    dvz_map_destroy(rd->map);
    delete rd->router;

    dvz_obj_destroyed(&rd->obj);
    FREE(rd);
    log_trace("renderer destroyed");
}
