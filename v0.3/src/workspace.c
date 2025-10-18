/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Workspace                                                                                    */
/*************************************************************************************************/

#include "workspace.h"
#include "board.h"
#include "canvas.h"
#include "host.h"
#include "render_utils.h"
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
    ANN(gpu);
    ANN(gpu->host);

    DvzWorkspace* ws = (DvzWorkspace*)calloc(1, sizeof(DvzWorkspace));
    ws->obj.type = DVZ_OBJECT_TYPE_WORKSPACE;
    ws->gpu = gpu;
    ws->flags = flags;
    ws->boards =
        dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzCanvas), DVZ_OBJECT_TYPE_BOARD);
    ws->canvases =
        dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzCanvas), DVZ_OBJECT_TYPE_CANVAS);

    // Create the renderpasses.
    cvec4 clear_color = {0};
    default_clear_color(flags, clear_color);

    ws->renderpass_overlay =
        dvz_gpu_renderpass(gpu, clear_color, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    ws->renderpass_offscreen =
        dvz_gpu_renderpass(gpu, clear_color, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    // NOTE: we only create the desktop renderpass if we use the glfw backend.
    // This avoids the following validation error:

    // vkutils.h:0174: validation layer: Validation Error: [
    // VUID-VkAttachmentDescription-finalLayout-parameter ] Object 0: handle = 0x561a27dda3a0, type
    // = VK_OBJECT_TYPE_DEVICE; | MessageID = 0xd072ad00 | vkCreateRenderPass: value of
    // pCreateInfo->pAttachments[0].finalLayout (1000001002) does not fall within the begin..end
    // range of the core VkImageLayout enumeration tokens and is not an extension added token. The
    // Vulkan spec states: finalLayout must be a valid VkImageLayout value

    // TODO: backend
    // if (gpu->host->backend == DVZ_BACKEND_GLFW)
    // {
    ws->renderpass_desktop = dvz_gpu_renderpass(gpu, clear_color, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    // }

    dvz_obj_init(&ws->obj);
    return ws;
}



DvzCanvas* dvz_workspace_board(DvzWorkspace* workspace, uint32_t width, uint32_t height, int flags)
{
    ANN(workspace);
    ANN(workspace->gpu);

    DvzCanvas* board = (DvzCanvas*)dvz_container_alloc(&workspace->boards);

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
    ANN(workspace);
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
    if (workspace == NULL)
        return;
    ANN(workspace);

    CONTAINER_DESTROY_ITEMS(DvzCanvas, workspace->boards, dvz_board_destroy)
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
