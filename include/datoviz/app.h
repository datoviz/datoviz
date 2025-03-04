/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* App                                                                                           */
/*************************************************************************************************/

#ifndef DVZ_HEADER_APP
#define DVZ_HEADER_APP



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_enums.h"
#include "_log.h"
#include "client.h"
#include "datoviz_math.h"
#include "gui.h"
#include "presenter.h"
#include "timer.h"



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
    DvzBackend backend;
    DvzHost* host;
    DvzClient* client;
    DvzGpu* gpu;
    DvzRenderer* rd;
    DvzPresenter* prt;
    DvzBatch* batch;
    DvzTimer* timer;
    DvzList* payloads;
    bool is_running;

    // Offscreen GUI.
    DvzGui* offscreen_gui;
    DvzMap* offscreen_guis;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/



EXTERN_C_OFF

#endif
