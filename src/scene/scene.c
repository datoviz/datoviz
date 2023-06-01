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
#include "scene/graphics.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Scene                                                                                        */
/*************************************************************************************************/

DvzScene* dvz_scene(DvzApp* app)
{
    DvzScene* scene = (DvzScene*)calloc(1, sizeof(DvzScene));
    scene->app = app;
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

    return NULL;
}



void dvz_figure_destroy(DvzFigure* fig)
{
    ANN(fig);

    // Destroy all panels.
    uint32_t n = dvz_list_count(fig->panels);
    for (uint32_t i = 0; i < n; i++)
    {
        dvz_panel_destroy((DvzPanel*)dvz_list_get(fig->panels, i).p);
    }

    // Destroy the list of panels.
    dvz_list_destroy(fig->panels);

    // Free the DvzFigure structure.
    FREE(fig);
}



/*************************************************************************************************/
/*  Panel                                                                                        */
/*************************************************************************************************/


DvzPanel* dvz_panel(DvzFigure* fig, float x, float y, float w, float h)
{
    ANN(fig);
    return NULL;
}



DvzPanel* dvz_panel_default(DvzFigure* fig)
{
    ANN(fig);
    return NULL;
}



void dvz_panel_destroy(DvzPanel* panel)
{
    ANN(panel);

    FREE(panel);
}



/*************************************************************************************************/
/*  Controllers                                                                                  */
/*************************************************************************************************/

DvzPanzoom* dvz_panel_panzoom(DvzPanel* panel)
{
    ANN(panel);
    return NULL;
}


DvzArcball* dvz_panel_arcball(DvzPanel* panel)
{
    ANN(panel);
    return NULL;
}



/*************************************************************************************************/
/*  Visuals                                                                                      */
/*************************************************************************************************/

DvzVisual* dvz_panel_pixel(DvzPanel* panel, int flags)
{
    ANN(panel);
    return NULL;
}
