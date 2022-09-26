/*************************************************************************************************/
/*  App                                                                                          */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/app.h"
#include "../render_utils.h"
#include "client.h"
#include "host.h"
#include "presenter.h"
#include "renderer.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  App functions                                                                                */
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



void vz_app_run(DvzApp* app, DvzScene* scene, uint64_t n_frames)
{
    ANN(app);
    ANN(scene);
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
