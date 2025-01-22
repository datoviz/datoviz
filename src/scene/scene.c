/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene                                                                                        */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/scene.h"
#include "_list.h"
#include "common.h"
#include "datoviz.h"
#include "datoviz_protocol.h"
#include "scene/app.h"
#include "scene/arcball.h"
#include "scene/baker.h"
#include "scene/camera.h"
#include "scene/graphics.h"
#include "scene/ortho.h"
#include "scene/panzoom.h"
#include "scene/transform.h"
#include "scene/viewset.h"
#include "scene/visuals/pixel.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _panzoom_ortho_size(DvzPanel* panel)
{
    ANN(panel);
    ANN(panel->view);

    // NOTE: need to take margins into account when setting the panzoom size.
    float t = panel->view->margins[0];
    float r = panel->view->margins[1];
    float b = panel->view->margins[2];
    float l = panel->view->margins[3];
    float w = panel->view->shape[0] - r - l;
    float h = panel->view->shape[1] - t - b;

    if (panel->panzoom != NULL)
    {
        dvz_panzoom_resize(panel->panzoom, w, h);
    }
    if (panel->ortho != NULL)
    {
        dvz_ortho_resize(panel->ortho, w, h);
    }
}


static inline bool _is_drag(DvzMouseEvent ev)
{
    return ev.type == DVZ_MOUSE_EVENT_DRAG ||       //
           ev.type == DVZ_MOUSE_EVENT_DRAG_START || //
           ev.type == DVZ_MOUSE_EVENT_DRAG_STOP;    //
}



/*************************************************************************************************/
/*  Scene                                                                                        */
/*************************************************************************************************/

DvzScene* dvz_scene(DvzBatch* batch)
{
    ANN(batch);
    DvzScene* scene = (DvzScene*)calloc(1, sizeof(DvzScene));
    scene->batch = batch;
    scene->figures = dvz_list();

    // HACK: even if we don't use textures, we have to bind an empty texture.
    // So we create a mock empty texture.
    dvz_create_tex(batch, DVZ_TEX_2D, DVZ_FORMAT_R8_UNORM, (uvec3){1, 1, 1}, 0);
    dvz_create_sampler(batch, DVZ_FILTER_NEAREST, DVZ_SAMPLER_ADDRESS_MODE_REPEAT);

    // HACK: we don't have a good mechanism to specify IDs manually so we do it here.
    ASSERT(batch->count >= 2);
    batch->requests[batch->count - 2].id = DVZ_SCENE_DEFAULT_TEX_ID;
    batch->requests[batch->count - 1].id = DVZ_SCENE_DEFAULT_SAMPLER_ID;

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
    ASSERT(width > 0);
    ASSERT(height > 0);

    // Environment variable.
    if (checkenv("DVZ_FPS"))
        flags |= DVZ_CANVAS_FLAGS_FPS;
    if (checkenv("DVZ_MONITOR"))
        flags |= DVZ_CANVAS_FLAGS_MONITOR;
    if (checkenv("DVZ_VSYNC"))
        flags |= DVZ_CANVAS_FLAGS_VSYNC;

    // Initialize the structure.
    DvzFigure* fig = (DvzFigure*)calloc(1, sizeof(DvzFigure));
    fig->scene = scene;
    fig->flags = flags;

    // NOTE: the size is in screen coordinates, not framebuffer coordinates.
    fig->shape[0] = width;
    fig->shape[1] = height;
    fig->shape_init[0] = width;
    fig->shape_init[1] = height;

    // Panels.
    fig->panels = dvz_list();

    // Requester.
    DvzBatch* batch = scene->batch;
    ANN(batch);

    // Check if the batch/app is offscreen.
    DvzRequest req = {0};
    // Create the canvas.
    req = dvz_create_canvas(batch, width, height, DVZ_DEFAULT_CLEAR_COLOR, flags);
    fig->canvas_id = req.id;

    // Create the viewset;
    fig->viewset = dvz_viewset(batch, fig->canvas_id);

    // Append the figure to the scene's figures.
    dvz_list_append(scene->figures, (DvzListItem){.p = (void*)fig});

    return fig;
}



DvzId dvz_figure_id(DvzFigure* figure)
{
    ANN(figure);
    return figure->canvas_id;
}



void dvz_figure_resize(DvzFigure* fig, uint32_t width, uint32_t height)
{
    ANN(fig);
    log_debug("resize figure to %dx%d", width, height);

    fig->shape[0] = width;
    fig->shape[1] = height;

    float width_init = fig->shape_init[0];
    float height_init = fig->shape_init[1];

    float x0 = 0, y0 = 0, w0 = 0, h0 = 0;
    float x = 0, y = 0, w = 0, h = 0;

    // Go through all panels.
    uint32_t n = dvz_list_count(fig->panels);
    DvzPanel* panel = NULL;
    for (uint32_t i = 0; i < n; i++)
    {
        panel = (DvzPanel*)dvz_list_get(fig->panels, i).p;
        ANN(panel);
        ANN(panel->view);

        // NOTE: in framebuffer coordinates.
        x0 = panel->offset_init[0];
        y0 = panel->offset_init[1];
        w0 = panel->shape_init[0];
        h0 = panel->shape_init[1];

        // NOTE: although width(init)/height(init) are in screen coordinates, we use a ratio here,
        // so x and x0 etc remain in framebuffer coordinates.
        x = x0 * width / width_init;
        y = y0 * height / height_init;
        w = w0 * width / width_init;
        h = h0 * height / height_init;

        // Update the view offset and shape.
        dvz_panel_resize(panel, x, y, w, h);
    }
}



DvzFigure* dvz_scene_figure(DvzScene* scene, DvzId id)
{
    // Return a figure from a canvas ID.
    ANN(scene);
    ANN(scene->figures);

    // Go through all figures.
    uint32_t n = dvz_list_count(scene->figures);
    DvzFigure* fig = NULL;
    for (uint32_t i = 0; i < n; i++)
    {
        fig = (DvzFigure*)dvz_list_get(scene->figures, i).p;
        ANN(fig);
        if (fig->canvas_id == id)
            return fig;
    }
    return NULL;
}



void dvz_figure_update(DvzFigure* figure)
{
    ANN(figure);
    ANN(figure->viewset);
    ANN(figure->viewset->status);
    dvz_atomic_set(figure->viewset->status, (int)DVZ_BUILD_DIRTY);
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

DvzPanel* dvz_panel(DvzFigure* fig, float x, float y, float width, float height)
{
    ANN(fig);
    ANN(fig->scene);
    ANN(fig->scene->batch);
    ANN(fig->viewset);

    // Instantiate the structure.
    DvzPanel* panel = (DvzPanel*)calloc(1, sizeof(DvzPanel));
    panel->figure = fig;

    panel->offset_init[0] = x;
    panel->offset_init[1] = y;
    panel->shape_init[0] = width;
    panel->shape_init[1] = height;

    // Create a view.
    panel->view = dvz_view(fig->viewset, (vec2){x, y}, (vec2){width, height});

    // Default static transform.
    panel->static_transform = dvz_transform(fig->scene->batch, 0);

    // Append the figure to the scene's figures.
    dvz_list_append(fig->panels, (DvzListItem){.p = (void*)panel});

    return panel;
}



void dvz_panel_flags(DvzPanel* panel, int flags)
{
    ANN(panel);
    panel->flags = flags;
}



DvzBatch* dvz_panel_batch(DvzPanel* panel)
{
    ANN(panel);
    ANN(panel->figure);
    ANN(panel->figure->scene);
    DvzBatch* batch = panel->figure->scene->batch;
    ANN(batch);
    return batch;
}



DvzFigure* dvz_panel_figure(DvzPanel* panel)
{
    ANN(panel);
    ANN(panel->figure);
    return panel->figure;
}



DvzPanel* dvz_panel_default(DvzFigure* fig)
{
    ANN(fig);
    return dvz_panel(fig, 0, 0, fig->shape[0], fig->shape[1]);
}



void dvz_panel_transform(DvzPanel* panel, DvzTransform* tr)
{
    ANN(panel);
    ANN(tr);
    panel->transform = tr;
}



void dvz_panel_mvp(DvzPanel* panel, DvzMVP* mvp)
{
    ANN(panel);
    ANN(panel->transform);
    ANN(mvp);
    dvz_transform_set(panel->transform, mvp);
}



void dvz_panel_mvpmat(DvzPanel* panel, mat4 model, mat4 view, mat4 proj)
{
    ANN(panel);
    DvzMVP mvp = dvz_mvp(model, view, proj);
    dvz_panel_mvp(panel, &mvp);
}



void dvz_panel_resize(DvzPanel* panel, float x, float y, float width, float height)
{
    ANN(panel);
    ANN(panel->view);

    if (width == 0 || height == 0)
    {
        log_warn("skip panel_resize of size 0x0");
        return;
    }

    log_debug("resize panel to %.0fx%.0f -> %.0fx%.0f", x, y, width, height);

    dvz_view_resize(panel->view, (vec2){x, y}, (vec2){width, height});

    // NOTE: need to resize the panzoom as well.
    _panzoom_ortho_size(panel);

    if (panel->arcball != NULL)
    {
        dvz_arcball_resize(panel->arcball, width, height);
    }

    if (panel->camera != NULL)
    {
        dvz_camera_resize(panel->camera, width, height);

        // NOTE: for the camera, we also need to update the MVP struct on the GPU because the
        // projection matrix depends on the panel's aspect ratio.
        DvzMVP* mvp = dvz_transform_mvp(panel->transform);
        dvz_camera_mvp(panel->camera, mvp); // set the view and proj matrices
        dvz_transform_update(panel->transform);
    }

    if (panel->ortho != NULL)
    {
        // NOTE: for the ortho, we also need to update the MVP struct on the GPU because the
        // projection matrix depends on the panel's aspect ratio.
        DvzMVP* mvp = dvz_transform_mvp(panel->transform);
        dvz_ortho_mvp(panel->ortho, mvp); // set the view and proj matrices
        dvz_transform_update(panel->transform);
    }
}



void dvz_panel_margins(DvzPanel* panel, float top, float right, float bottom, float left)
{
    ANN(panel);
    dvz_view_margins(panel->view, (vec4){top, right, bottom, left});

    // NOTE: need to resize the panzoom if needed.
    _panzoom_ortho_size(panel);
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



DvzPanel* dvz_panel_at(DvzFigure* fig, vec2 pos)
{
    ANN(fig);
    ANN(fig->panels);

    // Go through all panels.
    uint32_t n = dvz_list_count(fig->panels);
    DvzPanel* panel = NULL;
    for (uint32_t i = 0; i < n; i++)
    {
        panel = (DvzPanel*)dvz_list_get(fig->panels, i).p;
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
    ANN(panel->view);
    ANN(panel->figure);
    log_trace("destroy panel");

    // Destroy the transform.
    if (panel->transform != NULL && panel->transform_to_destroy)
    {
        // NOTE: double destruction causes segfault if a transform is shared between different
        // panels, the transform should be destroyed only once.
        dvz_transform_destroy(panel->transform);
        panel->transform = NULL;
    }
    dvz_transform_destroy(panel->static_transform);

    // Destroy the view.
    dvz_view_destroy(panel->view);

    // Remove the figure from the scene's figures.
    dvz_list_remove_pointer(panel->figure->panels, panel);

    FREE(panel);
}



/*************************************************************************************************/
/*  Controllers                                                                                  */
/*************************************************************************************************/

DvzPanzoom* dvz_panel_panzoom(DvzPanel* panel)
{
    ANN(panel);
    ANN(panel->view);
    ANN(panel->figure);

    DvzScene* scene = panel->figure->scene;
    ANN(scene);

    if (panel->panzoom)
        return panel->panzoom;

    if (panel->transform != NULL)
    {
        log_error("could not create a panzoom as the panel has already a transform");
        return NULL;
    }

    ASSERT(panel->view->shape[0] > 0);
    ASSERT(panel->view->shape[1] > 0);

    log_trace("create a new Panzoom instance");

    float w = panel->view->shape[0];
    float h = panel->view->shape[1];

    // NOTE: the size is in screen coordinates, not framebuffer coordinates.
    panel->panzoom = dvz_panzoom(w, h, 0);
    _panzoom_ortho_size(panel); // Takes panel margins into account.

    panel->transform = dvz_transform(scene->batch, 0);
    panel->transform_to_destroy = true;

    return panel->panzoom;
}



DvzOrtho* dvz_panel_ortho(DvzPanel* panel)
{
    ANN(panel);
    ANN(panel->view);
    ANN(panel->figure);

    DvzScene* scene = panel->figure->scene;
    ANN(scene);

    if (panel->ortho)
        return panel->ortho;

    if (panel->transform != NULL)
    {
        log_error("could not create an ortho as the panel has already a transform");
        return NULL;
    }

    ASSERT(panel->view->shape[0] > 0);
    ASSERT(panel->view->shape[1] > 0);

    log_trace("create a new Ortho instance");

    float w = panel->view->shape[0];
    float h = panel->view->shape[1];

    // NOTE: the size is in screen coordinates, not framebuffer coordinates.
    panel->ortho = dvz_ortho(w, h, 0);
    _panzoom_ortho_size(panel); // Takes panel margins into account.

    panel->transform = dvz_transform(scene->batch, 0);
    panel->transform_to_destroy = true;

    // Get the MVP struct of the panel, update it with the ortho, and update the buffer on the
    // GPU.
    DvzMVP* mvp = dvz_transform_mvp(panel->transform);
    dvz_ortho_mvp(panel->ortho, mvp); // set the view and proj matrices
    dvz_transform_update(panel->transform);

    return panel->ortho;
}



DvzArcball* dvz_panel_arcball(DvzPanel* panel)
{
    ANN(panel);
    ANN(panel->view);
    ANN(panel->figure);

    DvzScene* scene = panel->figure->scene;
    ANN(scene);

    if (panel->arcball)
        return panel->arcball;

    if (panel->transform != NULL)
    {
        log_error("could not create an arcball as the panel has already a transform");
        return NULL;
    }

    ASSERT(panel->view->shape[0] > 0);
    ASSERT(panel->view->shape[1] > 0);

    log_trace("create a new Arcball instance");
    // NOTE: the size is in screen coordinates, not framebuffer coordinates.
    panel->arcball = dvz_arcball(panel->view->shape[0], panel->view->shape[1], 0);
    // panel->transform = dvz_transform(scene->batch, 0);
    // panel->transform_to_destroy = true;

    // Also create a perspective camera.
    DvzCamera* camera = dvz_panel_camera(panel, DVZ_CAMERA_FLAGS_PERSPECTIVE);

    return panel->arcball;
}



/*************************************************************************************************/
/*  Visuals                                                                                      */
/*************************************************************************************************/

void dvz_panel_visual(DvzPanel* panel, DvzVisual* visual, int flags)
{
    ANN(panel);
    ANN(panel->figure);
    ANN(panel->figure->scene);
    ANN(panel->figure->scene->batch);
    ANN(visual);
    ANN(visual->baker);

    DvzView* view = panel->view;
    ANN(view);

    // No visuals.
    if (visual->item_count == 0)
    {
        log_error("cannot add empty visual, make sure to fill the visual's properties first.");
        return;
    }

    // Panel transform must be set.
    bool is_static = (flags & DVZ_VIEW_FLAGS_STATIC) != 0;

    DvzTransform* tr = NULL;
    // Static transform.
    if (is_static)
    {
        tr = panel->static_transform;
    }
    else if (panel->transform == NULL)
    {
        log_debug("the panel had no transform, creating one");
        panel->transform = dvz_transform(panel->figure->scene->batch, 0);
        panel->transform_to_destroy = true;
        tr = panel->transform;
    }
    else
    {
        tr = panel->transform;
    }
    ANN(tr);

    // Add the visual to the view, and bind the common (shared) descriptors.
    dvz_view_add(view, visual, 0, visual->item_count, 0, 1, tr, 0);

    // Send the buffer upload requests.
    dvz_visual_update(visual);
}



/*************************************************************************************************/
/*  Camera                                                                                       */
/*************************************************************************************************/

static void _camera_zoom(DvzCamera* camera, float dz)
{
    ANN(camera);
    vec3 pos = {0};
    _vec3_copy(camera->pos, pos);
    pos[2] *= (1 + .01 * dz);
    dvz_camera_position(camera, pos);
}



DvzCamera* dvz_panel_camera(DvzPanel* panel, int flags)
{
    ANN(panel);
    ANN(panel->view);
    ANN(panel->figure);
    ANN(panel->figure->scene);

    DvzBatch* batch = panel->figure->scene->batch;
    ANN(batch);

    if (panel->camera)
        return panel->camera;

    if (!panel->transform)
    {
        // log_error("need to set up a panel transform before creating a camera");
        // return NULL;
        panel->transform = dvz_transform(batch, 0);
        panel->transform_to_destroy = true;
    }
    ANN(panel->transform);

    // Create a camera.
    log_trace("create a new Camera instance");
    panel->camera = dvz_camera(panel->view->shape[0], panel->view->shape[1], flags);
    ANN(panel->camera);

    // Get the MVP struct of the panel, update it with the camera, and update the buffer on the
    // GPU.
    DvzMVP* mvp = dvz_transform_mvp(panel->transform);
    dvz_camera_mvp(panel->camera, mvp); // set the view and proj matrices
    dvz_transform_update(panel->transform);

    return panel->camera;
}



/*************************************************************************************************/
/*  Updates                                                                                      */
/*************************************************************************************************/

static void _update_panzoom(DvzPanel* panel)
{
    ANN(panel);

    DvzPanzoom* pz = panel->panzoom;
    ANN(pz);

    DvzTransform* tr = panel->transform;
    ANN(tr);

    // Update the MVP matrices.
    DvzMVP* mvp = dvz_transform_mvp(tr);
    dvz_panzoom_mvp(pz, mvp);
}


static void _update_ortho(DvzPanel* panel)
{
    ANN(panel);

    DvzOrtho* ortho = panel->ortho;
    ANN(ortho);

    DvzTransform* tr = panel->transform;
    ANN(tr);

    // Update the MVP matrices.
    DvzMVP* mvp = dvz_transform_mvp(tr);
    dvz_ortho_mvp(ortho, mvp);
}


static void _update_arcball(DvzPanel* panel)
{
    ANN(panel);

    DvzArcball* arcball = panel->arcball;
    ANN(arcball);

    DvzTransform* tr = panel->transform;
    ANN(tr);

    // Update the MVP matrices.
    DvzMVP* mvp = dvz_transform_mvp(tr);
    dvz_arcball_mvp(arcball, mvp);
}


static void _update_camera(DvzPanel* panel)
{
    ANN(panel);

    DvzTransform* tr = panel->transform;
    ANN(tr);

    DvzCamera* camera = panel->camera;
    ANN(camera);

    // Update the MVP matrices.
    DvzMVP* mvp = dvz_transform_mvp(tr);
    dvz_camera_mvp(camera, mvp); // set the model matrix
}



void dvz_panel_update(DvzPanel* panel)
{
    ANN(panel);

    if (panel->camera)
        _update_camera(panel);
    if (panel->panzoom)
        _update_panzoom(panel);
    if (panel->ortho)
        _update_ortho(panel);
    if (panel->arcball)
        _update_arcball(panel);

    DvzTransform* tr = panel->transform;
    ANN(tr);
    dvz_transform_update(tr);
}



/*************************************************************************************************/
/*  Run                                                                                          */
/*************************************************************************************************/

static void _scene_build(DvzScene* scene)
{
    ANN(scene);

    // Go through all figures.
    uint64_t n = dvz_list_count(scene->figures);
    uint64_t view_count = 0;
    uint64_t visual_count = 0;
    DvzFigure* fig = NULL;
    DvzView* view = NULL;
    DvzVisual* visual = NULL;
    DvzBuildStatus status = DVZ_BUILD_CLEAR;

    for (uint64_t viewset_idx = 0; viewset_idx < n; viewset_idx++)
    {
        fig = (DvzFigure*)dvz_list_get(scene->figures, viewset_idx).p;
        ANN(fig);
        ANN(fig->viewset);
        ANN(fig->viewset->views);

        // Build status.
        status = (DvzBuildStatus)dvz_atomic_get(fig->viewset->status);
        // if viewset state == dirty, build viewset, and set the viewset state to clear
        if (status == DVZ_BUILD_DIRTY)
        {
            log_debug("build figure #%d", viewset_idx);
            dvz_viewset_build(fig->viewset);
            dvz_atomic_set(fig->viewset->status, (int)DVZ_BUILD_CLEAR);
        }

        // Now, automatically call dvz_visual_update() on all dirty visuals.

        // Go through the views of the viewset.
        view_count = dvz_list_count(fig->viewset->views);
        view = NULL;
        for (uint64_t view_idx = 0; view_idx < view_count; view_idx++)
        {
            view = (DvzView*)dvz_list_get(fig->viewset->views, view_idx).p;
            ANN(view);

            // Go through the visuals of the view.
            visual_count = dvz_list_count(view->visuals);
            log_trace(
                "going through %d visuals of view #%d to find dirty visuals", //
                visual_count, view_idx);
            for (uint64_t visual_idx = 0; visual_idx < visual_count; visual_idx++)
            {
                visual = (DvzVisual*)dvz_list_get(view->visuals, visual_idx).p;
                ANN(visual);

                // This will only update the visual if it needs to be updated.
                dvz_visual_update(visual);
            }
        }
    }
}



static void _scene_onmouse(DvzApp* app, DvzId window_id, DvzMouseEvent ev)
{
    ANN(app);

    DvzScene* scene = (DvzScene*)ev.user_data;
    ANN(scene);

    DvzBatch* batch = scene->batch;
    ANN(batch);

    DvzFigure* fig = dvz_scene_figure(scene, window_id);
    ANN(fig);

    // Find the relevant panel for mouse interaction. Depends on whether this is a dragging action
    // or not.
    DvzPanel* panel = NULL;
    if (_is_drag(ev))
    {
        panel = dvz_panel_at(fig, ev.content.d.press_pos);
    }
    else
    {
        panel = dvz_panel_at(fig, ev.pos);
    }
    if (panel == NULL)
    {
        log_debug("no panel found with mouse event type %d", ev.type);
        return;
    }

    // Localize the mouse event (viewport offset).
    DvzMouseEvent mev =
        dvz_view_mouse(panel->view, ev, ev.content_scale, DVZ_MOUSE_REFERENCE_LOCAL);

    // Panzoom.
    DvzPanzoom* pz = panel->panzoom;
    if (pz != NULL)
    {
        DvzTransform* tr = panel->transform;
        if (tr == NULL)
        {
            log_warn("no transform set in panel");
            return;
        }
        // Pass the mouse event to the panzoom object.
        if (dvz_panzoom_mouse(pz, mev))
        {
            _update_panzoom(panel);
            dvz_transform_update(tr);
        }
    }

    // Ortho.
    DvzOrtho* ortho = panel->ortho;
    if (ortho != NULL)
    {
        DvzTransform* tr = panel->transform;
        if (tr == NULL)
        {
            log_warn("no transform set in panel");
            return;
        }
        // Pass the mouse event to the ortho object.
        if (dvz_ortho_mouse(ortho, mev))
        {
            _update_ortho(panel);
            dvz_transform_update(tr);
        }
    }

    // Arcball.
    DvzArcball* arcball = panel->arcball;
    if (arcball != NULL)
    {
        DvzTransform* tr = panel->transform;
        if (tr == NULL)
        {
            log_warn("no transform set in panel");
            return;
        }
        // Pass the mouse event to the arcball object.
        if (dvz_arcball_mouse(arcball, mev))
        {
            _update_arcball(panel);
        }

        // Camera zoom.
        if (ev.type == DVZ_MOUSE_EVENT_WHEEL)
        {
            _camera_zoom(panel->camera, ev.content.w.dir[1]);
            _update_camera(panel);
        }

        // Reset the camera after a double-click.
        if (mev.type == DVZ_MOUSE_EVENT_DOUBLE_CLICK)
        {
            dvz_camera_reset(panel->camera);
            dvz_arcball_reset(panel->arcball);
            _update_camera(panel);
        }

        dvz_transform_update(tr);
    }
}

static void _scene_onresize(DvzApp* app, DvzId window_id, DvzWindowEvent ev)
{
    ANN(app);

    float w = ev.screen_width;
    float h = ev.screen_height;

    log_debug("window 0x%" PRIx64 " resized to %.0fx%.0f", window_id, w, h);

    DvzScene* scene = (DvzScene*)ev.user_data;
    ANN(scene);

    // Retrieve the figure that is being resize, thanks to the id that is the same between the
    // canvas and the window.
    DvzFigure* fig = dvz_scene_figure(scene, window_id);
    ANN(fig);
    ANN(fig->viewset);

    //
    if (dvz_atomic_get(fig->viewset->status) == DVZ_BUILD_DIRTY)
    {
        log_warn("skip figure onresize callback because the viewset is already dirty");
        return;
    }

    // Resize the figure, compute each panel's new size and resize them.
    // This will also call panzoom/ortho/arcball/camera resize for each panel.
    dvz_figure_resize(fig, w, h);

    // Mark the viewset as dirty to trigger a command buffer record at the next frame.
    dvz_figure_update(fig);

    // dvz_app_submit(scene->app);
}

static void _scene_onframe(DvzApp* app, DvzId window_id, DvzFrameEvent ev)
{
    ANN(app);

    DvzScene* scene = (DvzScene*)ev.user_data;
    ANN(scene);

    _scene_build(scene);

    dvz_app_submit(scene->app);
}



void dvz_scene_run(DvzScene* scene, DvzApp* app, uint64_t n_frames)
{
    ANN(scene);
    ANN(app);

    // HACK: so that callbacks below have access to the app to submit to the presenter.
    // TODO: remove, no longer needed
    scene->app = app;

    // Scene callbacks.
    dvz_app_onmouse(app, _scene_onmouse, scene);
    dvz_app_onresize(app, _scene_onresize, scene);
    dvz_app_onframe(app, _scene_onframe, scene);

    // Initial build of the scene.
    _scene_build(scene);

    // Run the app.
    dvz_app_run(app, n_frames);
}
