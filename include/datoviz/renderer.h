/*************************************************************************************************/
/*  Renderer                                                                                     */
/*************************************************************************************************/

#ifndef DVZ_HEADER_RENDERER
#define DVZ_HEADER_RENDERER



/*************************************************************************************************/
/*  Includes                                                                                    */
/*************************************************************************************************/

#include "common.h"
#include "scene/graphics.h"
#include "pipelib.h"
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
typedef struct DvzCanvas DvzCanvas;
typedef struct DvzBoard DvzBoard;
typedef struct DvzMap DvzMap;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzRenderer
{
    DvzObject obj;
    DvzGpu* gpu;
    int flags;

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
 * Create a renderer.
 *
 * @param gpu the GPU
 * @param flags renderer creation flags
 * @returns the renderer
 */
DVZ_EXPORT DvzRenderer* dvz_renderer(DvzGpu* gpu, int flags);



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
 * Return a board.
 *
 * @param rd the renderer
 * @param id the board id
 * @returns the board
 */
DVZ_EXPORT DvzBoard* dvz_renderer_board(DvzRenderer* rd, DvzId id);



/**
 * Return a canvas.
 *
 * @param rd the renderer
 * @param id the canvas id
 * @returns the canvas
 */
DVZ_EXPORT DvzCanvas* dvz_renderer_canvas(DvzRenderer* rd, DvzId id);



/**
 * Return a dat.
 *
 * @param rd the renderer
 * @param id the dat id
 * @returns the dat
 */
DVZ_EXPORT DvzDat* dvz_renderer_dat(DvzRenderer* rd, DvzId id);



/**
 * Return a tex.
 *
 * @param rd the renderer
 * @param id the tex id
 * @returns the tex
 */
DVZ_EXPORT DvzTex* dvz_renderer_tex(DvzRenderer* rd, DvzId id);



/**
 * Return a pipe.
 *
 * @param rd the renderer
 * @param id the pipe id
 * @returns the pipe
 */
DVZ_EXPORT DvzPipe* dvz_renderer_pipe(DvzRenderer* rd, DvzId id);



/**
 * Retrieve the rendered image.
 *
 * @param rd the renderer
 * @param board_id the id of the board
 * @param size a pointer to a variable that will store the size, in bytes, of the downloaded image
 * @param rgb a pointer to the image, or NULL if this array should be handled by datoviz
 * @returns a pointer to the image
 */
DVZ_EXPORT uint8_t* dvz_renderer_image(DvzRenderer* rd, DvzId bc_id, DvzSize* size, uint8_t* rgb);



/**
 * Destroy a renderer.
 *
 * @param rd the renderer
 */
DVZ_EXPORT void dvz_renderer_destroy(DvzRenderer* rd);



EXTERN_C_OFF

#endif
