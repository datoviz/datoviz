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
#include "../timer.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzApp DvzApp;

// Forward declarations.
typedef struct DvzHost DvzHost;
typedef struct DvzClient DvzClient;
typedef struct DvzGpu DvzGpu;
typedef struct DvzRenderer DvzRenderer;
typedef struct DvzRequester DvzRequester;
typedef struct DvzPresenter DvzPresenter;
typedef struct DvzTimer DvzTimer;
typedef struct DvzTimerItem DvzTimerItem;

// Callback types.
// typedef void (*DvzAppCallback)(DvzApp* app, DvzClientEvent ev);


/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzApp
{
    DvzHost* host;
    DvzClient* client;
    DvzGpu* gpu;
    DvzRenderer* rd;
    DvzPresenter* prt;
    DvzRequester* rqr;
    DvzTimer* timer;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DVZ_EXPORT DvzApp* dvz_app(void);

DVZ_EXPORT DvzRequester* dvz_app_requester(DvzApp* app);

DVZ_EXPORT void dvz_app_frame(DvzApp* app);

DVZ_EXPORT void dvz_app_mouse(DvzApp* app, DvzClientCallback on_mouse, void* user_data);

DVZ_EXPORT void dvz_app_keyboard(DvzApp* app, DvzClientCallback on_keyboard, void* user_data);

DVZ_EXPORT void dvz_app_resize(DvzApp* app, DvzClientCallback on_resize, void* user_data);

DVZ_EXPORT DvzTimerItem* dvz_app_timer(
    DvzApp* app, double delay, double period, uint64_t max_count, DvzTimerCallback on_timer,
    void* user_data);

DVZ_EXPORT void dvz_app_run(DvzApp* app, uint64_t n_frames);

DVZ_EXPORT void dvz_app_destroy(DvzApp* app);



EXTERN_C_OFF

#endif
