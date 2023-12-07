/*************************************************************************************************/
/* App                                                                                           */
/*************************************************************************************************/

#ifndef DVZ_HEADER_APP
#define DVZ_HEADER_APP



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../_enums.h"
#include "../_log.h"
#include "../_math.h"
#include "../client.h"
#include "../gui.h"
#include "../presenter.h"
#include "../timer.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzApp DvzApp;
typedef struct DvzAppGuiPayload DvzAppGuiPayload;

// Forward declarations.
typedef struct DvzHost DvzHost;
typedef struct DvzClient DvzClient;
typedef struct DvzList DvzList;
typedef struct DvzGpu DvzGpu;
typedef struct DvzRenderer DvzRenderer;
typedef struct DvzBatch DvzBatch;
typedef struct DvzPresenter DvzPresenter;
typedef struct DvzTimer DvzTimer;
typedef struct DvzTimerItem DvzTimerItem;

// Callback types.
typedef void (*DvzAppGui)(DvzApp* app, DvzId canvas_id, void* user_data);



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzAppGuiPayload
{
    DvzApp* app;
    DvzId canvas_id;
    DvzAppGui callback;
    void* user_data;
};



struct DvzApp
{
    DvzHost* host;
    DvzClient* client;
    DvzGpu* gpu;
    DvzRenderer* rd;
    DvzPresenter* prt;
    DvzBatch* batch;
    DvzTimer* timer;
    DvzList* callbacks;
    bool is_running;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/
/**
 *
 */
DVZ_EXPORT DvzApp* dvz_app(int flags);



/**
 *
 */
DVZ_EXPORT DvzBatch* dvz_app_batch(DvzApp* app);



/**
 *
 */
DVZ_EXPORT void dvz_app_frame(DvzApp* app);



/**
 *
 */
DVZ_EXPORT void dvz_app_onframe(DvzApp* app, DvzClientCallback on_frame, void* user_data);



/**
 *
 */
DVZ_EXPORT void dvz_app_onmouse(DvzApp* app, DvzClientCallback on_mouse, void* user_data);



/**
 *
 */
DVZ_EXPORT void dvz_app_onkeyboard(DvzApp* app, DvzClientCallback on_keyboard, void* user_data);



/**
 *
 */
DVZ_EXPORT void dvz_app_onresize(DvzApp* app, DvzClientCallback on_resize, void* user_data);



/**
 *
 */
DVZ_EXPORT DvzTimerItem*
dvz_app_timer(DvzApp* app, double delay, double period, uint64_t max_count);



/**
 *
 */
DVZ_EXPORT void dvz_app_ontimer(DvzApp* app, DvzClientCallback on_timer, void* user_data);



/**
 *
 */
DVZ_EXPORT void dvz_app_gui(DvzApp* app, DvzId canvas_id, DvzAppGui callback, void* user_data);



/**
 *
 */
DVZ_EXPORT void dvz_app_run(DvzApp* app, uint64_t n_frames);



/**
 *
 */
DVZ_EXPORT void dvz_app_submit(DvzApp* app);



/**
 *
 */
DVZ_EXPORT void dvz_app_screenshot(DvzApp* app, DvzId canvas_id, const char* filename);



/**
 *
 */
DVZ_EXPORT void dvz_app_destroy(DvzApp* app);



EXTERN_C_OFF

#endif
