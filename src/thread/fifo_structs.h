/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  FIFO structs                                                                                 */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/common/mutex.h"
#include "datoviz/thread/atomic.h"
#include "datoviz/thread/fifo.h"



/*************************************************************************************************/
/*  FIFO queue                                                                                   */
/*************************************************************************************************/

struct DvzFifo
{
    int32_t tail, head;
    int32_t capacity;
    void** items;
    void* user_data;

    DvzMutex lock;
    DvzCond cond;

    DvzAtomic is_processing;
    DvzAtomic is_empty;
};



/*************************************************************************************************/
/*  Dequeues struct                                                                              */
/*************************************************************************************************/

struct DvzDeqCallbackRegister
{
    uint32_t deq_idx;
    int type;
    DvzDeqCallback callback;
    void* user_data;
    // bool is_default; // if true, the callback will be discarded if there are other callbacks
};



struct DvzDeqItem
{
    uint32_t deq_idx;
    int type;
    void* item;

    // Items to enqueue  *after* the current item has been dequeued and the callbacks have run.
    uint32_t next_count;
    DvzDeqItemNext* next_items;
};

struct DvzDeqItemNext
{
    bool enqueue_first; // whether the next item should be enqueued with enqueue_first() or not
    DvzDeqItem* next_item;
};



// A Proc represents a pair consumer/producer, where typically one thread enqueues items in a
// subset of the queues, and another thread dequeues items from that subset.
struct DvzDeqProc
{
    DvzDeqStrategy strategy; // dequeue strategy: breadth-first (default) or depth-first

    // Which queues constitute this process.
    uint32_t queue_count;
    uint32_t queue_indices[DVZ_DEQ_MAX_PROC_SIZE];
    uint32_t queue_offset; // offset that regularly increases at every call of dequeue()

    // // Callbacks called after every dequeue, independently of the deq idx and type, either
    // before
    // // or after the item callbacks.
    // uint32_t callback_count;
    // DvzDeqProcCallbackRegister callbacks[DVZ_DEQ_MAX_CALLBACKS];

    // // Callbacks called while waiting with a max_wait delay.
    // uint32_t wait_callback_count;
    // DvzDeqProcWaitCallbackRegister wait_callbacks[DVZ_DEQ_MAX_CALLBACKS];

    // // Callbacks called when dequeuing multiple items at once (batch dequeue).
    // uint32_t batch_callback_count;
    // DvzDeqProcBatchCallbackRegister batch_callbacks[DVZ_DEQ_MAX_CALLBACKS];

    // Mutex and cond to signal when the deq is non-empty, and when to dequeue the first non-empty
    // underlying FIFO queues.
    DvzMutex lock;
    DvzCond cond;
    uint32_t max_wait; // maximum number of milliseconds to wait between each queue size probing
    struct timespec wait;
    DvzAtomic is_processing;
};



struct DvzDeq
{
    DvzSize item_size;

    uint32_t queue_count;
    DvzFifo* queues[DVZ_DEQ_MAX_QUEUES];

    uint32_t callback_count;
    DvzDeqCallbackRegister callbacks[DVZ_DEQ_MAX_CALLBACKS];

    uint32_t proc_count;
    DvzDeqProc procs[DVZ_DEQ_MAX_PROCS];
    uint32_t q_to_proc[DVZ_DEQ_MAX_QUEUES]; // for each queue, which proc idx is handling it

    // One event type can have its callbacks called in reverse order.
    int reverse_callback_type;

    // bool has_default; // true if there is at least one registered default callback
};
