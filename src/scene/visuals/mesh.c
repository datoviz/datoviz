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

DvzVisual* dvz_mesh(DvzRequester* rqr, int flags)
{
    ANN(rqr);

    // NOTE: force indexed visual flag.
    flags |= DVZ_VISUALS_FLAGS_INDEXED;

    DvzVisual* mesh = dvz_visual(rqr, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, flags);
    ANN(mesh);

    // Visual shaders.
    dvz_visual_shader(mesh, "graphics_mesh");

    // Enable depth test.
    dvz_visual_depth(mesh, DVZ_DEPTH_TEST_ENABLE);

    // Vertex attributes.
    dvz_visual_attr(mesh, 0, FIELD(DvzMeshVertex, pos), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
    dvz_visual_attr(mesh, 1, FIELD(DvzMeshVertex, normal), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
    dvz_visual_attr(mesh, 2, FIELD(DvzMeshVertex, color), DVZ_FORMAT_R8G8B8A8_UNORM, 0);

    // dvz_visual_attr(mesh, 2, DVZ_FORMAT_R32G32_SFLOAT, 0);    // uv
    // dvz_visual_attr(mesh, 3, DVZ_FORMAT_R32_SFLOAT, 0); // alpha
    // TODO: fix alignment
    // dvz_visual_attr(mesh, 3, DVZ_FORMAT_R8_UNORM, 0);         // alpha

    // Uniforms.
    dvz_visual_dat(mesh, 0, sizeof(DvzMVP));
    dvz_visual_dat(mesh, 1, sizeof(DvzViewport));
    dvz_visual_dat(mesh, 2, sizeof(DvzMeshParams));
    // dvz_visual_tex(mesh, 3, DVZ_TEX_2D, 0);


    return mesh;
}



void dvz_mesh_alloc(DvzVisual* mesh, uint32_t vertex_count, uint32_t index_count)
{
    ANN(mesh);
    ASSERT(vertex_count > 0);
    ASSERT(index_count % 3 == 0);
    log_debug("allocating the mesh visual");

    DvzRequester* rqr = mesh->rqr;
    ANN(rqr);

    // Create the visual.
    // NOTE: with indexed visuals, item_count MUST correspond to the number of faces (triangles),
    // so the size of the index buffer divided by 3.
    // This is a convention in dvz_visual_alloc(visual, item_count, vertex_count) that
    // when using indexing, item_count is the number of triangles.
    dvz_visual_alloc(mesh, index_count / 3, vertex_count);
}



void dvz_mesh_index(DvzVisual* mesh, uint32_t first, uint32_t count, DvzIndex* values)
{
    ANN(mesh);
    dvz_visual_index(mesh, first, count, values);
}



void dvz_mesh_position(DvzVisual* mesh, uint32_t first, uint32_t count, vec3* values, int flags)
{
    ANN(mesh);
    dvz_visual_data(mesh, 0, first, count, (void*)values);
}



void dvz_mesh_normal(DvzVisual* mesh, uint32_t first, uint32_t count, vec3* values, int flags)
{
    ANN(mesh);
    dvz_visual_data(mesh, 1, first, count, (void*)values);
}



void dvz_mesh_color(DvzVisual* mesh, uint32_t first, uint32_t count, cvec4* values, int flags)
{
    ANN(mesh);
    dvz_visual_data(mesh, 2, first, count, (void*)values);
}
