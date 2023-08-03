/*************************************************************************************************/
/*  Segment                                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/segment.h"
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

DvzVisual* dvz_segment(DvzRequester* rqr, int flags)
{
    ANN(rqr);

    flags |= DVZ_VISUALS_FLAGS_INDEXED;
    DvzVisual* visual = dvz_visual(rqr, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, flags);
    ANN(visual);

    // Visual shaders.
    dvz_visual_shader(visual, "graphics_segment");

    // Vertex stride.
    dvz_visual_stride(visual, 0, sizeof(DvzSegmentVertex));

    // Vertex attributes.
    int attr_flags = DVZ_ATTR_FLAGS_REPEAT_X4;
    dvz_visual_attr(
        visual, 0, FIELD(DvzSegmentVertex, P0), DVZ_FORMAT_R32G32B32_SFLOAT, attr_flags);
    dvz_visual_attr(
        visual, 1, FIELD(DvzSegmentVertex, P1), DVZ_FORMAT_R32G32B32_SFLOAT, attr_flags);
    dvz_visual_attr(
        visual, 2, FIELD(DvzSegmentVertex, shift), DVZ_FORMAT_R32G32B32A32_SFLOAT, attr_flags);
    dvz_visual_attr(
        visual, 3, FIELD(DvzSegmentVertex, color), DVZ_FORMAT_R8G8B8A8_UNORM, attr_flags);
    dvz_visual_attr(
        visual, 4, FIELD(DvzSegmentVertex, linewidth), DVZ_FORMAT_R32_SFLOAT, attr_flags);
    dvz_visual_attr(visual, 5, FIELD(DvzSegmentVertex, cap0), DVZ_FORMAT_R32_SINT, attr_flags);
    dvz_visual_attr(visual, 6, FIELD(DvzSegmentVertex, cap1), DVZ_FORMAT_R32_SINT, attr_flags);
    dvz_visual_attr(visual, 7, FIELD(DvzSegmentVertex, transform), DVZ_FORMAT_R8_UINT, attr_flags);

    // Uniforms.
    dvz_visual_slot(visual, 0, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 1, DVZ_SLOT_DAT);

    // Visual draw callback.
    dvz_visual_callback(visual, _visual_callback);

    return visual;
}



void dvz_segment_alloc(DvzVisual* visual, uint32_t item_count)
{
    ANN(visual);
    log_debug("allocating the segment visual");

    DvzRequester* rqr = visual->rqr;
    ANN(rqr);

    // Allocate the visual.
    dvz_visual_alloc(visual, item_count, 4 * item_count, 6 * item_count);

    // Indices.
    DvzIndex* indices = (DvzIndex*)calloc(6 * item_count, sizeof(DvzIndex));
    for (uint32_t i = 0; i < item_count; i++)
    {
        indices[6 * i + 0] = 4 * i + 0;
        indices[6 * i + 1] = 4 * i + 1;
        indices[6 * i + 2] = 4 * i + 2;
        indices[6 * i + 3] = 4 * i + 0;
        indices[6 * i + 4] = 4 * i + 2;
        indices[6 * i + 5] = 4 * i + 3;
    }
    dvz_visual_index(visual, 0, item_count * 6, indices);
    FREE(indices);
}



void dvz_segment_initial(
    DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 0, first, count, (void*)values);
}



void dvz_segment_terminal(
    DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 1, first, count, (void*)values);
}



void dvz_segment_shift(DvzVisual* visual, uint32_t first, uint32_t count, vec4* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 2, first, count, (void*)values);
}



void dvz_segment_color(DvzVisual* visual, uint32_t first, uint32_t count, cvec4* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 3, first, count, (void*)values);
}



void dvz_segment_linewidth(
    DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 4, first, count, (void*)values);
}



void dvz_segment_initial_cap(
    DvzVisual* visual, uint32_t first, uint32_t count, DvzCapType* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 5, first, count, (void*)values);
}



void dvz_segment_terminal_cap(
    DvzVisual* visual, uint32_t first, uint32_t count, DvzCapType* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 6, first, count, (void*)values);
}
