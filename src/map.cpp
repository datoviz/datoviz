/*************************************************************************************************/
/*  Map                                                                                          */
/*************************************************************************************************/

#include "map.h"

#include <map>
#include <numeric>
#include <utility>



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

extern "C" struct DvzMap
{
    std::map<DvzId, void*> _map;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/


DvzMap* dvz_map(void)
{
    DvzMap* map = new DvzMap();
    map->_map = std::map<DvzId, void*>();
    return map;
}



DvzId dvz_map_id(DvzMap* map)
{
    ASSERT(map != NULL); //
}



void dvz_map_add(DvzMap* map, DvzId key, int type, void* value)
{
    ASSERT(map != NULL); //
}



void* dvz_map_get(DvzMap* map, DvzId key)
{
    ASSERT(map != NULL); //
}



uint32_t dvz_map_count(DvzMap* map, int type)
{
    ASSERT(map != NULL); //
}



void* dvz_map_first(DvzMap* map, int type)
{
    ASSERT(map != NULL); //
}



void* dvz_map_last(DvzMap* map, int type)
{
    ASSERT(map != NULL); //
}



void dvz_map_destroy(DvzMap* map)
{
    if (map != NULL)
        delete map;
}
