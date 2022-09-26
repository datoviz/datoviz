/*************************************************************************************************/
/*  Plot                                                                                         */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/plot.h"
#include "../render_utils.h"
#include "client.h"
#include "host.h"
#include "presenter.h"
#include "renderer.h"
#include "request.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Plot functions                                                                               */
/*************************************************************************************************/

DvzApp* dvz_app(DvzBackend backend)
{
    DvzApp* app = (DvzApp*)calloc(1, sizeof(DvzApp));
    app->host = dvz_host(backend);
    app->client = dvz_client(backend);
    return app;
}



DvzDevice* dvz_device(DvzApp* app)
{
    ANN(app);
    ANN(app->host);
    ANN(app->client);

    DvzDevice* device = (DvzDevice*)calloc(1, sizeof(DvzDevice));

    device->gpu = make_gpu(app->host);
    device->rd = dvz_renderer(device->gpu, 0);
    device->prt = dvz_presenter(device->rd, app->client, 0);

    return device;
}



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



void vz_app_run(DvzApp* app, DvzScene* scene, uint64_t n_frames)
{
    ANN(app);
    ANN(scene);
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



void dvz_device_destroy(DvzDevice* device)
{
    ANN(device);

    dvz_presenter_destroy(device->prt);
    dvz_renderer_destroy(device->rd);
    dvz_gpu_destroy(device->gpu);

    FREE(device);
}



void dvz_app_destroy(DvzApp* app)
{
    ANN(app);
    dvz_client_destroy(app->client);
    dvz_host_destroy(app->host);
    FREE(app);
}
