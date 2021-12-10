/*************************************************************************************************/
/*  Renderer                                                                                     */
/*************************************************************************************************/

#include <map>

#include "_log.h"
#include "board.h"
#include "context.h"
#include "graphics.h"
#include "map.h"
#include "pipe.h"
#include "pipelib.h"
#include "renderer.h"
#include "workspace.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define ROUTE(action, type, function)                                                             \
    rd->router->router[std::make_pair(DVZ_REQUEST_ACTION_##action, DVZ_REQUEST_OBJECT_##type)] =  \
        function;



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
    ASSERT(rd != NULL);

    DvzBoard* board = dvz_workspace_board(
        rd->workspace, req.content.board.width, req.content.board.height, req.flags);
    ASSERT(board != NULL);
    board->rgba = dvz_board_alloc(board);
    return (void*)board;
}



static void* _board_update(DvzRenderer* rd, DvzRequest req)
{
    ASSERT(rd != NULL);
    ASSERT(req.id != 0);

    DvzBoard* board = (DvzBoard*)dvz_map_get(rd->map, req.id);
    ASSERT(board != NULL);

    dvz_cmd_submit_sync(&board->cmds, DVZ_DEFAULT_QUEUE_RENDER);

    return NULL;
}



static void* _board_delete(DvzRenderer* rd, DvzRequest req)
{
    ASSERT(rd != NULL);
    ASSERT(req.id != 0);

    DvzBoard* board = (DvzBoard*)dvz_map_get(rd->map, req.id);
    ASSERT(board != NULL);

    dvz_board_free(board);
    dvz_board_destroy(board);
    return NULL;
}



/*************************************************************************************************/
/*  Graphics                                                                                     */
/*************************************************************************************************/

static void* _graphics_create(DvzRenderer* rd, DvzRequest req)
{
    ASSERT(rd != NULL);

    // Get the board.
    DvzId board_id = req.content.graphics.board;
    DvzBoard* board = (DvzBoard*)dvz_map_get(rd->map, board_id);
    ASSERT(board != NULL);

    // Create the pipe.
    uvec2 size = {board->width, board->height};
    DvzPipe* pipe = dvz_pipelib_graphics(
        rd->pipelib, rd->ctx, &board->renderpass, board->images.count, //
        size, req.content.graphics.type, req.flags);
    ASSERT(pipe != NULL);

    return (void*)pipe;
}



static void* _graphics_vertex(DvzRenderer* rd, DvzRequest req)
{
    ASSERT(rd != NULL);
    ASSERT(req.id != 0);

    // Get the graphics pipe.
    DvzPipe* pipe = (DvzPipe*)dvz_map_get(rd->map, req.id);
    ASSERT(pipe != NULL);

    // Get the dat with the vertex data.
    DvzDat* dat = (DvzDat*)dvz_map_get(rd->map, req.content.set_vertex.dat);
    ASSERT(dat != NULL);

    // Link the two.
    pipe->dat_vertex = dat;

    return NULL;
}



static void* _graphics_delete(DvzRenderer* rd, DvzRequest req)
{
    ASSERT(rd != NULL);
    ASSERT(req.id != 0);

    DvzPipe* pipe = (DvzPipe*)dvz_map_get(rd->map, req.id);
    ASSERT(pipe != NULL);

    dvz_pipe_destroy(pipe);
    return NULL;
}



/*************************************************************************************************/
/*  Resources                                                                                    */
/*************************************************************************************************/

static void* _dat_create(DvzRenderer* rd, DvzRequest req)
{
    ASSERT(rd != NULL);

    DvzDat* dat = dvz_dat(rd->ctx, req.content.dat.type, req.content.dat.size, req.flags);
    ASSERT(dat != NULL);

    return (void*)dat;
}



static void* _dat_upload(DvzRenderer* rd, DvzRequest req)
{
    ASSERT(rd != NULL);
    ASSERT(req.id != 0);

    DvzDat* dat = (DvzDat*)dvz_map_get(rd->map, req.id);
    ASSERT(dat != NULL);

    dvz_dat_upload(
        dat,                           //
        req.content.dat_upload.offset, //
        req.content.dat_upload.size,   //
        req.content.dat_upload.data,   //
        true);

    return NULL;
}



static void* _dat_delete(DvzRenderer* rd, DvzRequest req)
{
    ASSERT(rd != NULL);
    ASSERT(req.id != 0);

    DvzDat* dat = (DvzDat*)dvz_map_get(rd->map, req.id);
    ASSERT(dat != NULL);

    dvz_dat_destroy(dat);
    return NULL;
}



/*************************************************************************************************/
/*  Command buffer setting                                                                       */
/*************************************************************************************************/

static void* _set_begin(DvzRenderer* rd, DvzRequest req)
{
    ASSERT(rd != NULL);

    DvzBoard* board = (DvzBoard*)dvz_map_get(rd->map, req.id);
    ASSERT(board != NULL);

    dvz_cmd_reset(&board->cmds, 0);
    dvz_board_begin(board, &board->cmds, 0);

    return NULL;
}



static void* _set_viewport(DvzRenderer* rd, DvzRequest req)
{
    ASSERT(rd != NULL);

    DvzBoard* board = (DvzBoard*)dvz_map_get(rd->map, req.id);
    ASSERT(board != NULL);

    dvz_board_viewport(
        board, &board->cmds, 0, //
        req.content.set_viewport.offset, req.content.set_viewport.shape);

    return NULL;
}



static void* _set_draw(DvzRenderer* rd, DvzRequest req)
{
    ASSERT(rd != NULL);

    DvzBoard* board = (DvzBoard*)dvz_map_get(rd->map, req.id);
    ASSERT(board != NULL);

    DvzPipe* pipe = (DvzPipe*)dvz_map_get(rd->map, req.content.set_draw.graphics);
    ASSERT(pipe != NULL);

    dvz_pipe_draw(
        pipe, &board->cmds, 0, //
        req.content.set_draw.first_vertex, req.content.set_draw.vertex_count);

    return NULL;
}



static void* _set_end(DvzRenderer* rd, DvzRequest req)
{
    ASSERT(rd != NULL);

    DvzBoard* board = (DvzBoard*)dvz_map_get(rd->map, req.id);
    ASSERT(board != NULL);

    dvz_board_end(board, &board->cmds, 0);

    return NULL;
}



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _setup_router(DvzRenderer* rd)
{
    ASSERT(rd != NULL);

    rd->router = new DvzRouter();
    rd->router->router =
        std::map<std::pair<DvzRequestAction, DvzRequestObject>, DvzRouterCallback>();

    // Board.
    ROUTE(CREATE, BOARD, _board_create)
    ROUTE(UPDATE, BOARD, _board_update)
    ROUTE(DELETE, BOARD, _board_delete)

    // Graphics.
    ROUTE(CREATE, GRAPHICS, _graphics_create)
    ROUTE(SET, VERTEX, _graphics_vertex)
    ROUTE(DELETE, GRAPHICS, _graphics_delete)

    // Resources.
    ROUTE(CREATE, DAT, _dat_create)
    ROUTE(UPLOAD, DAT, _dat_upload)
    ROUTE(DELETE, DAT, _dat_delete)

    // Command buffer setting.
    ROUTE(SET, BEGIN, _set_begin)
    ROUTE(SET, VIEWPORT, _set_viewport)
    ROUTE(SET, DRAW, _set_draw)
    ROUTE(SET, END, _set_end)
}



static void _update_mapping(DvzRenderer* rd, DvzRequest req, void* obj)
{
    ASSERT(rd != NULL);

    // Handle the id-object mapping.
    switch (req.action)
    {
        // Creation.
    case DVZ_REQUEST_ACTION_CREATE:
        ASSERT(obj != NULL);

        // Generate a new id.
        log_trace("adding object type %d id %" PRIx64 " to mapping", req.type, req.id);

        // Register the id with the created object
        dvz_map_add(rd->map, req.id, DVZ_REQUEST_OBJECT_BOARD, obj);

        break;

        // Deletion.
    case DVZ_REQUEST_ACTION_DELETE:

        ASSERT(req.id != DVZ_ID_NONE);

        // Remove the id from the mapping.
        log_trace("removing object type %d id %" PRIx64 " from mapping", req.type, req.id);
        dvz_map_remove(rd->map, req.id);

        break;

    default:
        break;
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzRenderer* dvz_renderer_offscreen(DvzGpu* gpu)
{
    ASSERT(gpu != NULL);
    DvzRenderer* rd = (DvzRenderer*)calloc(1, sizeof(DvzRenderer));
    ASSERT(rd != NULL);
    rd->gpu = gpu;

    rd->ctx = dvz_context(gpu);
    rd->pipelib = dvz_pipelib(rd->ctx);
    rd->workspace = dvz_workspace(gpu);
    rd->map = dvz_map();

    _setup_router(rd);

    dvz_obj_init(&rd->obj);
    return rd;
}



DvzRenderer* dvz_renderer_glfw(DvzGpu* gpu)
{
    ASSERT(gpu != NULL);
    // TODO
    return NULL;
}



void dvz_renderer_request(DvzRenderer* rd, DvzRequest req)
{
    ASSERT(rd != NULL);

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
    ASSERT(rd != NULL);
    if (count == 0)
        return;
    ASSERT(count > 0);
    ASSERT(reqs != NULL);
    for (uint32_t i = 0; i < count; i++)
    {
        dvz_renderer_request(rd, reqs[i]);
    }
}



DvzBoard* dvz_renderer_board(DvzRenderer* rd, DvzId id)
{
    ASSERT(rd != NULL);

    DvzBoard* board = (DvzBoard*)dvz_map_get(rd->map, id);
    ASSERT(board != NULL);
    return board;
}



uint8_t* dvz_renderer_image(DvzRenderer* rd, DvzId board_id, DvzSize* size, uint8_t* rgba)
{
    ASSERT(rd != NULL);

    DvzBoard* board = (DvzBoard*)dvz_map_get(rd->map, board_id);
    ASSERT(board != NULL);

    // Find the pointer: either passed here, or the board-owned pointer.
    rgba = rgba != NULL ? rgba : board->rgba;
    ASSERT(rgba != NULL);

    // Download the image to the buffer.
    dvz_board_download(board, board->size, rgba);

    // Set the size.
    ASSERT(size != NULL);
    *size = board->size;

    // Return the pointer.
    return rgba;
}



void dvz_renderer_destroy(DvzRenderer* rd)
{
    ASSERT(rd != NULL);

    dvz_map_destroy(rd->map);
    dvz_workspace_destroy(rd->workspace);
    dvz_pipelib_destroy(rd->pipelib);
    dvz_context_destroy(rd->ctx);

    dvz_obj_destroyed(&rd->obj);
    FREE(rd);
}
