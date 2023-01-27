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
#include "_input.h"
#include "_map.h"
#include "_obj.h"
#include "_thread.h"
#include "_time.h"



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
    // DVZ_CLIENT_EVENT_WINDOW_REQUEST_DELETE, // w
    DVZ_CLIENT_EVENT_FRAME,    // f
    DVZ_CLIENT_EVENT_MOUSE,    // m
    DVZ_CLIENT_EVENT_KEYBOARD, // k
    DVZ_CLIENT_EVENT_TIMER,    // t
    DVZ_CLIENT_EVENT_REQUESTS, // r
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

// Forward declarations.
typedef uint64_t DvzId;
typedef struct DvzDeq DvzDeq;

// Callback types.
typedef void (*DvzClientCallback)(DvzClient* client, DvzClientEvent ev);



/*************************************************************************************************/
/*  Event structs                                                                                */
/*************************************************************************************************/

struct DvzClientEvent
{
    DvzClientEventType type;
    DvzId window_id;
    union
    {
        // Window.
        struct
        {
            uint32_t framebuffer_width;
            uint32_t framebuffer_height;
            uint32_t screen_width;
            uint32_t screen_height;
            int flags;
        } w;

        // Frame.
        struct
        {
            uint64_t frame_idx;
            double time;
        } f;

        // Requests.
        struct
        {
            uint32_t request_count;
            void* requests;
        } r;

        // Mouse.
        DvzMouseEvent m;

        // Keyboard.
        DvzKeyboardEvent k;
    } content;
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
DVZ_EXPORT DvzClient* dvz_client(DvzBackend backend);



DVZ_EXPORT void dvz_client_event(DvzClient* client, DvzClientEvent ev);



DVZ_EXPORT void dvz_client_callback(
    DvzClient* client, DvzClientEventType type, DvzClientCallbackMode mode,
    DvzClientCallback callback, void* user_data);



DVZ_EXPORT void dvz_client_process(DvzClient* client);



DVZ_EXPORT int dvz_client_frame(DvzClient* client);



DVZ_EXPORT void dvz_client_run(DvzClient* client, uint64_t n_frames);



DVZ_EXPORT void dvz_client_thread(DvzClient* client, uint64_t n_frames);



DVZ_EXPORT void dvz_client_join(DvzClient* client);



DVZ_EXPORT void dvz_client_stop(DvzClient* client);



void dvz_client_destroy(DvzClient* client);



EXTERN_C_OFF

#endif
