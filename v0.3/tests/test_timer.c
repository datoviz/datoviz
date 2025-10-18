/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing timer                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_timer.h"
#include "test.h"
#include "testing.h"
#include "timer.h"



/*************************************************************************************************/
/*  Timer tests macrps                                                                           */
/*************************************************************************************************/

#define AFT(t) assert_firing(item, t, true);
#define AFF(t) assert_firing(item, t, false);



/*************************************************************************************************/
/*  Timer tests utils                                                                            */
/*************************************************************************************************/

static int assert_firing(DvzTimerItem* item, double time, bool firing)
{
    ANN(item);
    DvzTimer* timer = item->timer;
    ANN(timer);

    dvz_timer_tick(timer, time);

    uint32_t firing_count = 0;
    DvzTimerItem** items = dvz_timer_firing(timer, &firing_count);
    AT(firing_count == (uint32_t)firing);

    if (firing)
    {
        AT(items != NULL);
        AT(items[0] == item);
    }
    else
    {
        AT(items == NULL);
    }
    return 0;
}



/*************************************************************************************************/
/*  Timer tests                                                                                  */
/*************************************************************************************************/

int test_timer_1(TstSuite* suite, TstItem* tstitem)
{
    DvzTimer* timer = dvz_timer();
    AT(dvz_timer_count(timer) == 0);

    double delay = .5;
    double period = 1.0;
    uint64_t max_count = 0;

    DvzTimerItem* item = dvz_timer_new(timer, delay, period, max_count);
    AT(dvz_timer_running(item));

    // The timer should fire at 0.5, 1.5, 2.5, 3.5, etc.
    AFF(0)
    AFF(0.49)
    AFT(0.5)
    AFF(0.99)
    AFF(1.01)
    AFF(1.49)
    AFT(1.60)
    AFF(2.49)
    AFT(2.51)

    // Pause.
    dvz_timer_pause(item);
    AFF(3.49)
    AFF(3.51)
    AFF(4.5)

    // Start again.
    AFF(15.0)
    dvz_timer_start(item);
    AFF(15.49)
    AFT(15.9)
    AFF(15.91)

    AFF(16.49)
    AFT(16.50)
    AFF(16.51)
    AFF(17.49)
    AFT(17.75)
    AFF(18.49)
    AFT(18.50)

    dvz_timer_remove(item);
    dvz_timer_destroy(timer);
    return 0;
}



static void _on_timer(DvzTimer* timer, DvzInternalTimerEvent ev)
{
    ANN(timer);

    double* res = (double*)ev.user_data;
    ANN(res);

    *res = ev.time;
}

int test_timer_2(TstSuite* suite, TstItem* tstitem)
{
    double period = .1;
    DvzTimer* timer = dvz_timer();
    DvzTimerItem* item = dvz_timer_new(timer, 0, period, 0);

    double res = 0;
    dvz_timer_callback(timer, item, _on_timer, &res);

    dvz_timer_tick(timer, 0.1);
    AT(res == .1);
    AT(item->count == 1);

    dvz_timer_tick(timer, 0.15);
    AT(res == .1);
    AT(item->count == 1);

    dvz_timer_tick(timer, 0.2);
    AT(res == .2);
    AT(item->count == 2);

    dvz_timer_destroy(timer);
    return 0;
}
