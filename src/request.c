/*************************************************************************************************/
/*  Request                                                                                      */
/*************************************************************************************************/

#include "request.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzRequest dvz_request()
{
    DvzRequest req = {0};
    req.version = DVZ_REQUEST_VERSION;
}



DvzId dvz_create_canvas(DvzRequest* req, uint32_t width, uint32_t height, int flags)
{
    ASSERT(req != NULL);
    // TODO
    return 0;
}



DvzId dvz_create_dat(DvzRequest* req, DvzBufferType type, DvzSize size, int flags)
{
    ASSERT(req != NULL);
    // TODO
    return 0;
}



DvzId dvz_create_tex(DvzRequest* req, DvzTexDims dims, uvec3 shape, DvzFormat format, int flags)
{
    ASSERT(req != NULL);
    // TODO
    return 0;
}



void dvz_set_viewport(DvzRequest* req, vec2 offset, vec2 shape) { ASSERT(req != NULL); }



void dvz_set_graphics(DvzRequest* req, DvzPipe* pipe) { ASSERT(req != NULL); }



void dvz_set_compute(DvzRequest* req, DvzPipe* pipe) { ASSERT(req != NULL); }
