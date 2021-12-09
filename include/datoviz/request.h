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

    // Graphics.
    struct
    {
        DvzGraphicsType type;
    } graphics;
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

// TODO: docstrings

DVZ_EXPORT DvzRequester dvz_requester(void);



DVZ_EXPORT void dvz_requester_destroy(DvzRequester* rqr);



/*************************************************************************************************/
/*  Request batch                                                                                */
/*************************************************************************************************/

DVZ_EXPORT void dvz_requester_begin(DvzRequester* rqr);



DVZ_EXPORT void dvz_requester_add(DvzRequester* rqr, DvzRequest req);



DVZ_EXPORT DvzRequest* dvz_requester_end(DvzRequester* rqr, uint32_t* count);



DVZ_EXPORT void dvz_request_print(DvzRequest* req);



/*************************************************************************************************/
/*  Board                                                                                        */
/*************************************************************************************************/

DVZ_EXPORT DvzRequest
dvz_create_board(DvzRequester* rqr, uint32_t width, uint32_t height, int flags);



DVZ_EXPORT DvzRequest dvz_delete_board(DvzRequester* rqr, DvzId id);



/*************************************************************************************************/
/*  Resources                                                                                    */
/*************************************************************************************************/

DVZ_EXPORT DvzRequest
dvz_create_dat(DvzRequester* rqr, DvzBufferType type, DvzSize size, int flags);



DVZ_EXPORT DvzRequest
dvz_create_tex(DvzRequester* rqr, DvzTexDims dims, uvec3 shape, DvzFormat format, int flags);



/*************************************************************************************************/
/*  Graphics                                                                                     */
/*************************************************************************************************/

DVZ_EXPORT DvzRequest dvz_create_graphics(DvzRequester* rqr, DvzGraphicsType type, int flags);



/*************************************************************************************************/
/*  Data                                                                                         */
/*************************************************************************************************/

DVZ_EXPORT DvzRequest
dvz_upload_dat(DvzRequester* rqr, DvzId dat, DvzSize offset, DvzSize size, void* data);



/*************************************************************************************************/
/*  Command buffer                                                                               */
/*************************************************************************************************/

DVZ_EXPORT DvzRequest dvz_set_begin(DvzRequester* rqr, DvzId board);



DVZ_EXPORT DvzRequest dvz_set_vertex(DvzRequester* rqr, DvzId graphics, DvzId dat);



DVZ_EXPORT DvzRequest dvz_set_viewport(DvzRequester* rqr, DvzId board, vec2 offset, vec2 shape);



DVZ_EXPORT DvzRequest
dvz_set_draw(DvzRequester* rqr, DvzId graphics, uint32_t first_vertex, uint32_t vertex_count);



DVZ_EXPORT DvzRequest dvz_set_end(DvzRequester* rqr, DvzId board);



EXTERN_C_OFF

#endif
