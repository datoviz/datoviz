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
    uint64_t instance_count = 0;
    DvzView* view = NULL;
    // bool indirect = false;
    DvzInstance* instance = NULL;

    // for each view
    for (uint64_t i = 0; i < view_count; i++)
    {
        view = (DvzView*)dvz_list_get(viewset->views, i).p;
        ANN(view);
        ANN(view->instances);

        // Set the current viewport, corresponding to the current view.
        dvz_record_viewport(rqr, canvas_id, view->offset, view->shape);

        // For each instance in the view
        instance_count = dvz_list_count(view->instances);
        for (uint64_t j = 0; j < instance_count; j++)
        {
            instance = (DvzInstance*)dvz_list_get(view->instances, i).p;
            ANN(instance);

            // Call the visual draw callback with the parameters stored in the DvzInstance struct.
            dvz_visual_draw(
                instance->visual, canvas_id, instance->first, instance->count,
                instance->first_instance, instance->instance_count);
        }
    }

    dvz_record_end(rqr, canvas_id);
}



void dvz_viewset_destroy(DvzViewset* viewset)
{
    ANN(viewset);
    log_trace("destroy viewset");

    // Destroy through all views and all instances.
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
    view->instances = dvz_list();
    dvz_view_resize(view, offset, shape);
    dvz_list_append(viewset->views, (DvzListItem){.p = view});
    return view;
}



void dvz_view_clear(DvzView* view)
{
    ANN(view);
    ANN(view->instances);
    log_trace("clear view");

    uint64_t count = dvz_list_count(view->instances);
    DvzInstance* instance = NULL;
    for (uint64_t i = 0; i < count; i++)
    {
        instance = (DvzInstance*)dvz_list_get(view->instances, i).p;
        ANN(instance);
        dvz_instance_destroy(instance);
    }
    dvz_list_clear(view->instances);
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

    // Destroy through all instances.
    dvz_view_clear(view);
    dvz_list_destroy(view->instances);
    dvz_list_remove_pointer(view->viewset->views, view);
    FREE(view);
}



/*************************************************************************************************/
/*  Instance                                                                                     */
/*************************************************************************************************/

DvzInstance* dvz_view_instance(
    DvzView* view, DvzVisual* visual,                 //
    uint32_t first, uint32_t count,                   // items
    uint32_t first_instance, uint32_t instance_count, // instances
    DvzTransform* tr,                                 // transform
    int flags)                                        // viewport flags
{
    ANN(view);
    log_trace("create instance");

    // Upload the MVP structure.
    // TODO: transforms
    dvz_visual_mvp(visual, dvz_mvp_default());
    if (tr == NULL)
    {
        log_trace("creating default transform for view instance");
        tr = dvz_transform(visual->rqr);
    }
    ANN(tr);

    // TODO: use a new #define DVZ_COMMON_MVP instead of hard-coded 0 here
    ANN(visual->baker);
    // HACK: handle the case where the visual does not have common bindings
    // if (visual->baker->slot_count > 0)
    //     dvz_baker_share_uniform(visual->baker, 0, &tr->dual);

    // Create the viewport and upload it to the uniform buffer.
    DvzViewport viewport = dvz_viewport(view->offset, view->shape, DVZ_VIEWPORT_FLAGS_NONE);
    dvz_visual_viewport(visual, viewport);

    // create a new instance and append it to view->instances
    DvzInstance* instance = (DvzInstance*)calloc(1, sizeof(DvzInstance));
    instance->view = view;
    instance->visual = visual;
    instance->tr = tr;
    instance->first = first;
    instance->count = count;
    instance->first_instance = first_instance;
    instance->instance_count = instance_count;
    instance->is_visible = true;
    dvz_list_append(view->instances, (DvzListItem){.p = instance});
    return instance;
}



void dvz_instance_visible(DvzInstance* instance, bool is_visible)
{
    ANN(instance);
    instance->is_visible = is_visible;
}



void dvz_instance_destroy(DvzInstance* instance)
{
    ANN(instance);
    log_trace("destroy instance");

    DvzView* view = instance->view;
    ANN(view);
    ANN(view->instances);
    dvz_list_remove_pointer(view->instances, instance);
    FREE(instance);
}
