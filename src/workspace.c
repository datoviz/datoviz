/*************************************************************************************************/
/*  Workspace                                                                                    */
/*************************************************************************************************/

#include "workspace.h"
#include "board.h"
#include "host.h"
#include "vklite.h"
#include "window.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzWorkspace dvz_workspace(DvzGpu* gpu)
{
    ASSERT(gpu != NULL);
    DvzWorkspace ws = {0};
    ws.gpu = gpu;
    ws.boards =
        dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzBoard), DVZ_OBJECT_TYPE_BOARD);
    // TODO
    // ws.canvases =
    //     dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzCanvas), DVZ_OBJECT_TYPE_CANVAS);
    dvz_obj_init(&ws.obj);
    return ws;
}



DvzBoard* dvz_workspace_board(DvzWorkspace* workspace, uint32_t width, uint32_t height, int flags)
{
    ASSERT(workspace != NULL);
    ASSERT(workspace->gpu != NULL);

    DvzBoard* board = (DvzBoard*)dvz_container_alloc(&workspace->boards);
    *board = dvz_board(workspace->gpu, width, height);
    dvz_board_create(board);

    return board;
}



DvzCanvas*
dvz_workspace_canvas(DvzWorkspace* workspace, uint32_t width, uint32_t height, int flags)
{
    ASSERT(workspace != NULL);
    DvzCanvas* canvas = (DvzCanvas*)dvz_container_alloc(&workspace->canvases);

    // TODO: create canvas.

    return canvas;
}



void dvz_workspace_destroy(DvzWorkspace* workspace)
{
    ASSERT(workspace != NULL);

    CONTAINER_DESTROY_ITEMS(DvzBoard, workspace->boards, dvz_board_destroy)
    dvz_container_destroy(&workspace->boards);

    dvz_container_destroy(&workspace->canvases);

    dvz_obj_destroyed(&workspace->obj);
}
