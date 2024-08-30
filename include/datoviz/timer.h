/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

/*************************************************************************************************/
/*  Timer                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_TIMER
#define DVZ_HEADER_TIMER



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_list.h"
#include "common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_TIMER_MAX_FIRING 16



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzTimer DvzTimer;
typedef struct DvzTimerItem DvzTimerItem;
typedef struct DvzInternalTimerEvent DvzInternalTimerEvent;
typedef struct DvzTimerPayload DvzTimerPayload;

typedef void (*DvzTimerCallback)(DvzTimer* timer, DvzInternalTimerEvent ev);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzTimerItem
{
    DvzTimer* timer;
    uint32_t timer_idx;
    double delay, period;
    uint64_t count, max_count;
    double start_time, last_fire;
    bool is_running;
};



struct DvzTimer
{
    double time;
    DvzList* items;
    uint32_t firing_count;
    DvzTimerItem* firing[DVZ_TIMER_MAX_FIRING];

    DvzList* callbacks;
};



struct DvzInternalTimerEvent
{
    DvzTimerItem* item;
    double time;
    void* user_data;
};



struct DvzTimerPayload
{
    DvzTimerItem* item;
    DvzTimerCallback callback;
    void* user_data;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Timer functions                                                                              */
/*************************************************************************************************/

DvzTimer* dvz_timer(void);



uint32_t dvz_timer_count(DvzTimer* timer);



DvzTimerItem* dvz_timer_new(DvzTimer* timer, double delay, double period, uint64_t max_count);



void dvz_timer_start(DvzTimerItem* item);



void dvz_timer_pause(DvzTimerItem* item);



void dvz_timer_remove(DvzTimerItem* item);



bool dvz_timer_running(DvzTimerItem* item);



void dvz_timer_tick(DvzTimer* timer, double time);



DvzTimerItem** dvz_timer_firing(DvzTimer* timer, uint32_t* count);



void dvz_timer_callback(
    DvzTimer* timer, DvzTimerItem* item, DvzTimerCallback callback, void* user_data);



void dvz_timer_destroy(DvzTimer* timer);



EXTERN_C_OFF

#endif
