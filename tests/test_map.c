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

    DvzId id = dvz_map_id(map);
    ASSERT(id == 1);

    int type = 3;
    int data[2] = {42, 103};

    dvz_map_add(map, id, type, &data);
    AT(dvz_map_get(map, id) == &data);

    id = dvz_map_id(map);
    ASSERT(id == 2);
    dvz_map_add(map, id, type, &data[1]);

    AT(dvz_map_first(map, type) == &data[0]);
    AT(dvz_map_last(map, type) == &data[1]);

    dvz_map_destroy(map);
    return 0;
}
