/*************************************************************************************************/
/*  Marker                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/marker.h"
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

DvzVisual* dvz_marker(DvzBatch* batch, int flags)
{
    ANN(batch);

    DvzVisual* visual = dvz_visual(batch, DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST, flags);
    ANN(visual);

    // Visual shaders.
    dvz_visual_shader(visual, "graphics_marker");

    // Vertex attributes.
    dvz_visual_attr(visual, 0, FIELD(DvzMarkerVertex, pos), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
    dvz_visual_attr(visual, 1, FIELD(DvzMarkerVertex, size), DVZ_FORMAT_R32_SFLOAT, 0);
    dvz_visual_attr(visual, 2, FIELD(DvzMarkerVertex, angle), DVZ_FORMAT_R32_SFLOAT, 0);
    dvz_visual_attr(visual, 3, FIELD(DvzMarkerVertex, color), DVZ_FORMAT_R8G8B8A8_UNORM, 0);

    // Vertex stride.
    dvz_visual_stride(visual, 0, sizeof(DvzMarkerVertex));

    // Uniforms.
    dvz_visual_slot(visual, 0, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 1, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 2, DVZ_SLOT_DAT);

    // Params.
    DvzParams* params = dvz_visual_params(visual, 2, sizeof(DvzMarkerParams));
    dvz_params_attr(params, 0, FIELD(DvzMarkerParams, edge_color));
    dvz_params_attr(params, 1, FIELD(DvzMarkerParams, edge_width));

    // Specialization constant #0: mode.
    dvz_visual_specialization(
        visual, DVZ_SHADER_FRAGMENT, 0, sizeof(int32_t), (int32_t[]){DVZ_MARKER_MODE_CODE});

    // Specialization constant #1: aspect.
    dvz_visual_specialization(
        visual, DVZ_SHADER_FRAGMENT, 1, sizeof(int32_t), (int32_t[]){DVZ_MARKER_ASPECT_OUTLINE});

    // Specialization constant #2: shape.
    dvz_visual_specialization(
        visual, DVZ_SHADER_FRAGMENT, 2, sizeof(int32_t), (int32_t[]){DVZ_MARKER_SHAPE_DISC});

    return visual;
}



void dvz_marker_shape(DvzVisual* visual, DvzMarkerShape shape)
{
    ANN(visual);
    dvz_visual_specialization(visual, DVZ_SHADER_FRAGMENT, 2, sizeof(int32_t), (int32_t[]){shape});
}



void dvz_marker_alloc(DvzVisual* visual, uint32_t item_count)
{
    ANN(visual);
    log_debug("allocating the marker visual");

    DvzBatch* batch = visual->batch;
    ANN(batch);

    // Create the visual.
    dvz_visual_alloc(visual, item_count, item_count, 0);
}



void dvz_marker_position(
    DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 0, first, count, (void*)values);
}



void dvz_marker_size(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 1, first, count, (void*)values);
}



void dvz_marker_angle(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 2, first, count, (void*)values);
}



void dvz_marker_color(DvzVisual* visual, uint32_t first, uint32_t count, cvec4* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 3, first, count, (void*)values);
}



void dvz_marker_edge_color(DvzVisual* visual, cvec4 value)
{
    // NOTE: convert from cvec4 into vec4 as GLSL uniforms do not support cvec4 (?)
    float r = value[0] / 255.0;
    float g = value[1] / 255.0;
    float b = value[2] / 255.0;
    float a = value[3] / 255.0;
    dvz_visual_param(visual, 2, 0, (vec4){r, g, b, a});
}



void dvz_marker_edge_width(DvzVisual* visual, float value)
{
    dvz_visual_param(visual, 2, 1, &value);
}
