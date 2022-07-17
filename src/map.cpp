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
    // DvzPrng* prng;
    // DvzId last_id;
};



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

// https://stackoverflow.com/a/21510202/1595060
template <typename It> class Range
{
    It b_, e_;

public:
    Range(It b, It e) : b_(b), e_(e) {}
    It begin() const { return b_; }
    It end() const { return e_; }
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
    // map->prng = dvz_prng();
    return map;
}



bool dvz_map_exists(DvzMap* map, DvzId key)
{
    ASSERT(map != NULL);
    ASSERT(key != DVZ_ID_NONE);

    return map->_map.count(key) > 0;
}



void dvz_map_add(DvzMap* map, DvzId key, int type, void* value)
{
    ASSERT(map != NULL);
    ASSERT(key > 0);
    ASSERT(value != NULL);

    if (map->_map.count(key) > 0)
    {
        log_warn("key 0x%" PRIx64 " already exists (type %d)", key, type);
        return;
    }

    log_trace("add key 0x%" PRIx64 " with type %d", key, type);
    map->_map[key] = std::pair<int, void*>(type, value);
}



void dvz_map_remove(DvzMap* map, DvzId key)
{

    ASSERT(map != NULL);
    ASSERT(key != DVZ_ID_NONE);

    map->_map.erase(key);
}



void* dvz_map_get(DvzMap* map, DvzId key)
{
    ASSERT(map != NULL);
    ASSERT(key != DVZ_ID_NONE);

    return map->_map[key].second;
}



int dvz_map_type(DvzMap* map, DvzId key)
{
    ASSERT(map != NULL);
    ASSERT(key != DVZ_ID_NONE);

    return map->_map[key].first;
}



uint64_t dvz_map_count(DvzMap* map, int type)
{
    ASSERT(map != NULL);

    if (type == 0)
        return map->_map.size();
    else
    {
        uint64_t count = 0;
        for (const auto& [id, pair] : map->_map)
        {
            if (pair.first == type)
                count++;
        }
        return count;
    }
}



void* dvz_map_first(DvzMap* map, int type)
{
    ASSERT(map != NULL);

    for (const auto& [id, pair] : map->_map)
    {
        if (type == 0 || pair.first == type)
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
        if (type == 0 || pair.first == type)
            return pair.second;
    }
    log_trace("no item with type %d found in map", type);
    return NULL;
}



void dvz_map_destroy(DvzMap* map)
{
    // if (map->prng != NULL)
    // {
    //     dvz_prng_destroy(map->prng);
    // }
    if (map != NULL)
    {
        delete map;
    }
}
