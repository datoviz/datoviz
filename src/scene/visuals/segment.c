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



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzVisual* dvz_segment(DvzRequester* rqr, int flags)
{
    ANN(rqr);

    DvzVisual* segment = dvz_visual(rqr, DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST, flags);
    ANN(segment);

    // Visual shaders.
    dvz_visual_shader(segment, "graphics_basic");

    // Vertex attributes.
    dvz_visual_attr(segment, 0, FIELD(DvzSegmentVertex, pos), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
    dvz_visual_attr(segment, 1, FIELD(DvzSegmentVertex, color), DVZ_FORMAT_R8G8B8A8_UNORM, 0);

    // Uniforms.
    dvz_visual_dat(segment, 0, sizeof(DvzMVP));
    dvz_visual_dat(segment, 1, sizeof(DvzViewport));

    return segment;
}



void dvz_segment_alloc(DvzVisual* segment, uint32_t item_count)
{
    ANN(segment);
    log_debug("allocating the segment visual");

    DvzRequester* rqr = segment->rqr;
    ANN(rqr);

    // Create the visual.
    dvz_visual_alloc(segment, item_count, item_count);
}



void dvz_segment_position(
    DvzVisual* segment, uint32_t first, uint32_t count, vec3* values, int flags)
{
    ANN(segment);
    dvz_visual_data(segment, 0, first, count, (void*)values);
}



void dvz_segment_color(
    DvzVisual* segment, uint32_t first, uint32_t count, cvec4* values, int flags)
{
    ANN(segment);
    dvz_visual_data(segment, 1, first, count, (void*)values);
}
