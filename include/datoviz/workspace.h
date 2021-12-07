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

// TODO: docstrings

DVZ_EXPORT DvzWorkspace dvz_workspace(DvzGpu* gpu);



DVZ_EXPORT DvzBoard*
dvz_workspace_board(DvzWorkspace* workspace, uint32_t width, uint32_t height, int flags);



DVZ_EXPORT DvzCanvas*
dvz_workspace_canvas(DvzWorkspace* workspace, uint32_t width, uint32_t height, int flags);



DVZ_EXPORT void dvz_workspace_destroy(DvzWorkspace* workspace);



EXTERN_C_OFF

#endif
