/*************************************************************************************************/
/*  Demo examples included in the library for testing purposes                                   */
/*************************************************************************************************/

#ifndef DVZ_DEMO_HEADER
#define DVZ_DEMO_HEADER

#include "canvas.h"
#include "common.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Scatter demo.
 *
 * @param n number of points
 * @param pos point positions
 */
DVZ_EXPORT int dvz_demo_scatter(int32_t n, dvec3* pos);

/**
 * Gui demo (Dear ImGui).
 */
DVZ_EXPORT int dvz_demo_gui(void);



#endif
