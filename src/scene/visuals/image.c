/*************************************************************************************************/
/*  Image                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/image.h"
#include "fileio.h"
#include "request.h"
#include "scene/graphics.h"
#include "scene/viewset.h"
#include "scene/visual.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Internal functions                                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzVisual* dvz_image(DvzRequester* rqr, int flags)
{
    ANN(rqr);

    DvzVisual* visual = dvz_visual(rqr, DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST, flags);
    ANN(visual);

    // Visual shaders.
    dvz_visual_shader(visual, "graphics_basic");

    // Vertex attributes.
    dvz_visual_attr(visual, 0, FIELD(DvzImageVertex, pos), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
    dvz_visual_attr(visual, 1, FIELD(DvzImageVertex, color), DVZ_FORMAT_R8G8B8A8_UNORM, 0);

    // Slots.
    dvz_visual_slot(visual, 0, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 1, DVZ_SLOT_DAT);

    return visual;
}



void dvz_image_alloc(DvzVisual* visual, uint32_t item_count)
{
    ANN(visual);
    log_debug("allocating the image visual");

    DvzRequester* rqr = visual->rqr;
    ANN(rqr);

    // Create the visual.
    dvz_visual_alloc(visual, item_count, item_count, 0);
}



void dvz_image_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 0, first, count, (void*)values);
}



void dvz_image_color(DvzVisual* visual, uint32_t first, uint32_t count, cvec4* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 1, first, count, (void*)values);
}
