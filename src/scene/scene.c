/*************************************************************************************************/
/*  Scene                                                                                        */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/scene.h"
#include "_list.h"
#include "common.h"
#include "request.h"
#include "scene/app.h"
#include "scene/arcball.h"
#include "scene/graphics.h"
#include "scene/panzoom.h"
#include "scene/transform.h"
#include "scene/viewset.h"
#include "scene/visuals/pixel.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Scene                                                                                        */
/*************************************************************************************************/

DvzScene* dvz_scene(DvzRequester* rqr)
{
    ANN(rqr);
    DvzScene* scene = (DvzScene*)calloc(1, sizeof(DvzScene));
    scene->rqr = rqr;
    scene->figures = dvz_list();
    return scene;
}



void dvz_scene_destroy(DvzScene* scene)
{
    ANN(scene);
    dvz_list_destroy(scene->figures);
    FREE(scene);
}



/*************************************************************************************************/
/*  Figure                                                                                       */
/*************************************************************************************************/

DvzFigure* dvz_figure(DvzScene* scene, uint32_t width, uint32_t height, int flags)
{
    ANN(scene);

    // Initialize the structure.
    DvzFigure* fig = (DvzFigure*)calloc(1, sizeof(DvzFigure));
    fig->scene = scene;
    fig->flags = flags;

    // NOTE: the size is in screen coordinates, not framebuffer coordinates.
    fig->width = width;
    fig->height = height;

    // Panels.
    fig->panels = dvz_list();

    // Requester.
    DvzRequester* rqr = scene->rqr;
    ANN(rqr);

    // Create the canvas.
    DvzRequest req = dvz_create_canvas(rqr, width, height, DVZ_DEFAULT_CLEAR_COLOR, flags);
    fig->canvas_id = req.id;

    // Create the viewset;
    fig->viewset = dvz_viewset(rqr, fig->canvas_id);

    // Append the figure to the scene's figures.
    dvz_list_append(scene->figures, (DvzListItem){.p = (void*)fig});

    return fig;
}



void dvz_figure_destroy(DvzFigure* fig)
{
    ANN(fig);

    // Destroy the viewset.
    dvz_viewset_destroy(fig->viewset);

    // Destroy all panels.
    uint32_t n = dvz_list_count(fig->panels);
    for (uint32_t i = 0; i < n; i++)
    {
        dvz_panel_destroy((DvzPanel*)dvz_list_get(fig->panels, i).p);
    }

    // Destroy the list of panels.
    dvz_list_destroy(fig->panels);

    // Remove the figure from the scene's figures.
    dvz_list_remove_pointer(fig->scene->figures, fig);

    // Free the DvzFigure structure.
    FREE(fig);
}



/*************************************************************************************************/
/*  Panel                                                                                        */
/*************************************************************************************************/

DvzPanel* dvz_panel(DvzFigure* fig, float x, float y, float w, float h)
{
    ANN(fig);
    ANN(fig->scene);
    ANN(fig->viewset);

    // Instantiate the structure.
    DvzPanel* panel = (DvzPanel*)calloc(1, sizeof(DvzPanel));
    panel->figure = fig;

    // Create a view.
    panel->view = dvz_view(fig->viewset, (vec2){x, y}, (vec2){w, h});

    // Create a transform.
    panel->transform = dvz_transform(fig->scene->rqr);

    // Append the figure to the scene's figures.
    dvz_list_append(fig->panels, (DvzListItem){.p = (void*)panel});

    return panel;
}



DvzPanel* dvz_panel_default(DvzFigure* fig)
{
    ANN(fig);
    return dvz_panel(fig, 0, 0, fig->width, fig->height);
}



bool dvz_panel_contains(DvzPanel* panel, vec2 pos)
{
    ANN(panel);
    ANN(panel->view);
    float x0 = panel->view->offset[0];
    float y0 = panel->view->offset[1];
    float w = panel->view->shape[0];
    float h = panel->view->shape[1];
    float x1 = x0 + w;
    float y1 = y0 + h;
    float x = pos[0];
    float y = pos[1];
    return (x0 <= x) && (x < x1) && (y0 <= y) && (y < y1);
}



DvzPanel* dvz_panel_at(DvzFigure* figure, vec2 pos)
{
    ANN(figure);
    ANN(figure->panels);

    // Go through all panels.
    uint32_t n = dvz_list_count(figure->panels);
    DvzPanel* panel = NULL;
    for (uint32_t i = 0; i < n; i++)
    {
        panel = (DvzPanel*)dvz_list_get(figure->panels, i).p;
        if (panel != NULL)
        {
            // Return the first panel that contains the position.
            if (dvz_panel_contains(panel, pos))
                return panel;
        }
    }
    return NULL;
}



void dvz_panel_destroy(DvzPanel* panel)
{
    ANN(panel);

    // Destroy the transform.
    dvz_transform_destroy(panel->transform);

    // Destroy the view.
    dvz_view_destroy(panel->view);

    // Remove the figure from the scene's figures.
    dvz_list_remove_pointer(panel->figure->panels, panel);

    FREE(panel);
}



/*************************************************************************************************/
/*  Controllers                                                                                  */
/*************************************************************************************************/

static void _panel_resize(DvzClient* client, DvzClientEvent ev)
{
    ANN(client);

    uint32_t width = ev.content.w.screen_width;
    uint32_t height = ev.content.w.screen_height;
    log_debug("window 0x%" PRIx64 " resized to %dx%d", ev.window_id, width, height);

    DvzPanel* panel = (DvzPanel*)ev.user_data;
    ANN(panel);

    DvzPanzoom* pz = panel->panzoom;
    ANN(pz);

    dvz_panzoom_resize(pz, width, height);
}

static void _panel_mouse(DvzClient* client, DvzClientEvent ev)
{
    ANN(client);

    DvzPanel* panel = (DvzPanel*)ev.user_data;
    ANN(panel);
    ANN(panel->figure);
    ANN(panel->figure->scene);

    DvzPanzoom* pz = panel->panzoom;
    ANN(pz);

    DvzRequester* rqr = panel->figure->scene->rqr;
    ANN(rqr);

    DvzTransform* tr = panel->transform;
    ANN(tr);

    // Localize the mouse event (viewport offset).
    DvzMouseEvent mev =
        dvz_view_mouse(panel->view, ev.content.m, ev.content_scale, DVZ_MOUSE_REFERENCE_LOCAL);

    // Pass the mouse event to the panzoom object.
    dvz_panzoom_mouse(pz, mev);

    // Update the MVP matrices.
    DvzMVP* mvp = dvz_transform_mvp(tr);
    dvz_panzoom_mvp(pz, mvp);

    dvz_requester_begin(rqr);
    dvz_transform_update(tr, *mvp);
    dvz_requester_end(rqr, NULL);
}



DvzPanzoom* dvz_panel_panzoom(DvzApp* app, DvzPanel* panel)
{
    ANN(app);
    ANN(panel);
    ANN(panel->view);
    ANN(panel->figure);
    ANN(panel->figure->scene);

    ASSERT(panel->view->shape[0] > 0);
    ASSERT(panel->view->shape[1] > 0);

    // NOTE: the size is in screen coordinates, not framebuffer coordinates.
    panel->panzoom = dvz_panzoom(panel->view->shape[0], panel->view->shape[1], 0);

    // Callbacks.
    dvz_app_onmouse(app, _panel_mouse, panel);
    dvz_app_onresize(app, _panel_resize, panel);

    return panel->panzoom;
}


DvzArcball* dvz_panel_arcball(DvzApp* app, DvzPanel* panel)
{
    ANN(app);
    ANN(panel);
    // TODO: camera
    return NULL;
}



/*************************************************************************************************/
/*  Visuals                                                                                      */
/*************************************************************************************************/

void dvz_panel_visual(DvzPanel* panel, DvzVisual* visual)
{
    ANN(panel);

    DvzView* view = panel->view;
    ANN(view);

    if (visual->item_count == 0)
    {
        log_error("cannot add empty visual, make sure to fill the visual's properties first.");
        return;
    }

    // By default, add all items, using a single instance, and the panel's transform.
    dvz_view_add(view, visual, 0, visual->item_count, 0, 1, panel->transform, 0);
}
