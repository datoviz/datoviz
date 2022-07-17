/*************************************************************************************************/
/*  Client                                                                                       */
/*************************************************************************************************/

#ifndef DVZ_HEADER_CLIENT
#define DVZ_HEADER_CLIENT



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "common.h"
#include "fifo.h"
#include "map.h"



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
    DVZ_CLIENT_EVENT_WINDOW_CREATE, // w
    DVZ_CLIENT_EVENT_WINDOW_RESIZE, // w
    DVZ_CLIENT_EVENT_WINDOW_DELETE, // w
    DVZ_CLIENT_EVENT_FRAME,         // f
    DVZ_CLIENT_EVENT_MOUSE,         // m
    DVZ_CLIENT_EVENT_KEYBOARD,      // k
    DVZ_CLIENT_EVENT_TIMER,         // t
    DVZ_CLIENT_EVENT_REQUESTS,      // r
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

typedef uint64_t DvzId;

// Callback types.
typedef void (*DvzClientCallback)(DvzClient* client, DvzClientEvent ev, void* user_data);



/*************************************************************************************************/
/*  Event structs                                                                                */
/*************************************************************************************************/

struct DvzClientEvent
{
    DvzClientEventType type;
    DvzId window_id;
    union
    {
        struct
        {
            uint32_t width;
            uint32_t height;
            int flags;
        } w;

        struct
        {
            uint64_t frame_idx;
        } f;

        struct
        {
            uint32_t request_count;
            void* requests;
        } r;
    } content;
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

    DvzDeq deq;
    uint64_t frame_idx;

    // Windows.
    DvzContainer windows;
    DvzMap* map;

    // Callbacks.
    uint32_t callback_count;
    DvzClientPayload callbacks[DVZ_CLIENT_MAX_CALLBACKS];
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
DVZ_EXPORT DvzClient* dvz_client(DvzBackend backend);



void dvz_client_event(DvzClient* client, DvzClientEvent ev);



void dvz_client_callback(
    DvzClient* client, DvzClientEventType type, DvzClientCallbackMode mode,
    DvzClientCallback callback, void* user_data);



void dvz_client_process(DvzClient* client);



int dvz_client_frame(DvzClient* client);



void dvz_client_run(DvzClient* client, uint64_t n_frames);



void dvz_client_destroy(DvzClient* client);



EXTERN_C_OFF

#endif
