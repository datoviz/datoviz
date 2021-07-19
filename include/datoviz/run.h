/*************************************************************************************************/
/*  Event loop                                                                                   */
/*************************************************************************************************/

#ifndef DVZ_RUN_HEADER
#define DVZ_RUN_HEADER

#include "canvas.h"

#ifdef __cplusplus
extern "C" {
#endif



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Run state.
typedef enum
{
    // DVZ_RUN_STATE_STOP,
    DVZ_RUN_STATE_PAUSED,
    DVZ_RUN_STATE_RUNNING,
} DvzRunState;



// Run canvas events.
typedef enum
{
    DVZ_RUN_CANVAS_NONE, //

    // FRAME queue
    DVZ_RUN_CANVAS_FRAME, // new frame for a canvas

    // MAIN queue
    DVZ_RUN_CANVAS_NEW,         //
    DVZ_RUN_CANVAS_RECREATE,    // need to recreate the canvas
    DVZ_RUN_CANVAS_RUNNING,     // whether to run frames or not
    DVZ_RUN_CANVAS_VISIBLE,     // to hide or show a canvas
    DVZ_RUN_CANVAS_RESIZE,      // the canvas has been resized, need to enqueue first a REFILL
    DVZ_RUN_CANVAS_CLEAR_COLOR, // to change the clear color, will enqueue first a REFILL
    DVZ_RUN_CANVAS_DPI,         // change the DPI scaling of the canvas
    DVZ_RUN_CANVAS_FPS,         // whether to show or hide FPS
    DVZ_RUN_CANVAS_DELETE,      // need to delete the canvas

    // REFILL queue
    DVZ_RUN_CANVAS_REFILL, // need to refill the canvas, the user should have registered a callback

    // PRESENT queue
    DVZ_RUN_CANVAS_PRESENT, // need to present the frame to the swapchain

} DvzRunCanvasEvent;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzAutorun DvzAutorun;
typedef struct DvzRun DvzRun;

// Event structs.
typedef struct DvzRunCanvasFrame DvzRunCanvasFrame;
typedef struct DvzRunCanvasDefaultEvent DvzRunCanvasDefaultEvent;



/*************************************************************************************************/
/*  Event structs                                                                                */
/*************************************************************************************************/

struct DvzRunCanvasFrame
{
    DvzCanvas* canvas;
    uint64_t frame_idx;
};



struct DvzRunCanvasDefaultEvent
{
    DvzCanvas* canvas;
};



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzAutorun
{
    bool enable;          // whether to enable autorun or not
    uint64_t frame_count; // total number of frames to run
    bool offscreen;       // whether to run the canvas offscreen or not
    char* filepath;       // screenshot or video
};



struct DvzRun
{
    DvzApp* app;
    DvzRunState state;

    DvzDeq deq;

    DvzAutorun autorun;

    uint64_t global_frame_idx;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a run instance to run the event loop and manage the lifecycle of the canvases.
 *
 * @param app the app
 * @returns a Run struct
 */
DVZ_EXPORT DvzRun* dvz_run(DvzApp* app);

/**
 * Run one frame for all active canvases.
 *
 * @param run the run instance
 */
DVZ_EXPORT int dvz_run_frame(DvzRun* run);

/**
 * Run the event loop.
 *
 * @param run the run instance
 * @param frame_count the maximum number of frames, or 0 for an infinite loop
 */
DVZ_EXPORT int dvz_run_loop(DvzRun* run, uint64_t frame_count);

/**
 * Setup the autorun.
 *
 * @param run the run instance
 * @param frame_count the maximum number of frames
 * @param offscreen whether the canvases should run offscreen
 * @param filepath save a screenshot or a video
 */
DVZ_EXPORT void dvz_run_setup(DvzRun* run, uint64_t frame_count, bool offscreen, char* filepath);

/**
 * Setup the autorun using environment variables.
 *
 * @param the run instance
 */
DVZ_EXPORT void dvz_run_setupenv(DvzRun* run);

/**
 * Start the run, using the autorun if it was set, or an infinite loop by default.
 *
 * @param the run instance
 */
DVZ_EXPORT int dvz_run_auto(DvzRun* run);

/**
 * Destroy the run.
 *
 * @param the run instance
 */
DVZ_EXPORT void dvz_run_destroy(DvzRun* run);



#ifdef __cplusplus
}
#endif

#endif