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
    // log_info("local time is %f, last fire is %f", local_time, item->last_fire);

    // The timer item has not started running yet.
    if (local_time < 0)
        return false;

    // First firing
    if (local_time >= 0 && item->last_fire < 0)
    {
        return true;
    }

    // When is the next expected time?
    double t = local_time;
    double p = item->period;
    double last = item->last_fire - item->start_time;
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

    // Add it to the list.
    dvz_list_append(&timer->items, (DvzListItem){.p = item});

    // Start the timer when creating it.
    dvz_timer_start(item);

    return item;
}



void dvz_timer_start(DvzTimerItem* item)
{
    ASSERT(item != NULL);
    DvzTimer* timer = item->timer;
    ASSERT(timer != NULL);

    // Mark the timer item as running.
    item->is_running = true;

    // Reset the timer item start time.
    item->start_time = timer->time + item->delay;

    // NOTE: a negative value means the timer item has never fired yet
    item->last_fire = -1;
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

    FREE(item);
}



bool dvz_timer_running(DvzTimerItem* item)
{
    ASSERT(item != NULL);
    return item->is_running;
}



void dvz_timer_tick(DvzTimer* timer, double time)
{
    // Determine which timers are firing now.
    // To be called at every frame

    ASSERT(timer != NULL);
    ASSERT(time >= 0);

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
            // Keep track of the firing.
            item->last_fire = timer->time;
        }
    }
}



DvzTimerItem** dvz_timer_firing(DvzTimer* timer, uint32_t* count)
{
    // return an array with the timers that are firing at that tick: timer->firing
    ASSERT(timer != NULL);
    ASSERT(count != NULL);

    *count = timer->firing_count;
    return *count > 0 ? timer->firing : NULL;
}



void dvz_timer_destroy(DvzTimer* timer)
{
    ASSERT(timer != NULL);
    DvzList* list = &timer->items;
    uint64_t n = dvz_list_count(list);

    // Remove all timer items.
    DvzTimerItem* item = NULL;
    for (uint64_t i = 0; i < n; i++)
    {
        item = (DvzTimerItem*)dvz_list_get(list, i).p;
        ASSERT(item != NULL);
        // This will also free the heap-allocated DvzTimerItem objects before destroying the list.
        dvz_timer_remove(item);
    }

    // The list should now be empty and all item pointers should have been free-ed.
    ASSERT(dvz_list_count(list) == 0);

    // Destroy the list.
    dvz_list_destroy(list);

    // Free the memory associated to the timer struct.
    FREE(timer);
}
