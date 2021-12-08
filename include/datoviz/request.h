/*************************************************************************************************/
/*  Request                                                                                      */
/*************************************************************************************************/

#ifndef DVZ_HEADER_REQUEST
#define DVZ_HEADER_REQUEST



/*************************************************************************************************/
/*  Includes                                                                                    */
/*************************************************************************************************/

#include "common.h"
#include "resources.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_ID_AUTO 0



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

typedef uint64_t DvzId;

// Forward declarations.
typedef struct DvzPipe DvzPipe;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

union DvzRequestContent
{
    struct
    {
        int a;
    } a;
};



struct DvzRequest
{
    uint32_t version;          // request version
    DvzRequestAction action;   // type of action
    DvzObjectType type;        // type of the object targetted by the action
    DvzId id;                  // id of the object
    DvzRequestContent content; // details on the action
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

// TODO: docstrings

DVZ_EXPORT DvzId dvz_id(void);



DVZ_EXPORT DvzRequest dvz_request();



DVZ_EXPORT DvzId dvz_create_canvas(DvzRequest* req, uint32_t width, uint32_t height, int flags);



DVZ_EXPORT DvzId dvz_create_dat(DvzRequest* req, DvzBufferType type, DvzSize size, int flags);



DVZ_EXPORT DvzId
dvz_create_tex(DvzRequest* req, DvzTexDims dims, uvec3 shape, VkFormat format, int flags);



DVZ_EXPORT void dvz_set_viewport(DvzRequest* req, vec2 offset, vec2 shape);



DVZ_EXPORT void dvz_set_graphics(DvzRequest* req, DvzPipe* pipe);



DVZ_EXPORT void dvz_set_compute(DvzRequest* req, DvzPipe* pipe);



// DVZ_EXPORT void dvz_set_push(DvzRequest* req, ...);



// DVZ_EXPORT void dvz_set_barrier(DvzRequest* req, ...);



EXTERN_C_OFF

#endif
