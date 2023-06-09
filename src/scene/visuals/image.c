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

    DvzVisual* image = dvz_visual(rqr, DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST, flags);
    ANN(image);

    // Visual shaders.
    dvz_visual_shader(image, "graphics_basic");

    // Vertex attributes.
    dvz_visual_attr(image, 0, DVZ_FORMAT_R32G32B32_SFLOAT, 0); // pos
    dvz_visual_attr(image, 1, DVZ_FORMAT_R8G8B8A8_UNORM, 0);   // color

    // Uniforms.
    dvz_visual_dat(image, 0, sizeof(DvzMVP));
    dvz_visual_dat(image, 1, sizeof(DvzViewport));

    return image;
}



void dvz_image_alloc(DvzVisual* image, uint32_t item_count)
{
    ANN(image);
    log_debug("allocating the image visual");

    DvzRequester* rqr = image->rqr;
    ANN(rqr);

    // Create the visual.
    dvz_visual_alloc(image, item_count, item_count);
}



void dvz_image_position(DvzVisual* image, uint32_t first, uint32_t count, vec3* values, int flags)
{
    ANN(image);
    // _auto_create(image, first, count);
    dvz_visual_data(image, 0, first, count, (void*)values);
}



void dvz_image_color(DvzVisual* image, uint32_t first, uint32_t count, cvec4* values, int flags)
{
    ANN(image);
    // _auto_create(image, first, count);
    dvz_visual_data(image, 1, first, count, (void*)values);
}
