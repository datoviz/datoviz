/*************************************************************************************************/
/*  Testing segment                                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/test_segment.h"
#include "renderer.h"
#include "request.h"
#include "scene/scene_testing_utils.h"
#include "scene/viewport.h"
#include "scene/visual.h"
#include "scene/visuals/segment.h"
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
/*  Segment tests                                                                                */
/*************************************************************************************************/

int test_segment_1(TstSuite* suite)
{
    DvzRequester* rqr = dvz_requester();
    dvz_requester_begin(rqr);

    // Upload the data.
    const uint32_t n = 1;

    DvzVisual* visual = dvz_segment(rqr, 0);
    dvz_segment_alloc(visual, n);
    dvz_segment_initial(visual, 0, n, (vec3[]){{-1, 0, 0}}, 0);
    dvz_segment_terminal(visual, 0, n, (vec3[]){{+1, 0, 0}}, 0);
    dvz_segment_color(visual, 0, n, (cvec4[]){{255, 0, 0, 255}}, 0);
    dvz_segment_linewidth(visual, 0, n, (float[]){10.0f}, 0);

    // // Position.
    // vec3* pos = (vec3*)calloc(n, sizeof(vec3));
    // for (uint32_t i = 0; i < n; i++)
    // {
    //     pos[i][0] = .25 * dvz_rand_normal();
    //     pos[i][1] = .25 * dvz_rand_normal();
    // }
    // dvz_segment_position(visual, 0, n, pos, 0);

    // // Color.
    // cvec4* color = (cvec4*)calloc(n, sizeof(cvec4));
    // for (uint32_t i = 0; i < n; i++)
    // {
    //     dvz_colormap(DVZ_CMAP_HSV, i % n, color[i]);
    //     color[i][3] = 128;
    // }
    // dvz_segment_color(visual, 0, n, color, 0);

    // Important: upload the data to the GPU.
    dvz_visual_update(visual);


    // Manual setting of common bindings.

    // MVP.
    DvzMVP mvp = dvz_mvp_default();
    dvz_visual_mvp(visual, &mvp);

    // Viewport.
    DvzViewport viewport = dvz_viewport_default(WIDTH, HEIGHT);
    dvz_visual_viewport(visual, &viewport);


    // Create a board.
    DvzRequest req = dvz_create_board(rqr, WIDTH, HEIGHT, DVZ_DEFAULT_CLEAR_COLOR, 0);
    DvzId board_id = req.id;
    req = dvz_set_background(rqr, board_id, (cvec4){32, 64, 128, 255});

    // Record commands.
    dvz_record_begin(rqr, board_id);
    dvz_record_viewport(rqr, board_id, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
    // dvz_visual_instance(visual, board_id, 0, 0, n, 0, 1);
    dvz_visual_record(visual, board_id);
    dvz_record_end(rqr, board_id);

    // Render to a PNG.
    render_requests(rqr, get_gpu(suite), board_id, "visual_segment");

    // Cleanup
    dvz_visual_destroy(visual);
    dvz_requester_destroy(rqr);
    // FREE(pos);
    // FREE(color);
    return 0;
}
