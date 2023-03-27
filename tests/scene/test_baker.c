/*************************************************************************************************/
/*  Testing baker                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/test_baker.h"
#include "request.h"
#include "scene/array.h"
#include "scene/baker.h"
#include "scene/dual.h"
#include "test.h"
#include "testing.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Baker tests                                                                                  */
/*************************************************************************************************/

int test_baker_1(TstSuite* suite)
{
    DvzRequester* rqr = dvz_requester();
    dvz_requester_begin(rqr);

    // attr0: 10 // binding 0
    // attr1: 20 // binding 0
    // attr2: 40 // binding 1
    // slot0: 50
    // slot1: 60

    DvzBaker* baker = dvz_baker(rqr, 0);

    // Declare the vertex bindings and attributes.
    dvz_baker_vertex(baker, 0, 30);
    dvz_baker_vertex(baker, 1, 40);

    dvz_baker_attr(baker, 0, 0, 0, 10);
    dvz_baker_attr(baker, 1, 0, 0, 20);
    dvz_baker_attr(baker, 2, 1, 0, 40);

    // Declare the descriptor slots.
    dvz_baker_slot(baker, 0, 50);
    dvz_baker_slot(baker, 1, 60);

    // Create the arrays and emit the dat creation requests.
    dvz_baker_duals(baker, 1);

    // Check the requests.
    // dvz_requester_print(rqr);

    AT(rqr->count == 4)

    AT(rqr->requests[0].content.dat.size == 30);
    AT(rqr->requests[0].content.dat.type == DVZ_BUFFER_TYPE_VERTEX);

    AT(rqr->requests[1].content.dat.size == 40);
    AT(rqr->requests[1].content.dat.type == DVZ_BUFFER_TYPE_VERTEX);

    AT(rqr->requests[2].content.dat.size == 50);
    AT(rqr->requests[2].content.dat.type == DVZ_BUFFER_TYPE_UNIFORM);

    AT(rqr->requests[3].content.dat.size == 60);
    AT(rqr->requests[3].content.dat.type == DVZ_BUFFER_TYPE_UNIFORM);

    // Cleanup.
    dvz_baker_destroy(baker);
    dvz_requester_destroy(rqr);
    return 0;
}
