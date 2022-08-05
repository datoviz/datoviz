/*************************************************************************************************/
/*  Timer                                                                                        */
/*************************************************************************************************/

#include "timer.h"
#include "common.h"



/*************************************************************************************************/
/*  Timer util functions                                                                         */
/*************************************************************************************************/

static bool _timer_item_firing(DvzTimerItem* item)
{
    ASSERT(item != NULL);

    DvzTimer* timer = item->timer;
    ASSERT(timer != NULL);

    // If the numbers of ticks was exceeded, stop the timer.
    if (item->max_count > 0 && item->count >= item->max_count)
    {
        item->is_running = false;
    }

    // Paused timers should not tick.
    if (!item->is_running)
        return false;

    // Local time, taking into account the global current time, the item start time and delay
    // (encoded in start time).
    double local_time = timer->time - item->start_time;

    // The timer item has not started running yet.
    if (local_time < 0)
        return false;

    // When is the next expected time?
    double t = local_time;
    double p = item->period;
    double last = item->last_fire;
    ASSERT(t >= 0);
    ASSERT(last >= 0);
    ASSERT(p > 0);

    // The timer item fires if the interval since the last fire is larger than the period.
    // NOTE: assumption: the fire always occurs AT or AFTER the last theoretical firing at K*P.
    return t - ((int64_t)floor(last / p)) * p >= p;
}



/*************************************************************************************************/
/*  Timer functions                                                                              */
/*************************************************************************************************/

DvzTimer* dvz_timer()
{
    DvzTimer* timer = (DvzTimer*)calloc(1, sizeof(DvzTimer));
    timer->items = dvz_list();
    return timer;
}



uint32_t dvz_timer_count(DvzTimer* timer)
{
    ASSERT(timer != NULL);
    return dvz_list_count(&timer->items);
}



DvzTimerItem* dvz_timer_new(DvzTimer* timer, double delay, double period, uint64_t max_count)
{
    ASSERT(timer != NULL);

    ASSERT(period > 0);

    // Create the DvzTimerItem struct.
    DvzTimerItem* item = (DvzTimerItem*)calloc(1, sizeof(DvzTimerItem));
    item->timer = timer;
    item->delay = delay;
    item->period = period;
    item->max_count = max_count;
    item->is_running = true;

    // Add it to the list.
    dvz_list_append(&timer->items, (DvzListItem){.p = item});

    return item;
}



void dvz_timer_start(DvzTimerItem* item)
{
    ASSERT(item != NULL);
    item->is_running = true;

    DvzTimer* timer = item->timer;
    ASSERT(timer != NULL);

    // Reset the timer item start time.
    item->start_time = timer->time + item->delay;
}



void dvz_timer_pause(DvzTimerItem* item)
{
    ASSERT(item != NULL);
    item->is_running = false;
}



void dvz_timer_remove(DvzTimerItem* item)
{
    ASSERT(item != NULL);
    ASSERT(item->timer != NULL);

    DvzList* list = &item->timer->items;
    ASSERT(list != NULL);

    dvz_list_remove_pointer(list, item);
}



void dvz_timer_tick(DvzTimer* timer, double time)
{
    // Determine which timers are firing now.
    // To be called at every frame

    ASSERT(timer != NULL);

    // Set the global time.
    timer->time = time;

    // Reset the number of firing timer items.
    timer->firing_count = 0;

    // Go through all timer items.
    uint64_t n = dvz_list_count(&timer->items);
    DvzTimerItem* item = NULL;
    for (uint64_t i = 0; i < n; i++)
    {
        item = (DvzTimerItem*)dvz_list_get(&timer->items, i).p;
        ASSERT(item != NULL);

        // If the current timer item is firing, append it to the list of currently-firing items.
        if (_timer_item_firing(item))
        {
            ASSERT(timer->firing_count < DVZ_TIMER_MAX_FIRING - 1);
            timer->firing[timer->firing_count++] = item;
        }
    }
}



DvzTimerItem** dvz_timer_firing(DvzTimer* timer, uint32_t* count)
{
    // return an array with the timers that are firing at that tick: timer->firing
    ASSERT(timer != NULL);
    ASSERT(count != NULL);

    *count = timer->firing_count;
    return timer->firing;
}



void dvz_timer_destroy(DvzTimer* timer)
{
    ASSERT(timer != NULL);
    uint64_t n = dvz_list_count(&timer->items);

    // Free the heap-allocated DvzTimerItem objects before destroying the list.
    DvzTimerItem* item = NULL;
    for (uint64_t i = 0; i < n; i++)
    {
        item = (DvzTimerItem*)dvz_list_get(&timer->items, i).p;
        ASSERT(item != NULL);
        FREE(item);
    }

    dvz_list_destroy(&timer->items);
    FREE(timer);
}
