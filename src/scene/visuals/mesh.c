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

    DvzVisual* mesh = dvz_visual(rqr, DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST, flags);
    ANN(mesh);

    // Visual shaders.
    dvz_visual_shader(mesh, "graphics_basic");

    // Vertex attributes.
    dvz_visual_attr(mesh, 0, DVZ_FORMAT_R32G32B32_SFLOAT, 0); // pos
    dvz_visual_attr(mesh, 1, DVZ_FORMAT_R8G8B8A8_UNORM, 0);   // color

    // Uniforms.
    dvz_visual_dat(mesh, 0, sizeof(DvzMVP));
    dvz_visual_dat(mesh, 1, sizeof(DvzViewport));

    return mesh;
}



void dvz_mesh_alloc(DvzVisual* mesh, uint32_t item_count)
{
    ANN(mesh);
    log_debug("allocating the mesh visual");

    DvzRequester* rqr = mesh->rqr;
    ANN(rqr);

    // Create the visual.
    dvz_visual_alloc(mesh, item_count, item_count);
}



void dvz_mesh_position(DvzVisual* mesh, uint32_t first, uint32_t count, vec3* values, int flags)
{
    ANN(mesh);
    // _auto_create(mesh, first, count);
    dvz_visual_data(mesh, 0, first, count, (void*)values);
}



void dvz_mesh_color(DvzVisual* mesh, uint32_t first, uint32_t count, cvec4* values, int flags)
{
    ANN(mesh);
    // _auto_create(mesh, first, count);
    dvz_visual_data(mesh, 1, first, count, (void*)values);
}
