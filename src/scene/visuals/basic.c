/*************************************************************************************************/
/*  Basic                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/basic.h"
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

DvzVisual* dvz_basic(DvzRequester* rqr, DvzPrimitiveTopology topology, int flags)
{
    ANN(rqr);

    DvzVisual* basic = dvz_visual(rqr, topology, flags);
    ANN(basic);

    // Visual shaders.
    dvz_visual_shader(basic, "graphics_basic");

    // Vertex attributes.
    dvz_visual_attr(basic, 0, DVZ_FORMAT_R32G32B32_SFLOAT, 0); // pos
    dvz_visual_attr(basic, 1, DVZ_FORMAT_R8G8B8A8_UNORM, 0);   // color

    // Uniforms.
    dvz_visual_dat(basic, 0, sizeof(DvzMVP));
    dvz_visual_dat(basic, 1, sizeof(DvzViewport));

    return basic;
}



void dvz_basic_alloc(DvzVisual* basic, uint32_t item_count)
{
    ANN(basic);
    log_debug("allocating the basic visual");

    DvzRequester* rqr = basic->rqr;
    ANN(rqr);

    // Create the visual.
    dvz_visual_alloc(basic, item_count, item_count);
}



void dvz_basic_position(DvzVisual* basic, uint32_t first, uint32_t count, vec3* values, int flags)
{
    ANN(basic);
    dvz_visual_data(basic, 0, first, count, (void*)values);
}



void dvz_basic_color(DvzVisual* basic, uint32_t first, uint32_t count, cvec4* values, int flags)
{
    ANN(basic);
    dvz_visual_data(basic, 1, first, count, (void*)values);
}
