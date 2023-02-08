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

#define RETURN_REQUEST                                                                            \
    if (rqr->count != UINT32_MAX)                                                                 \
        dvz_requester_add(rqr, req);                                                              \
    return req;

#define STR_ACTION(r)                                                                             \
    case DVZ_REQUEST_ACTION_##r:                                                                  \
        str = #r;                                                                                 \
        break

#define STR_OBJECT(r)                                                                             \
    case DVZ_REQUEST_OBJECT_##r:                                                                  \
        str = #r;                                                                                 \
        break

#define IF_VERBOSE if (getenv("DVZ_VERBOSE") != NULL)

// Maximum size of buffers encoded in base64 when printing the commands
#define VERBOSE_MAX_BASE64 1048576



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

    // NOTE: special value to indicate that requester_begin() as not been called, so requests calls
    // should not automatically append requests to the batch. One has to call requester_begin() to
    // initialize count to 1 and make the requests append themselves to the batch.
    rqr->count = UINT32_MAX;

    IF_VERBOSE
    printf("---\n"
           "version: '1.0'\n"
           "requests:\n");

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
    log_trace("requester destroyed");
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
    DvzRequest* requests = (DvzRequest*)dvz_read_file(filename, &size);
    if (requests == NULL)
    {
        log_error("unable to read `%s`", filename);
        return;
    }
    ASSERT(size > 0);

    // Number of requests.
    uint32_t count = size / sizeof(DvzRequest);


    // Write additional files for uploaded data.
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

        if (req->action == DVZ_REQUEST_ACTION_UPLOAD)
        {
            // Increment the filename.
            snprintf(filename_bin, 30, "%s.%03d", filename, k++);
            log_trace("saving secondary dump file `%s`", filename_bin);

            ANN(c);
            if (req->type == DVZ_REQUEST_OBJECT_DAT)
            {
                c->dat_upload.data = (void*)dvz_read_file(filename_bin, &c->dat_upload.size);
                dvz_list_append(rqr->pointers_to_free, (DvzListItem){.p = c->dat_upload.data});
            }
            else if (req->type == DVZ_REQUEST_OBJECT_TEX)
            {
                c->tex_upload.data = (void*)dvz_read_file(filename_bin, &c->tex_upload.size);
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
    if (rqr->count == UINT32_MAX)
    {
        log_error("cannot flush requester as dvz_requester_begin() was not called");
        return NULL;
    }
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
    // NOTE: setting the count to 0 means we're automatically beginning a new batch.
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
    RETURN_REQUEST
}



DvzRequest dvz_update_board(DvzRequester* rqr, DvzId id)
{
    CREATE_REQUEST(UPDATE, BOARD);
    req.id = id;
    RETURN_REQUEST
}



DvzRequest dvz_resize_board(DvzRequester* rqr, DvzId board, uint32_t width, uint32_t height)
{
    CREATE_REQUEST(RESIZE, BOARD);
    req.id = board;
    req.content.board.width = width;
    req.content.board.height = height;
    RETURN_REQUEST
}



DvzRequest dvz_set_background(DvzRequester* rqr, DvzId id, cvec4 background)
{
    CREATE_REQUEST(SET, BACKGROUND);
    req.id = id;
    memcpy(req.content.board.background, background, sizeof(cvec4));
    RETURN_REQUEST
}



DvzRequest dvz_delete_board(DvzRequester* rqr, DvzId id)
{
    CREATE_REQUEST(DELETE, BOARD);
    req.id = id;
    RETURN_REQUEST
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

    IF_VERBOSE
    printf(
        "- action: create\n"
        "  type: canvas\n"
        "  id: 0x%" PRIx64 "\n"
        "  flags: %d\n"
        "  content:\n"
        "    width: %d\n"
        "    height: %d\n",
        req.id, flags, width, height);

    RETURN_REQUEST
}



// DvzRequest dvz_update_canvas(DvzRequester* rqr, DvzId id)
// {
//     CREATE_REQUEST(UPDATE, CANVAS);
//     req.id = id;
//     RETURN_REQUEST
// }



DvzRequest dvz_delete_canvas(DvzRequester* rqr, DvzId id)
{
    CREATE_REQUEST(DELETE, CANVAS);
    req.id = id;
    RETURN_REQUEST
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

    IF_VERBOSE
    printf(
        "- action: create\n"
        "  type: dat\n"
        "  id: 0x%" PRIx64 "\n"
        "  flags: %d\n"
        "  content:\n"
        "    type: %d\n"
        "    size: %" PRIx64 "\n",
        req.id, flags, type, size);

    RETURN_REQUEST
}



DvzRequest dvz_resize_dat(DvzRequester* rqr, DvzId dat, DvzSize size)
{
    ASSERT(size > 0);

    CREATE_REQUEST(RESIZE, DAT);
    req.id = dat;
    req.content.dat.size = size;
    RETURN_REQUEST
}



DvzRequest dvz_upload_dat(DvzRequester* rqr, DvzId dat, DvzSize offset, DvzSize size, void* data)
{
    ASSERT(size > 0);
    ANN(data);

    CREATE_REQUEST(UPLOAD, DAT);
    req.id = dat;
    req.content.dat_upload.offset = offset;
    req.content.dat_upload.size = size;

    // NOTE: we make a copy of the data to ensure it lives until the renderer has done processing
    // it.
    void* data_cpy = malloc(size);
    memcpy(data_cpy, data, size);
    req.content.dat_upload.data = data_cpy;

    if (getenv("DVZ_VERBOSE") != NULL)
    {
        char* encoded = NULL;
        // NOTE: avoid computing the base64 of large arrays.
        if (size < VERBOSE_MAX_BASE64)
            encoded = b64_encode((const unsigned char*)data, size);
        else
            encoded = "<snip>";
        printf(
            "- action: upload\n"
            "  type: dat\n"
            "  id: 0x%" PRIx64 "\n"
            "  content:\n"
            "    offset: %" PRIx64 "\n"
            "    size: %" PRIx64 "\n"
            "    data:\n"
            "      mode: base64\n"
            "      buffer: %s\n",
            dat, offset, size, encoded);
        if (size < VERBOSE_MAX_BASE64)
            free(encoded);
    }
    RETURN_REQUEST
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
    RETURN_REQUEST
}



DvzRequest dvz_resize_tex(DvzRequester* rqr, DvzId tex, uvec3 shape)
{
    CREATE_REQUEST(RESIZE, TEX);
    req.id = tex;
    memcpy(req.content.tex.shape, shape, sizeof(uvec3));
    RETURN_REQUEST
}



DvzRequest
dvz_upload_tex(DvzRequester* rqr, DvzId tex, uvec3 offset, uvec3 shape, DvzSize size, void* data)
{
    CREATE_REQUEST(UPLOAD, TEX);
    req.id = tex;

    memcpy(req.content.tex_upload.offset, offset, sizeof(uvec3));
    memcpy(req.content.tex_upload.shape, shape, sizeof(uvec3));
    req.content.tex_upload.size = size;

    // NOTE: we make a copy of the data to ensure it lives until the renderer has done processing
    // it.
    void* data_cpy = malloc(size);
    memcpy(data_cpy, data, size);
    req.content.tex_upload.data = data_cpy;

    RETURN_REQUEST
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
    RETURN_REQUEST
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

    IF_VERBOSE
    printf(
        "- action: create\n"
        "  type: graphics\n"
        "  id: 0x%" PRIx64 "\n"
        "  flags: %d\n"
        "  content:\n"
        "    type: %d\n",
        req.id, flags, type);

    RETURN_REQUEST
}



DvzRequest
dvz_bind_vertex(DvzRequester* rqr, DvzId graphics, uint32_t binding_idx, DvzId dat, DvzSize offset)
{
    CREATE_REQUEST(BIND, VERTEX);
    req.id = graphics;
    req.content.bind_vertex.binding_idx = binding_idx;
    req.content.bind_vertex.dat = dat;
    req.content.bind_vertex.offset = offset;

    IF_VERBOSE
    printf(
        "- action: bind\n"
        "  type: vertex\n"
        "  id: 0x%" PRIx64 "\n"
        "  content:\n"
        "    binding_idx: %d\n"
        "    dat: 0x%" PRIx64 "\n"
        "    offset: 0x%" PRIx64 "\n",
        req.id, binding_idx, dat, offset);

    RETURN_REQUEST
}



DvzRequest dvz_set_primitive(DvzRequester* rqr, DvzId graphics, DvzPrimitiveTopology primitive)
{
    CREATE_REQUEST(SET, PRIMITIVE);
    req.id = graphics;
    req.content.set_primitive.primitive = primitive;

    IF_VERBOSE
    printf(
        "- action: set\n"
        "  type: primitive\n"
        "  id: 0x%" PRIx64 "\n"
        "  content:\n"
        "    primitive: %d\n",
        req.id, primitive);

    RETURN_REQUEST
}



DvzRequest dvz_set_blend(DvzRequester* rqr, DvzId graphics, DvzBlendType blend_type)
{
    CREATE_REQUEST(SET, BLEND);
    req.id = graphics;
    req.content.set_blend.blend = blend_type;

    IF_VERBOSE
    printf(
        "- action: set\n"
        "  type: blend\n"
        "  id: 0x%" PRIx64 "\n"
        "  content:\n"
        "    blend: %d\n",
        req.id, blend_type);

    RETURN_REQUEST
}



DvzRequest dvz_set_depth(DvzRequester* rqr, DvzId graphics, DvzDepthTest depth_test)
{
    CREATE_REQUEST(SET, DEPTH);
    req.id = graphics;
    req.content.set_depth.depth = depth_test;

    IF_VERBOSE
    printf(
        "- action: set\n"
        "  type: depth\n"
        "  id: 0x%" PRIx64 "\n"
        "  content:\n"
        "    depth: %d\n",
        req.id, depth_test);

    RETURN_REQUEST
}



DvzRequest dvz_set_polygon(DvzRequester* rqr, DvzId graphics, DvzPolygonMode polygon_mode)
{
    CREATE_REQUEST(SET, POLYGON);
    req.id = graphics;
    req.content.set_polygon.polygon = polygon_mode;

    IF_VERBOSE
    printf(
        "- action: set\n"
        "  type: polygon\n"
        "  id: 0x%" PRIx64 "\n"
        "  content:\n"
        "    polygon: %d\n",
        req.id, polygon_mode);

    RETURN_REQUEST
}



DvzRequest dvz_set_cull(DvzRequester* rqr, DvzId graphics, DvzCullMode cull_mode)
{
    CREATE_REQUEST(SET, CULL);
    req.id = graphics;
    req.content.set_cull.cull = cull_mode;

    IF_VERBOSE
    printf(
        "- action: set\n"
        "  type: cull\n"
        "  id: 0x%" PRIx64 "\n"
        "  content:\n"
        "    cull: %d\n",
        req.id, cull_mode);

    RETURN_REQUEST
}



DvzRequest dvz_set_front(DvzRequester* rqr, DvzId graphics, DvzFrontFace front_face)
{
    CREATE_REQUEST(SET, FRONT);
    req.id = graphics;
    req.content.set_front.front = front_face;

    IF_VERBOSE
    printf(
        "- action: set\n"
        "  type: front\n"
        "  id: 0x%" PRIx64 "\n"
        "  content:\n"
        "    front: %d\n",
        req.id, front_face);

    RETURN_REQUEST
}



DvzRequest dvz_set_glsl(
    DvzRequester* rqr, DvzId graphics, DvzShaderType shader_type, DvzSize size, const char* code)
{
    CREATE_REQUEST(SET, GLSL);
    req.id = graphics;
    req.content.set_glsl.shader_type = shader_type;
    req.content.set_glsl.size = size;
    req.content.set_glsl.code = code;

    if (getenv("DVZ_VERBOSE") != NULL)
    {
        char* encoded = NULL;
        // NOTE: avoid computing the base64 of large arrays.
        if (size < VERBOSE_MAX_BASE64)
            encoded = b64_encode((const unsigned char*)code, size);
        else
            encoded = "<snip>";
        printf(
            "- action: set\n"
            "  type: glsl\n"
            "  id: 0x%" PRIx64 "\n"
            "  content:\n"
            "    shader_type: %d\n"
            "    size: %" PRIx64 "\n"
            "    data:\n"
            "      mode: base64\n"
            "      buffer: %s\n",
            req.id, shader_type, size, encoded);
        if (size < VERBOSE_MAX_BASE64)
            free(encoded);
    }
    RETURN_REQUEST
}



DvzRequest dvz_set_spirv(
    DvzRequester* rqr, DvzId graphics, DvzShaderType shader_type, DvzSize size,
    const uint32_t* buffer)
{
    CREATE_REQUEST(SET, SPIRV);
    req.id = graphics;
    req.content.set_spirv.shader_type = shader_type;
    req.content.set_spirv.size = size;
    req.content.set_spirv.buffer = buffer;

    if (getenv("DVZ_VERBOSE") != NULL)
    {
        char* encoded = NULL;
        // NOTE: avoid computing the base64 of large arrays.
        if (size < VERBOSE_MAX_BASE64)
            encoded = b64_encode((const unsigned char*)buffer, size);
        else
            encoded = "<snip>";
        printf(
            "- action: set\n"
            "  type: spirv\n"
            "  id: 0x%" PRIx64 "\n"
            "  content:\n"
            "    shader_type: %d\n"
            "    size: %" PRIx64 "\n"
            "    data:\n"
            "      mode: base64\n"
            "      buffer: %s\n",
            req.id, shader_type, size, encoded);
        if (size < VERBOSE_MAX_BASE64)
            free(encoded);
    }
    RETURN_REQUEST
}



DvzRequest dvz_set_vertex(
    DvzRequester* rqr, DvzId graphics, uint32_t binding_idx, DvzSize stride,
    DvzVertexInputRate input_rate)
{
    CREATE_REQUEST(SET, VERTEX);
    req.id = graphics;
    req.content.set_vertex.binding_idx = binding_idx;
    req.content.set_vertex.stride = stride;
    req.content.set_vertex.binding_idx = input_rate;

    IF_VERBOSE
    printf(
        "- action: set\n"
        "  type: vertex\n"
        "  id: 0x%" PRIx64 "\n"
        "  content:\n"
        "    binding_idx: %d\n"
        "    stride: %" PRIx64 "\n"
        "    input_rate: %d\n",
        req.id, binding_idx, stride, input_rate);

    RETURN_REQUEST
}



DvzRequest dvz_set_attr(
    DvzRequester* rqr, DvzId graphics, uint32_t binding_idx, uint32_t location, DvzFormat format,
    DvzSize offset)
{
    CREATE_REQUEST(SET, VERTEX_ATTR);
    req.id = graphics;
    req.content.set_attr.binding_idx = binding_idx;
    req.content.set_attr.location = location;
    req.content.set_attr.format = format;
    req.content.set_attr.offset = offset;

    IF_VERBOSE
    printf(
        "- action: set\n"
        "  type: attr\n"
        "  id: 0x%" PRIx64 "\n"
        "  content:\n"
        "    binding_idx: %d\n"
        "    location: %d\n"
        "    format: %d\n"
        "    offset: %" PRIx64 "\n",
        req.id, binding_idx, location, format, offset);

    RETURN_REQUEST
}



DvzRequest
dvz_set_slot(DvzRequester* rqr, DvzId graphics, uint32_t slot_idx, DvzDescriptorType type)
{
    CREATE_REQUEST(SET, SLOT);
    req.id = graphics;
    req.content.set_slot.slot_idx = slot_idx;
    req.content.set_slot.type = type;

    IF_VERBOSE
    printf(
        "- action: set\n"
        "  type: slot\n"
        "  id: 0x%" PRIx64 "\n"
        "  content:\n"
        "    slot_idx: %d\n"
        "    type: %d\n",
        req.id, slot_idx, type);

    RETURN_REQUEST
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
    RETURN_REQUEST
}



DvzRequest dvz_bind_tex(DvzRequester* rqr, DvzId pipe, uint32_t slot_idx, DvzId tex, DvzId sampler)
{
    CREATE_REQUEST(BIND, TEX);
    req.id = pipe;
    req.content.set_tex.slot_idx = slot_idx;
    req.content.set_tex.tex = tex;
    req.content.set_tex.sampler = sampler;
    RETURN_REQUEST
}



/*************************************************************************************************/
/*  Command buffer                                                                               */
/*************************************************************************************************/

DvzRequest dvz_record_begin(DvzRequester* rqr, DvzId canvas_or_board_id)
{
    CREATE_REQUEST(RECORD, RECORD);
    req.id = canvas_or_board_id;
    req.content.record.command.type = DVZ_RECORDER_BEGIN;

    IF_VERBOSE
    printf(
        "- action: record\n"
        "  type: begin\n"
        "  id: 0x%" PRIx64 "\n",
        req.id);

    RETURN_REQUEST
}



DvzRequest
dvz_record_viewport(DvzRequester* rqr, DvzId canvas_or_board_id, vec2 offset, vec2 shape)
{
    CREATE_REQUEST(RECORD, RECORD);
    req.id = canvas_or_board_id;
    req.content.record.command.type = DVZ_RECORDER_VIEWPORT;
    memcpy(req.content.record.command.contents.v.offset, offset, sizeof(vec2));
    memcpy(req.content.record.command.contents.v.shape, shape, sizeof(vec2));

    IF_VERBOSE
    printf(
        "- action: record\n"
        "  type: viewport\n"
        "  id: 0x%" PRIx64 "\n"
        "  content:\n"
        "    offset:\n"
        "    - %.3f\n"
        "    - %.3f\n"
        "    shape:\n"
        "    - %.3f\n"
        "    - %.3f\n",
        canvas_or_board_id, offset[0], offset[1], shape[0], shape[1]);

    RETURN_REQUEST
}



DvzRequest dvz_record_draw(
    DvzRequester* rqr, DvzId canvas_or_board_id, DvzId graphics, //
    uint32_t first_vertex, uint32_t vertex_count,                //
    uint32_t first_instance, uint32_t instance_count)
{
    CREATE_REQUEST(RECORD, RECORD);
    req.id = canvas_or_board_id;
    req.content.record.command.type = DVZ_RECORDER_DRAW;
    req.content.record.command.contents.draw.pipe_id = graphics;
    req.content.record.command.contents.draw.first_vertex = first_vertex;
    req.content.record.command.contents.draw.vertex_count = vertex_count;
    req.content.record.command.contents.draw.first_instance = first_instance;
    req.content.record.command.contents.draw.instance_count = instance_count;

    IF_VERBOSE
    printf(
        "- action: record\n"
        "  type: draw\n"
        "  id: 0x%" PRIx64 "\n"
        "  content:\n"
        "    graphics: 0x%" PRIx64 "\n"
        "    first_vertex: %u\n"
        "    vertex_count: %u\n"
        "    first_instance: %u\n"
        "    instance_count: %u\n",
        canvas_or_board_id, graphics, first_vertex, vertex_count, first_instance, instance_count);

    RETURN_REQUEST
}



DvzRequest dvz_record_draw_indexed(
    DvzRequester* rqr, DvzId canvas_or_board_id, DvzId graphics,        //
    uint32_t first_index, uint32_t vertex_offset, uint32_t index_count, //
    uint32_t first_instance, uint32_t instance_count)
{
    CREATE_REQUEST(RECORD, RECORD);
    req.id = canvas_or_board_id;
    req.content.record.command.type = DVZ_RECORDER_DRAW_INDEXED;
    req.content.record.command.contents.draw_indexed.pipe_id = graphics;
    req.content.record.command.contents.draw_indexed.first_index = first_index;
    req.content.record.command.contents.draw_indexed.vertex_offset = vertex_offset;
    req.content.record.command.contents.draw_indexed.index_count = index_count;
    req.content.record.command.contents.draw_indexed.first_instance = first_instance;
    req.content.record.command.contents.draw_indexed.instance_count = instance_count;

    IF_VERBOSE
    printf(
        "- action: record\n"
        "  type: draw_indexed\n"
        "  id: 0x%" PRIx64 "\n"
        "  content:\n"
        "    graphics: 0x%" PRIx64 "\n"
        "    first_index: %u\n"
        "    vertex_offset: %u\n"
        "    index_count: %u\n"
        "    first_instance: %u\n"
        "    instance_count: %u\n",
        canvas_or_board_id, graphics, first_index, vertex_offset, index_count, first_instance,
        instance_count);

    RETURN_REQUEST
}



DvzRequest dvz_record_draw_indirect(
    DvzRequester* rqr, DvzId canvas_or_board_id, DvzId graphics, DvzId indirect,
    uint32_t draw_count)
{
    CREATE_REQUEST(RECORD, RECORD);
    req.id = canvas_or_board_id;
    req.content.record.command.type = DVZ_RECORDER_DRAW_INDIRECT;
    req.content.record.command.contents.draw_indirect.pipe_id = graphics;
    req.content.record.command.contents.draw_indirect.dat_indirect_id = indirect;
    req.content.record.command.contents.draw_indirect.draw_count = draw_count;

    IF_VERBOSE
    printf(
        "- action: record\n"
        "  type: draw_indirect\n"
        "  id: 0x%" PRIx64 "\n"
        "  content:\n"
        "    graphics: 0x%" PRIx64 "\n"
        "    indirect: 0x%" PRIx64 "\n"
        "    draw_count: %u\n",
        canvas_or_board_id, graphics, indirect, draw_count);

    RETURN_REQUEST
}



DvzRequest dvz_record_draw_indexed_indirect(
    DvzRequester* rqr, DvzId canvas_or_board_id, DvzId graphics, DvzId indirect,
    uint32_t draw_count)
{
    CREATE_REQUEST(RECORD, RECORD);
    req.id = canvas_or_board_id;
    req.content.record.command.type = DVZ_RECORDER_DRAW_INDEXED_INDIRECT;
    req.content.record.command.contents.draw_indirect.pipe_id = graphics;
    req.content.record.command.contents.draw_indirect.dat_indirect_id = indirect;
    req.content.record.command.contents.draw_indirect.draw_count = draw_count;

    IF_VERBOSE
    printf(
        "- action: record\n"
        "  type: draw_indirect\n"
        "  id: 0x%" PRIx64 "\n"
        "  content:\n"
        "    graphics: 0x%" PRIx64 "\n"
        "    indirect: 0x%" PRIx64 "\n"
        "    draw_count: %u\n",
        canvas_or_board_id, graphics, indirect, draw_count);

    RETURN_REQUEST
}



DvzRequest dvz_record_end(DvzRequester* rqr, DvzId canvas_or_board_id)
{
    CREATE_REQUEST(RECORD, RECORD);
    req.id = canvas_or_board_id;
    req.content.record.command.type = DVZ_RECORDER_END;

    IF_VERBOSE
    printf(
        "- action: record\n"
        "  type: end\n"
        "  id: 0x%" PRIx64 "\n",
        req.id);

    RETURN_REQUEST
}
