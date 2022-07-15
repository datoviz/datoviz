/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_CLIENT_UTILS
#define DVZ_HEADER_CLIENT_UTILS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_glfw.h"
#include "client.h"
#include "common.h"
#include "window.h"



/*************************************************************************************************/
/*  Client utils                                                                                 */
/*************************************************************************************************/

static DvzWindow* id2window(DvzClient* client, DvzId id)
{
    ASSERT(client != NULL);
    DvzWindow* window = dvz_map_get(client->map, id);
    return window;
}



static DvzId window2id(DvzWindow* window)
{
    ASSERT(window != NULL);
    return (DvzId)window->obj.id;
}



static DvzWindow*
create_client_window(DvzClient* client, DvzId id, uint32_t width, uint32_t height, int flags)
{
    ASSERT(client != NULL);
    DvzWindow* window = dvz_container_alloc(&client->windows);
    *window = dvz_window(client->backend, width, height, flags);

    // Register the window id.
    window->obj.id = (uint64_t)id;
    dvz_map_add(client->map, id, DVZ_OBJECT_TYPE_WINDOW, window);

    return window;
}



#endif
