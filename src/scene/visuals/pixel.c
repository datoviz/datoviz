/*************************************************************************************************/
/*  Pixel                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/pixel.h"
#include "request.h"



/*************************************************************************************************/
/*  Internal functions                                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzPixel* dvz_pixel(DvzRequester* rqr, int flags)
{
    ANN(rqr);
    DvzPixel* pixel = (DvzPixel*)calloc(1, sizeof(DvzPixel));
    pixel->rqr = rqr;
    pixel->flags = flags;
    return pixel;
}



void dvz_pixel_position(DvzPixel* pixel, uint32_t first, uint32_t count, vec3* values, int flags)
{
    ANN(pixel);
}



void dvz_pixel_color(DvzPixel* pixel, uint32_t first, uint32_t count, cvec4* values, int flags)
{
    ANN(pixel);
}



void dvz_pixel_draw(DvzPixel* pixel, DvzId canvas, uint32_t first, uint32_t count, int flags)
{
    ANN(pixel);
    // to be called between dvz_record_begin() and dvz_record_end()
    // count is the number of *items*
    // dvz_pixels_update(); // emit the dat update commands
    // dvz_record_draw(); // emit the record commands
}



void dvz_pixel_create(DvzPixel* pixel)
{
    ANN(pixel);
    /*
    // NOTE: check that props of a given vertex binding idx are either all constant or all not
    constant dvz_set_primitive(); dvz_set_polygon();

    // Determine the vertex bindings as a function of the flags.
    // Assume the highest binding_idx of all props +1 is the number of different bindings
    // Array with the stride of each attribute
        uint32_t binding_count;
        DvzSize strides[DVZ_PROP_MAX_ATTRS]; // for each GLSL attribution, the number of bytes per
    item DvzSize offsets[DVZ_PROP_MAX_BINDINGS]; // for each binding, the offset of the last
    visited prop for each prop if (binding_count == 0 || binding >= binding_count) // new binding
            dvz_set_vertex() // declare new binding
        dvz_set_attr() // manually called for each prop, with the known GLSL location, and the
    binding stored in the corresponding DvzProp*
    // Uniform and texture bindings.
    dvz_set_slot()
    // Set up the baker

    */
}



void dvz_pixel_update(DvzPixel* pixel) { ANN(pixel); }



void dvz_pixel_destroy(DvzPixel* pixel)
{
    ANN(pixel);
    FREE(pixel);
}
