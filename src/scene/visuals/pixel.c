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

DvzPixel* dvz_pixel(DvzRequester* rqr, uint32_t item_count, int flags)
{
    ANN(rqr);

    DvzPixel* pixel = (DvzPixel*)calloc(1, sizeof(DvzPixel));
    pixel->rqr = rqr;
    pixel->flags = flags;

    pixel->visual = dvz_visual(rqr, DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST, 0);
    ANN(pixel->visual);

    // Visual shaders.
    dvz_visual_shader(pixel->visual, "graphics_basic");

    // Vertex attributes.
    dvz_visual_attr(pixel->visual, 0, DVZ_FORMAT_R32G32B32_SFLOAT, 0); // pos
    dvz_visual_attr(pixel->visual, 1, DVZ_FORMAT_R8G8B8A8_UNORM, 0);   // color

    // Uniforms.
    dvz_visual_dat(pixel->visual, 0, sizeof(DvzMVP));
    dvz_visual_dat(pixel->visual, 1, sizeof(DvzViewport));

    // Create the visual.
    dvz_visual_create(pixel->visual, item_count, item_count);

    dvz_obj_init(&pixel->obj);
    return pixel;
}



void dvz_pixel_viewport(DvzPixel* pixel, DvzViewport viewport)
{
    ANN(pixel);
    ANN(pixel->visual);

    dvz_visual_mvp(pixel->visual, dvz_mvp_default());
    dvz_visual_viewport(pixel->visual, viewport);
}



void dvz_pixel_position(DvzPixel* pixel, uint32_t first, uint32_t count, vec3* values, int flags)
{
    ANN(pixel);
    // if (!dvz_obj_is_created(&pixel->obj))
    //     dvz_pixel_create(pixel);
    dvz_visual_data(pixel->visual, 0, first, count, (void*)values);
}



void dvz_pixel_color(DvzPixel* pixel, uint32_t first, uint32_t count, cvec4* values, int flags)
{
    ANN(pixel);
    // if (!dvz_obj_is_created(&pixel->obj))
    //     dvz_pixel_create(pixel);
    // dvz_baker_data(pixel->baker, 1, first, count, (void*)values);
    dvz_visual_data(pixel->visual, 1, first, count, (void*)values);
}



void dvz_pixel_create(DvzPixel* pixel)
{
    ANN(pixel);
    log_debug("creating pixel visual");

    // NOTE: needed?

    // NOTE: for now static assignments
    DvzRequester* rqr = pixel->rqr;
    ANN(rqr);

    dvz_obj_created(&pixel->obj);
}



void dvz_pixel_draw(DvzPixel* pixel, DvzId canvas, uint32_t first, uint32_t count, int flags)
{
    ANN(pixel);

    // Needed?
    // dvz_visual_update(pixel);

    // Emit the record commands.
    dvz_visual_draw(pixel->visual, canvas, first, count);
}



void dvz_pixel_update(DvzPixel* pixel)
{
    ANN(pixel);
    dvz_visual_update(pixel->visual);
}



void dvz_pixel_destroy(DvzPixel* pixel)
{
    ANN(pixel);
    dvz_visual_destroy(pixel->visual);
    FREE(pixel);
}
