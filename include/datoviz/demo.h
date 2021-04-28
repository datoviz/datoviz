/*************************************************************************************************/
/*  Demo examples included in the library for testing purposes                                   */
/*************************************************************************************************/

#ifndef DVZ_DEMO_HEADER
#define DVZ_DEMO_HEADER

#include "canvas.h"
#include "common.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

static uint64_t _get_nframes()
{
    const char* s = getenv("DVZ_RUN_NFRAMES");
    if (s == NULL)
        return 0;
    ASSERT(s != NULL);
    return strtoull(s, NULL, 10);
}

#define NFRAMES (_get_nframes())

static void _screenshot(DvzCanvas* canvas)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->app != NULL);
    const char* path = getenv("DVZ_RUN_SCREENSHOT");
    if (path != NULL)
    {
        dvz_app_run(canvas->app, 5);
        dvz_screenshot_file(canvas, path);
    }
}

#define SCREENSHOT _screenshot(canvas);



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
