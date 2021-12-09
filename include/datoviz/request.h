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

typedef enum
{
    DVZ_REQUEST_ACTION_NONE,
    DVZ_REQUEST_ACTION_CREATE,
    DVZ_REQUEST_ACTION_RESIZE,
    DVZ_REQUEST_ACTION_SET,
    DVZ_REQUEST_ACTION_UPDATE,
    DVZ_REQUEST_ACTION_UPLOAD,
    DVZ_REQUEST_ACTION_DOWNLOAD,
    DVZ_REQUEST_ACTION_UPFILL,
    DVZ_REQUEST_ACTION_DELETE,
    DVZ_REQUEST_ACTION_GET,
} DvzRequestAction;



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
    struct
    {
        uint32_t width, height;
        int flags;
    } board;

    struct
    {
        uint32_t width, height;
        int flags;
    } canvas;

    struct
    {
        DvzBufferType type;
        DvzSize size;
        int flags;
    } dat;

    struct
    {
        DvzTexDims dims;
        uvec3 shape;
        DvzFormat format;
        int flags;
    } tex;
};



struct DvzRequest
{
    uint32_t version;          // request version
    DvzRequestAction action;   // type of action
    DvzObjectType type;        // type of the object targetted by the action
    DvzId id;                  // id of the object
    DvzRequestContent content; // details on the action
};



struct DvzRequester
{
    DvzObject obj;
    DvzPrng* prng;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

// TODO: docstrings

DVZ_EXPORT DvzRequester dvz_requester(void);



DVZ_EXPORT void dvz_requester_destroy(DvzRequester* rqr);



// DVZ_EXPORT DvzRequest dvz_request(void);



DVZ_EXPORT void dvz_request_print(DvzRequest* req);



/*************************************************************************************************/
/*  Board                                                                                        */
/*************************************************************************************************/

DVZ_EXPORT DvzRequest
dvz_create_board(DvzRequester* rqr, uint32_t width, uint32_t height, int flags);



DVZ_EXPORT DvzRequest dvz_delete_board(DvzRequester* rqr, DvzId id);



// DVZ_EXPORT void dvz_create_canvas(DvzRequest* req, uint32_t width, uint32_t height, int flags);



/*************************************************************************************************/
/*  Resources                                                                                    */
/*************************************************************************************************/

DVZ_EXPORT DvzRequest
dvz_create_dat(DvzRequester* rqr, DvzBufferType type, DvzSize size, int flags);



// DVZ_EXPORT void
// dvz_create_tex(DvzRequest* req, DvzTexDims dims, uvec3 shape, DvzFormat format, int flags);



/*************************************************************************************************/
/*  Command buffer                                                                               */
/*************************************************************************************************/

// DVZ_EXPORT void dvz_set_viewport(DvzRequest* req, vec2 offset, vec2 shape);



// DVZ_EXPORT void dvz_set_graphics(DvzRequest* req, DvzPipe* pipe);



// DVZ_EXPORT void dvz_set_compute(DvzRequest* req, DvzPipe* pipe);



// DVZ_EXPORT void dvz_set_push(DvzRequest* req, ...);



// DVZ_EXPORT void dvz_set_barrier(DvzRequest* req, ...);



EXTERN_C_OFF

#endif
