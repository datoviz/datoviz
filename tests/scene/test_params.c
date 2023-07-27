/*************************************************************************************************/
/*  Testing params                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/test_params.h"
#include "request.h"
#include "scene/params.h"
#include "test.h"
#include "testing.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct TestParams TestParams;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct TestParams
{
    char a;
    uint32_t b;
};



/*************************************************************************************************/
/*  Params tests */
/*************************************************************************************************/

int test_params_1(TstSuite* suite)
{
    DvzRequester* rqr = dvz_requester();
    dvz_requester_begin(rqr);

    DvzParams* params = dvz_params(rqr, sizeof(TestParams));
    dvz_params_attr(params, 0, offsetof(TestParams, a), sizeof(char));
    dvz_params_attr(params, 1, offsetof(TestParams, b), sizeof(uint32_t));

    // Bind the dat to a graphics.
    DvzId graphics_id = 10;
    dvz_params_bind(params, graphics_id, 1);

    // Set some data.
    char a = 42;
    uint32_t b = 2048;
    TestParams p = {0};
    p.a = a;
    p.b = b;
    dvz_params_set(params, 0, &a);
    dvz_params_set(params, 1, &b);

    AT(rqr->count == 2);

    AT(rqr->requests[0].action == DVZ_REQUEST_ACTION_CREATE);
    AT(rqr->requests[0].type == DVZ_REQUEST_OBJECT_DAT);
    AT(rqr->requests[0].content.dat.size == sizeof(TestParams));

    AT(rqr->requests[1].action == DVZ_REQUEST_ACTION_BIND);
    AT(rqr->requests[1].type == DVZ_REQUEST_OBJECT_DAT);
    AT(rqr->requests[1].content.bind_dat.slot_idx == 1);

    // Update the dat dual.
    dvz_params_update(params);
    AT(rqr->count == 3);

    // Check that the data upload request is correct.
    // dvz_show_buffer(4, 8, sizeof(TestParams), rqr->requests[2].content.dat_upload.data);
    // dvz_show_buffer(4, 8, sizeof(TestParams), &p);
    // dvz_requester_print(rqr);

    AT(rqr->requests[2].content.dat_upload.size == sizeof(TestParams));
    AT(memcmp(rqr->requests[2].content.dat_upload.data, &p, sizeof(TestParams)) == 0);

    // Check direct upload of the dat's data.
    dvz_params_data(params, &p);
    AT(rqr->count == 4);
    AT(memcmp(rqr->requests[3].content.dat_upload.data, &p, sizeof(TestParams)) == 0);

    dvz_requester_destroy(rqr);
    return 0;
}
