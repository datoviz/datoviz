/*************************************************************************************************/
/*  Renderer                                                                                     */
/*************************************************************************************************/

#include "renderer.h"
#include "_log.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzRenderer dvz_renderer_offscreen(DvzGpu* gpu)
{
    ASSERT(gpu != NULL);
    DvzRenderer rd = {0};
    rd.gpu = gpu;

    rd.ctx = dvz_context(gpu);
    ASSERT(rd.ctx != NULL);

    rd.lib = dvz_pipelib(rd.ctx);
    rd.workspace = dvz_workspace(gpu);
    rd.map = dvz_map();

    dvz_obj_init(&rd.obj);
    return rd;
}



DvzRenderer dvz_renderer_glfw(DvzGpu* gpu)
{
    ASSERT(gpu != NULL);
    // TODO
}



DvzId dvz_renderer_request(DvzRenderer* rd, DvzRequest req)
{
    ASSERT(rd != NULL); //
}



void dvz_renderer_image(DvzRenderer* rd, DvzId canvas_id, DvzSize size, uint8_t* rgba)
{
    ASSERT(rd != NULL);
}



void dvz_renderer_destroy(DvzRenderer* rd)
{
    ASSERT(rd != NULL);

    dvz_map_destroy(rd->map);
    dvz_pipelib_destroy(&rd->lib);
    dvz_workspace_destroy(&rd->workspace);
    dvz_context_destroy(rd->ctx);
    dvz_obj_destroyed(&rd->obj);
}
