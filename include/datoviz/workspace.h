/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

/*************************************************************************************************/
/*  Workspace                                                                                    */
/*************************************************************************************************/

#ifndef DVZ_HEADER_WORKSPACE
#define DVZ_HEADER_WORKSPACE



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "common.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

typedef enum
{
    DVZ_WORKSPACE_FLAGS_NONE = 0x00,
    DVZ_WORKSPACE_FLAGS_OVERLAY = 0x01,
} DvzWorkspaceFlags;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzWorkspace DvzWorkspace;

// Forward declarations.
typedef struct DvzGpu DvzGpu;
typedef struct DvzCanvas DvzCanvas;
typedef struct DvzBoard DvzBoard;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzWorkspace
{
    DvzObject obj;
    DvzGpu* gpu;
    int flags;

    DvzContainer boards;
    DvzContainer canvases;

    DvzRenderpass renderpass_offscreen;
    DvzRenderpass renderpass_desktop;
    DvzRenderpass renderpass_overlay; // if overlay, same renderpass between offscreen and desktop
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a workspace, handling a number of boards and canvases.
 *
 * @param gpu the GPU
 * @param flags the flags
 * @returns the workspace
 */
DvzWorkspace* dvz_workspace(DvzGpu* gpu, int flags);



/**
 * Create a new board.
 *
 * @param workspace the workspace
 * @param width the board width
 * @param height the board height
 * @param flags the board creation flags
 */
DvzBoard* dvz_workspace_board(DvzWorkspace* workspace, uint32_t width, uint32_t height, int flags);



/**
 * Create a new canvas.
 *
 * @param workspace the workspace
 * @param width the canvas width
 * @param height the canvas height
 * @param flags the canvas creation flags
 */
DvzCanvas*
dvz_workspace_canvas(DvzWorkspace* workspace, uint32_t width, uint32_t height, int flags);



/**
 * Destroy a workspace.
 *
 * @param workspace the workspace
 */
void dvz_workspace_destroy(DvzWorkspace* workspace);



EXTERN_C_OFF

#endif
