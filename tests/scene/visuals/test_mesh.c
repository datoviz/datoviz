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
#include "scene/shape.h"
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

    // Disc shape parameters.
    const uint32_t count = 30;
    cvec4 color = {255, 0, 0, 255};

    // Disc mesh.
    DvzShape disc = dvz_shape_disc(count, color);
    DvzVisual* mesh = dvz_mesh_shape(rqr, &disc);

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
    params.light_params[0] = 0.2;  // ambient coefficient
    params.light_params[1] = 0.5;  // diffuse coefficient
    params.light_params[2] = 0.3;  // specular coefficient
    params.light_params[3] = 32.0; // specular exponent
    params.light_pos[0] = -1;      // light position
    params.light_pos[1] = 1;       //
    params.light_pos[2] = +10;     //
    // params.tex_coefs[0] = 1;             // texture blending coefficients

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
    dvz_visual_instance(mesh, board_id, 0, 0, disc.index_count, 0, 1);
    dvz_record_end(rqr, board_id);

    // Render to a PNG.
    render_requests(rqr, get_gpu(suite), board_id, "visual_mesh");

    // Cleanup
    dvz_shape_destroy(&disc);
    dvz_visual_destroy(mesh);
    dvz_requester_destroy(rqr);
    return 0;
}
