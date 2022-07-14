/*************************************************************************************************/
/*  Testing client                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_client.h"
#include "_glfw.h"
#include "client.h"
#include "test.h"
#include "testing.h"



/*************************************************************************************************/
/*  Client tests                                                                                 */
/*************************************************************************************************/

int test_client_1(TstSuite* suite)
{
    DvzClient* client = dvz_client();
    dvz_client_destroy(client);
    return 0;
}