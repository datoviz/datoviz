/*************************************************************************************************/
/*  Workspace                                                                                    */
/*************************************************************************************************/

#include "workspace.h"
#include "board.h"
#include "canvas.h"
#include "host.h"
#include "vklite.h"
#include "window.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzWorkspace* dvz_workspace(DvzGpu* gpu)
{
    ASSERT(gpu != NULL);
    DvzWorkspace* ws = calloc(1, sizeof(DvzWorkspace));
    ws->obj.type = DVZ_OBJECT_TYPE_WORKSPACE;
    ws->gpu = gpu;
    ws->boards =
        dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzBoard), DVZ_OBJECT_TYPE_BOARD);
    ws->canvases =
        dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzCanvas), DVZ_OBJECT_TYPE_CANVAS);
    dvz_obj_init(&ws->obj);
    return ws;
}



DvzBoard* dvz_workspace_board(
    DvzWorkspace* workspace, uint32_t width, uint32_t height, cvec4 background, int flags)
{
    ASSERT(workspace != NULL);
    ASSERT(workspace->gpu != NULL);

    DvzBoard* board = (DvzBoard*)dvz_container_alloc(&workspace->boards);
    *board = dvz_board(workspace->gpu, width, height, flags);
    dvz_board_clear_color(board, background);
    dvz_board_create(board);

    return board;
}



DvzCanvas* dvz_workspace_canvas(
    DvzWorkspace* workspace, uint32_t width, uint32_t height, cvec4 background, int flags)
{
    ASSERT(workspace != NULL);
    DvzCanvas* canvas = (DvzCanvas*)dvz_container_alloc(&workspace->canvases);
    *canvas = dvz_canvas(workspace->gpu, width, height, flags);

    // TODO: wrap the following line in a new function dvz_canvas_clear_color()?
    memcpy(canvas->clear_color, background, sizeof(cvec4));

    // NOTE: dvz_canvas_create() must be called, but only AFTER a window and a surface have been
    // created, and this requires a Client. This is done by the Presenter.

    return canvas;
}



void dvz_workspace_destroy(DvzWorkspace* workspace)
{
    ASSERT(workspace != NULL);

    CONTAINER_DESTROY_ITEMS(DvzBoard, workspace->boards, dvz_board_destroy)
    dvz_container_destroy(&workspace->boards);

    CONTAINER_DESTROY_ITEMS(DvzCanvas, workspace->canvases, dvz_canvas_destroy)
    dvz_container_destroy(&workspace->canvases);

    dvz_obj_destroyed(&workspace->obj);
    FREE(workspace);
}
