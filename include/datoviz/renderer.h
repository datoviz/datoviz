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

// TODO: docstrings

DVZ_EXPORT DvzRenderer* dvz_renderer_offscreen(DvzGpu* gpu);



DVZ_EXPORT DvzRenderer* dvz_renderer_glfw(DvzGpu* gpu);



DVZ_EXPORT void dvz_renderer_request(DvzRenderer* rd, DvzRequest req);



DVZ_EXPORT void dvz_renderer_image(DvzRenderer* rd, DvzId canvas_id, DvzSize size, uint8_t* rgba);



DVZ_EXPORT void dvz_renderer_destroy(DvzRenderer* rd);



EXTERN_C_OFF

#endif
