/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing list                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_assertions.h"
#include "datoviz/ds/list.h"
#include "test_ds.h"
#include "testing.h"



/*************************************************************************************************/
/*  Map tests                                                                                    */
/*************************************************************************************************/

int test_list_1(TstSuite* suite, TstItem* tstitem)
{
    DvzList* list = dvz_list();

    int a = 10;
    int b = 20;
    int c = 30;

    // Append and count.
    AT(dvz_list_count(list) == 0);
    dvz_list_append(list, (DvzListItem){.i = a});
    AT(dvz_list_count(list) == 1);
    dvz_list_append(list, (DvzListItem){.i = b});
    AT(dvz_list_count(list) == 2);
    dvz_list_append(list, (DvzListItem){.i = c});
    AT(dvz_list_count(list) == 3);

    // Get.
    AT(dvz_list_get(list, 0).i == a);
    AT(dvz_list_get(list, 1).i == b);
    AT(dvz_list_get(list, 2).i == c);

    // Indexing.
    AT(dvz_list_index(list, a) == 0);
    AT(dvz_list_index(list, b) == 1);
    AT(dvz_list_index(list, c) == 2);
    AT(dvz_list_index(list, a + 1) == UINT64_MAX);

    AT(dvz_list_has(list, a));
    AT(!dvz_list_has(list, a + 1));

    // Insert
    int d = 40;
    dvz_list_insert(list, 0, (DvzListItem){.i = d});

    AT(dvz_list_count(list) == 4);
    AT(dvz_list_get(list, 0).i == d);
    AT(dvz_list_get(list, 1).i == a);
    AT(dvz_list_get(list, 2).i == b);
    AT(dvz_list_get(list, 3).i == c);

    AT(dvz_list_index(list, d) == 0);
    AT(dvz_list_index(list, a) == 1);
    AT(dvz_list_index(list, b) == 2);
    AT(dvz_list_index(list, c) == 3);
    AT(dvz_list_index(list, c + 1) == UINT64_MAX);

    // Remove.
    dvz_list_remove(list, 1);
    AT(dvz_list_count(list) == 3);

    AT(dvz_list_get(list, 0).i == d);
    AT(dvz_list_get(list, 1).i == b);
    AT(dvz_list_get(list, 2).i == c);

    AT(dvz_list_index(list, d) == 0);
    AT(dvz_list_index(list, b) == 1);
    AT(dvz_list_index(list, c) == 2);
    AT(dvz_list_index(list, c + 1) == UINT64_MAX);

    dvz_list_clear(list);
    AT(dvz_list_count(list) == 0);

    // Destroy the list.
    dvz_list_destroy(list);
    return 0;
}



int test_list_remove_pointer(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    // Verify that removing by pointer handles duplicates without skipping entries.
    int payload = 7;
    DvzList* list = dvz_list();
    for (uint32_t i = 0; i < 3; i++)
        dvz_list_append(list, (DvzListItem){.p = &payload});
    AT(dvz_list_count(list) == 3);

    dvz_list_remove_pointer(list, &payload);
    AT(dvz_list_count(list) == 0);

    // Ensure the list dynamically grows beyond the initial capacity.
    for (uint32_t i = 0; i < DVZ_LIST_INITIAL_CAPACITY * 4; i++)
        dvz_list_append(list, (DvzListItem){.i = (int)i});
    AT(dvz_list_count(list) == DVZ_LIST_INITIAL_CAPACITY * 4);

    dvz_list_destroy(list);
    return 0;
}
