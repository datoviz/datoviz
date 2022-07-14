/*************************************************************************************************/
/*  Testing window                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdio.h>

#include "host.h"
#include "test_window.h"
#include "testing.h"
#include "window.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_window_1(TstSuite* suite)
{
    ASSERT(suite != NULL);

    DvzHost* host = dvz_host(DVZ_BACKEND_GLFW);
    DvzWindow* window = dvz_window(host, 100, 100);
    AT(window != NULL);
    if (window->host != NULL)
        AT(window->host == host);

    DvzWindow* window2 = dvz_window(host, 100, 100);
    AT(window2 != NULL);
    if (window2->host != NULL)
        AT(window2->host == host);

    dvz_host_destroy(host);
    return 0;
}
