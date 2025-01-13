/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Client                                                                                       */
/*************************************************************************************************/

#ifndef DVZ_HEADER_CLIENT
#define DVZ_HEADER_CLIENT



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_atomic.h"
#include "_enums.h"
#include "_map.h"
#include "_obj.h"
#include "_thread_utils.h"
#include "_time_utils.h"
#include "datoviz_types.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_CLIENT_MAX_CALLBACKS 16



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_CLIENT_EVENT_NONE,
    DVZ_CLIENT_EVENT_INIT,
    DVZ_CLIENT_EVENT_WINDOW_CREATE, // w
    DVZ_CLIENT_EVENT_WINDOW_RESIZE, // w
    DVZ_CLIENT_EVENT_WINDOW_DELETE, // w
    // DVZ_CLIENT_EVENT_WINDOW_REQUEST_DELETE, // w
    DVZ_CLIENT_EVENT_FRAME,    // f
    DVZ_CLIENT_EVENT_MOUSE,    // m
    DVZ_CLIENT_EVENT_KEYBOARD, // k
    DVZ_CLIENT_EVENT_TIMER,    // t
    DVZ_CLIENT_EVENT_REQUESTS, // r
    DVZ_CLIENT_EVENT_DESTROY,
} DvzClientEventType;



typedef enum
{
    DVZ_CLIENT_CALLBACK_SYNC,
    DVZ_CLIENT_CALLBACK_ASYNC,
} DvzClientCallbackMode;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzClient DvzClient;
typedef struct DvzClientPayload DvzClientPayload;
typedef struct DvzClientEvent DvzClientEvent;

typedef union DvzClientEventUnion DvzClientEventUnion;

// Forward declarations.
typedef uint64_t DvzId;
typedef struct DvzDeq DvzDeq;
typedef struct DvzBatch DvzBatch;
typedef struct DvzTimerItem DvzTimerItem;

// Callback types.
typedef void (*DvzClientCallback)(DvzClient* client, DvzClientEvent ev);



/*************************************************************************************************/
/*  Event structs                                                                                */
/*************************************************************************************************/

union DvzClientEventUnion
{
    // Window.
    DvzWindowEvent w;

    // Frame.
    DvzFrameEvent f;

    // Timer.
    DvzTimerEvent t;

    // Requests.
    DvzRequestsEvent r;

    // Mouse.
    DvzMouseEvent m;

    // Keyboard.
    DvzKeyboardEvent k;
};

struct DvzClientEvent
{
    DvzClientEventType type;
    DvzId window_id;
    DvzClientEventUnion content;
    float content_scale; // ratio between framebuffer width and screen width
    void* user_data;
};



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzClientPayload
{
    DvzClient* client;
    DvzClientCallback callback;
    DvzClientCallbackMode mode;
    void* user_data;
};



struct DvzClient
{
    DvzBackend backend;

    DvzDeq* deq;
    uint64_t frame_idx, n_frames;

    DvzClock clock;

    // Windows.
    DvzContainer windows;
    DvzMap* map;

    // Callbacks.
    uint32_t callback_count;
    DvzClientPayload callbacks[DVZ_CLIENT_MAX_CALLBACKS];

    void* user_data;
    DvzThread* thread;
    DvzAtomic to_stop;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Client functions                                                                             */
/*************************************************************************************************/

/**
 * Create a client.
 *
 * @returns a client pointer
 */
DvzClient* dvz_client(DvzBackend backend);



void dvz_client_event(DvzClient* client, DvzClientEvent ev);



void dvz_client_callback(
    DvzClient* client, DvzClientEventType type, DvzClientCallbackMode mode,
    DvzClientCallback callback, void* user_data);



void dvz_client_process(DvzClient* client);



int dvz_client_frame(DvzClient* client);



void dvz_client_run(DvzClient* client, uint64_t n_frames);



void dvz_client_thread(DvzClient* client, uint64_t n_frames);



void dvz_client_join(DvzClient* client);



void dvz_client_stop(DvzClient* client);



void dvz_client_destroy(DvzClient* client);



EXTERN_C_OFF

#endif
