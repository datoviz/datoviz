/*************************************************************************************************/
/*  Testing map                                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdio.h>

#include "map.h"
#include "test.h"
#include "test_map.h"
#include "testing.h"



/*************************************************************************************************/
/*  Map tests                                                                                    */
/*************************************************************************************************/

int test_map_1(TstSuite* suite)
{

    DvzMap* map = dvz_map();

    // Empty map.
    DvzId id = dvz_map_id(map);
    AT(id == 1);
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
    id = dvz_map_id(map);
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
