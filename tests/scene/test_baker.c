/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing baker                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/test_baker.h"
#include "datoviz_protocol.h"
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

int test_baker_1(TstSuite* suite, TstItem* tstitem)
{
    DvzBatch* batch = dvz_batch();

    // Vertex attributes:
    // attr0: 1 // binding 0
    // attr1: 2 // binding 0
    // attr2: 4 // binding 1

    // Uniforms:
    // slot0: 5 // uniform 0
    // slot1: 6 // uniform 1

    DvzBaker* baker = dvz_baker(batch, 0);

    // Declare the vertex bindings and attributes.
    dvz_baker_vertex(baker, 0, 3);
    dvz_baker_vertex(baker, 1, 4);

    dvz_baker_attr(baker, 0, 0, 0, 1); // attrO
    dvz_baker_attr(baker, 1, 0, 1, 2); // attr1
    dvz_baker_attr(baker, 2, 1, 0, 4); // attr2

    // Declare the descriptor dslots.
    // dvz_baker_slot_dat(baker, 0, 5);
    // dvz_baker_slot_dat(baker, 1, 6);

    // Create the arrays and emit the dat creation requests.
    uint32_t count = 2;
    dvz_baker_create(baker, 0, count);

    // Check the dat creations.
    {
        AT(batch->count == 2)

        AT(batch->requests[0].content.dat.size == 3 * count);
        AT(batch->requests[0].content.dat.type == DVZ_BUFFER_TYPE_VERTEX);

        AT(batch->requests[1].content.dat.size == 4 * count);
        AT(batch->requests[1].content.dat.type == DVZ_BUFFER_TYPE_VERTEX);

        // AT(batch->requests[2].content.dat.size == 5);
        // AT(batch->requests[2].content.dat.type == DVZ_BUFFER_TYPE_UNIFORM);

        // AT(batch->requests[3].content.dat.size == 6);
        // AT(batch->requests[3].content.dat.type == DVZ_BUFFER_TYPE_UNIFORM);
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

        // // uniform0
        // memset(data, 5, 5);
        // dvz_baker_uniform(baker, 0, 5, data);

        // // uniform1
        // memset(data, 6, 6);
        // dvz_baker_uniform(baker, 1, 6, data);
    }

    // Trigger the dat uploads.
    dvz_baker_update(baker);

    // Show the dual data.
    IF_VERBOSE
    {
        // dvz_requester_print(batch);

        dvz_show_buffer(3, 3, 6, baker->vertex_bindings[0].dual.array->data);
        dvz_show_buffer(2, 4, 8, baker->vertex_bindings[1].dual.array->data);
        // dvz_show_buffer(5, 5, 5, baker->descriptors[0].u.dat.dual.array->data);
        // dvz_show_buffer(6, 6, 6, baker->descriptors[1].u.dat.dual.array->data);
    }

    // Check the dat uploads.
    for (uint32_t i = 2; i < 4; i++)
    {
        AT(batch->requests[i].action == DVZ_REQUEST_ACTION_UPLOAD);
        AT(batch->requests[i].type == DVZ_REQUEST_OBJECT_DAT);
    }

    // buffer with size 6 bytes:
    // +-------+
    // | 1 2 2 |
    // | 2 4 4 |
    // +-------+
    AT(batch->requests[2].content.dat_upload.size == 6);
    AT(memcmp(batch->requests[2].content.dat_upload.data, (char[]){1, 2, 2, 2, 4, 4}, 6) == 0);

    // buffer with size 8 bytes:
    // +-----+-----+
    // | 4 4 | 8 8 |
    // | 4 4 | 8 8 |
    // +-----+-----+
    AT(batch->requests[3].content.dat_upload.size == 8);
    AT(memcmp(batch->requests[3].content.dat_upload.data, (char[]){4, 4, 8, 8, 4, 4, 8, 8}, 8) ==
       0);

    // buffer with size 5 bytes:
    // +-----------+
    // | 5 5 5 5 5 |
    // +-----------+
    // AT(batch->requests[6].content.dat_upload.size == 5);
    // AT(memcmp(batch->requests[6].content.dat_upload.data, (char[]){5, 5, 5, 5, 5}, 5) == 0);

    // buffer with size 6 bytes:
    // +-------------+
    // | 6 6 6 6 6 6 |
    // +-------------+
    // AT(batch->requests[7].content.dat_upload.size == 6);
    // AT(memcmp(batch->requests[7].content.dat_upload.data, (char[]){6, 6, 6, 6, 6, 6}, 6) == 0);

    // Cleanup.
    dvz_baker_destroy(baker);
    dvz_batch_destroy(batch);
    return 0;
}



int test_baker_2(TstSuite* suite, TstItem* tstitem)
{
    // DvzBatch* batch = dvz_requester();
    // dvz_requester_begin(batch);

    // // Declare a descriptor slot.
    // DvzBaker* baker = dvz_baker(batch, 0);
    // dvz_baker_slot_dat(baker, 0, 1);

    // // Create a dual dat manually.
    // DvzDual dual = dvz_dual_dat(batch, 1, 0);

    // // Use it as baker's dat.
    // dvz_baker_share_binding(baker, 0);
    // baker->descriptors[0].u.dat.dual = dual;

    // // Create the baker.
    // dvz_baker_create(baker, 0, 1);

    // // Set some data with the baker.
    // uint8_t data = 42;
    // dvz_baker_uniform(baker, 0, 1, &data);

    // AT(*((uint8_t*)dvz_array_item(dual.array, 0)) == 42);

    // // Destroy the objects.
    // dvz_baker_destroy(baker);
    // dvz_dual_destroy(&dual);
    // dvz_requester_destroy(batch);
    return 0;
}



// int test_baker_3(TstSuite* suite, TstItem* tstitem)
// {
//     DvzBatch* batch = dvz_requester();
//     dvz_requester_begin(batch);

//     // Declare a descriptor slot.
//     DvzBaker* baker = dvz_baker(batch, 0);
//     dvz_baker_slot_dat(baker, 0, 6);

//     // Properties.
//     dvz_baker_property(baker, 0, 0, 0, 2);
//     dvz_baker_property(baker, 1, 0, 2, 4);

//     // Create the baker.
//     dvz_baker_create(baker, 0, 1);

//     // Set baker parameters.
//     dvz_baker_param(baker, 0, (char[]){1, 2});
//     dvz_baker_param(baker, 1, (char[]){3, 4, 5, 6});

//     // Emit the dat upload requests.
//     dvz_baker_update(baker);

//     // Check the upload data.
//     AT(memcmp(batch->requests[1].content.dat_upload.data, (char[]){1, 2, 3, 4, 5, 6}, 6) == 0);
//     // dvz_show_buffer(2, 6, 6, baker->descriptors[0].dual.array->data);

//     // Destroy the objects.
//     dvz_baker_destroy(baker);
//     dvz_requester_destroy(batch);
//     return 0;
// }
