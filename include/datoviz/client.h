/*************************************************************************************************/
/*  Input                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_CLIENT
#define DVZ_HEADER_CLIENT



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "common.h"
#include "fifo.h"



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
    DVZ_CLIENT_EVENT_CANVAS_CREATE,
    DVZ_CLIENT_EVENT_CANVAS_RESIZE,
    DVZ_CLIENT_EVENT_CANVAS_DELETE,
    DVZ_CLIENT_EVENT_FRAME,
    DVZ_CLIENT_EVENT_MOUSE,
    DVZ_CLIENT_EVENT_KEYBOARD,
    DVZ_CLIENT_EVENT_TIMER,
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
typedef void (*DvzClientCallback)(DvzClient*, DvzClientEvent, void*);



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
            DvzId id;
        } c;
    } u;
};



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzClientPayload
{
    DvzClientCallback callback;
    void* user_data;
};



struct DvzClient
{
    DvzDeq deq;

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
DVZ_EXPORT DvzClient* dvz_client(void);



void dvz_client_window(DvzClient* client, DvzId id, uint32_t width, uint32_t height, int flags);



void dvz_client_event(DvzClient* client, DvzClientEvent ev);



void dvz_client_callback(
    DvzClient* client, DvzClientEventType type, DvzClientCallbackMode mode,
    DvzClientCallback callback, const void* user_data);



void dvz_client_frame(DvzClient* client);



void dvz_client_run(DvzClient* client, uint64_t n_frames);



void dvz_client_destroy(DvzClient* client);



EXTERN_C_OFF

#endif
