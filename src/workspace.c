/*************************************************************************************************/
/*  Workspace                                                                                    */
/*************************************************************************************************/

#include "workspace.h"
#include "host.h"
#include "vklite.h"
#include "window.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzWorkspace dvz_workspace(DvzGpu* gpu)
{
    ASSERT(gpu != NULL); //
}



DvzBoard* dvz_workspace_board(DvzWorkspace* workspace, uint32_t width, uint32_t height, int flags)
{
    ASSERT(workspace != NULL);
}



DvzCanvas*
dvz_workspace_canvas(DvzWorkspace* workspace, uint32_t width, uint32_t height, int flags)
{
    ASSERT(workspace != NULL);
}



void dvz_workspace_destroy(DvzWorkspace* workspace)
{
    ASSERT(workspace != NULL);
    //
}
