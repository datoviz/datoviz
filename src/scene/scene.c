/*************************************************************************************************/
/*  Scene                                                                                        */
/*  NOTE: obsolete? to remove                                                                    */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/scene.h"
#include "common.h"
#include "request.h"
#include "scene/graphics.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define RQ(FUN, ...)                                                                              \
    req = (FUN)(scene->rqr, __VA_ARGS__);                                                         \
    dvz_requester_add(scene->rqr, req);                                                           \
    dvz_request_print(&req);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Scene functions                                                                              */
/*************************************************************************************************/

DvzScene* dvz_scene(void)
{
    DvzScene* scene = (DvzScene*)calloc(1, sizeof(DvzScene));
    scene->rqr = dvz_requester();

    // NOTE: we need to manually begin recording the requester, otherwise requests won't be
    // automatically recorded in the requester batch.
    dvz_requester_begin(scene->rqr);

    return scene;
}



DvzFigure* dvz_figure(
    DvzScene* scene, uint32_t width, uint32_t height, uint32_t n_rows, uint32_t n_cols, int flags)
{
    ANN(scene);
    ANN(scene->rqr);

    DvzFigure* figure = (DvzFigure*)calloc(1, sizeof(DvzFigure));

    DvzRequest req = {0};
    RQ(dvz_create_canvas, width, height, DVZ_DEFAULT_CLEAR_COLOR, flags);
    figure->id = req.id;

    return figure;
}



DvzPanel* dvz_panel(DvzFigure* fig, uint32_t row, uint32_t col, DvzPanelType type, int flags)
{
    ANN(fig);

    DvzPanel* panel = (DvzPanel*)calloc(1, sizeof(DvzPanel));
    panel->figure = fig;
    // TODO
    return panel;
}



DvzVisual* dvz_visual(DvzScene* scene, DvzVisualType vtype, int flags)
{
    ANN(scene);

    // TODO: conversion between visual type and graphics type
    DvzVisual* visual = (DvzVisual*)calloc(1, sizeof(DvzVisual));

    DvzRequest req = {0};
    RQ(dvz_create_graphics, (DvzGraphicsType)vtype, flags);
    visual->id = req.id; // visual id is the graphics id
    visual->scene = scene;

    return visual;
}



void dvz_visual_data(
    DvzVisual* visual, DvzPropType ptype, uint64_t index, uint64_t count, void* data)
{
    ANN(visual);
    DvzId visual_id = visual->id;

    DvzScene* scene = visual->scene;
    ANN(scene);

    // TODO determine this as a function of the visual
    DvzSize item_size = sizeof(DvzGraphicsPointVertex);

    visual->count = count;

    DvzRequest req = {0};
    RQ(dvz_create_dat, DVZ_BUFFER_TYPE_VERTEX, count * item_size, 0);
    visual->vertex = req.id;
    RQ(dvz_bind_vertex, visual_id, visual->vertex);
    RQ(dvz_upload_dat, visual->vertex, 0, count * item_size, data);

    RQ(dvz_create_dat, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzMVP), 0);
    DvzId mvp_id = req.id;
    RQ(dvz_bind_dat, visual_id, 0, mvp_id);

    visual->mvp = dvz_mvp_default();
    RQ(dvz_upload_dat, mvp_id, 0, sizeof(DvzMVP), &visual->mvp);
}



void dvz_visual_update(
    DvzVisual* visual, DvzPropType ptype, uint64_t index, uint64_t count, void* data)
{
    ANN(visual);

    DvzScene* scene = visual->scene;
    ANN(scene);

    DvzSize item_size = sizeof(DvzGraphicsPointVertex);
    DvzRequest req = {0};
    RQ(dvz_upload_dat, visual->vertex, 0, count * item_size, data);
}



void dvz_panel_visual(DvzPanel* panel, DvzVisual* visual, int pos)
{
    ANN(panel);
    ANN(visual);
    DvzId visual_id = visual->id;

    DvzScene* scene = visual->scene;
    ANN(scene);

    DvzFigure* figure = panel->figure;
    ANN(figure);

    DvzId canvas_id = figure->id;
    uint32_t w = figure->width;
    uint32_t h = figure->height;

    DvzRequest req = {0};
    RQ(dvz_create_dat, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzViewport), 0);
    DvzId viewport_id = req.id;
    RQ(dvz_bind_dat, visual_id, 1, viewport_id);

    visual->viewport = dvz_viewport_default(w, h);
    RQ(dvz_upload_dat, viewport_id, 0, sizeof(DvzViewport), &visual->viewport);

    RQ(dvz_record_begin, canvas_id);
    RQ(dvz_record_viewport, canvas_id, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
    RQ(dvz_record_draw, canvas_id, visual_id, 0, visual->count, 0, 1);
    RQ(dvz_record_end, canvas_id);
}



void dvz_visual_destroy(DvzVisual* visual)
{
    ANN(visual);
    FREE(visual);
}



void dvz_panel_destroy(DvzPanel* panel)
{
    ANN(panel);
    FREE(panel);
}



void dvz_figure_destroy(DvzFigure* figure)
{
    ANN(figure);
    FREE(figure);
}



void dvz_scene_destroy(DvzScene* scene)
{
    ANN(scene);
    dvz_requester_destroy(scene->rqr);
    FREE(scene);
}
