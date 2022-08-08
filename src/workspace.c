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
/*  Utils                                                                                        */
/*************************************************************************************************/

static bool _has_overlay(int flags) { return (flags & DVZ_WORKSPACE_FLAGS_OVERLAY); }



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzWorkspace* dvz_workspace(DvzGpu* gpu, int flags)
{
    ASSERT(gpu != NULL);
    DvzWorkspace* ws = calloc(1, sizeof(DvzWorkspace));
    ws->obj.type = DVZ_OBJECT_TYPE_WORKSPACE;
    ws->gpu = gpu;
    ws->flags = flags;
    ws->boards =
        dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzBoard), DVZ_OBJECT_TYPE_BOARD);
    ws->canvases =
        dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzCanvas), DVZ_OBJECT_TYPE_CANVAS);

    // Create the renderpasses.
    ws->renderpass_overlay =
        dvz_gpu_renderpass(gpu, DVZ_DEFAULT_CLEAR_COLOR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    ws->renderpass_offscreen =
        dvz_gpu_renderpass(gpu, DVZ_DEFAULT_CLEAR_COLOR, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    ws->renderpass_desktop =
        dvz_gpu_renderpass(gpu, DVZ_DEFAULT_CLEAR_COLOR, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    dvz_obj_init(&ws->obj);
    return ws;
}



DvzBoard* dvz_workspace_board(DvzWorkspace* workspace, uint32_t width, uint32_t height, int flags)
{
    ASSERT(workspace != NULL);
    ASSERT(workspace->gpu != NULL);

    DvzBoard* board = (DvzBoard*)dvz_container_alloc(&workspace->boards);

    DvzRenderpass* renderpass =
        _has_overlay(flags) ? &workspace->renderpass_overlay : &workspace->renderpass_offscreen;

    *board = dvz_board(workspace->gpu, renderpass, width, height, flags);
    // dvz_board_clear_color(board, background);
    dvz_board_create(board);

    return board;
}



DvzCanvas*
dvz_workspace_canvas(DvzWorkspace* workspace, uint32_t width, uint32_t height, int flags)
{
    ASSERT(workspace != NULL);
    DvzCanvas* canvas = (DvzCanvas*)dvz_container_alloc(&workspace->canvases);

    DvzRenderpass* renderpass =
        _has_overlay(flags) ? &workspace->renderpass_overlay : &workspace->renderpass_desktop;

    *canvas = dvz_canvas(workspace->gpu, renderpass, width, height, flags);

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

    // Destroy the renderpasses.
    dvz_renderpass_destroy(&workspace->renderpass_overlay);
    dvz_renderpass_destroy(&workspace->renderpass_offscreen);
    dvz_renderpass_destroy(&workspace->renderpass_desktop);

    dvz_obj_destroyed(&workspace->obj);
    FREE(workspace);
}
