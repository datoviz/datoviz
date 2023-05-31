/*************************************************************************************************/
/*  Viewset                                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/viewset.h"
#include "_list.h"
#include "_map.h"
#include "common.h"
#include "request.h"
#include "scene/baker.h"
#include "scene/transform.h"
#include "scene/visual.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Util functions                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Viewset                                                                                      */
/*************************************************************************************************/

DvzViewset* dvz_viewset(DvzRequester* rqr, DvzId canvas_id)
{
    ANN(rqr);
    log_trace("create viewset");

    DvzViewset* viewset = (DvzViewset*)calloc(1, sizeof(DvzViewset));
    viewset->rqr = rqr;
    viewset->canvas_id = canvas_id;
    viewset->views = dvz_list();
    // viewset->root = dvz_view(viewset, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
    return viewset;
}



void dvz_viewset_clear(DvzViewset* viewset)
{
    ANN(viewset);
    ANN(viewset->views);
    log_trace("clear viewset");

    uint64_t count = dvz_list_count(viewset->views);
    DvzView* view = NULL;
    for (uint64_t i = 0; i < count; i++)
    {
        view = (DvzView*)dvz_list_get(viewset->views, i).p;
        ANN(view);
        dvz_view_destroy(view);
    }
    dvz_list_clear(viewset->views);

    // Recreate the root view.
    // viewset->root = dvz_view(viewset, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
}



void dvz_viewset_build(DvzViewset* viewset)
{
    // this function recreates the command buffer
    ANN(viewset);
    ANN(viewset->views);
    log_trace("build viewset");

    DvzId canvas_id = viewset->canvas_id;
    ASSERT(canvas_id != DVZ_ID_NONE);

    // TODO
    // a viewset flag indicates whether the command buffer needs to be rebuilt
    // it is set to true whenever a view_*() function is called
    // at every frame, dvz_viewset_build() is called (no op if !is_dirty)

    DvzRequester* rqr = viewset->rqr;
    dvz_record_begin(rqr, canvas_id);

    uint64_t view_count = dvz_list_count(viewset->views);
    uint64_t count = 0;
    DvzView* view = NULL;
    // bool indirect = false;
    DvzVisual* visual = NULL;

    // for each view
    for (uint64_t i = 0; i < view_count; i++)
    {
        view = (DvzView*)dvz_list_get(viewset->views, i).p;
        ANN(view);
        ANN(view->visuals);

        // Set the current viewport, corresponding to the current view.
        dvz_record_viewport(rqr, canvas_id, view->offset, view->shape);

        // For each visual in the view
        count = dvz_list_count(view->visuals);
        for (uint64_t j = 0; j < count; j++)
        {
            visual = (DvzVisual*)dvz_list_get(view->visuals, i).p;
            ANN(visual);

            if (!visual->is_visible)
            {
                log_debug("skipping invisible visual");
                continue;
            }

            // Call the visual draw callback with the parameters stored in the visual.
            dvz_visual_record(visual, canvas_id);
        }
    }

    dvz_record_end(rqr, canvas_id);
}



void dvz_viewset_destroy(DvzViewset* viewset)
{
    ANN(viewset);
    log_trace("destroy viewset");

    // Destroy through all views.
    dvz_viewset_clear(viewset);
    dvz_list_destroy(viewset->views);
    FREE(viewset);
}



/*************************************************************************************************/
/*  View                                                                                         */
/*************************************************************************************************/

DvzView* dvz_view(DvzViewset* viewset, vec2 offset, vec2 shape)
{
    ANN(viewset);
    log_trace("create view");

    DvzView* view = (DvzView*)calloc(1, sizeof(DvzView));
    view->viewset = viewset;
    view->visuals = dvz_list();
    dvz_view_resize(view, offset, shape);
    dvz_list_append(viewset->views, (DvzListItem){.p = view});
    return view;
}



void dvz_view_add(
    DvzView* view, DvzVisual* visual,                 //
    uint32_t first, uint32_t count,                   // items
    uint32_t first_instance, uint32_t instance_count, // instances
    DvzTransform* transform, int viewport_flags)      // transform and viewport flags
{
    ANN(view);
    ANN(visual);

    ASSERT(count > 0);
    ASSERT(instance_count > 0);

    visual->draw_first = first;
    visual->draw_count = count;
    visual->first_instance = first_instance;
    visual->instance_count = instance_count;

    dvz_list_append(view->visuals, (DvzListItem){.p = visual});

    // TODO

    // MVP.
    dvz_visual_mvp(visual, dvz_mvp_default());

    // Viewport.
    DvzViewport viewport = dvz_viewport_default(view->shape[0], view->shape[1]);
    dvz_visual_viewport(visual, viewport);
}



void dvz_view_clear(DvzView* view)
{
    ANN(view);
    ANN(view->visuals);
    log_trace("clear view");
    dvz_list_clear(view->visuals);
}



void dvz_view_resize(DvzView* view, vec2 offset, vec2 shape)
{
    ANN(view);
    memcpy(view->offset, offset, sizeof(vec2));
    memcpy(view->shape, shape, sizeof(vec2));
}



void dvz_view_destroy(DvzView* view)
{
    ANN(view);
    ANN(view->viewset);
    log_trace("destroy view");

    dvz_list_destroy(view->visuals);
    dvz_list_remove_pointer(view->viewset->views, view);
    FREE(view);
}
