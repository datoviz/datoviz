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



void dvz_create_canvas(DvzRequest* req, DvzId id, uint32_t width, uint32_t height, int flags)
{
    ASSERT(req != NULL);
    ASSERT(id > 0);
    req->id = id;
    req->action = DVZ_REQUEST_ACTION_CREATE;
    req->type = DVZ_OBJECT_TYPE_CANVAS;
    return 0;
}



void dvz_create_dat(DvzRequest* req, DvzId id, DvzBufferType type, DvzSize size, int flags)
{
    ASSERT(id > 0);
    ASSERT(req != NULL);
    req->id = id;
    req->action = DVZ_REQUEST_ACTION_CREATE;
    req->type = DVZ_OBJECT_TYPE_DAT;
    return 0;
}



void dvz_create_tex(
    DvzRequest* req, DvzId id, DvzTexDims dims, uvec3 shape, DvzFormat format, int flags)
{
    ASSERT(id > 0);
    ASSERT(req != NULL);
    req->id = id;
    req->action = DVZ_REQUEST_ACTION_CREATE;
    req->type = DVZ_OBJECT_TYPE_TEX;
    return 0;
}



void dvz_set_viewport(DvzRequest* req, vec2 offset, vec2 shape) { ASSERT(req != NULL); }



void dvz_set_graphics(DvzRequest* req, DvzPipe* pipe) { ASSERT(req != NULL); }



void dvz_set_compute(DvzRequest* req, DvzPipe* pipe) { ASSERT(req != NULL); }
