/*************************************************************************************************/
/*  Canvas                                                                                       */
/*************************************************************************************************/

#include "canvas.h"
#include "canvas_utils.h"
#include "common.h"
#include "vklite.h"
#include "window.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzCanvas dvz_canvas(DvzGpu* gpu, uint32_t width, uint32_t height, int flags)
{
    DvzCanvas canvas = {0};
    canvas.flags = flags;
    canvas.size_init[0] = width;
    canvas.size_init[1] = height;

    dvz_obj_init(&canvas.obj);
    return canvas;
}


void dvz_canvas_create(DvzCanvas* canvas)
{
    ASSERT(canvas != NULL);
    // TODO
}



void dvz_canvas_destroy(DvzCanvas* canvas)
{
    ASSERT(canvas != NULL);
    // TODO
}
