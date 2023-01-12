/*************************************************************************************************/
/*  Request                                                                                      */
/*************************************************************************************************/

#include "request.h"
#include "_debug.h"
#include "_list.h"
#include "fileio.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define CREATE_REQUEST(_action, _type)                                                            \
    ANN(rqr);                                                                                     \
    DvzRequest req = _request();                                                                  \
    req.action = DVZ_REQUEST_ACTION_##_action;                                                    \
    req.type = DVZ_REQUEST_OBJECT_##_type;

#define STR_ACTION(r)                                                                             \
    case DVZ_REQUEST_ACTION_##r:                                                                  \
        str = #r;                                                                                 \
        break

#define STR_OBJECT(r)                                                                             \
    case DVZ_REQUEST_OBJECT_##r:                                                                  \
        str = #r;                                                                                 \
        break

// #define PRT(x) printf(x "\n");



/*************************************************************************************************/
/*  Util functions                                                                               */
/*************************************************************************************************/

static DvzRequest _request(void)
{
    DvzRequest req = {0};
    req.version = DVZ_REQUEST_VERSION;
    return req;
}



static int write_file(const char* filename, DvzSize block_size, uint32_t block_count, void* data)
{
    ANN(filename);
    ASSERT(block_size > 0);
    ASSERT(block_count > 0);
    ANN(data);

    log_trace("saving binary `%s`", filename);
    FILE* fp = fopen(filename, "wb");
    if (fp == NULL)
    {
        log_error("error writing `%s`", filename);
        return 1;
    }
    fwrite(data, block_size, block_count, fp);
    fclose(fp);
    return 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzRequester* dvz_requester(void)
{
    log_trace("create requester");
    DvzRequester* rqr = calloc(1, sizeof(DvzRequester));
    rqr->prng = dvz_prng();

    // Initialize the list of requests for batchs.
    rqr->capacity = DVZ_CONTAINER_DEFAULT_COUNT;
    rqr->requests = (DvzRequest*)calloc(rqr->capacity, sizeof(DvzRequest));

    rqr->pointers_to_free = dvz_list();

    dvz_obj_init(&rqr->obj);
    return rqr;
}



void dvz_requester_destroy(DvzRequester* rqr)
{
    log_trace("destroy requester");
    ANN(rqr);

    // NOTE: free all pointers created when loading requests dumps.
    uint32_t n = dvz_list_count(rqr->pointers_to_free);
    void* pointer = NULL;
    for (uint32_t i = 0; i < n; i++)
    {
        pointer = dvz_list_get(rqr->pointers_to_free, i).p;
        FREE(pointer);
    }

    FREE(rqr->requests);
    dvz_prng_destroy(rqr->prng);
    dvz_list_destroy(rqr->pointers_to_free);
    dvz_obj_destroyed(&rqr->obj);
    FREE(rqr);
}



/*************************************************************************************************/
/*  Request batch                                                                                */
/*************************************************************************************************/

void dvz_requester_begin(DvzRequester* rqr)
{
    ANN(rqr);
    rqr->count = 0;
}



void dvz_requester_add(DvzRequester* rqr, DvzRequest req)
{
    ANN(rqr);
    // Resize the array if needed.
    if (rqr->count == rqr->capacity)
    {
        rqr->capacity *= 2;
        REALLOC(rqr->requests, rqr->capacity * sizeof(DvzRequest));
    }
    ASSERT(rqr->count < rqr->capacity);

    // Append the request.
    rqr->requests[rqr->count++] = req;

    // if (getenv("DVZ_VERBOSE") != NULL)
    //     dvz_request_print(&req);
}



DvzRequest* dvz_requester_end(DvzRequester* rqr, uint32_t* count)
{
    ANN(rqr);
    if (count != NULL)
        *count = rqr->count;
    return rqr->requests;
}



int dvz_requester_dump(DvzRequester* rqr, const char* filename)
{
    ANN(rqr);
    int res = 0;
    if (rqr->count == 0)
    {
        log_error("empty requester, aborting requester dump");
        return 1;
    }
    ANN(rqr->requests);

    log_trace("start serializing %d requests", rqr->count);

    // Dump the DvzRequest structures.
    log_trace("saving main dump file `%s`", filename);
    res = write_file(filename, sizeof(DvzRequest), rqr->count, rqr->requests);
    if (res != 0)
        return res;

    // Write additional files for uploaded data.
    DvzRequest* req = NULL;
    DvzRequestContent* c = NULL;
    char filename_bin[32] = {0};
    uint32_t k = 1;
    for (uint32_t i = 0; i < rqr->count; i++)
    {
        req = &rqr->requests[i];
        c = &req->content;
        ANN(req);

        if (req->action == DVZ_REQUEST_ACTION_UPLOAD)
        {
            // Increment the filename.
            snprintf(filename_bin, 30, "%s.%03d", filename, k++);
            log_trace("saving secondary dump file `%s`", filename_bin);

            ANN(c);
            if (req->type == DVZ_REQUEST_OBJECT_DAT)
            {
                if (write_file(filename_bin, c->dat_upload.size, 1, c->dat_upload.data) != 0)
                    return 1;
            }
            else if (req->type == DVZ_REQUEST_OBJECT_TEX)
            {
                if (write_file(filename_bin, c->tex_upload.size, 1, c->tex_upload.data) != 0)
                    return 1;
            }
        }
    }

    return 0;
}



void dvz_requester_load(DvzRequester* rqr, const char* filename)
{
    ANN(rqr);
    ANN(filename);
    ANN(rqr->requests);

    // int res = 0;
    log_trace("start deserializing requests from file `%s`", filename);

    // Dump the DvzRequest structures.
    log_trace("load main dump file `%s`", filename);

    DvzSize size = 0;
    uint32_t* contents = dvz_read_file(filename, &size);
    if (contents == NULL)
    {
        log_error("unable to read `%s`", filename);
        return;
    }
    ASSERT(size > 0);

    // Number of requests.
    uint32_t count = size / sizeof(DvzRequest);


    // Write additional files for uploaded data.
    DvzRequest* requests = (DvzRequest*)contents;
    DvzRequest* req = NULL;
    DvzRequestContent* c = NULL;
    char filename_bin[32] = {0};
    uint32_t k = 1;

    dvz_requester_begin(rqr);

    for (uint32_t i = 0; i < count; i++)
    {
        req = &requests[i];
        c = &req->content;
        ANN(req);

        log_info("%u", req->id);

        if (req->action == DVZ_REQUEST_ACTION_UPLOAD)
        {
            // Increment the filename.
            snprintf(filename_bin, 30, "%s.%03d", filename, k++);
            log_trace("saving secondary dump file `%s`", filename_bin);

            ANN(c);
            if (req->type == DVZ_REQUEST_OBJECT_DAT)
            {
                c->dat_upload.data = dvz_read_file(filename_bin, &c->dat_upload.size);
                dvz_list_append(rqr->pointers_to_free, (DvzListItem){.p = c->dat_upload.data});
            }
            else if (req->type == DVZ_REQUEST_OBJECT_TEX)
            {
                c->tex_upload.data = dvz_read_file(filename_bin, &c->tex_upload.size);
                dvz_list_append(rqr->pointers_to_free, (DvzListItem){.p = c->tex_upload.data});
            }
        }

        dvz_requester_add(rqr, *req);
    }

    dvz_requester_end(rqr, NULL);
}



DvzRequest* dvz_requester_flush(DvzRequester* rqr, uint32_t* count)
{
    ANN(rqr);
    ANN(count);
    uint32_t n = rqr->count;

    // Modify the count pointer to the number of returned requests.
    *count = n;

    // Make a copy of the pending requests.
    DvzRequest* requests = calloc(n, sizeof(DvzRequest));
    memcpy(requests, rqr->requests, n * sizeof(DvzRequest));

    if (getenv("DVZ_DUMP") != NULL)
    {
        if (dvz_requester_dump(rqr, DVZ_DUMP_FILENAME) == 0)
            log_info("wrote %d Datoviz requests to `%s`", n, DVZ_DUMP_FILENAME);
        else
            log_error("error writing Datoviz requests to dump file `%s`", DVZ_DUMP_FILENAME);
    }

    // Flush the requests.
    rqr->count = 0;

    return requests;
}



void dvz_request_print(DvzRequest* req)
{
    ANN(req);

    char* str = "UNKNOWN";
    switch (req->action)
    {
        STR_ACTION(NONE);
        STR_ACTION(CREATE);
        STR_ACTION(DELETE);
        STR_ACTION(RESIZE);
        STR_ACTION(UPDATE);
        STR_ACTION(BIND);
        STR_ACTION(RECORD);
        STR_ACTION(UPLOAD);
        STR_ACTION(UPFILL);
        STR_ACTION(DOWNLOAD);
        STR_ACTION(SET);
        STR_ACTION(GET);
    default:
        break;
    }
    char* action = str;

    // str[0] = 0;
    switch (req->type)
    {
        STR_OBJECT(NONE);
        STR_OBJECT(BOARD);
        STR_OBJECT(CANVAS);
        STR_OBJECT(DAT);
        STR_OBJECT(TEX);
        STR_OBJECT(SAMPLER);
        STR_OBJECT(COMPUTE);
        STR_OBJECT(GRAPHICS);
        STR_OBJECT(BACKGROUND);
        STR_OBJECT(VERTEX);
        STR_OBJECT(RECORD);
    default:
        break;
    }
    char* type = str;

    log_info("Request %s %s 0x%" PRIx64, action, type, req->id);
}



/*************************************************************************************************/
/*  Board                                                                                        */
/*************************************************************************************************/

DvzRequest
dvz_create_board(DvzRequester* rqr, uint32_t width, uint32_t height, cvec4 background, int flags)
{
    CREATE_REQUEST(CREATE, BOARD);
    req.id = dvz_prng_uuid(rqr->prng);
    req.flags = flags;
    req.content.board.width = width;
    req.content.board.height = height;
    memcpy(req.content.board.background, background, sizeof(cvec4));
    return req;
}



DvzRequest dvz_update_board(DvzRequester* rqr, DvzId id)
{
    CREATE_REQUEST(UPDATE, BOARD);
    req.id = id;
    return req;
}



DvzRequest dvz_resize_board(DvzRequester* rqr, DvzId board, uint32_t width, uint32_t height)
{
    CREATE_REQUEST(RESIZE, BOARD);
    req.id = board;
    req.content.board.width = width;
    req.content.board.height = height;
    return req;
}



DvzRequest dvz_set_background(DvzRequester* rqr, DvzId id, cvec4 background)
{
    CREATE_REQUEST(SET, BACKGROUND);
    req.id = id;
    memcpy(req.content.board.background, background, sizeof(cvec4));
    return req;
}



DvzRequest dvz_delete_board(DvzRequester* rqr, DvzId id)
{
    CREATE_REQUEST(DELETE, BOARD);
    req.id = id;
    return req;
}



/*************************************************************************************************/
/*  Canvas                                                                                       */
/*************************************************************************************************/

DvzRequest
dvz_create_canvas(DvzRequester* rqr, uint32_t width, uint32_t height, cvec4 background, int flags)
{
    CREATE_REQUEST(CREATE, CANVAS);
    req.id = dvz_prng_uuid(rqr->prng);
    req.flags = flags;
    req.content.canvas.screen_width = width;
    req.content.canvas.screen_height = height;
    // NOTE: the framebuffer size will have to be determined once the window has been created.
    // As a fallback, the window size will be taken as equal to the screen size.
    memcpy(req.content.canvas.background, background, sizeof(cvec4));
    return req;
}



// DvzRequest dvz_update_canvas(DvzRequester* rqr, DvzId id)
// {
//     CREATE_REQUEST(UPDATE, CANVAS);
//     req.id = id;
//     return req;
// }



DvzRequest dvz_delete_canvas(DvzRequester* rqr, DvzId id)
{
    CREATE_REQUEST(DELETE, CANVAS);
    req.id = id;
    return req;
}



/*************************************************************************************************/
/*  Dat                                                                                          */
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



DvzRequest dvz_resize_dat(DvzRequester* rqr, DvzId dat, DvzSize size)
{
    CREATE_REQUEST(RESIZE, DAT);
    req.id = dat;
    req.content.dat.size = size;
    return req;
}



DvzRequest dvz_upload_dat(DvzRequester* rqr, DvzId dat, DvzSize offset, DvzSize size, void* data)
{
    // WARNING: the data pointer must live through the next frame in the main rendering loop.
    CREATE_REQUEST(UPLOAD, DAT);
    req.id = dat;
    req.content.dat_upload.offset = offset;
    req.content.dat_upload.size = size;
    req.content.dat_upload.data = data;
    return req;
}



/*************************************************************************************************/
/*  Tex                                                                                          */
/*************************************************************************************************/

DvzRequest
dvz_create_tex(DvzRequester* rqr, DvzTexDims dims, DvzFormat format, uvec3 shape, int flags)
{
    CREATE_REQUEST(CREATE, TEX);
    req.id = dvz_prng_uuid(rqr->prng);
    req.flags = flags;
    req.content.tex.dims = dims;
    memcpy(req.content.tex.shape, shape, sizeof(uvec3));
    req.content.tex.format = format;
    return req;
}



DvzRequest dvz_resize_tex(DvzRequester* rqr, DvzId tex, uvec3 shape)
{
    CREATE_REQUEST(RESIZE, TEX);
    req.id = tex;
    memcpy(req.content.tex.shape, shape, sizeof(uvec3));
    return req;
}



DvzRequest
dvz_upload_tex(DvzRequester* rqr, DvzId tex, uvec3 offset, uvec3 shape, DvzSize size, void* data)
{
    // WARNING: the data pointer must live through the next frame in the main rendering loop.
    CREATE_REQUEST(UPLOAD, TEX);
    req.id = tex;

    memcpy(req.content.tex_upload.offset, offset, sizeof(uvec3));
    memcpy(req.content.tex_upload.shape, shape, sizeof(uvec3));
    req.content.tex_upload.size = size;
    req.content.tex_upload.data = data;
    return req;
}



/*************************************************************************************************/
/*  Sampler                                                                                      */
/*************************************************************************************************/

DvzRequest dvz_create_sampler(DvzRequester* rqr, DvzFilter filter, DvzSamplerAddressMode mode)
{
    CREATE_REQUEST(CREATE, SAMPLER);
    req.id = dvz_prng_uuid(rqr->prng);
    req.content.sampler.filter = filter;
    req.content.sampler.mode = mode;
    return req;
}



/*************************************************************************************************/
/*  Graphics                                                                                     */
/*************************************************************************************************/

DvzRequest dvz_create_graphics(DvzRequester* rqr, DvzGraphicsType type, int flags)
{
    CREATE_REQUEST(CREATE, GRAPHICS);
    req.id = dvz_prng_uuid(rqr->prng);
    req.flags = flags;
    req.content.graphics.type = type;
    return req;
}



DvzRequest dvz_set_vertex(DvzRequester* rqr, DvzId graphics, DvzId dat)
{
    CREATE_REQUEST(SET, VERTEX);
    req.id = graphics;
    req.content.set_vertex.dat = dat;
    return req;
}



/*************************************************************************************************/
/*  Bindings                                                                                     */
/*************************************************************************************************/

DvzRequest dvz_bind_dat(DvzRequester* rqr, DvzId pipe, uint32_t slot_idx, DvzId dat)
{
    CREATE_REQUEST(BIND, DAT);
    req.id = pipe;
    req.content.set_dat.slot_idx = slot_idx;
    req.content.set_dat.dat = dat;
    return req;
}



DvzRequest dvz_bind_tex(DvzRequester* rqr, DvzId pipe, uint32_t slot_idx, DvzId tex, DvzId sampler)
{
    CREATE_REQUEST(BIND, TEX);
    req.id = pipe;
    req.content.set_tex.slot_idx = slot_idx;
    req.content.set_tex.tex = tex;
    req.content.set_tex.sampler = sampler;
    return req;
}



/*************************************************************************************************/
/*  Command buffer                                                                               */
/*************************************************************************************************/

DvzRequest dvz_record_begin(DvzRequester* rqr, DvzId board)
{
    CREATE_REQUEST(RECORD, RECORD);
    req.id = board;
    req.content.record.command.type = DVZ_RECORDER_BEGIN;
    return req;
}



DvzRequest dvz_record_viewport(DvzRequester* rqr, DvzId board, vec2 offset, vec2 shape)
{
    CREATE_REQUEST(RECORD, RECORD);
    req.id = board;
    req.content.record.command.type = DVZ_RECORDER_VIEWPORT;
    memcpy(req.content.record.command.contents.v.offset, offset, sizeof(vec2));
    memcpy(req.content.record.command.contents.v.shape, shape, sizeof(vec2));
    return req;
}



DvzRequest dvz_record_draw(
    DvzRequester* rqr, DvzId board, DvzId graphics, uint32_t first_vertex, uint32_t vertex_count)
{
    CREATE_REQUEST(RECORD, RECORD);
    req.id = board;
    req.content.record.command.type = DVZ_RECORDER_DRAW_DIRECT;
    req.content.record.command.contents.draw_direct.pipe_id = graphics;
    req.content.record.command.contents.draw_direct.first_vertex = first_vertex;
    req.content.record.command.contents.draw_direct.vertex_count = vertex_count;
    return req;
}



DvzRequest dvz_record_end(DvzRequester* rqr, DvzId board)
{
    CREATE_REQUEST(RECORD, RECORD);
    req.id = board;
    req.content.record.command.type = DVZ_RECORDER_END;
    return req;
}
