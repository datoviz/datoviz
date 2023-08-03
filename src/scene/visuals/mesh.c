/*************************************************************************************************/
/*  Mesh                                                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/mesh.h"
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
    // NOTE: here, count is item_count, so index_count/3 (number of faces).
    // We need to multiply by three to retrieve the number of elements to draw using
    // indexing.
    dvz_visual_instance(visual, canvas, first, 0, 3 * count, first_instance, instance_count);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzVisual* dvz_mesh(DvzRequester* rqr, int flags)
{
    ANN(rqr);

    // NOTE: force indexed visual flag.
    // flags |= DVZ_VISUALS_FLAGS_INDEXED;

    DvzVisual* visual = dvz_visual(rqr, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, flags);
    ANN(visual);

    // Visual shaders.
    dvz_visual_shader(visual, "graphics_mesh");

    // Enable depth test.
    dvz_visual_depth(visual, DVZ_DEPTH_TEST_ENABLE);
    dvz_visual_front(visual, DVZ_FRONT_FACE_COUNTER_CLOCKWISE);
    dvz_visual_cull(visual, DVZ_CULL_MODE_NONE);

    // Vertex attributes.
    dvz_visual_attr(visual, 0, FIELD(DvzMeshVertex, pos), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
    dvz_visual_attr(visual, 1, FIELD(DvzMeshVertex, normal), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
    dvz_visual_attr(visual, 2, FIELD(DvzMeshVertex, color), DVZ_FORMAT_R8G8B8A8_UNORM, 0);

    // dvz_visual_attr(visual, 2, DVZ_FORMAT_R32G32_SFLOAT, 0);    // uv
    // dvz_visual_attr(visual, 3, DVZ_FORMAT_R32_SFLOAT, 0); // alpha
    // TODO: fix alignment
    // dvz_visual_attr(visual, 3, DVZ_FORMAT_R8_UNORM, 0);         // alpha

    // Slots.
    dvz_visual_slot(visual, 0, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 1, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 2, DVZ_SLOT_DAT);
    // dvz_visual_slot(visual, 3, DVZ_SLOT_TEX);

    // Params.
    DvzParams* params = dvz_params(visual->rqr, sizeof(DvzMeshParams), false);
    dvz_visual_params(visual, 2, params);

    dvz_params_attr(params, 0, FIELD(DvzMeshParams, light_pos));
    dvz_params_attr(params, 1, FIELD(DvzMeshParams, light_params));

    // Visual draw callback.
    dvz_visual_callback(visual, _visual_callback);

    return visual;
}



void dvz_mesh_alloc(DvzVisual* visual, uint32_t vertex_count, uint32_t index_count)
{
    ANN(visual);
    ASSERT(vertex_count > 0);
    ASSERT(index_count % 3 == 0);
    log_debug("allocating the mesh visual");

    DvzRequester* rqr = visual->rqr;
    ANN(rqr);

    // Create the visual.

    // NOTE: by convention in this visual, 1 item = 1 triangle.
    // This is why item_count is index_count / 3 below.
    dvz_visual_alloc(visual, index_count / 3, vertex_count, index_count);
}



void dvz_mesh_index(DvzVisual* visual, uint32_t first, uint32_t count, DvzIndex* values)
{
    ANN(visual);
    dvz_visual_index(visual, first, count, values);
}



void dvz_mesh_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 0, first, count, (void*)values);
}



void dvz_mesh_normal(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 1, first, count, (void*)values);
}



void dvz_mesh_color(DvzVisual* visual, uint32_t first, uint32_t count, cvec4* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 2, first, count, (void*)values);
}



void dvz_mesh_light_pos(DvzVisual* visual, vec4 pos)
{
    ANN(visual);
    dvz_visual_param(visual, 2, 0, pos);
}



void dvz_mesh_light_params(DvzVisual* visual, vec4 params)
{
    ANN(visual);
    dvz_visual_param(visual, 2, 1, params);
}



DvzVisual* dvz_mesh_shape(DvzRequester* rqr, DvzShape* shape)
{
    ANN(shape);
    ANN(shape->pos);

    uint32_t vertex_count = shape->vertex_count;
    uint32_t index_count = shape->index_count;
    ASSERT(vertex_count > 0);

    // NOTE: set the visual flag to indexed or non-indexed (default) depending on whether the shape
    // has an index buffer or not.
    int flags = index_count > 0 ? DVZ_VISUALS_FLAGS_INDEXED : DVZ_VISUALS_FLAGS_DEFAULT;
    DvzVisual* visual = dvz_mesh(rqr, flags);

    dvz_mesh_alloc(visual, vertex_count, index_count);

    dvz_mesh_position(visual, 0, vertex_count, shape->pos, 0);

    if (shape->normal)
        dvz_mesh_normal(visual, 0, vertex_count, shape->normal, 0);

    if (shape->color)
        dvz_mesh_color(visual, 0, vertex_count, shape->color, 0);

    if (shape->index_count > 0)
        dvz_mesh_index(visual, 0, index_count, shape->index);

    return visual;
}
