/*************************************************************************************************/
/*  Testing mesh                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/test_mesh.h"
#include "renderer.h"
#include "request.h"
#include "scene/dual.h"
#include "scene/scene_testing_utils.h"
#include "scene/viewport.h"
#include "scene/visual.h"
#include "scene/visuals/mesh.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Mesh tests                                                                                   */
/*************************************************************************************************/

int test_mesh_1(TstSuite* suite)
{
    DvzRequester* rqr = dvz_requester();
    dvz_requester_begin(rqr);

    // Upload the data.
    const uint32_t triangle_count = 30;
    const uint32_t vertex_count = triangle_count + 1;
    const uint32_t index_count = 3 * triangle_count;

    DvzVisual* mesh = dvz_mesh(rqr, 0);
    dvz_mesh_alloc(mesh, vertex_count, index_count);


    // Position.
    vec3* pos = (vec3*)calloc(vertex_count, sizeof(vec3));
    // NOTE: start at i=1 because the first vertex is the origin (0,0)
    float a = WIDTH / (float)HEIGHT;
    for (uint32_t i = 1; i < vertex_count; i++)
    {
        pos[i][0] = .5 * cos(M_2PI * (float)i / (vertex_count - 2));
        pos[i][1] = .5 * a * sin(M_2PI * (float)i / (vertex_count - 2));
    }
    dvz_mesh_position(mesh, 0, vertex_count, pos, 0);

    // Normal.
    vec3* normal = (vec3*)calloc(vertex_count, sizeof(vec3));
    for (uint32_t i = 0; i < vertex_count; i++)
    {
        normal[i][2] = 1;
    }
    dvz_mesh_normal(mesh, 0, vertex_count, normal, 0);

    // Color.
    cvec4* color = (cvec4*)calloc(vertex_count, sizeof(cvec4));
    for (uint32_t i = 1; i < vertex_count; i++)
    {
        dvz_colormap_scale(DVZ_CMAP_HSV, i, 0, vertex_count, color[i]);
        color[i][3] = 255;
    }
    dvz_mesh_color(mesh, 0, vertex_count, color, 0);

    // Index.
    DvzIndex* index = (DvzIndex*)calloc(index_count, sizeof(DvzIndex));
    for (uint32_t i = 0; i < vertex_count - 1; i++)
    {
        ASSERT(3 * i + 2 < index_count);
        index[3 * i + 0] = 0;
        index[3 * i + 1] = i + 1;
        index[3 * i + 2] = 1 + (i + 1) % triangle_count;
    }
    dvz_mesh_index(mesh, 0, index_count, index);


    // Important: upload the data to the GPU.
    dvz_visual_update(mesh);


    // Manual setting of common bindings.

    // MVP.
    DvzMVP mvp = dvz_mvp_default();
    dvz_visual_mvp(mesh, &mvp);

    // Viewport.
    DvzViewport viewport = dvz_viewport_default(WIDTH, HEIGHT);
    dvz_visual_viewport(mesh, &viewport);

    // Params.
    DvzMeshParams params = {0};
    params.lights_params_0[0][0] = 0.2;  // ambient coefficient
    params.lights_params_0[0][1] = 0.5;  // diffuse coefficient
    params.lights_params_0[0][2] = 0.3;  // specular coefficient
    params.lights_params_0[0][3] = 32.0; // specular exponent
    params.lights_pos_0[0][0] = -1;      // light position
    params.lights_pos_0[0][1] = 1;       //
    params.lights_pos_0[0][2] = +10;     //
    params.tex_coefs[0] = 1;             // texture blending coefficients

    DvzDual params_dual = dvz_dual_dat(rqr, sizeof(params), 0);
    dvz_dual_data(&params_dual, 0, 1, &params);
    dvz_dual_update(&params_dual);
    dvz_bind_dat(rqr, mesh->graphics_id, 2, params_dual.dat, 0);


    // Create a board.
    DvzRequest req = dvz_create_board(rqr, WIDTH, HEIGHT, DVZ_DEFAULT_CLEAR_COLOR, 0);
    DvzId board_id = req.id;
    req = dvz_set_background(rqr, board_id, (cvec4){32, 64, 128, 255});

    // Record commands.
    dvz_record_begin(rqr, board_id);
    dvz_record_viewport(rqr, board_id, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
    dvz_visual_instance(mesh, board_id, 0, 0, index_count, 0, 1);
    dvz_record_end(rqr, board_id);

    // Render to a PNG.
    render_requests(rqr, get_gpu(suite), board_id, "visual_mesh");

    // Cleanup
    dvz_visual_destroy(mesh);
    dvz_requester_destroy(rqr);
    FREE(pos);
    FREE(index);
    return 0;
}
