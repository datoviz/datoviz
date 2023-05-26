/*************************************************************************************************/
/*  Testing viewset                                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/test_viewset.h"
#include "renderer.h"
#include "request.h"
#include "scene/scene_testing_utils.h"
#include "scene/viewset.h"
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

int test_viewset_1(TstSuite* suite)
{
    DvzRequester* rqr = dvz_requester();
    dvz_requester_begin(rqr);

    uint32_t n = 10;

    // Create a visual.
    DvzVisual* visual = dvz_visual(rqr, DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST, 0);

    DvzId canvas_id = 1;
    vec2 offset = {0, 0};
    vec2 shape = {0, 0};

    // Create a viewset.
    DvzViewset* viewset = dvz_viewset(rqr, canvas_id);

    // Create a view.
    DvzView* view = dvz_view(viewset, offset, shape);
    dvz_view_clear(view);

    // Create an instance.
    DvzInstance* instance = dvz_view_instance(view, visual, 0, 0, n, 0, 1);
    dvz_instance_visible(instance, true);

    dvz_viewset_build(viewset);
    // dvz_requester_print(rqr);

    dvz_instance_destroy(instance);
    dvz_view_destroy(view);

    dvz_viewset_destroy(viewset);
    dvz_requester_destroy(rqr);
    return 0;
}