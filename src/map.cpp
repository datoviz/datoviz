/*************************************************************************************************/
/*  Map                                                                                          */
/*************************************************************************************************/

#include "map.h"
#include "_log.h"

#include <map>
#include <numeric>
#include <utility>



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

extern "C" struct DvzMap
{
    std::map<DvzId, std::pair<int, void*>> _map;
    DvzId last_id;
};


/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

// https://stackoverflow.com/a/21510202/1595060
template <typename It> class Range
{
    It b, e;

public:
    Range(It b, It e) : b(b), e(e) {}
    It begin() const { return b; }
    It end() const { return e; }
};

template <
    typename ORange, typename OIt = decltype(std::begin(std::declval<ORange>())),
    typename It = std::reverse_iterator<OIt>>
Range<It> reverse(ORange&& originalRange)
{
    return Range<It>(It(std::end(originalRange)), It(std::begin(originalRange)));
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzMap* dvz_map(void)
{
    DvzMap* map = new DvzMap();
    map->_map = std::map<DvzId, std::pair<int, void*>>();
    return map;
}



DvzId dvz_map_id(DvzMap* map)
{
    ASSERT(map != NULL);
    return ++map->last_id;
}



void dvz_map_add(DvzMap* map, DvzId key, int type, void* value)
{
    ASSERT(map != NULL);
    map->_map[key] = std::pair<int, void*>(type, value);
}



void* dvz_map_get(DvzMap* map, DvzId key)
{
    ASSERT(map != NULL);
    return map->_map[key].second;
}



uint32_t dvz_map_count(DvzMap* map, int type)
{
    ASSERT(map != NULL);
    return map->_map.size();
}



void* dvz_map_first(DvzMap* map, int type)
{
    ASSERT(map != NULL);
    for (const auto& [id, pair] : map->_map)
    {
        if (pair.first == type)
            return pair.second;
    }
    log_trace("no item with type %d found in map", type);
    return NULL;
}



void* dvz_map_last(DvzMap* map, int type)
{
    ASSERT(map != NULL);
    for (const auto& [id, pair] : reverse(map->_map))
    {
        if (pair.first == type)
            return pair.second;
    }
    log_trace("no item with type %d found in map", type);
    return NULL;
}



void dvz_map_destroy(DvzMap* map)
{
    if (map != NULL)
        delete map;
}
