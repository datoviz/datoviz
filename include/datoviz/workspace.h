/*************************************************************************************************/
/*  Workspace                                                                                    */
/*************************************************************************************************/

#ifndef DVZ_HEADER_WORKSPACE
#define DVZ_HEADER_WORKSPACE



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



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
    DvzContainer boards;
    DvzContainer canvases;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a workspace, handling a number of boards and canvases.
 *
 * @param gpu the GPU
 * @returns the workspace
 */
DVZ_EXPORT DvzWorkspace* dvz_workspace(DvzGpu* gpu);



/**
 * Create a new board.
 *
 * @param workspace the workspace
 * @param width the board width
 * @param height the board height
 * @param flags the board creation flags
 */
DVZ_EXPORT DvzBoard*
dvz_workspace_board(DvzWorkspace* workspace, uint32_t width, uint32_t height, int flags);



/**
 * Create a new canvas.
 *
 * @param workspace the workspace
 * @param width the canvas width
 * @param height the canvas height
 * @param flags the canvas creation flags
 */
DVZ_EXPORT DvzCanvas*
dvz_workspace_canvas(DvzWorkspace* workspace, uint32_t width, uint32_t height, int flags);



/**
 * Destroy a workspace.
 *
 * @param workspace the workspace
 */
DVZ_EXPORT void dvz_workspace_destroy(DvzWorkspace* workspace);



EXTERN_C_OFF

#endif
