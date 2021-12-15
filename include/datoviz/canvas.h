/*************************************************************************************************/
/*  Canvas                                                                                       */
/*************************************************************************************************/

#ifndef DVZ_HEADER_CANVAS
#define DVZ_HEADER_CANVAS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "common.h"
#include "input.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzCanvas DvzCanvas;
typedef struct DvzRender DvzRender;
typedef struct DvzSync DvzSync;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/


struct DvzRender
{
    DvzSwapchain swapchain;
    DvzImages depth_image;

    DvzImages pick_image;
    DvzImages pick_staging;

    DvzBuffer screencast_staging;
    DvzImages* screencast_img;

    DvzFramebuffers framebuffers;
    DvzFramebuffers framebuffers_overlay; // used by the overlay renderpass

    // Renderpasses.
    DvzRenderpass renderpass;         // default renderpass
    DvzRenderpass renderpass_overlay; // GUI overlay renderpass

    DvzSubmit submit;
};



struct DvzSync
{
    DvzSemaphores sem_img_available;
    DvzSemaphores sem_render_finished;
    DvzSemaphores* present_semaphores;
    DvzFences fences_render_finished;
    DvzFences fences_flight;
};



/*************************************************************************************************/
/*  Canvas struct                                                                                */
/*************************************************************************************************/

struct DvzCanvas
{
    DvzObject obj;
    DvzGpu* gpu;
    uvec2 size_init;
    int flags;

    DvzWindow* window;
    DvzInput input;
    DvzCommands cmds;
    DvzRender render;
    DvzSync sync;

    // Frames.
    uint32_t cur_frame; // current frame within the images in flight
    uint64_t frame_idx;
    bool resized;

    void* user_data;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Canvas functions                                                                             */
/*************************************************************************************************/

/**
 * Initialize a canvas.
 *
 * @returns a canvas
 */
DVZ_EXPORT DvzCanvas dvz_canvas(DvzGpu* gpu, uint32_t width, uint32_t height, int flags);



/**
 * Create a canvas.
 *
 * @param canvas a canvas
 */
DVZ_EXPORT void dvz_canvas_create(DvzCanvas* canvas);



/**
 * Destroy a canvas.
 *
 * @param canvas the canvas to destroy
 */
DVZ_EXPORT void dvz_canvas_destroy(DvzCanvas* canvas);



EXTERN_C_OFF

#endif
