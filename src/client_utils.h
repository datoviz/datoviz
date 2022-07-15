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



#endif
