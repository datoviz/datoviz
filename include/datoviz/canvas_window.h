/*************************************************************************************************/
/*  Canvas window                                                                                */
/*************************************************************************************************/

#ifndef DVZ_HEADER_CANVAS_WINDOW
#define DVZ_HEADER_CANVAS_WINDOW



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "canvas.h"
#include "common.h"
#include "vklite.h"
#include "window.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Canvas window functions                                                                      */
/*************************************************************************************************/

/**
 * Run a simple event loop for a single canvas.
 *
 * The window must be created beforehand.
 *
 * @param canvas a canvas
 * @param window a window
 * @param n_frames maximum number of frames
 */
DVZ_EXPORT void dvz_canvas_loop(DvzCanvas* canvas, DvzWindow* window, uint64_t n_frames);



EXTERN_C_OFF

#endif
