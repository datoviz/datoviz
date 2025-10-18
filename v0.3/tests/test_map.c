/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing map                                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdio.h>

#include "_map.h"
#include "test.h"
#include "test_map.h"
#include "testing.h"



/*************************************************************************************************/
/*  Map tests                                                                                    */
/*************************************************************************************************/

int test_map_1(TstSuite* suite, TstItem* tstitem)
{

    DvzMap* map = dvz_map();

    DvzId id = 1;

    int data = 123;

    // Add one element.
    AT(!dvz_map_exists(map, id));
    AT(dvz_map_count(map, id) == 0);
    dvz_map_add(map, id, 0, &data);
    AT(dvz_map_exists(map, id));
    AT(dvz_map_count(map, 0) == 1);

    AT(dvz_map_get(map, id) == &data);

    // Try to add the same element.
    int data2 = 456;
    dvz_map_add(map, id, 0, &data2);
    AT(dvz_map_exists(map, id));
    // The call below should have failed and the data should not have been updated.
    AT(*(int*)dvz_map_get(map, id) == 123);

    // Try again by removing the existing object first.
    dvz_map_remove(map, id);
    dvz_map_add(map, id, 0, &data2);
    AT(dvz_map_exists(map, id));
    AT(*(int*)dvz_map_get(map, id) == 456);

    // Destroy the map.
    dvz_map_destroy(map);
    return 0;
}



int test_map_2(TstSuite* suite, TstItem* tstitem)
{

    DvzMap* map = dvz_map();

    // Empty map.
    DvzId id = 1; // dvz_map_id(map);
    AT(dvz_map_count(map, 0) == 0);

    int type = 3;
    int data[2] = {42, 103};

    // Add one element.
    dvz_map_add(map, id, type, &data);
    AT(dvz_map_get(map, id) == &data);
    AT(dvz_map_count(map, 0) == 1);
    AT(dvz_map_count(map, type) == 1);
    AT(dvz_map_count(map, -1) == 0);
    AT(id != DVZ_ID_NONE);

    // Add another element.
    id = 2; // dvz_map_id(map);
    AT(id == 2);
    dvz_map_add(map, id, type, &data[1]);

    // Check getting the first and last elements.
    AT(dvz_map_first(map, type) == &data[0]);
    AT(dvz_map_last(map, type) == &data[1]);
    AT(dvz_map_count(map, 0) == 2);
    AT(dvz_map_count(map, type) == 2);
    AT(dvz_map_count(map, -1) == 0);

    // Remove one element.
    dvz_map_remove(map, id);
    AT(dvz_map_last(map, type) == &data[0]);
    AT(dvz_map_count(map, 0) == 1);
    AT(dvz_map_count(map, type) == 1);
    AT(dvz_map_count(map, -1) == 0);

    // Destroy the map.
    dvz_map_destroy(map);
    return 0;
}
