/*************************************************************************************************/
/*  Request                                                                                      */
/*************************************************************************************************/

#include "request.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define CREATE_REQUEST(_action, _type)                                                            \
    ASSERT(rqr != NULL);                                                                          \
    DvzRequest req = _request();                                                                  \
    req.action = DVZ_REQUEST_ACTION_##_action;                                                    \
    req.type = DVZ_REQUEST_OBJECT_##_type;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

static DvzRequest _request(void)
{
    DvzRequest req = {0};
    req.version = DVZ_REQUEST_VERSION;
    return req;
}



DvzRequester dvz_requester(void)
{
    DvzRequester rqr = {0};
    rqr.prng = dvz_prng();

    // Initialize the list of requests for batchs.
    rqr.capacity = DVZ_CONTAINER_DEFAULT_COUNT;
    rqr.requests = (DvzRequest*)calloc(rqr.capacity, sizeof(DvzRequest));

    dvz_obj_init(&rqr.obj);
    return rqr;
}



void dvz_requester_destroy(DvzRequester* rqr)
{
    ASSERT(rqr != NULL);
    FREE(rqr->requests);
    dvz_prng_destroy(rqr->prng);
    dvz_obj_destroyed(&rqr->obj);
}



/*************************************************************************************************/
/*  Request batch                                                                                */
/*************************************************************************************************/

void dvz_requester_begin(DvzRequester* rqr)
{
    ASSERT(rqr != NULL);
    rqr->count = 0;
}



void dvz_requester_add(DvzRequester* rqr, DvzRequest req)
{
    ASSERT(rqr != NULL);
    // Resize the array if needed.
    if (rqr->count == rqr->capacity)
    {
        rqr->capacity *= 2;
        REALLOC(rqr->requests, rqr->capacity * sizeof(DvzRequest));
    }
    ASSERT(rqr->count < rqr->capacity);

    // Append the request.
    rqr->requests[rqr->count++] = req;
}



DvzRequest* dvz_requester_end(DvzRequester* rqr, uint32_t* count)
{
    ASSERT(rqr != NULL);
    ASSERT(count != NULL);
    *count = rqr->count;
    return rqr->requests;
}



void dvz_request_print(DvzRequest* req)
{
    ASSERT(req != NULL);
    log_info("Request action %d <type %d> <id %" PRIx64 ">", req->action, req->type, req->id);
}



/*************************************************************************************************/
/*  Board                                                                                        */
/*************************************************************************************************/

DvzRequest dvz_create_board(DvzRequester* rqr, uint32_t width, uint32_t height, int flags)
{
    CREATE_REQUEST(CREATE, BOARD);
    req.id = dvz_prng_uuid(rqr->prng);
    req.flags = flags;
    req.content.board.width = width;
    req.content.board.height = height;
    return req;
}



DvzRequest dvz_delete_board(DvzRequester* rqr, DvzId id)
{
    CREATE_REQUEST(DELETE, BOARD);
    req.id = id;
    return req;
}



/*************************************************************************************************/
/*  Resources                                                                                    */
/*************************************************************************************************/

DvzRequest dvz_create_dat(DvzRequester* rqr, DvzBufferType type, DvzSize size, int flags)
{
    CREATE_REQUEST(CREATE, DAT);
    req.id = dvz_prng_uuid(rqr->prng);
    req.flags = flags;
    req.content.dat.type = type;
    req.content.dat.size = size;
    return req;
}



DvzRequest
dvz_create_tex(DvzRequester* rqr, DvzTexDims dims, uvec3 shape, DvzFormat format, int flags)
{
    CREATE_REQUEST(CREATE, TEX);
    req.flags = flags;
    req.content.tex.dims = dims;
    memcpy(req.content.tex.shape, shape, sizeof(uvec3));
    req.content.tex.format = format;
    return req;
}


/*************************************************************************************************/
/*  Graphics                                                                                     */
/*************************************************************************************************/

DvzRequest dvz_create_graphics(DvzRequester* rqr, DvzGraphicsType type, int flags)
{
    CREATE_REQUEST(CREATE, GRAPHICS);
    return req;
}



/*************************************************************************************************/
/*  Data                                                                                         */
/*************************************************************************************************/

DvzRequest dvz_upload_dat(DvzRequester* rqr, DvzId dat, DvzSize offset, DvzSize size, void* data)
{
    CREATE_REQUEST(UPLOAD, DAT);
    return req;
}



/*************************************************************************************************/
/*  Command buffer                                                                               */
/*************************************************************************************************/

DvzRequest dvz_set_begin(DvzRequester* rqr, DvzId board)
{
    CREATE_REQUEST(SET, BEGIN);
    return req;
}



DvzRequest dvz_set_vertex(DvzRequester* rqr, DvzId graphics, DvzId dat)
{
    CREATE_REQUEST(SET, VERTEX);
    return req;
}



DvzRequest dvz_set_viewport(DvzRequester* rqr, DvzId board, vec2 offset, vec2 shape)
{
    CREATE_REQUEST(SET, VIEWPORT);
    return req;
}



DvzRequest
dvz_set_draw(DvzRequester* rqr, DvzId graphics, uint32_t first_vertex, uint32_t vertex_count)
{

    CREATE_REQUEST(SET, DRAW);
    return req;
}



DvzRequest dvz_set_end(DvzRequester* rqr, DvzId board)
{
    CREATE_REQUEST(SET, END);
    return req;
}
