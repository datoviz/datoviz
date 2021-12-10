/*************************************************************************************************/
/*  Request                                                                                      */
/*************************************************************************************************/

#ifndef DVZ_HEADER_REQUEST
#define DVZ_HEADER_REQUEST



/*************************************************************************************************/
/*  Includes                                                                                    */
/*************************************************************************************************/

#include "_enums.h"
#include "_log.h"
#include "_macros.h"
#include "_math.h"
#include "_obj.h"
#include "_prng.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_REQUEST_VERSION 1



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Request action.
typedef enum
{
    DVZ_REQUEST_ACTION_NONE,
    DVZ_REQUEST_ACTION_CREATE,
    DVZ_REQUEST_ACTION_DELETE,
    DVZ_REQUEST_ACTION_RESIZE,
    DVZ_REQUEST_ACTION_UPDATE,
    DVZ_REQUEST_ACTION_UPLOAD,
    DVZ_REQUEST_ACTION_UPFILL,
    DVZ_REQUEST_ACTION_DOWNLOAD,
    DVZ_REQUEST_ACTION_SET,
    DVZ_REQUEST_ACTION_GET,
} DvzRequestAction;



// Request object.
typedef enum
{
    DVZ_REQUEST_OBJECT_NONE,
    DVZ_REQUEST_OBJECT_BOARD,
    DVZ_REQUEST_OBJECT_CANVAS,
    DVZ_REQUEST_OBJECT_DAT,
    DVZ_REQUEST_OBJECT_TEX,
    DVZ_REQUEST_OBJECT_SAMPLER,
    DVZ_REQUEST_OBJECT_COMPUTE,
    DVZ_REQUEST_OBJECT_GRAPHICS,

    DVZ_REQUEST_OBJECT_BEGIN,
    DVZ_REQUEST_OBJECT_VIEWPORT,
    DVZ_REQUEST_OBJECT_VERTEX,
    DVZ_REQUEST_OBJECT_BARRIER,
    DVZ_REQUEST_OBJECT_DRAW,
    DVZ_REQUEST_OBJECT_END,
} DvzRequestObject;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzRequest DvzRequest;
typedef union DvzRequestContent DvzRequestContent;
typedef struct DvzRequester DvzRequester;

// Forward declarations.
typedef struct DvzPipe DvzPipe;
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
    } board;

    // Canvas.
    struct
    {
        uint32_t width, height;
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

    // Dat upload.
    struct
    {
        int upload_type; // 0=direct (data pointer), otherwise custom transfer method
        DvzSize offset, size;
        void* data;
    } dat_upload;

    // Graphics.
    struct
    {
        DvzId board;
        DvzGraphicsType type;
    } graphics;

    // Set vertex.
    struct
    {
        DvzId dat;
    } set_vertex;

    // Set viewport.
    struct
    {
        vec2 offset, shape; // in framebuffer pixels
    } set_viewport;

    // Set draw.
    struct
    {
        DvzId graphics;
        uint32_t first_vertex, vertex_count;
    } set_draw;
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
DVZ_EXPORT DvzRequester dvz_requester(void);



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
 * @param rqr the requester
 * @param width the board width
 * @param height the board height
 * @param flags the board creation flags
 * @returns the request, containing a newly-generated id for the board to be created
 */
DVZ_EXPORT DvzRequest
dvz_create_board(DvzRequester* rqr, uint32_t width, uint32_t height, int flags);



/**
 * Create a request for a board redraw (command buffer submission).
 *
 * @param rqr the requester
 * @param id the board id
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_update_board(DvzRequester* rqr, DvzId id);



/**
 * Create a request for a board deletion.
 *
 * @param rqr the requester
 * @param id the board id
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_delete_board(DvzRequester* rqr, DvzId id);



/*************************************************************************************************/
/*  Resources                                                                                    */
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
 * Create a request for a tex creation.
 *
 * @param rqr the requester
 * @param dims the number of dimensions, 1, 2, or 3
 * @param shape the texture shape
 * @param format the image format
 * @param flags the dat creation flags
 * @returns the request, containing a newly-generated id for the tex to be created
 */
DVZ_EXPORT DvzRequest
dvz_create_tex(DvzRequester* rqr, DvzTexDims dims, uvec3 shape, DvzFormat format, int flags);



/*************************************************************************************************/
/*  Graphics                                                                                     */
/*************************************************************************************************/

/**
 * Create a request for a builtin graphics pipe creation.
 *
 * @param rqr the requester
 * @param the board
 * @param type the graphics type
 * @param flags the graphics creation flags
 * @returns the request, containing a newly-generated id for the graphics pipe to be created
 */
DVZ_EXPORT DvzRequest
dvz_create_graphics(DvzRequester* rqr, DvzId board, DvzGraphicsType type, int flags);



/**
 * Create a request for associating a vertex dat to a graphics pipe.
 *
 * @param rqr the requester
 * @param graphics the id of the graphics pipe
 * @param dat the id of the dat with the vertex data
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_set_vertex(DvzRequester* rqr, DvzId graphics, DvzId dat);



/*************************************************************************************************/
/*  Data                                                                                         */
/*************************************************************************************************/

/**
 * Create a request for dat upload.
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
/*  Command buffer                                                                               */
/*************************************************************************************************/

/**
 * Create a request for starting recording of command buffer.
 *
 * @param rqr the requester
 * @param board the id of the board
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_set_begin(DvzRequester* rqr, DvzId board);



/**
 * Create a request for setting the viewport during command buffer recording.
 *
 * @param rqr the requester
 * @param board the id of the board
 * @param offset the viewport offset, in framebuffer pixels
 * @param shape the viewport size, in framebuffer pixels
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_set_viewport(DvzRequester* rqr, DvzId board, vec2 offset, vec2 shape);



/**
 * Create a request for drawing a graphics during command buffer recording.
 *
 * @param rqr the requester
 * @param board the id of the board
 * @param graphics the id of the graphics pipe to draw
 * @param first_vertex the index of the first vertex to draw
 * @param vertex_count the number of vertices to draw
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_set_draw(
    DvzRequester* rqr, DvzId board, DvzId graphics, //
    uint32_t first_vertex, uint32_t vertex_count);



/**
 * Create a request for ending recording of command buffer.
 *
 * @param rqr the requester
 * @param board the id of the board
 * @returns the request
 */
DVZ_EXPORT DvzRequest dvz_set_end(DvzRequester* rqr, DvzId board);



EXTERN_C_OFF

#endif
