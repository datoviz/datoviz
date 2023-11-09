/*************************************************************************************************/
/*  Image                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/image.h"
#include "../src/resources_utils.h"
#include "_map.h"
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

static void _visual_callback(
    DvzVisual* visual, DvzId canvas, //
    uint32_t first, uint32_t count,  //
    uint32_t first_instance, uint32_t instance_count)
{
    ANN(visual);
    ASSERT(count > 0);
    dvz_visual_instance(visual, canvas, 6 * first, 0, 6 * count, first_instance, instance_count);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzVisual* dvz_image(DvzBatch* batch, int flags)
{
    ANN(batch);

    DvzVisual* visual = dvz_visual(batch, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, flags);
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

    // Visual draw callback.
    dvz_visual_callback(visual, _visual_callback);

    return visual;
}



void dvz_image_alloc(DvzVisual* visual, uint32_t item_count)
{
    ANN(visual);
    log_debug("allocating the image visual");

    DvzBatch* batch = visual->batch;
    ANN(batch);

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



void dvz_image_texture(
    DvzVisual* visual, DvzId tex, DvzFilter filter, DvzSamplerAddressMode address_mode)
{
    ANN(visual);

    DvzBatch* batch = visual->batch;
    ANN(batch);

    DvzId sampler = dvz_create_sampler(batch, filter, address_mode).id;

    // Bind texture to the visual.
    dvz_visual_tex(visual, 2, tex, sampler, DVZ_ZERO_OFFSET);
}



DvzId dvz_tex_image(DvzBatch* batch, DvzFormat format, uint32_t width, uint32_t height, void* data)
{
    ANN(batch);
    ANN(data);
    ASSERT(width > 0);
    ASSERT(height > 0);

    uvec3 shape = {width, height, 1};
    DvzSize size = width * height * _format_size(format);
    DvzId tex = dvz_create_tex(batch, DVZ_TEX_2D, format, shape, 0).id;
    dvz_upload_tex(batch, tex, DVZ_ZERO_OFFSET, shape, size, data);

    return tex;
}
