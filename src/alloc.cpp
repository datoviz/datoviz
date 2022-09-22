/*************************************************************************************************/
/*  Allocation algorithm                                                                         */
/*************************************************************************************************/

#include "alloc.h"

#include <list>
#include <map>
#include <numeric>
#include <utility>



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

extern "C" struct DvzAlloc
{
    std::map<DvzSize, DvzSize> occupied; // offset: (aligned) block size
    std::map<DvzSize, DvzSize> free;     // offset: (aligned) block size
    DvzSize alignment, alloc_size, buf_size;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzAlloc* dvz_alloc(DvzSize size, DvzSize alignment)
{
    ASSERT(size > 0);

    DvzAlloc* alloc = new DvzAlloc();
    alloc->occupied = std::map<DvzSize, DvzSize>();
    alloc->free = std::map<DvzSize, DvzSize>();

    alloc->alignment = alignment;
    alloc->buf_size = size;
    alloc->alloc_size = 0;

    return alloc;
}



DvzSize dvz_alloc_new(DvzAlloc* alloc, DvzSize req_size, DvzSize* resized)
{
    ANN(alloc);

    DvzSize req = _align(req_size, alloc->alignment);
    ASSERT(req >= req_size);

    // Find a free slot large enough for the req size.
    for (const auto& [o, s] : alloc->free)
    {
        // The free slot is larger than the requested size: we can take it!
        if (req <= s)
        {
            // Make sure this slot is not already occupied.
            ASSERT(alloc->occupied.count(o) == 0);
            // Add the new allocated slot.
            alloc->occupied[o] = req;
            // If empty space remains, update the existing empty space.
            if (s > req)
            {
                alloc->free[o + req] = s - req;
            }
            // In all cases, remove the existing free slot.
            alloc->free.erase(o);

            log_trace("alloc new %d, found offset %d", req_size, o);
            return o;
        }
    }



    // If we're here, it means we weren't able to find a free slot. We must create a new slot after
    // all existing slots.
    _check_align(alloc->alloc_size, alloc->alignment);

    // Add a new occupied slot.
    // Ensure the new slot doesn't already exist.
    ASSERT(alloc->occupied.count(alloc->alloc_size) == 0);
    alloc->occupied[alloc->alloc_size] = req;
    DvzSize out = alloc->alloc_size;
    // Increase the total allocated size.
    alloc->alloc_size += req;

    // Need to resize the underlying buffer?
    if (alloc->alloc_size > alloc->buf_size)
    {
        // Double the buffer size until it is larger or equal than the allocated size.
        // DvzSize new_size = alloc->buf_size;
        while (alloc->buf_size < alloc->alloc_size)
            alloc->buf_size *= 2;
        ASSERT(alloc->alloc_size <= alloc->buf_size);
        // Change the passed pointer to let the caller know that the underlying buffer had to be
        // resized.
        if (resized != NULL)
            *resized = alloc->buf_size;
        log_trace("will need to resize alloc buffer to %s", pretty_size(alloc->buf_size));
    }

    log_trace("alloc new %d, found offset %d", req_size, out);
    return out;
}



DvzSize dvz_alloc_get(DvzAlloc* alloc, DvzSize offset)
{
    ANN(alloc);
    if (alloc->occupied.count(offset) > 0)
    {
        return alloc->occupied[offset];
    }
    return 0;
}



DvzSize dvz_alloc_size(DvzAlloc* alloc)
{
    ANN(alloc);
    return alloc->alloc_size;
}



void dvz_alloc_stats(DvzAlloc* alloc)
{
    ANN(alloc);
    uint32_t dat_count = alloc->occupied.size();
    DvzSize alloc_occupied = (DvzSize)std::accumulate(
        std::begin(alloc->occupied), std::end(alloc->occupied), 0,
        [](const DvzSize previous, const std::pair<DvzSize, DvzSize>& p) {
            return previous + p.second;
        });
    DvzSize alloc_total = alloc->alloc_size;
    log_info(
        "%d allocations, total size: %s / %s (%.1f%)", dat_count, //
        pretty_size(alloc_occupied), pretty_size(alloc_total),    //
        alloc_total ? 100 * (float)alloc_occupied / (float)alloc_total : 0);
}



void dvz_alloc_free(DvzAlloc* alloc, DvzSize offset)
{
    ANN(alloc);

    if (alloc->occupied.count(offset) == 0)
    {
        log_debug("cannot free unoccupied slot at offset %u", offset);
        return;
    }

    // Check that the slot is occupied.
    ASSERT(alloc->occupied.count(offset) > 0);
    ASSERT(alloc->free.count(offset) == 0);
    DvzSize size = alloc->occupied[offset];
    ASSERT(size > 0);

    // If the slot is the last one, just remove it completely and decrease the alloc size.
    // note: get largest key in the map: https://stackoverflow.com/a/1660220/1595060
    if (offset == alloc->occupied.rbegin()->first)
    {
        log_trace("freeing last occupied slot");
        alloc->occupied.erase(offset);
        ASSERT(alloc->alloc_size >= size);
        alloc->alloc_size -= size;
    }
    // Otherwise, remove the slot and put it in the free map.
    else
    {
        log_trace("freeing an occupied slot");
        alloc->occupied.erase(offset);
        alloc->free[offset] = size;
    }
}



void dvz_alloc_clear(DvzAlloc* alloc)
{
    ANN(alloc);
    alloc->occupied.clear();
    alloc->free.clear();
    alloc->alloc_size = 0;
}



void dvz_alloc_destroy(DvzAlloc* alloc)
{
    if (alloc != NULL)
        delete alloc;
}
