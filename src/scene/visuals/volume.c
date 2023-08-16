/*************************************************************************************************/
/*  Volume */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/volume.h"
#include "fileio.h"
#include "request.h"
#include "scene/graphics.h"
#include "scene/shape.h"
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
    dvz_visual_instance(visual, canvas, first, 0, 36 * count, first_instance, instance_count);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzVisual* dvz_volume(DvzRequester* rqr, int flags)
{
    ANN(rqr);

    // NOTE: force indexed visual flag.
    // flags |= DVZ_VISUALS_FLAGS_INDEXED;

    DvzVisual* visual = dvz_visual(rqr, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, flags);
    ANN(visual);

    // Visual shaders.
    dvz_visual_shader(visual, "graphics_volume");

    // Enable depth test.
    dvz_visual_depth(visual, DVZ_DEPTH_TEST_DISABLE);
    // dvz_visual_front(visual, DVZ_FRONT_FACE_COUNTER_CLOCKWISE);
    // dvz_visual_cull(visual, DVZ_CULL_MODE_NONE);

    // Vertex attributes.
    dvz_visual_attr(visual, 0, FIELD(DvzVolumeVertex, pos), DVZ_FORMAT_R32G32B32_SFLOAT, 0);

    // Vertex stride.
    dvz_visual_stride(visual, 0, sizeof(DvzVolumeVertex));

    // dvz_visual_attr(visual, 2, DVZ_FORMAT_R32G32_SFLOAT, 0);    // uv
    // dvz_visual_attr(visual, 3, DVZ_FORMAT_R32_SFLOAT, 0); // alpha
    // TODO: fix alignment
    // dvz_visual_attr(visual, 3, DVZ_FORMAT_R8_UNORM, 0);         // alpha

    // Slots.
    dvz_visual_slot(visual, 0, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 1, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 2, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 3, DVZ_SLOT_TEX);

    // Params.
    DvzParams* params = dvz_params(visual->rqr, sizeof(DvzVolumeParams), false);
    dvz_visual_params(visual, 2, params);

    dvz_params_attr(params, 0, FIELD(DvzVolumeParams, box_size));
    dvz_params_attr(params, 1, FIELD(DvzVolumeParams, uvw0));
    dvz_params_attr(params, 2, FIELD(DvzVolumeParams, uvw1));

    // float u = .5;
    // float x0 = -u, x1 = +u, y0 = -u, y1 = +u, z0 = -u, z1 = +u;

    dvz_visual_param(visual, 2, 0, (vec4){1, 1, 1, 0});
    dvz_visual_param(visual, 2, 1, (vec4){0, 0, 0, 0});
    dvz_visual_param(visual, 2, 2, (vec4){1, 1, 1, 0});

    // Visual draw callback.
    dvz_visual_callback(visual, _visual_callback);

    return visual;
}



void dvz_volume_alloc(DvzVisual* visual, uint32_t item_count)
{
    ANN(visual);
    log_debug("allocating the volume visual");

    DvzRequester* rqr = visual->rqr;
    ANN(rqr);

    const uint32_t K = 36;

    // Allocate the visual.
    dvz_visual_alloc(visual, item_count, K * item_count, 0);

    // TODO
    float u = .5;
    float x0 = -u, x1 = +u, y0 = -u, y1 = +u, z0 = -u, z1 = +u;

    // Vertex positions.
    vec3* pos = (vec3*)calloc(K * item_count, sizeof(vec3));
    for (uint32_t i = 0; i < item_count; i++)
    {
        memcpy(
            &pos[K * i],
            (vec3[]){
                {x0, y0, z1}, // front
                {x1, y0, z1}, //
                {x1, y1, z1}, //
                {x1, y1, z1}, //
                {x0, y1, z1}, //
                {x0, y0, z1}, //
                              //
                {x1, y0, z1}, // right
                {x1, y0, z0}, //
                {x1, y1, z0}, //
                {x1, y1, z0}, //
                {x1, y1, z1}, //
                {x1, y0, z1}, //
                              //
                {x0, y1, z0}, // back
                {x1, y1, z0}, //
                {x1, y0, z0}, //
                {x1, y0, z0}, //
                {x0, y0, z0}, //
                {x0, y1, z0}, //
                              //
                {x0, y0, z0}, // left
                {x0, y0, z1}, //
                {x0, y1, z1}, //
                {x0, y1, z1}, //
                {x0, y1, z0}, //
                {x0, y0, z0}, //
                              //
                {x0, y0, z0}, // bottom
                {x1, y0, z0}, //
                {x1, y0, z1}, //
                {x1, y0, z1}, //
                {x0, y0, z1}, //
                {x0, y0, z0}, //
                              //
                {x0, y1, z1}, // top
                {x1, y1, z1}, //
                {x1, y1, z0}, //
                {x1, y1, z0}, //
                {x0, y1, z0}, //
                {x0, y1, z1}, //
            },
            K * sizeof(vec3));
    }
    dvz_visual_data(visual, 0, 0, item_count * K, pos);
    FREE(pos);
}
