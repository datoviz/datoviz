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
typedef struct DvzScene DvzScene;
typedef struct DvzClient DvzClient;
typedef struct DvzGpu DvzGpu;
typedef struct DvzRenderer DvzRenderer;
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
DVZ_EXPORT void dvz_device_run(DvzDevice* device, DvzScene* scene, uint64_t n_frames);

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
