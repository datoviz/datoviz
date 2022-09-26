/*************************************************************************************************/
/*  Scene                                                                                        */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/scene.h"
#include "common.h"
#include "request.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define RQ(FUN, ...)                                                                              \
    DvzRequest req = (FUN)(scene->rqr, ##__VA_ARGS__);                                            \
    dvz_requester_add(scene->rqr, req);



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
    return scene;
}



DvzFigure* dvz_figure(
    DvzScene* scene, uint32_t width, uint32_t height, uint32_t n_rows, uint32_t n_cols, int flags)
{
    ANN(scene);
    ANN(scene->rqr);

    DvzFigure* figure = (DvzFigure*)calloc(1, sizeof(DvzFigure));

    RQ(dvz_create_canvas, width, height, DVZ_DEFAULT_CLEAR_COLOR, flags);
    figure->id = req.id;

    return figure;
}



DvzPanel* dvz_panel(DvzFigure* fig, uint32_t row, uint32_t col, DvzPanelType type, int flags)
{
    ANN(fig);

    DvzPanel* panel = (DvzPanel*)calloc(1, sizeof(DvzPanel));
    // TODO
    return panel;
}



DvzVisual* dvz_visual(DvzScene* scene, DvzVisualType vtype, int flags)
{
    ANN(scene);

    // TODO: conversion between visual type and graphics type
    DvzVisual* visual = (DvzVisual*)calloc(1, sizeof(DvzVisual));

    RQ(dvz_create_graphics, (DvzGraphicsType)vtype, flags);
    visual->id = req.id;

    return visual;
}



void dvz_visual_data(
    DvzVisual* visual, DvzPropType ptype, uint64_t index, uint64_t count, void* data)
{
    ANN(visual);

    // TODO
}



void dvz_panel_visual(DvzPanel* panel, DvzVisual* visual, int pos)
{
    ANN(panel);
    ANN(visual);

    // TODO
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
