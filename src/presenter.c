/*************************************************************************************************/
/*  Presenter                                                                                    */
/*************************************************************************************************/

#include "../include/datoviz/presenter.h"
#include "../include/datoviz/canvas.h"
#include "../include/datoviz/map.h"
#include "../include/datoviz/vklite.h"
#include "canvas_utils.h"
#include "vklite_utils.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

static void _frame_callback(DvzClient* client, DvzClientEvent ev, void* user_data)
{
    ASSERT(client != NULL);
    ASSERT(user_data != NULL);
    DvzPresenter* prt = (DvzPresenter*)user_data;
    dvz_presenter_frame(prt);
}



/*************************************************************************************************/
/*  Presenter                                                                                    */
/*************************************************************************************************/

DvzPresenter* dvz_presenter(DvzRenderer* rnd)
{
    ASSERT(rnd != NULL);
    DvzPresenter* prt = calloc(1, sizeof(DvzPresenter));
    prt->rnd = rnd;
    return prt;
}



void dvz_presenter_frame(DvzPresenter* prt)
{
    ASSERT(prt != NULL);

    DvzClient* client = prt->client;
    ASSERT(client != NULL);

    uint64_t frame_idx = client->frame_idx;
    log_debug("frame %d", frame_idx);

    // flush the renderer queue, process the rendering commands, swapchain logic, submit the
    // command buffer

    // go through the pending requests in the requester
    // submit them to the renderer
    // special handling of canvas requests
    //     handle them also in the client
    //     canvas creation
    //         create a window with the client
    //         window <-> canvas pointer references

    // do the swapchain logic
    //     go through the client windows
    //         get the associated canvas in DvzWindow.canvas void* pointer
    //         acquire swapchain, recreate canvas if out of date
    //         if resized, issue RESIZE command to the renderer
    //         the user may have registered RESIZE callbacks in the client that will submit
    //         specific requests if window to close, issue DELETE canvas command to the renderer

    // need to go through the pending requests again in the requester (eg those raise in the RESIZE
    // callbacks)
    // UPFILL: when there is a command refill + data uploads in the same batch, register
    // the cmd buf at the moment when the GPU-blocking upload really occurs
}



void dvz_presenter_client(DvzPresenter* prt, DvzClient* client)
{
    ASSERT(prt != NULL);
    ASSERT(client != NULL);

    prt->client = client;

    // Register a FRAME callback which calls dvz_presenter_frame().
    dvz_client_callback(
        client, DVZ_CLIENT_EVENT_FRAME, DVZ_CLIENT_CALLBACK_SYNC, _frame_callback, prt);
}



void dvz_presenter_destroy(DvzPresenter* prt)
{
    ASSERT(prt != NULL);
    FREE(prt);
}
