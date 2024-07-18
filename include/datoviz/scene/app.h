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
#include "../client.h"
#include "../gui.h"
#include "../presenter.h"
#include "../timer.h"
#include "datoviz_math.h"



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
    DvzAppGuiCallback callback;
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
    DvzList* payloads;
    bool is_running;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/



EXTERN_C_OFF

#endif
