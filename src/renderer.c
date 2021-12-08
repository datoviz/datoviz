/*************************************************************************************************/
/*  Renderer                                                                                     */
/*************************************************************************************************/

#include "renderer.h"
#include "_log.h"
#include "context.h"
#include "map.h"
#include "pipelib.h"
#include "workspace.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzRenderer* dvz_renderer_offscreen(DvzGpu* gpu)
{
    ASSERT(gpu != NULL);
    DvzRenderer* rd = calloc(1, sizeof(DvzRenderer));
    ASSERT(rd != NULL);
    rd->gpu = gpu;

    rd->ctx = dvz_context(gpu);
    rd->pipelib = dvz_pipelib(rd->ctx);
    rd->workspace = dvz_workspace(gpu);
    rd->map = dvz_map();

    dvz_obj_init(&rd->obj);
    return rd;
}



DvzRenderer* dvz_renderer_glfw(DvzGpu* gpu)
{
    ASSERT(gpu != NULL);
    // TODO
    return NULL;
}



DvzId dvz_renderer_request(DvzRenderer* rd, DvzRequest req)
{
    ASSERT(rd != NULL); //
    return 0;
}



void dvz_renderer_image(DvzRenderer* rd, DvzId canvas_id, DvzSize size, uint8_t* rgba)
{
    ASSERT(rd != NULL);
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
