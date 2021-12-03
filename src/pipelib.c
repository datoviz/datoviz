/*************************************************************************************************/
/*  Holds graphics and computes pipes                                                            */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "pipelib.h"
#include "context.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzPipelib dvz_pipelib(DvzGpu* gpu)
{
    ASSERT(gpu != NULL);
    DvzPipelib lib = {0};
    lib.gpu = gpu;
    lib.graphics =
        dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzGraphics), DVZ_OBJECT_TYPE_GRAPHICS);
    lib.computes =
        dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzCompute), DVZ_OBJECT_TYPE_COMPUTE);
    dvz_obj_created(&lib.obj);
    return lib;
}



DvzPipe*
dvz_pipelib_graphics(DvzPipelib* lib, DvzRenderpass* renderpass, DvzGraphicsType type, int flags)
{
    ASSERT(lib != NULL); //
    return NULL;
}



DvzPipe* dvz_pipelib_compute_file(DvzPipelib* lib, const char* shader_path)
{
    ASSERT(lib != NULL); //
    return NULL;
}



void dvz_pipelib_pipe_destroy(DvzPipelib* lib, DvzPipe* pipe)
{
    ASSERT(lib != NULL); //
}



void dvz_pipelib_destroy(DvzPipelib* lib)
{
    ASSERT(lib != NULL);
    dvz_container_destroy(&lib->graphics);
    dvz_container_destroy(&lib->computes);
    dvz_obj_destroyed(&lib->obj);
}
