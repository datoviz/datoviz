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

    DvzVisual* visual = dvz_visual(rqr, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, flags);
    ANN(visual);

    // Visual shaders.
    dvz_visual_shader(visual, "graphics_image");

    // Vertex attributes.
    dvz_visual_attr(visual, 0, FIELD(DvzImageVertex, pos), DVZ_FORMAT_R32G32_SFLOAT, 0);
    dvz_visual_attr(visual, 1, FIELD(DvzImageVertex, uv), DVZ_FORMAT_R32G32_SFLOAT, 0);

    // Vertex stride.
    dvz_visual_stride(visual, 0, sizeof(DvzImageVertex));

    // Slots.
    dvz_visual_slot(visual, 0, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 1, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 2, DVZ_SLOT_TEX);

    return visual;
}



void dvz_image_alloc(DvzVisual* visual, uint32_t item_count)
{
    ANN(visual);
    log_debug("allocating the image visual");

    DvzRequester* rqr = visual->rqr;
    ANN(rqr);

    // Create the visual.
    dvz_visual_alloc(visual, item_count, 6 * item_count, 0);
}



void dvz_image_position(DvzVisual* visual, uint32_t first, uint32_t count, vec4* ul_lr, int flags)
{
    ANN(visual);
    dvz_visual_quads(visual, 0, first, count, ul_lr);
}



void dvz_image_texcoords(DvzVisual* visual, uint32_t first, uint32_t count, vec4* ul_lr, int flags)
{
    ANN(visual);
    dvz_visual_quads(visual, 1, first, count, ul_lr);
}



DvzId dvz_image_texture(
    DvzVisual* visual, uvec3 shape, DvzFormat format, DvzFilter filter, DvzSize size, void* data)
{
    ANN(visual);

    DvzRequester* rqr = visual->rqr;
    ANN(rqr);

    DvzId tex = dvz_create_tex(rqr, DVZ_TEX_2D, format, shape, 0).id;
    DvzId sampler = dvz_create_sampler(rqr, filter, DVZ_SAMPLER_ADDRESS_MODE_REPEAT).id;

    // Bind texture to the visual.
    dvz_visual_tex(visual, 2, tex, sampler, DVZ_ZERO_OFFSET);

    // Upload the texture data.
    if (size > 0 && data != NULL)
        dvz_upload_tex(rqr, tex, DVZ_ZERO_OFFSET, shape, size, data);

    return tex;
}
