/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

/*************************************************************************************************/
/*  Presenter                                                                                    */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PRESENTER
#define DVZ_HEADER_PRESENTER



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "canvas.h"
#include "client.h"
#include "fps.h"
#include "gui.h"
#include "renderer.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzPresenter DvzPresenter;
typedef struct DvzGuiCallbackPayload DvzGuiCallbackPayload;

// Forward declarations.
typedef struct DvzList DvzList;

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

    // HACK: need to keep a list of surfaces so that we can delete them when the presenter is
    // destroyed.
    DvzList* surfaces;

    // NOTE: this is used to keep track of the state of sem_img_available, true: image has been
    // acquired and the semaphore will be unsignaled when a command buffer is submitted. If that
    // does not happen (for example while resizing), then no further swapchain acquisition should
    // occur until a command buffer has been submitted.
    bool awaiting_submit;

    // GUI callbacks.
    DvzGui* gui;
    DvzList* callbacks;

    DvzFps fps;

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

DvzPresenter* dvz_presenter(DvzRenderer* rd, DvzClient* client, int flags);



void dvz_presenter_frame(DvzPresenter* prt, DvzId window_id);



void dvz_presenter_gui(
    DvzPresenter* prt, DvzId window_id, DvzGuiCallback callback, void* user_data);



void dvz_presenter_submit(DvzPresenter* prt, DvzBatch* batch);



void dvz_presenter_destroy(DvzPresenter* prt);



EXTERN_C_OFF

#endif
