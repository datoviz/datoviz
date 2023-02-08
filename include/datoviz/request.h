/*************************************************************************************************/
/*  Request                                                                                      */
/*************************************************************************************************/

#ifndef DVZ_HEADER_REQUEST
#define DVZ_HEADER_REQUEST



/*************************************************************************************************/
/*  Includes                                                                                    */
/*************************************************************************************************/

#include "_prng.h"
#include "recorder.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_REQUEST_VERSION 1
// filename for the dump, to activate with the DVZ_DUMP=1 environment variable
#define DVZ_DUMP_FILENAME "requests.dvz"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Request flags.
typedef enum
{
    DVZ_REQUEST_FLAGS_NONE = 0x0000,
    DVZ_REQUEST_FLAGS_OFFSCREEN = 0x1000,
} DvzRequestFlags;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzRequest DvzRequest;
typedef union DvzRequestContent DvzRequestContent;
typedef struct DvzRequester DvzRequester;

// Forward declarations.
typedef struct DvzPipe DvzPipe;
typedef struct DvzList DvzList;
typedef uint64_t DvzId;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

union DvzRequestContent
{
    // Board.
    struct
    {
        uint32_t width, height;
        cvec4 background;
    } board;

    // Canvas.
    struct
    {
        uint32_t framebuffer_width, framebuffer_height;
        uint32_t screen_width, screen_height;
        cvec4 background;
    } canvas;

    // Dat.
    struct
    {
        DvzBufferType type;
        DvzSize size;
    } dat;

    // Tex.
    struct
    {
        DvzTexDims dims;
        uvec3 shape;
        DvzFormat format;
    } tex;

    // Sampler.
    struct
    {
        DvzFilter filter;
        DvzSamplerAddressMode mode;
    } sampler;

    // Dat upload.
    struct
    {
        int upload_type; // 0=direct (data pointer), otherwise custom transfer method
        DvzSize offset, size;
        void* data;
    } dat_upload;

    // Tex upload.
    struct
    {
        int upload_type; // 0=direct (data pointer), otherwise custom transfer method
        uvec3 offset, shape;
        DvzSize size;
        void* data;
    } tex_upload;

    // Graphics.
    struct
    {
        DvzGraphicsType type;
    } graphics;

    // Set vertex.
    struct
    {
        DvzId dat;
    } set_vertex;

    // Set primitive.
    struct
    {
        DvzPrimitiveTopology primitive;
    } set_primitive;

    // Set pipe binding with dat.
    struct
    {
        uint32_t slot_idx;
        DvzId dat;
    } set_dat;

    // Set pipe binding with tex.
    struct
    {
        uint32_t slot_idx;
        DvzId tex;
        DvzId sampler;
    } set_tex;

    // Record a command.
    struct
    {
        DvzRecorderCommand command;
    } record;
};



struct DvzRequest
{
    uint32_t version;          // request version
    DvzRequestAction action;   // type of action
    DvzRequestObject type;     // type of the object targetted by the action
    DvzId id;                  // id of the object
    DvzRequestContent content; // details on the action
    int tag;                   // optional tag
    int flags;                 // custom flags
};



struct DvzRequester
{
    DvzObject obj;
    DvzPrng* prng;

    // Used for creating batch requests.
    uint32_t count;
    uint32_t capacity;
    DvzRequest* requests;

    DvzList* pointers_to_free; // HACK: list of pointers created when loading requests dumps
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Requester                                                                                    */
/*************************************************************************************************/

/**
 * Create a requester, used to create requests.
 *
 * @returns the requester struct
 */
DVZ_EXPORT DvzRequester* dvz_requester(void);



/**
 * Destroy a requester.
 *
 * @param rqr the requester
 */
DVZ_EXPORT void dvz_requester_destroy(DvzRequester* rqr);



/*************************************************************************************************/
/*  Request batch                                                                                */
/*************************************************************************************************/

/**
 * Start a batch request.
 *
 * @param rqr the requester
 */
DVZ_EXPORT void dvz_requester_begin(DvzRequester* rqr);



/**
 * Add a request to a batch request.
 *
 * @param rqr the requester
 * @param req the request
 */
DVZ_EXPORT void dvz_requester_add(DvzRequester* rqr, DvzRequest req);



/**
 * End a request batch.
 *
 * @param rqr the requester
 * @param count a pointer to the number of requests added, that will be set by this function
 * @returns a pointer to the array of requests
 */
DVZ_EXPORT DvzRequest* dvz_requester_end(DvzRequester* rqr, uint32_t* count);



/**
 */
DVZ_EXPORT int dvz_requester_dump(DvzRequester* rqr, const char* filename);



/**
 */
DVZ_EXPORT void dvz_requester_load(DvzRequester* rqr, const char* filename);



/**
 * Return all pending requests and flush the requester.
 *
 * @param rqr the requester
 * @param[out] count the number of requests
 * @returns a pointer to an array of requests (NOTE: must be free-ed by the caller)
 */
DVZ_EXPORT DvzRequest* dvz_requester_flush(DvzRequester* rqr, uint32_t* count);



/**
 * Show information about a request.
 *
 * @param req the request
 */
DVZ_EXPORT void dvz_request_print(DvzRequest* req);



/*************************************************************************************************/
/*  Board                                                                                        */
/*************************************************************************************************/

/**
 * Create a request for board creation.
 *
 * A board is an offscreen rectangular area on which to render.
 *
 * NOTE: background color not implemented yet
 *
 * @param rqr the requester
 * @param width the board width
 * @param height the board height
 * @param background the background color
 * @param flags the board creation flags
 * @returns the request, containing a newly-generated id for the board to be created
 */
DVZ_EXPORT DvzRequest
dvz_create_board(DvzRequester* rqr, uint32_t width, uint32_t height, cvec4 background, int flags);



/**
 * Change the background color of the board.
 *
 * @param rqr the requester
 * @param id the board id
 * @param background the background color
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_set_background(DvzRequester* rqr, DvzId id, cvec4 background);



/**
 * Create a request for a board redraw (command buffer submission).
 *
 * @param rqr the requester
 * @param id the board id
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_update_board(DvzRequester* rqr, DvzId id);



/**
 * Create a request to resize a board.
 *
 * @param rqr the requester
 * @param board the board id
 * @param width the new board width
 * @param height the new board height
 * @returns the request
 */
DVZ_EXPORT DvzRequest
dvz_resize_board(DvzRequester* rqr, DvzId board, uint32_t width, uint32_t height);



/**
 * Create a request for a board deletion.
 *
 * @param rqr the requester
 * @param id the board id
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_delete_board(DvzRequester* rqr, DvzId id);



/*************************************************************************************************/
/*  Canvas                                                                                       */
/*************************************************************************************************/

/**
 * Create a request for canvas creation.
 *
 * A canvas is a live window on which to render.
 *
 * NOTE: background color not implemented yet
 *
 * @param rqr the requester
 * @param width the canvas width (in screen pixels)
 * @param height the canvas height (in screen pixels)
 * @param background the background color
 * @param flags the canvas creation flags
 * @returns the request, containing a newly-generated id for the canvas to be created
 */
DVZ_EXPORT DvzRequest
dvz_create_canvas(DvzRequester* rqr, uint32_t width, uint32_t height, cvec4 background, int flags);



/**
 * Create a request for a canvas redraw (command buffer submission).
 *
 * @param rqr the requester
 * @param id the canvas id
 * @returns the request
 */
// DVZ_EXPORT DvzRequest dvz_update_canvas(DvzRequester* rqr, DvzId id);



/**
 * Create a request for a canvas deletion.
 *
 * @param rqr the requester
 * @param id the canvas id
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_delete_canvas(DvzRequester* rqr, DvzId id);



/*************************************************************************************************/
/*  Dat                                                                                          */
/*************************************************************************************************/

/**
 * Create a request for a dat creation.
 *
 * @param rqr the requester
 * @param type the buffer type
 * @param size the dat size, in bytes
 * @param flags the dat creation flags
 * @returns the request, containing a newly-generated id for the dat to be created
 */
DVZ_EXPORT DvzRequest
dvz_create_dat(DvzRequester* rqr, DvzBufferType type, DvzSize size, int flags);



/**
 * Create a request to resize a dat.
 *
 * @param rqr the requester
 * @param dat the dat id
 * @param size the new dat size, in bytes
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_resize_dat(DvzRequester* rqr, DvzId dat, DvzSize size);



/**
 * Create a request for dat upload.
 *
 * WARNING: the passed pointer will *not* be copied and the caller must ensure the pointer will
 * live long enough until it is processed by the rendering main loop in the main thread.
 *
 * @param rqr the requester
 * @param dat the id of the dat to upload to
 * @param offset the byte offset of the upload transfer
 * @param size the number of bytes in data to transfer
 * @param data a pointer to the data to upload
 * @returns the request
 */
DVZ_EXPORT DvzRequest
dvz_upload_dat(DvzRequester* rqr, DvzId dat, DvzSize offset, DvzSize size, void* data);



/*************************************************************************************************/
/*  Tex                                                                                          */
/*************************************************************************************************/

/**
 * Create a request for a tex creation.
 *
 * @param rqr the requester
 * @param dims the number of dimensions, 1, 2, or 3
 * @param format the image format
 * @param shape the texture shape
 * @param flags the dat creation flags
 * @returns the request, containing a newly-generated id for the tex to be created
 */
DVZ_EXPORT DvzRequest
dvz_create_tex(DvzRequester* rqr, DvzTexDims dims, DvzFormat format, uvec3 shape, int flags);



/**
 * Create a request to resize a tex.
 *
 * @param rqr the requester
 * @param tex the tex id
 * @param shape the new tex shape
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_resize_tex(DvzRequester* rqr, DvzId tex, uvec3 shape);



/**
 * Create a request for tex upload.
 *
 * WARNING: the passed pointer will *not* be copied and the caller must ensure the pointer will
 * live long enough until it is processed by the rendering main loop in the main thread.
 *
 * @param rqr the requester
 * @param tex the id of the tex to upload to
 * @param offset the offset
 * @param shape the shape
 * @param size the number of bytes in data to transfer
 * @param data a pointer to the data to upload
 * @returns the request
 */
DVZ_EXPORT DvzRequest
dvz_upload_tex(DvzRequester* rqr, DvzId tex, uvec3 offset, uvec3 shape, DvzSize size, void* data);



/*************************************************************************************************/
/*  Sampler                                                                                      */
/*************************************************************************************************/

/**
 * Create a request for a sampler creation.
 *
 * @param rqr the requester
 * @param filter the sampler filter
 * @param mode the sampler address mode
 * @returns the request, containing a newly-generated id for the sampler to be created
 */
DVZ_EXPORT DvzRequest
dvz_create_sampler(DvzRequester* rqr, DvzFilter filter, DvzSamplerAddressMode mode);



/*************************************************************************************************/
/*  Graphics                                                                                     */
/*************************************************************************************************/

/**
 * Create a request for a builtin graphics pipe creation.
 *
 * @param rqr the requester
 * @param parent either the parent board or canvas id
 * @param type the graphics type
 * @param flags the graphics creation flags
 * @returns the request, containing a newly-generated id for the graphics pipe to be created
 */
DVZ_EXPORT DvzRequest dvz_create_graphics(DvzRequester* rqr, DvzGraphicsType type, int flags);



/**
 * Create a request for associating a vertex dat to a graphics pipe.
 *
 * @param rqr the requester
 * @param graphics the id of the graphics pipe
 * @param dat the id of the dat with the vertex data
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_set_vertex(DvzRequester* rqr, DvzId graphics, DvzId dat);



DvzRequest dvz_set_primitive(DvzRequester* rqr, DvzId graphics, DvzPrimitiveTopology topology);



/*************************************************************************************************/
/*  Bindings                                                                                     */
/*************************************************************************************************/

/**
 * Create a request for associating a dat to a pipe's slot.
 *
 * @param rqr the requester
 * @param pipe the id of the pipe
 * @param slot_idx the index of the binding slot
 * @param dat the id of the dat to bind to the pipe
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_bind_dat(DvzRequester* rqr, DvzId pipe, uint32_t slot_idx, DvzId dat);



/**
 * Create a request for associating a tex to a pipe's slot.
 *
 * @param rqr the requester
 * @param pipe the id of the pipe
 * @param slot_idx the index of the binding slot
 * @param tex the id of the tex to bind to the pipe
 * @param tex the id of the sampler
 * @returns the request
 */
DVZ_EXPORT DvzRequest
dvz_bind_tex(DvzRequester* rqr, DvzId pipe, uint32_t slot_idx, DvzId tex, DvzId sampler);



/*************************************************************************************************/
/*  Command buffer                                                                               */
/*************************************************************************************************/

/**
 * Create a request for starting recording of command buffer.
 *
 * @param rqr the requester
 * @param canvas_or_board_id the id of the canvas or board
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_record_begin(DvzRequester* rqr, DvzId canvas_or_board_id);



/**
 * Create a request for setting the viewport during command buffer recording.
 *
 * @param rqr the requester
 * @param canvas_or_board_id the id of the canvas or board
 * @param offset the viewport offset, in framebuffer pixels
 * @param shape the viewport size, in framebuffer pixels
 * @returns the request
 */
DVZ_EXPORT DvzRequest
dvz_record_viewport(DvzRequester* rqr, DvzId canvas_or_board_id, vec2 offset, vec2 shape);



/**
 * Create a request for a direct draw of a graphics during command buffer recording.
 *
 * @param rqr the requester
 * @param canvas_or_board_id the id of the canvas or board
 * @param graphics the id of the graphics pipe to draw
 * @param first_vertex the index of the first vertex to draw
 * @param vertex_count the number of vertices to draw
 * @param first_instance the index of the first instance to draw
 * @param instance_count the number of instances to draw
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_record_draw(
    DvzRequester* rqr, DvzId canvas_or_board_id, DvzId graphics, //
    uint32_t first_vertex, uint32_t vertex_count,                //
    uint32_t first_instance, uint32_t instance_count);



/**
 * Create a request for an indexed draw of a graphics during command buffer recording.
 *
 * @param rqr the requester
 * @param canvas_or_board_id the id of the canvas or board
 * @param graphics the id of the graphics pipe to draw
 * @param first_index the index of the first index to draw
 * @param vertex_offset the vertex offset within the vertices indexed by the indexes
 * @param index_count the number of indexes to draw
 * @param first_instance the index of the first instance to draw
 * @param instance_count the number of instances to draw
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_record_draw_indexed(
    DvzRequester* rqr, DvzId canvas_or_board_id, DvzId graphics,        //
    uint32_t first_index, uint32_t vertex_offset, uint32_t index_count, //
    uint32_t first_instance, uint32_t instance_count);



/**
 * Create a request for an indirect draw of a graphics during command buffer recording.
 *
 * @param rqr the requester
 * @param canvas_or_board_id the id of the canvas or board
 * @param graphics the id of the graphics pipe to draw
 * @param indirect the id of the dat containing the indirect draw data
 * @param draw_count the number of draws to make
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_record_draw_indirect(
    DvzRequester* rqr, DvzId canvas_or_board_id, DvzId graphics, DvzId indirect,
    uint32_t draw_count);



/**
 * Create a request for an indexed indirect draw of a graphics during command buffer recording.
 *
 * @param rqr the requester
 * @param canvas_or_board_id the id of the canvas or board
 * @param graphics the id of the graphics pipe to draw
 * @param indirect the id of the dat containing the indirect draw data
 * @param draw_count the number of draws to make
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_record_draw_indexed_indirect(
    DvzRequester* rqr, DvzId canvas_or_board_id, DvzId graphics, DvzId indirect,
    uint32_t draw_count);



/**
 * Create a request for ending recording of command buffer.
 *
 * @param rqr the requester
 * @param canvas_or_board_id the id of the canvas or board
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_record_end(DvzRequester* rqr, DvzId canvas_or_board_id);



EXTERN_C_OFF

#endif
