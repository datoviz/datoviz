/*************************************************************************************************/
/*  Pixel                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/pixel.h"
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

DvzVisual* dvz_pixel(DvzRequester* rqr, uint32_t item_count, int flags)
{
    ANN(rqr);

    // DvzVisual* pixel = (DvzVisual*)calloc(1, sizeof(DvzVisual));
    // pixel->rqr = rqr;
    // pixel->flags = flags;

    DvzVisual* pixel = dvz_visual(rqr, DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST, flags);
    ANN(pixel);

    // Visual shaders.
    dvz_visual_shader(pixel, "graphics_basic");

    // Vertex attributes.
    dvz_visual_attr(pixel, 0, DVZ_FORMAT_R32G32B32_SFLOAT, 0); // pos
    dvz_visual_attr(pixel, 1, DVZ_FORMAT_R8G8B8A8_UNORM, 0);   // color

    // Uniforms.
    dvz_visual_dat(pixel, 0, sizeof(DvzMVP));
    dvz_visual_dat(pixel, 1, sizeof(DvzViewport));

    // Create the visual.
    dvz_visual_create(pixel, item_count, item_count);

    // dvz_obj_init(&pixel->obj);
    return pixel;
}



void dvz_pixel_viewport(DvzVisual* pixel, DvzViewport viewport)
{
    ANN(pixel);
    ANN(pixel);

    dvz_visual_mvp(pixel, dvz_mvp_default());
    dvz_visual_viewport(pixel, viewport);
}



void dvz_pixel_position(DvzVisual* pixel, uint32_t first, uint32_t count, vec3* values, int flags)
{
    ANN(pixel);
    // if (!dvz_obj_is_created(&pixel->obj))
    //     dvz_pixel_create(pixel);
    dvz_visual_data(pixel, 0, first, count, (void*)values);
}



void dvz_pixel_color(DvzVisual* pixel, uint32_t first, uint32_t count, cvec4* values, int flags)
{
    ANN(pixel);
    // if (!dvz_obj_is_created(&pixel->obj))
    //     dvz_pixel_create(pixel);
    // dvz_baker_data(pixel->baker, 1, first, count, (void*)values);
    dvz_visual_data(pixel, 1, first, count, (void*)values);
}



void dvz_pixel_create(DvzVisual* pixel)
{
    ANN(pixel);
    log_debug("creating pixel visual");

    // NOTE: needed?

    // NOTE: for now static assignments
    DvzRequester* rqr = pixel->rqr;
    ANN(rqr);

    // dvz_obj_created(&pixel->obj);
}



void dvz_pixel_draw(DvzVisual* pixel, DvzId canvas, uint32_t first, uint32_t count, int flags)
{
    ANN(pixel);

    // Needed?
    // dvz_visual_update(pixel);

    // Emit the record commands.
    dvz_visual_draw(pixel, canvas, first, count, 0, 1);
}



DvzInstance* dvz_pixel_instance(DvzVisual* pixel, DvzView* view, uint32_t first, uint32_t count)
{
    ANN(pixel);
    ANN(view);
    return dvz_view_instance(view, pixel, first, 0, count, 0, 1);
}



void dvz_pixel_update(DvzVisual* pixel)
{
    ANN(pixel);
    dvz_visual_update(pixel);
}



// void dvz_pixel_destroy(DvzVisual* pixel)
// {
//     ANN(pixel);
//     dvz_visual_destroy(pixel);
//     FREE(pixel);
// }
