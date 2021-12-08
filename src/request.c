/*************************************************************************************************/
/*  Request                                                                                      */
/*************************************************************************************************/

#include "request.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define CREATE_REQUEST(_action, _type)                                                            \
    ASSERT(req != NULL);                                                                          \
    ASSERT(id > 0);                                                                               \
    req->id = id;                                                                                 \
    req->action = DVZ_REQUEST_ACTION_##_action;                                                   \
    req->type = DVZ_OBJECT_TYPE_##_type;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzRequest dvz_request()
{
    DvzRequest req = {0};
    req.version = DVZ_REQUEST_VERSION;
}



void dvz_request_print(DvzRequest* req)
{
    ASSERT(req != NULL);
    log_info("Request action %d <type %d> <id %d>", req->action, req->type, req->id);
}



void dvz_create_canvas(DvzRequest* req, DvzId id, uint32_t width, uint32_t height, int flags)
{
    CREATE_REQUEST(CREATE, CANVAS)
    req->content.canvas.width = width;
    req->content.canvas.height = height;
    req->content.canvas.flags = flags;
    return 0;
}



void dvz_create_dat(DvzRequest* req, DvzId id, DvzBufferType type, DvzSize size, int flags)
{
    CREATE_REQUEST(CREATE, DAT)
    req->content.dat.type = type;
    req->content.dat.size = size;
    req->content.dat.flags = flags;
    return 0;
}



void dvz_create_tex(
    DvzRequest* req, DvzId id, DvzTexDims dims, uvec3 shape, DvzFormat format, int flags)
{
    CREATE_REQUEST(CREATE, TEX)
    req->content.tex.dims = dims;
    memcpy(req->content.tex.shape, shape, sizeof(uvec3));
    req->content.tex.format = format;
    req->content.tex.flags = flags;
    return 0;
}



void dvz_set_viewport(DvzRequest* req, vec2 offset, vec2 shape) { ASSERT(req != NULL); }



void dvz_set_graphics(DvzRequest* req, DvzPipe* pipe) { ASSERT(req != NULL); }



void dvz_set_compute(DvzRequest* req, DvzPipe* pipe) { ASSERT(req != NULL); }
