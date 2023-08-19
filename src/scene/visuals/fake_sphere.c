/*************************************************************************************************/
/*  FakeSphere                                                                                   */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/fake_sphere.h"
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

DvzVisual* dvz_fake_sphere(DvzRequester* rqr, int flags)
{
    ANN(rqr);

    DvzVisual* visual = dvz_visual(rqr, DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST, flags);
    ANN(visual);

    // Enable depth test.
    dvz_visual_depth(visual, DVZ_DEPTH_TEST_ENABLE);
    dvz_visual_front(visual, DVZ_FRONT_FACE_COUNTER_CLOCKWISE);
    dvz_visual_cull(visual, DVZ_CULL_MODE_NONE);

    // Visual shaders.
    dvz_visual_shader(visual, "graphics_fake_sphere");

    // Vertex attributes.
    dvz_visual_attr(visual, 0, FIELD(DvzFakeSphereVertex, pos), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
    dvz_visual_attr(visual, 1, FIELD(DvzFakeSphereVertex, color), DVZ_FORMAT_R8G8B8A8_UNORM, 0);
    dvz_visual_attr(visual, 2, FIELD(DvzFakeSphereVertex, size), DVZ_FORMAT_R32_SFLOAT, 0);

    // Vertex stride.
    dvz_visual_stride(visual, 0, sizeof(DvzFakeSphereVertex));

    // Slots.
    dvz_visual_slot(visual, 0, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 1, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 2, DVZ_SLOT_DAT);

    // Params.
    DvzParams* params = dvz_visual_params(visual, 2, sizeof(DvzFakeSphereParams));

    dvz_params_attr(params, 0, FIELD(DvzFakeSphereParams, light_pos));

    return visual;
}



void dvz_fake_sphere_alloc(DvzVisual* visual, uint32_t item_count)
{
    ANN(visual);
    log_debug("allocating the fake sphere visual");

    DvzRequester* rqr = visual->rqr;
    ANN(rqr);

    // Create the visual.
    dvz_visual_alloc(visual, item_count, item_count, 0);
}



void dvz_fake_sphere_position(
    DvzVisual* visual, uint32_t first, uint32_t count, vec3* data, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 0, first, count, data);
}



void dvz_fake_sphere_color(
    DvzVisual* visual, uint32_t first, uint32_t count, cvec4* data, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 1, first, count, data);
}



void dvz_fake_sphere_size(
    DvzVisual* visual, uint32_t first, uint32_t count, float* data, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 2, first, count, data);
}



void dvz_fake_sphere_light_pos(DvzVisual* visual, vec3 pos)
{
    ANN(visual);
    dvz_visual_param(visual, 2, 0, pos);
}
