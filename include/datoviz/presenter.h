/*************************************************************************************************/
/*  Presenter                                                                                    */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PRESENTER
#define DVZ_HEADER_PRESENTER



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_map.h"
#include "canvas.h"
#include "client.h"
#include "gui.h"
#include "renderer.h"
#include "request.h"
#include "resources.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_GUI_MAX_CALLBACKS 16



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzPresenter DvzPresenter;
typedef struct DvzGuiCallbackPayload DvzGuiCallbackPayload;

// Forward declarations.
// typedef struct DvzWindow DvzWindow;

// Callback types.
typedef void (*DvzGuiCallback)(DvzGuiWindow* gui_window, void* user_data);



/*************************************************************************************************/
/*  Event structs                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzGuiCallbackPayload
{
    DvzId window_id;
    DvzGuiCallback callback;
    void* user_data;
};



struct DvzPresenter
{
    DvzRenderer* rd;
    DvzClient* client;
    int flags;

    // GUI callbacks.
    DvzGui* gui;
    uint32_t callback_count;
    DvzGuiCallbackPayload callbacks[DVZ_GUI_MAX_CALLBACKS];

    // Mappings.
    struct
    {
        DvzMap* guis;
    } maps;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DVZ_EXPORT DvzPresenter* dvz_presenter(DvzRenderer* rd, DvzClient* client, int flags);



DVZ_EXPORT void dvz_presenter_frame(DvzPresenter* prt, DvzId window_id);



DVZ_EXPORT void
dvz_presenter_gui(DvzPresenter* prt, DvzId window_id, DvzGuiCallback callback, void* user_data);



DVZ_EXPORT void dvz_presenter_submit(DvzPresenter* prt, DvzRequester* rqr);



DVZ_EXPORT void dvz_presenter_destroy(DvzPresenter* prt);



EXTERN_C_OFF

#endif
