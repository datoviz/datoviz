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

#define DVZ_CANVAS_DEFAULT_FORMAT DVZ_FORMAT_B8G8R8A8_UNORM
#define DVZ_CANVAS_DEFAULT_CLEAR_COLOR                                                            \
    (cvec4) { 0, 8, 18, 255 }
#define DVZ_PICK_IMAGE_FORMAT VK_FORMAT_R32G32B32A32_SINT
#define DVZ_PICK_STAGING_SIZE 8

#define DVZ_MIN_SWAPCHAIN_IMAGE_COUNT 3
#define DVZ_SEMAPHORE_IMG_AVAILABLE   0
#define DVZ_SEMAPHORE_RENDER_FINISHED 1
#define DVZ_FENCE_RENDER_FINISHED     0
#define DVZ_FENCES_FLIGHT             1
#define DVZ_DEFAULT_COMMANDS_TRANSFER 0
#define DVZ_DEFAULT_COMMANDS_RENDER   1
#define DVZ_MAX_FRAMES_IN_FLIGHT      2



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/


// Canvas creation flags.
typedef enum
{
    DVZ_CANVAS_FLAGS_NONE = 0x0000,
    DVZ_CANVAS_FLAGS_IMGUI = 0x0001,
    DVZ_CANVAS_FLAGS_FPS = 0x0003, // NOTE: 1 bit for ImGUI, 1 bit for FPS
    DVZ_CANVAS_FLAGS_PICK = 0x0004,
} DvzCanvasFlags;



// Canvas size type
typedef enum
{
    DVZ_CANVAS_SIZE_SCREEN,
    DVZ_CANVAS_SIZE_FRAMEBUFFER,
} DvzCanvasSizeType;



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
