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



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzApp DvzApp;
typedef struct DvzDevice DvzDevice;

// Forward declarations.
typedef struct DvzHost DvzHost;
typedef struct DvzClient DvzClient;
typedef struct DvzGpu DvzGpu;
typedef struct DvzRenderer DvzRenderer;
typedef struct DvzRequester DvzRequester;
typedef struct DvzPresenter DvzPresenter;
typedef struct DvzList DvzList;



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
};



struct DvzDevice // depends on DvzApp
{
    DvzApp* app;
    DvzGpu* gpu;
    DvzRenderer* rd;
    DvzPresenter* prt;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzApp* dvz_app(DvzBackend backend);

/**
 *
 */
DVZ_EXPORT DvzDevice* dvz_device(DvzApp* app);

/**
 *
 */
DVZ_EXPORT void dvz_device_frame(DvzDevice* device, DvzRequester* rqr);

/**
 *
 */
DVZ_EXPORT void dvz_device_run(DvzDevice* device, DvzRequester* rqr, uint64_t n_frames);

/**
 * NOTE: does not appear to work on macOS (it requires the swapchain commands to be emitted from
 * the main thread)
 */
DVZ_EXPORT void dvz_device_async(DvzDevice* device, DvzRequester* rqr, uint64_t n_frames);

/**
 *
 */
DVZ_EXPORT void dvz_device_wait(DvzDevice* device);

/**
 *
 */
DVZ_EXPORT void dvz_device_stop(DvzDevice* device);

/**
 *
 */
DVZ_EXPORT void dvz_device_update(DvzDevice* device, DvzRequester* rqr);

/**
 *
 */
DVZ_EXPORT void dvz_device_destroy(DvzDevice* device);

/**
 *
 */
DVZ_EXPORT void dvz_app_destroy(DvzApp* app);



EXTERN_C_OFF

#endif