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

    // Vertex attributes:
    // attr0: 1 // binding 0
    // attr1: 2 // binding 0
    // attr2: 4 // binding 1

    // Uniforms:
    // slot0: 5 // uniform 0
    // slot1: 6 // uniform 1

    DvzBaker* baker = dvz_baker(rqr, 0);

    // Declare the vertex bindings and attributes.
    dvz_baker_vertex(baker, 0, 3);
    dvz_baker_vertex(baker, 1, 4);

    dvz_baker_attr(baker, 0, 0, 0, 1); // attrO
    dvz_baker_attr(baker, 1, 0, 1, 2); // attr1
    dvz_baker_attr(baker, 2, 1, 0, 4); // attr2

    // Declare the descriptor slots.
    dvz_baker_slot(baker, 0, 5);
    dvz_baker_slot(baker, 1, 6);

    // Create the arrays and emit the dat creation requests.
    uint32_t count = 2;
    dvz_baker_create(baker, 0, count);

    // Check the dat creations.
    {
        AT(rqr->count == 4)

        AT(rqr->requests[0].content.dat.size == 3 * count);
        AT(rqr->requests[0].content.dat.type == DVZ_BUFFER_TYPE_VERTEX);

        AT(rqr->requests[1].content.dat.size == 4 * count);
        AT(rqr->requests[1].content.dat.type == DVZ_BUFFER_TYPE_VERTEX);

        AT(rqr->requests[2].content.dat.size == 5);
        AT(rqr->requests[2].content.dat.type == DVZ_BUFFER_TYPE_UNIFORM);

        AT(rqr->requests[3].content.dat.size == 6);
        AT(rqr->requests[3].content.dat.type == DVZ_BUFFER_TYPE_UNIFORM);
    }


    // Set some data.
    {
        // Vertex buffer:
        // bind0 | bind1
        // 1,2 2 | 4 4 8 8
        // 2,4 4 | 4 4 8 8

        // Uniforms:
        // dat0: 5 5 5 5 5
        // dat1: 6 6 6 6 6 6

        char data[6] = {0};

        // attr0
        data[0] = 1;
        data[1] = 2;
        dvz_baker_data(baker, 0, 0, 2, data);

        // attr1
        data[0] = 2;
        data[1] = 2;
        data[2] = 4;
        data[3] = 4;
        dvz_baker_data(baker, 1, 0, 2, data);

        // attr2
        data[0] = 4;
        data[1] = 4;
        data[2] = 8;
        data[3] = 8;
        dvz_baker_repeat(baker, 2, 0, 1, 2, data);

        // uniform0
        memset(data, 5, 5);
        dvz_baker_uniform(baker, 0, 5, data);

        // uniform1
        memset(data, 6, 6);
        dvz_baker_uniform(baker, 1, 6, data);
    }

    // Trigger the dat uploads.
    dvz_baker_update(baker);

    // Show the dual data.
    IF_VERBOSE
    {
        dvz_requester_print(rqr);

        dvz_show_buffer(3, 3, 6, baker->vertex_bindings[0].dual.array->data);
        dvz_show_buffer(2, 4, 8, baker->vertex_bindings[1].dual.array->data);
        dvz_show_buffer(5, 5, 5, baker->descriptors[0].dual.array->data);
        dvz_show_buffer(6, 6, 6, baker->descriptors[1].dual.array->data);
    }

    // Check the dat uploads.
    for (uint32_t i = 4; i < 8; i++)
    {
        AT(rqr->requests[i].action == DVZ_REQUEST_ACTION_UPLOAD);
        AT(rqr->requests[i].type == DVZ_REQUEST_OBJECT_DAT);
    }

    // buffer with size 6 bytes:
    // +-------+
    // | 1 2 2 |
    // | 2 4 4 |
    // +-------+
    AT(rqr->requests[4].content.dat_upload.size == 6);
    AT(memcmp(rqr->requests[4].content.dat_upload.data, (char[]){1, 2, 2, 2, 4, 4}, 6) == 0);

    // buffer with size 8 bytes:
    // +-----+-----+
    // | 4 4 | 8 8 |
    // | 4 4 | 8 8 |
    // +-----+-----+
    AT(rqr->requests[5].content.dat_upload.size == 8);
    AT(memcmp(rqr->requests[5].content.dat_upload.data, (char[]){4, 4, 8, 8, 4, 4, 8, 8}, 8) == 0);

    // buffer with size 5 bytes:
    // +-----------+
    // | 5 5 5 5 5 |
    // +-----------+
    AT(rqr->requests[6].content.dat_upload.size == 5);
    AT(memcmp(rqr->requests[6].content.dat_upload.data, (char[]){5, 5, 5, 5, 5}, 5) == 0);

    // buffer with size 6 bytes:
    // +-------------+
    // | 6 6 6 6 6 6 |
    // +-------------+
    AT(rqr->requests[7].content.dat_upload.size == 6);
    AT(memcmp(rqr->requests[7].content.dat_upload.data, (char[]){6, 6, 6, 6, 6, 6}, 6) == 0);

    // Cleanup.
    dvz_baker_destroy(baker);
    dvz_requester_destroy(rqr);
    return 0;
}
