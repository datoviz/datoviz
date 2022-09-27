/*************************************************************************************************/
/*  Testing animation                                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_animation.h"
#include "presenter.h"
#include "renderer.h"
#include "scene/animation.h"
#include "scene/app.h"
#include "scene/scene.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define EPSILON 1e-10



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct Anim Anim;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct Anim
{
    DvzRequester* rqr;
    DvzPresenter* prt;
    DvzId dat_id;
    DvzSize size;
    uint32_t n;
    void* data;
};



/*************************************************************************************************/
/*  Animation tests                                                                              */
/*************************************************************************************************/

int test_animation_1(TstSuite* suite)
{
    ANN(suite);
    double t = 0;
    for (int i = 0; i < (int)DVZ_EASING_COUNT; i++)
    {
        AC(dvz_easing((DvzEasing)i, t), 0, EPSILON);
        AC(dvz_easing((DvzEasing)i, 1), 1, EPSILON);
    }
    return 0;
}



static void _on_frame(DvzClient* client, DvzClientEvent ev)
{
    ANN(client);

    Anim* anim = (Anim*)ev.user_data;
    ANN(anim);

    DvzRequester* rqr = anim->rqr;
    ANN(rqr);

    DvzPresenter* prt = anim->prt;
    ANN(prt);

    DvzGraphicsPointVertex* data = (DvzGraphicsPointVertex*)anim->data;
    ANN(data);

    uint32_t n = anim->n;
    ASSERT(n > 0);

    const double dur = 2.0;
    double t = fmod(ev.content.f.time / dur, 1);
    for (uint32_t i = 0; i < n; i++)
    {
        data[i].pos[1] = .9 * (-1 + 2 * dvz_easing((DvzEasing)i, t));
    }

    DvzRequest req = dvz_upload_dat(rqr, anim->dat_id, 0, anim->size, data);
    dvz_requester_add(rqr, req);
    dvz_presenter_submit(prt, rqr);
}

int test_animation_2(TstSuite* suite)
{
    ANN(suite);

    // GPU-side.
    DvzHost* host = get_host(suite);

    DvzGpu* gpu = make_gpu(host);
    ANN(gpu);

    // Create a renderer.
    DvzRenderer* rd = dvz_renderer(gpu, 0);

    // Client-side.
    DvzClient* client = dvz_client(DVZ_BACKEND_GLFW);
    DvzRequester* rqr = dvz_requester();

    // Presenter linking the renderer and the client.
    DvzPresenter* prt = dvz_presenter(rd, client, 0);

    const uint32_t n = (uint32_t)DVZ_EASING_COUNT;
    GraphicsWrapper wrapper = {0};
    graphics_request(rqr, n, &wrapper);


    // Upload the data.
    DvzGraphicsPointVertex* data =
        (DvzGraphicsPointVertex*)calloc(n, sizeof(DvzGraphicsPointVertex));
    double t = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        t = i / (double)(n - 1);
        data[i].pos[0] = .9 * (-1 + 2 * t);
        data[i].pos[1] = 0;

        data[i].size = 50;

        dvz_colormap(DVZ_CMAP_HSV, TO_BYTE(t), data[i].color);
        data[i].color[3] = 128;
    }

    DvzSize size = n * sizeof(DvzGraphicsPointVertex);
    DvzRequest req = dvz_upload_dat(rqr, wrapper.dat_id, 0, size, data);
    dvz_requester_add(rqr, req);


    // Submit a client event with type REQUESTS and with a pointer to the requester.
    // The Presenter will register a REQUESTS callback sending the requests to the underlying
    // renderer.
    dvz_presenter_submit(prt, rqr);

    Anim anim = {
        .data = data, .rqr = rqr, .dat_id = wrapper.dat_id, .prt = prt, .n = n, .size = size};
    dvz_client_callback(
        client, DVZ_CLIENT_EVENT_FRAME, DVZ_CLIENT_CALLBACK_SYNC, _on_frame, &anim);


    // Dequeue and process all pending events.
    dvz_client_run(client, N_FRAMES);

    // End.

    // Destroying all objects.
    dvz_presenter_destroy(prt);

    dvz_client_destroy(client);
    dvz_requester_destroy(rqr);

    dvz_renderer_destroy(rd);
    dvz_gpu_destroy(gpu);

    FREE(data);
    return 0;
}
