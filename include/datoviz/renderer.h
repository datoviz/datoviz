/*************************************************************************************************/
/*  Renderer                                                                                     */
/*************************************************************************************************/

#ifndef DVZ_HEADER_RENDERER
#define DVZ_HEADER_RENDERER



/*************************************************************************************************/
/*  Includes                                                                                    */
/*************************************************************************************************/

#include "common.h"
#include "pipe.h"
#include "request.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzRenderer DvzRenderer;
typedef struct DvzRouter DvzRouter;

// Forward declarations.
typedef struct DvzContext DvzContext;
typedef struct DvzPipelib DvzPipelib;
typedef struct DvzWorkspace DvzWorkspace;
typedef struct DvzMap DvzMap;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzRenderer
{
    DvzObject obj;
    DvzGpu* gpu;

    DvzContext* ctx;         // data: the "what"
    DvzPipelib* pipelib;     // GLSL programs: the "how"
    DvzWorkspace* workspace; // boards and canvases: the "where"
    DvzMap* map;             // mapping between uuid and <type, objects>
    DvzRouter* router;       // mapping between pairs (action, obj_type) and functions
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create an offscreen renderer.
 *
 * @param gpu the GPU
 * @returns the renderer
 */
DVZ_EXPORT DvzRenderer* dvz_renderer_offscreen(DvzGpu* gpu);



/**
 * Create a live glfw renderer.
 *
 * @param gpu the GPU
 * @returns the renderer
 */
DVZ_EXPORT DvzRenderer* dvz_renderer_glfw(DvzGpu* gpu);



/**
 * Submit a request to the renderer.
 *
 * @param rd the renderer
 * @param req the request
 */
DVZ_EXPORT void dvz_renderer_request(DvzRenderer* rd, DvzRequest req);



/**
 * Submit a batch of requests to the renderer.
 *
 * @param rd the renderer
 * @param count the number of requests to submit
 * @param reqs an array of requests
 */
DVZ_EXPORT void dvz_renderer_requests(DvzRenderer* rd, uint32_t count, DvzRequest* reqs);



/**
 * Retrieve the rendered image.
 *
 * @param rd the renderer
 * @param board_id the id of the board
 * @param size a pointer to a variable that will store the size, in bytes, of the downloaded image
 * @param rgba a pointer to the image, or NULL if this array should be handled by datoviz
 * @returns a pointer to the image
 */
DVZ_EXPORT uint8_t*
dvz_renderer_image(DvzRenderer* rd, DvzId board_id, DvzSize* size, uint8_t* rgba);



/**
 * Destroy a renderer.
 *
 * @param rd the renderer
 */
DVZ_EXPORT void dvz_renderer_destroy(DvzRenderer* rd);



EXTERN_C_OFF

#endif
