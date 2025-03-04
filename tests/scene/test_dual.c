/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing dual                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/test_dual.h"
#include "_map.h"
#include "datoviz_protocol.h"
#include "scene/array.h"
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
/*  Dual tests                                                                                   */
/*************************************************************************************************/

int test_dual_1(TstSuite* suite, TstItem* tstitem)
{
    DvzBatch* batch = dvz_batch();
    DvzArray* array = dvz_array(16, DVZ_DTYPE_CHAR);
    DvzId dat = 1;

    DvzDual dual = dvz_dual(batch, array, dat);

    dvz_dual_dirty(&dual, 2, 3);
    AT(dual.dirty_first == 2);
    AT(dual.dirty_last == 2 + 3);

    dvz_dual_dirty(&dual, 7, 2);
    AT(dual.dirty_first == 2);
    AT(dual.dirty_last == 9);

    dvz_dual_dirty(&dual, 3, 7);
    AT(dual.dirty_first == 2);
    AT(dual.dirty_last == 10);

    dvz_dual_clear(&dual);
    char data[16] = {0};
    for (uint32_t i = 0; i < 16; i++)
    {
        data[i] = i;
    }
    dvz_dual_data(&dual, 4, 8, &data[4]);
    dvz_dual_update(&dual);

    // Check the array has been updated.
    for (uint32_t i = 0; i < 16; i++)
    {
        if (i < 4)
        {
            AT(*((char*)dvz_array_item(array, i)) == 0);
        }
        else if (i < 12)
        {
            AT(*((char*)dvz_array_item(array, i)) == (char)i);
        }
        else
        {
            AT(*((char*)dvz_array_item(array, i)) == 0);
        }
    }

    AT(batch->count == 1);
    DvzRequest* req = &batch->requests[0];
    AT(req->action == DVZ_REQUEST_ACTION_UPLOAD);
    AT(req->type == DVZ_REQUEST_OBJECT_DAT);
    AT(req->id == dat);
    AT(req->content.dat_upload.offset == 4);
    AT(req->content.dat_upload.size == 8);
    AT(*(char*)req->content.dat_upload.data == 4);

    dvz_array_destroy(array);
    dvz_dual_destroy(&dual);
    dvz_batch_destroy(batch);

    return 0;
}



int test_dual_2(TstSuite* suite, TstItem* tstitem)
{
    DvzBatch* batch = dvz_batch();
    // dvz_requester_begin(batch);

    DvzSize item_size = 4 + 8;
    uint32_t count = 12;

    DvzArray* array = dvz_array_struct(count, item_size);
    DvzId dat = 1;

    DvzDual dual = dvz_dual(batch, array, dat);

    char data[8 * 4] = {0};

    // |-4-|-8-|
    // | 0 | 0 |
    // | 0 | 0 |
    // | 1 | 0 |
    // | 1 | 0 |
    // | 1 | 2 |
    // | 1 | 2 |
    // | 0 | 2 |
    // | 0 | 2 |

    memset(data, 1, 4 * 4);
    dvz_dual_column(&dual, 0, 4, 2, 4, 1, data);

    memset(data, 2, 8);
    dvz_dual_column(&dual, 4, 8, 4, 1, 4, data);

    dvz_dual_update(&dual);

    AT(batch->count == 1);
    DvzRequest* req = &batch->requests[0];

    AT(req->content.dat_upload.offset == 2 * item_size);
    AT(req->content.dat_upload.size == 6 * item_size);

    char* upload = (char*)req->content.dat_upload.data;

    uint32_t row, col;
    for (uint32_t i = 0; i < req->content.dat_upload.size; i++)
    {
        col = i % 12;
        row = i / 12;
        if (col < 4 && row < 4)
        {
            AT(upload[i] == 1);
        }
        else if (col >= 4 && row >= 2)
        {
            AT(upload[i] == 2);
        }
        else
        {
            AT(upload[i] == 0);
        }
    }

    IF_VERBOSE
    {
        for (uint32_t i = 0; i < count * item_size; i++)
        {
            printf("%u ", ((char*)array->data)[i]);
            if ((i > 0) && (i % 4 == 3))
                printf("| ");
            if ((i > 0) && (i % 12 == 11))
                printf("\n");
        }
        printf("\n");
    }

    dvz_array_destroy(array);
    dvz_dual_destroy(&dual);
    dvz_batch_destroy(batch);
    return 0;
}
