/*************************************************************************************************/
/*  Testing visual                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/test_visual.h"
#include "renderer.h"
#include "request.h"
#include "scene/scene_testing_utils.h"
#include "scene/visual.h"
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
/*  Visual tests                                                                                 */
/*************************************************************************************************/

int test_visual_1(TstSuite* suite)
{
    DvzRequester* rqr = dvz_requester();
    dvz_requester_begin(rqr);

    uint32_t n = 10000;

    // Create a visual.
    DvzVisual* visual = dvz_visual(rqr, DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST, 0);

    // Visual shaders.
    dvz_visual_shader(visual, "graphics_basic");

    // Vertex attributes.
    dvz_visual_attr(visual, 0, 0, sizeof(vec3), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
    dvz_visual_attr(visual, 1, sizeof(vec3), sizeof(cvec4), DVZ_FORMAT_R8G8B8A8_UNORM, 0);

    // Slots.
    dvz_visual_slot(visual, 0, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 1, DVZ_SLOT_DAT);

    // MVP.
    DvzMVP mvp = dvz_mvp_default();
    dvz_visual_mvp(visual, &mvp);

    // Viewport.
    DvzViewport viewport = dvz_viewport_default(WIDTH, HEIGHT);
    dvz_visual_viewport(visual, &viewport);

    // Create the visual.
    dvz_visual_alloc(visual, n, n);

    // Vertex data.
    // Position.
    vec3* pos = (vec3*)calloc(n, sizeof(vec3));
    for (uint32_t i = 0; i < n; i++)
    {
        pos[i][0] = .25 * dvz_rand_normal();
        pos[i][1] = .25 * dvz_rand_normal();
    }
    dvz_visual_data(visual, 0, 0, n, pos);

    // Color.
    cvec4* color = (cvec4*)calloc(n, sizeof(cvec4));
    for (uint32_t i = 0; i < n; i++)
    {
        dvz_colormap(DVZ_CMAP_HSV, i % n, color[i]);
        color[i][3] = 128;
    }
    dvz_visual_data(visual, 1, 0, n, color);

    // Important: upload the data to the GPU.
    dvz_visual_update(visual);

    // Create a board.
    DvzRequest req = dvz_create_board(rqr, WIDTH, HEIGHT, DVZ_DEFAULT_CLEAR_COLOR, 0);
    DvzId board_id = req.id;
    req = dvz_set_background(rqr, board_id, (cvec4){32, 64, 128, 255});

    // Record the commands.
    dvz_record_begin(rqr, board_id);
    dvz_record_viewport(rqr, board_id, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
    dvz_visual_instance(visual, board_id, 0, 0, n, 0, 1);
    dvz_record_end(rqr, board_id);

    // Render to a PNG.
    render_requests(rqr, get_gpu(suite), board_id, "visual_1");

    // Cleanup
    dvz_visual_destroy(visual);
    dvz_requester_destroy(rqr);
    FREE(pos);
    FREE(color);
    return 0;
}
