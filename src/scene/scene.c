/*************************************************************************************************/
/*  Scene                                                                                        */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/scene.h"
#include "request.h"



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

    DvzFigure* figure = (DvzFigure*)calloc(1, sizeof(DvzFigure));

    return figure;
}



DvzPanel* dvz_panel(DvzFigure* fig, uint32_t row, uint32_t col, DvzPanelType type, int flags)
{
    ANN(fig);


    DvzPanel* panel = (DvzPanel*)calloc(1, sizeof(DvzPanel));

    return panel;
}



DvzVisual* dvz_visual(DvzScene* scene, DvzVisualType vtype, int flags)
{
    ANN(scene);

    DvzVisual* visual = (DvzVisual*)calloc(1, sizeof(DvzVisual));

    return visual;
}



void dvz_visual_data(
    DvzVisual* visual, DvzPropType ptype, uint64_t index, uint64_t count, void* data)
{
    ANN(visual);
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
