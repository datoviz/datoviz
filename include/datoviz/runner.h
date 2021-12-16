/*************************************************************************************************/
/*  Runner                                                                                       */
/*************************************************************************************************/

#ifndef DVZ_HEADER_RUNNER
#define DVZ_HEADER_RUNNER



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "canvas.h"
#include "request.h"
#include "resources.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_RUNNER_DEQ_FRAME   0
#define DVZ_RUNNER_DEQ_MAIN    1
#define DVZ_RUNNER_DEQ_REFILL  2
#define DVZ_RUNNER_DEQ_PRESENT 3



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// // Run state.
// typedef enum
// {
//     DVZ_RUNNER_STATE_PAUSED,
//     DVZ_RUNNER_STATE_RUNNING,
// } DvzRunnerState;



// Run canvas events.
typedef enum
{
    DVZ_RUNNER_CANVAS_NONE, //

    // FRAME queue
    DVZ_RUNNER_CANVAS_FRAME, // new frame for a canvas

    // MAIN queue
    DVZ_RUNNER_CANVAS_NEW,         //
    DVZ_RUNNER_CANVAS_RECREATE,    // need to recreate the canvas
    DVZ_RUNNER_CANVAS_RUNNING,     // whether to runner frames or not
    DVZ_RUNNER_CANVAS_VISIBLE,     // to hide or show a canvas
    DVZ_RUNNER_CANVAS_RESIZE,      // the canvas has been resized, need to enqueue_first a REFILL
    DVZ_RUNNER_CANVAS_CLEAR_COLOR, // to change the clear color, will enqueue_first a REFILL
    DVZ_RUNNER_CANVAS_DPI,         // change the DPI scaling of the canvas
    DVZ_RUNNER_CANVAS_FPS,         // whether to show or hide FPS
    DVZ_RUNNER_CANVAS_UPFILL,      // a Dat upload immediately followed by a REFILL
    DVZ_RUNNER_CANVAS_DELETE,      // need to delete the canvas

    // REFILL queue
    DVZ_RUNNER_CANVAS_TO_REFILL,   // trigger a REFILL for the next few frames
    DVZ_RUNNER_CANVAS_REFILL_WRAP, // internal event used to implement TO_REFILL/REFILL block
    DVZ_RUNNER_CANVAS_REFILL,      // MAJOR EVENT: user callback that does the cmd buf refill

    // PRESENT queue
    DVZ_RUNNER_CANVAS_PRESENT, // need to present the frame to the swapchain

    DVZ_RUNNER_REQUEST, // rendering request

} DvzCanvasEventType;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzAutorun DvzAutorun;
typedef struct DvzRunner DvzRunner;

// Event structs.
typedef struct DvzCanvasEventFrame DvzCanvasEventFrame;
typedef struct DvzCanvasEventRefill DvzCanvasEventRefill;
// typedef struct DvzCanvasEventNew DvzCanvasEventNew;
// typedef struct DvzCanvasEventClearColor DvzCanvasEventClearColor;
typedef struct DvzCanvasEventUpfill DvzCanvasEventUpfill;
typedef struct DvzCanvasEvent DvzCanvasEvent;

// Forward declarations.
typedef struct DvzRenderer DvzRenderer;



/*************************************************************************************************/
/*  Event structs                                                                                */
/*************************************************************************************************/

struct DvzCanvasEvent
{
    DvzCanvas* canvas;
};



struct DvzCanvasEventFrame
{
    DvzCanvas* canvas;
    uint64_t frame_idx;
};



struct DvzCanvasEventRefill
{
    DvzCanvas* canvas;
    DvzCommands* cmds;
    uint32_t cmd_idx;
};



// struct DvzCanvasEventNew
// {
//     DvzGpu* gpu;
//     uint32_t width, height;
//     int flags;
// };



// struct DvzCanvasEventClearColor
// {
//     DvzCanvas* canvas;
//     float r, g, b;
// };



struct DvzCanvasEventUpfill
{
    DvzCanvas* canvas;
    DvzDat* dat;
    VkDeviceSize offset;
    VkDeviceSize size;
    void* data;
};



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzAutorun
{
    bool enable;          // whether to enable autorun or not
    uint64_t frame_count; // total number of frames to runner
    bool offscreen;       // whether to runner the canvas offscreen or not
    char* filepath;       // screenshot or video
};



struct DvzRunner
{
    // DvzHost* host;
    // DvzRunnerState state; // to remove??

    DvzDeq deq;
    DvzRenderer* renderer;

    uint64_t global_frame_idx;

    // bool destroying;   // true as soon as the canvas is being destroyed
    // DvzAutorun autorun;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a runner instance to run the event loop and manage the lifecycle of the canvases.
 *
 * @param renderer the renderer
 * @returns a DvzRunner struct
 */
DVZ_EXPORT DvzRunner* dvz_runner(DvzRenderer* renderer);



/**
 * Run one frame on all active canvases.
 *
 * @param runner the runner instance
 */
DVZ_EXPORT int dvz_runner_frame(DvzRunner* runner);



/**
 * Run the event loop.
 *
 * @param runner the runner instance
 * @param frame_count the maximum number of frames, or 0 for an infinite loop
 */
DVZ_EXPORT int dvz_runner_loop(DvzRunner* runner, uint64_t frame_count);



/**
 * Submit a request to the runner.
 *
 * The request will be enqueued in the runner's queue, and then processed by the underlying
 * renderer.
 *
 * @param runner the runner instance
 * @param request the request
 */
DVZ_EXPORT void dvz_runner_request(DvzRunner* runner, DvzRequest request);


/**
 * Setup the autorun.
 *
 * @param runner the runner instance
 * @param frame_count the maximum number of frames
 * @param offscreen whether the canvases should runner offscreen
 * @param filepath save a screenshot or a video
 */
// DVZ_EXPORT void
// dvz_runner_setup(DvzRunner* runner, uint64_t frame_count, bool offscreen, char* filepath);



/**
 * Setup the autorun using environment variables.
 *
 * @param the runner instance
 */
// DVZ_EXPORT void dvz_runner_setupenv(DvzRunner* runner);



/**
 * Start the runner, using the autorun if it was set, or an infinite loop by default.
 *
 * @param the runner instance
 */
// DVZ_EXPORT int dvz_runner_auto(DvzRunner* runner);



/**
 * Upload data to a Dat resource, immediately followed by a command buffer refill.
 *
 * Useful when modifying a vertex buffer with a different number of vertices which requires a
 * refill.
 *
 * !!! note
 *     May be called from a background thread.
 *
 * @param canvas the canvas
 * @param dat the dat
 * @param offset the offset
 * @param size the size
 * @param data the data to upload
 */
// DVZ_EXPORT void dvz_dat_upfill(
//     DvzCanvas* canvas, DvzDat* dat, //
//     VkDeviceSize offset, VkDeviceSize size, void* data);



/**
 * Destroy a runner.
 *
 * @param runner the runner instance
 */
DVZ_EXPORT void dvz_runner_destroy(DvzRunner* runner);



EXTERN_C_OFF

#endif
