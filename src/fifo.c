/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  FIFO code                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "fifo.h"
#include "_map.h"



/*************************************************************************************************/
/*  Thread-safe FIFO queue                                                                       */
/*************************************************************************************************/

DvzFifo* dvz_fifo(int32_t capacity)
{
    log_trace("creating generic FIFO queue with a capacity of %d items", capacity);
    ASSERT(capacity >= 2);
    DvzFifo* fifo = (DvzFifo*)calloc(1, sizeof(DvzFifo));
    ASSERT(capacity <= DVZ_MAX_FIFO_CAPACITY);
    fifo->capacity = capacity;
    fifo->items = (void**)calloc((uint32_t)capacity, sizeof(void*));

    // Create atomic variables.
    fifo->is_empty = dvz_atomic();
    dvz_atomic_set(fifo->is_empty, 1);

    fifo->is_processing = dvz_atomic();
    dvz_atomic_set(fifo->is_processing, 0);

    fifo->lock = dvz_mutex();
    fifo->cond = dvz_cond();

    return fifo;
}



static void _fifo_resize(DvzFifo* fifo)
{
    // Old size
    int size = fifo->tail - fifo->head;
    if (size < 0)
        size += fifo->capacity;

    // Old capacity
    int old_cap = fifo->capacity;

    // Resize if queue is full.
    if ((fifo->tail + 1) % fifo->capacity == fifo->head)
    {
        ANN(fifo->items);
        ASSERT(size == fifo->capacity - 1);
        ASSERT(fifo->capacity <= DVZ_MAX_FIFO_CAPACITY);

        fifo->capacity *= 2;
        log_debug("FIFO queue is full, enlarging it to %d", fifo->capacity);
        REALLOC(void**, fifo->items, (uint32_t)fifo->capacity * sizeof(void*));
    }

    if ((fifo->tail + 1) % fifo->capacity == fifo->head)
    {
        // Here, the queue buffer has been resized, but the new space should be used instead of the
        // part of the buffer before the head.

        ASSERT(fifo->tail > 0);
        ASSERT(old_cap < fifo->capacity);
        memcpy(&fifo->items[old_cap], &fifo->items[0], (uint32_t)fifo->tail * sizeof(void*));

        // Move the tail to the new position.
        fifo->tail += old_cap;

        // Check new size.
        ASSERT(fifo->tail - fifo->head > 0);
        ASSERT(fifo->tail - fifo->head == size);
    }
}



void dvz_fifo_enqueue(DvzFifo* fifo, void* item)
{
    ANN(fifo);
    dvz_mutex_lock(&fifo->lock);

    // Resize the FIFO queue if needed.
    _fifo_resize(fifo);

    ASSERT((fifo->tail + 1) % fifo->capacity != fifo->head);
    fifo->items[fifo->tail] = item;
    fifo->tail++;
    if (fifo->tail >= fifo->capacity)
        fifo->tail -= fifo->capacity;
    dvz_atomic_set(fifo->is_empty, 0);

    ASSERT(0 <= fifo->tail && fifo->tail < fifo->capacity);
    dvz_cond_signal(&fifo->cond);
    dvz_mutex_unlock(&fifo->lock);
}



void dvz_fifo_enqueue_first(DvzFifo* fifo, void* item)
{
    ANN(fifo);
    dvz_mutex_lock(&fifo->lock);

    // Resize the FIFO queue if needed.
    _fifo_resize(fifo);

    ASSERT((fifo->tail + 1) % fifo->capacity != fifo->head);
    fifo->head--;
    if (fifo->head < 0)
        fifo->head += fifo->capacity;
    ASSERT(0 <= fifo->head && fifo->head < fifo->capacity);

    fifo->items[fifo->head] = item;
    dvz_atomic_set(fifo->is_empty, 0);

    ASSERT(0 <= fifo->tail && fifo->tail < fifo->capacity);
    int size = fifo->tail - fifo->head;
    if (size < 0)
        size += fifo->capacity;
    ASSERT(0 <= size && size < fifo->capacity);

    dvz_cond_signal(&fifo->cond);
    dvz_mutex_unlock(&fifo->lock);
}



void* dvz_fifo_dequeue(DvzFifo* fifo, bool wait)
{
    ANN(fifo);
    dvz_mutex_lock(&fifo->lock);

    // Wait until the queue is not empty.
    if (wait)
    {
        log_trace("waiting for the queue to be non-empty");
        while (fifo->tail == fifo->head)
            dvz_cond_wait(&fifo->cond, &fifo->lock);
    }

    // Empty queue.
    if (fifo->tail == fifo->head)
    {
        // log_trace("FIFO queue was empty");
        // Don't forget to unlock the mutex before exiting this function.
        dvz_mutex_unlock(&fifo->lock);
        dvz_atomic_set(fifo->is_empty, 1);
        return NULL;
    }

    ASSERT(0 <= fifo->head && fifo->head < fifo->capacity);

    // log_trace("dequeue item, tail %d, head %d", fifo->tail, fifo->head);
    void* item = fifo->items[fifo->head];

    fifo->head++;
    if (fifo->head >= fifo->capacity)
        fifo->head -= fifo->capacity;

    ASSERT(0 <= fifo->head && fifo->head < fifo->capacity);

    if (fifo->tail == fifo->head)
        dvz_atomic_set(fifo->is_empty, 1);

    dvz_mutex_unlock(&fifo->lock);
    return item;
}



int dvz_fifo_size(DvzFifo* fifo)
{
    ANN(fifo);
    dvz_mutex_lock(&fifo->lock);
    // log_debug("tail %d head %d", fifo->tail, fifo->head);
    int size = fifo->tail - fifo->head;
    if (size < 0)
        size += fifo->capacity;
    ASSERT(0 <= size && size <= fifo->capacity);
    dvz_mutex_unlock(&fifo->lock);
    return size;
}



void* dvz_fifo_get(DvzFifo* fifo, int32_t idx)
{
    ANN(fifo);
    idx = (fifo->head + idx) % fifo->capacity;
    ASSERT(0 <= idx && idx < fifo->capacity);
    return fifo->items[idx];
}



void dvz_fifo_discard(DvzFifo* fifo, int max_size)
{
    ANN(fifo);
    if (max_size == 0)
        return;
    dvz_mutex_lock(&fifo->lock);
    int size = fifo->tail - fifo->head;
    if (size < 0)
        size += fifo->capacity;
    ASSERT(0 <= size && size <= fifo->capacity);
    if (size > max_size)
    {
        log_trace(
            "discarding %d items in the FIFO queue which is getting overloaded", size - max_size);
        fifo->head = fifo->tail - max_size;
        if (fifo->head < 0)
            fifo->head += fifo->capacity;
    }
    dvz_mutex_unlock(&fifo->lock);
}



void dvz_fifo_reset(DvzFifo* fifo)
{
    ANN(fifo);
    dvz_mutex_lock(&fifo->lock);
    fifo->tail = 0;
    fifo->head = 0;
    dvz_cond_signal(&fifo->cond);
    dvz_mutex_unlock(&fifo->lock);
}



void dvz_fifo_destroy(DvzFifo* fifo)
{
    log_trace("destroy FIFO queue");
    ANN(fifo);
    dvz_mutex_destroy(&fifo->lock);
    dvz_cond_destroy(&fifo->cond);

    dvz_atomic_destroy(fifo->is_empty);
    dvz_atomic_destroy(fifo->is_processing);

    ANN(fifo->items);
    FREE(fifo->items);
    FREE(fifo);
}



/*************************************************************************************************/
/*  Dequeue utils                                                                                */
/*************************************************************************************************/

static DvzFifo* _deq_fifo(DvzDeq* deq, uint32_t deq_idx)
{
    ANN(deq);
    ASSERT(deq_idx < deq->queue_count);

    DvzFifo* fifo = deq->queues[deq_idx];
    ANN(fifo);
    ASSERT(fifo->capacity > 0);
    return fifo;
}



// Call all callback functions registered with a deq_idx and type on a deq item.
static void _deq_callbacks(DvzDeq* deq, DvzDeqItem* item)
{
    ANN(deq);
    ANN(item->item);
    DvzDeqCallbackRegister* reg = NULL;
    uint32_t n = deq->callback_count;

    // Now, we go through all callbacks and we call them or not depending on whether they're
    // default, and whether we call the default or not.
    uint32_t k = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        // Call the callbacks in reverse order if the item is of the only event type marked as
        // reverse.
        k = ((deq->reverse_callback_type != 0) && (deq->reverse_callback_type == item->type))
                ? n - 1 - i
                : i;
        reg = &deq->callbacks[k];
        ANN(reg);
        if (reg->callback != NULL && reg->deq_idx == item->deq_idx && reg->type == item->type)
        {
            // NOTE: we do not call the callback if we should not call the default callbacks, and
            // if the current callback is a default one.
            // if (do_call_default || !reg->is_default)
            reg->callback(deq, item->item, reg->user_data);
        }
    }
}



// Return the total size of the deq->
static int _deq_size(DvzDeq* deq, uint32_t queue_count, uint32_t* queue_ids)
{
    ANN(deq);
    ASSERT(queue_count > 0);
    ANN(queue_ids);
    int size = 0;
    uint32_t deq_idx = 0;
    for (uint32_t i = 0; i < queue_count; i++)
    {
        deq_idx = queue_ids[i];
        ASSERT(deq_idx < deq->queue_count);
        size += dvz_fifo_size(deq->queues[deq_idx]);
    }
    return size;
}



// Return 0 if an item was dequeued, or a non-zero integer if the timeout-ed wait was unsuccessful
// (in which case we need to continue waiting).
static int _proc_wait(DvzDeqProc* proc)
{
    ANN(proc);

    if (proc->max_wait == 0)
    {
        // NOTE: this call automatically releases the mutex while waiting, and reacquires it
        // afterwards
        return dvz_cond_wait(&proc->cond, &proc->lock);
    }
    else
    {
        struct timeval now;
        uint32_t wait_s = proc->max_wait / 1000; // in seconds
        //                  ^^ in ms  ^^   ^^^^ convert to s
        uint32_t wait_us = (proc->max_wait - 1000 * wait_s) * 1000; // in us
        //                  ^^ in ms  ^^^    ^^^ in ms ^^^    ^^^ to us
        // Determine until when to wait for the cond.

        gettimeofday(&now, NULL);

        // How many seconds after now?
#ifdef OS_WINDOWS
        proc->wait.tv_sec = now.tv_sec + (int32_t)wait_s;
        // How many nanoseconds after the X seconds?
        proc->wait.tv_nsec = (now.tv_usec + (int32_t)wait_us) * 1000; // from us to ns
#else
        proc->wait.tv_sec = (uint32_t)now.tv_sec + wait_s;
        // How many nanoseconds after the X seconds?
        proc->wait.tv_nsec = ((uint32_t)now.tv_usec + wait_us) * 1000; // from us to ns
#endif

        // NOTE: this call automatically releases the mutex while waiting, and reacquires it
        // afterwards
        return dvz_cond_timedwait(&proc->cond, &proc->lock, &proc->wait);
    }
}



static void _enqueue_next(DvzDeq* deq, uint32_t item_count, DvzDeqItem* items)
{
    ANN(deq);
    if (item_count == 0)
        return;
    ASSERT(item_count > 0);
    ANN(items);

    // Go through all items.
    DvzDeqItemNext* next = NULL;
    for (uint32_t i = 0; i < item_count; i++)
    {
        // Go through every next item.
        for (uint32_t j = 0; j < items[i].next_count; j++)
        {
            next = &items[i].next_items[j];
            // Enqueue the next item.
            dvz_deq_enqueue_submit(deq, next->next_item, next->enqueue_first);
        }
        // Free the next_items array.
        FREE(items[i].next_items);
    }
}



/*************************************************************************************************/
/*  Dequeues                                                                                     */
/*************************************************************************************************/

DvzDeq* dvz_deq(uint32_t nq, DvzSize item_size)
{
    ASSERT(item_size > 0);
    DvzDeq* deq = (DvzDeq*)calloc(1, sizeof(DvzDeq));
    ASSERT(nq <= DVZ_DEQ_MAX_QUEUES);
    deq->queue_count = nq;
    deq->item_size = item_size;
    for (uint32_t i = 0; i < nq; i++)
        deq->queues[i] = dvz_fifo(DVZ_MAX_FIFO_CAPACITY);
    return deq;
}



void dvz_deq_callback(
    DvzDeq* deq, uint32_t deq_idx, int type, DvzDeqCallback callback, void* user_data)
{
    ANN(deq);
    ANN(callback);

    ASSERT(deq->callback_count < DVZ_DEQ_MAX_CALLBACKS);
    DvzDeqCallbackRegister* reg = &deq->callbacks[deq->callback_count++];
    ANN(reg);

    reg->deq_idx = deq_idx;
    reg->type = type;
    reg->callback = callback;
    reg->user_data = user_data;
}



void dvz_deq_callback_clear(DvzDeq* deq, int type)
{
    ANN(deq);
    for (uint32_t i = 0; i < deq->callback_count; i++)
    {
        if (deq->callbacks[i].type == type)
        {
            deq->callbacks[i].callback = NULL;
        }
    }
}



void dvz_deq_order(DvzDeq* deq, int type, DvzDeqOrder order)
{
    ANN(deq);
    if (order == DVZ_DEQ_ORDER_REVERSE)
    {
        if (deq->reverse_callback_type != 0)
            log_warn(
                "event type %d is already set for reverse calback order, will be replaced by new "
                "event type %d",
                deq->reverse_callback_type, type);
        deq->reverse_callback_type = type;
    }
    else
    {
        deq->reverse_callback_type = 0;
    }
}



void dvz_deq_proc(DvzDeq* deq, uint32_t proc_idx, uint32_t queue_count, uint32_t* queue_ids)
{
    ANN(deq);
    ANN(queue_ids);

    // HACK: calls to dvz_deq_proc(deq, proc_idx, ...) must be with proc_idx strictly increasing:
    // 0, 1, 2...
    ASSERT(proc_idx == deq->proc_count);

    ASSERT(deq->proc_count < DVZ_DEQ_MAX_PROCS);
    DvzDeqProc* proc = &deq->procs[deq->proc_count++];
    ANN(proc);

    ASSERT(queue_count <= DVZ_DEQ_MAX_PROC_SIZE);
    proc->queue_count = queue_count;
    // Copy the queue ids to the DvzDeqProc struct.
    for (uint32_t i = 0; i < queue_count; i++)
    {
        ASSERT(queue_ids[i] < deq->queue_count);
        proc->queue_indices[i] = queue_ids[i];

        // Register, for each of the indicated queue, which proc idx is handling it.
        ASSERT(queue_ids[i] < DVZ_DEQ_MAX_QUEUES);
        deq->q_to_proc[queue_ids[i]] = proc_idx;
    }

    // Initialize the thread objects.
    proc->lock = dvz_mutex();
    proc->cond = dvz_cond();
    proc->is_processing = dvz_atomic();
}



static DvzDeqItem* _deq_item(
    uint32_t deq_idx, int type, DvzSize item_size, void* item, uint32_t next_count,
    DvzDeqItemNext* next_items)
{
    DvzDeqItem* deq_item = (DvzDeqItem*)calloc(1, sizeof(DvzDeqItem));
    ANN(deq_item);
    deq_item->deq_idx = deq_idx;
    deq_item->type = type;

    // NOTE: make a copy on the heap of the item pointer. The deq will free it later.
    if (item != NULL)
    {
        deq_item->item = malloc(item_size);
        memcpy(deq_item->item, item, item_size);
    }

    deq_item->next_count = next_count;
    deq_item->next_items = next_items;

    return deq_item;
}

static void
_deq_enqueue_item(DvzDeq* deq, uint32_t deq_idx, DvzDeqItem* deq_item, bool enqueue_first)
{
    ANN(deq);
    ASSERT(deq_idx < deq->queue_count);
    ASSERT(deq_idx < DVZ_DEQ_MAX_QUEUES);

    // Find the proc that processes the specified queue.
    uint32_t proc_idx = deq->q_to_proc[deq_idx];
    ASSERT(proc_idx < deq->proc_count);
    DvzDeqProc* proc = &deq->procs[proc_idx];

    // We signal that proc that an item has been enqueued to one of its queues.
    dvz_mutex_lock(&proc->lock);
    DvzFifo* fifo = _deq_fifo(deq, deq_idx);
    if (!enqueue_first)
        dvz_fifo_enqueue(fifo, deq_item);
    else
        dvz_fifo_enqueue_first(fifo, deq_item);
    // log_trace("signal cond of proc #%d", proc_idx);
    dvz_cond_signal(&proc->cond);
    dvz_mutex_unlock(&proc->lock);
}

void dvz_deq_enqueue(DvzDeq* deq, uint32_t deq_idx, int type, void* item)
{
    DvzDeqItem* deq_item = _deq_item(deq_idx, type, deq->item_size, item, 0, NULL);
    _deq_enqueue_item(deq, deq_idx, deq_item, false);
}

void dvz_deq_enqueue_first(DvzDeq* deq, uint32_t deq_idx, int type, void* item)
{
    DvzDeqItem* deq_item = _deq_item(deq_idx, type, deq->item_size, item, 0, NULL);
    _deq_enqueue_item(deq, deq_idx, deq_item, true);
}



DvzDeqItem* dvz_deq_enqueue_custom(uint32_t deq_idx, int type, DvzSize item_size, void* item)
{
    return _deq_item(deq_idx, type, item_size, item, 0, NULL);
}



void dvz_deq_enqueue_next(DvzDeqItem* deq_item, DvzDeqItem* next, bool enqueue_first)
{
    ANN(deq_item);
    ANN(next);

    if (deq_item->next_items == NULL)
    {
        ASSERT(deq_item->next_count == 0);
        deq_item->next_items = calloc(2, sizeof(DvzDeqItemNext));
    }
    ANN(deq_item->next_items);
    if (deq_item->next_count >= 2)
    {
        // TO DO: implement this, just need a REALLOC with a 2x size
        log_error("more than 2 next items: not currently supported");
        return;
    }

    DvzDeqItemNext* next_item = &deq_item->next_items[deq_item->next_count++];
    next_item->next_item = next;
}



void dvz_deq_enqueue_submit(DvzDeq* deq, DvzDeqItem* deq_item, bool enqueue_first)
{
    ANN(deq);
    ANN(deq_item);

    _deq_enqueue_item(deq, deq_item->deq_idx, deq_item, enqueue_first);
}



void dvz_deq_discard(DvzDeq* deq, uint32_t deq_idx, int max_size)
{
    ANN(deq);
    ASSERT(deq_idx < deq->queue_count);
    DvzFifo* fifo = _deq_fifo(deq, deq_idx);
    dvz_fifo_discard(fifo, max_size);
}



DvzDeqItem dvz_deq_peek_first(DvzDeq* deq, uint32_t deq_idx)
{
    ANN(deq);
    ASSERT(deq_idx < deq->queue_count);
    DvzFifo* fifo = _deq_fifo(deq, deq_idx);
    return *((DvzDeqItem*)(fifo->items[fifo->head]));
}



DvzDeqItem dvz_deq_peek_last(DvzDeq* deq, uint32_t deq_idx)
{
    ANN(deq);
    ASSERT(deq_idx < deq->queue_count);
    DvzFifo* fifo = _deq_fifo(deq, deq_idx);
    int32_t last = fifo->tail - 1;
    if (last < 0)
        last += fifo->capacity;
    ASSERT(0 <= last && last < fifo->capacity);
    return *((DvzDeqItem*)(fifo->items[last]));
}



// WARNING: the deq_item.item pointer must be FREE-ed by the caller.
DvzDeqItem dvz_deq_dequeue_return(DvzDeq* deq, uint32_t proc_idx, bool wait)
{
    ANN(deq);

    DvzFifo* fifo = NULL;
    DvzDeqItem* deq_item = NULL;
    DvzDeqItem item_s = {0};

    ASSERT(proc_idx < deq->proc_count);
    DvzDeqProc* proc = &deq->procs[proc_idx];

    dvz_mutex_lock(&proc->lock);

    // Wait until the queue is not empty.
    if (wait)
    {
        log_trace("waiting for one of the queues in proc #%d to be non-empty", proc_idx);
        while (_deq_size(deq, proc->queue_count, proc->queue_indices) == 0)
        {
            log_trace("waiting for proc #%d cond", proc_idx);
            if (_proc_wait(proc) != 0)
            {
                // If the timeout-ed wait was unsuccessful, we will continue waiting at the
                // next iteration. But before that, we call the proc wait callbacks.
                // _proc_wait_callbacks(deq, proc_idx);
            }
        }
        log_trace("proc #%d has an item", proc_idx);
    }

    // Here, we know there is at least one item to dequeue because one of the queues is non-empty.
    log_trace("finished waiting dequeue");

    // Go through the passed queue indices.
    uint32_t deq_idx = 0;
    for (uint32_t i = 0; i < proc->queue_count; i++)
    {
        // This is the ID of the queue.
        // NOTE: process the queues circularly so that all queues successively get a chance to be
        // dequeued, even if another queue is getting filled more quickly.
        deq_idx = proc->queue_indices[(i + proc->queue_offset) % proc->queue_count];
        ASSERT(deq_idx < deq->queue_count);

        // Get that FIFO queue.
        fifo = _deq_fifo(deq, deq_idx);

        // Dequeue it immediately, return NULL if the queue was empty.
        deq_item = dvz_fifo_dequeue(fifo, false);
        if (deq_item != NULL)
        {
            // Make a copy of the struct.
            item_s = *deq_item;
            // Consistency check.
            ASSERT(deq_idx == item_s.deq_idx);
            // log_trace("dequeue item from FIFO queue #%d with type %d", deq_idx, item_s.type);
            FREE(deq_item);
            break;
        }
        // log_trace("queue #%d was empty", deq_idx);
    }
    // IMPORTANT: we must unlock BEFORE calling the callbacks if we want to permit callbacks to
    // enqueue new tasks.
    dvz_mutex_unlock(&proc->lock);

    // Then, call the typed callbacks.
    if (item_s.item != NULL)
    {
        dvz_atomic_set(proc->is_processing, 1);
        _deq_callbacks(deq, &item_s);
    }

    dvz_atomic_set(proc->is_processing, 0);

    // Enqueue the next items.
    _enqueue_next(deq, 1, &item_s);

    // Implement the dequeue strategy here. If queue_offset remains at 0, the dequeue will first
    // empty the first queue, then move to the second queue, etc. Otherwise, all queues will be
    // handled one after the other.
    if (proc->strategy == DVZ_DEQ_STRATEGY_BREADTH_FIRST)
        proc->queue_offset = (proc->queue_offset + 1) % proc->queue_count;

    return item_s;
}



// NOTE: this function FREEs the item and does not return it.
void dvz_deq_dequeue(DvzDeq* deq, uint32_t proc_idx, bool wait)
{
    DvzDeqItem item = dvz_deq_dequeue_return(deq, proc_idx, wait);
    if (item.item != NULL)
        FREE(item.item);
}



void dvz_deq_dequeue_loop(DvzDeq* deq, uint32_t proc_idx)
{
    ANN(deq);
    ASSERT(proc_idx < deq->proc_count);
    DvzDeqItem item = {0};

    while (true)
    {
        log_trace("waiting for proc #%d", proc_idx);
        // This call dequeues an item and also calls all registered callbacks if the item is not
        // null.
        item = dvz_deq_dequeue_return(deq, proc_idx, true);
        if (item.item == NULL)
        {
            log_debug("stop the deq loop for proc #%d", proc_idx);
            break;
        }
        else
        {
            // We free the item copy made by the enqueue function. The user-specified item pointer
            // is never used directly.
            log_trace("free item");
            FREE(item.item);
        }
        log_trace("got a deq item on proc #%d", proc_idx);
    }
}



void dvz_deq_dequeue_batch(DvzDeq* deq, uint32_t proc_idx)
{
    ANN(deq);

    DvzFifo* fifo = NULL;
    DvzDeqItem* deq_item = NULL;
    DvzDeqItem item_s = {0};

    ASSERT(proc_idx < deq->proc_count);
    DvzDeqProc* proc = &deq->procs[proc_idx];

    dvz_mutex_lock(&proc->lock);

    // Find the number of items that should be dequeued now.
    int size = _deq_size(deq, proc->queue_count, proc->queue_indices);
    ASSERT(size >= 0);
    uint32_t item_count = (uint32_t)size;
    DvzDeqItem* items = NULL;
    // Allocate the memory for the items to be dequeued.
    if (item_count > 0)
        items = calloc(item_count, sizeof(DvzDeqItem));
    else
    {
        // Skip this function if there are no pending items in the dequeue.
        dvz_mutex_unlock(&proc->lock);
        return;
    }
    uint32_t k = 0;     // item index for each queue
    uint32_t k_tot = 0; // item index across all queues

    // Call the BEGIN batch callbacks.
    dvz_atomic_set(proc->is_processing, 1);
    // NOTE: we cannot pass the items array at BEGIN because we haven't dequeued the items yet.
    // _proc_batch_callbacks(deq, proc_idx, DVZ_DEQ_PROC_BATCH_BEGIN, item_count, NULL);
    dvz_atomic_set(proc->is_processing, 0);

    // Go through the queue indices.
    uint32_t deq_idx = 0;
    for (uint32_t i = 0; i < proc->queue_count; i++)
    {
        k = 0;
        // This is the ID of the queue.
        deq_idx = proc->queue_indices[i];
        ASSERT(deq_idx < deq->queue_count);

        // Get that FIFO queue.
        fifo = _deq_fifo(deq, deq_idx);

        // Dequeue it immediately, return NULL if the queue was empty.
        // NOTE: the dequeue strategie is implemented in this function.
        deq_item = dvz_fifo_dequeue(fifo, false);
        while (deq_item != NULL)
        {
            // Make a copy of the struct.
            item_s = *deq_item;
            // Consistency check.
            ASSERT(deq_idx == item_s.deq_idx);
            // log_trace("dequeue item from FIFO queue #%d with type %d", deq_idx, item_s.type);
            FREE(deq_item);
            // Copy the item into the array allocated above.
            items[k++] = item_s;
            k_tot++;
            // Dequeue the next item, if any.
            deq_item = dvz_fifo_dequeue(fifo, false);
        }
        // log_trace("%d items batch-dequeued from queue #%d", k, deq_idx);
    }
    // log_trace("%d items batch-dequeued from %d queues", k_tot, proc->queue_count);
    ASSERT(k_tot == item_count);

    // IMPORTANT: we must unlock BEFORE calling the callbacks if we want to permit callbacks to
    // enqueue new tasks.
    dvz_mutex_unlock(&proc->lock);

    // Call the typed callbacks.
    dvz_atomic_set(proc->is_processing, 1);
    for (uint32_t i = 0; i < item_count; i++)
    {
        if (items[i].item != NULL)
        {
            _deq_callbacks(deq, &items[i]);
        }
    }

    dvz_atomic_set(proc->is_processing, 0);

    // Enqueue the next items.
    _enqueue_next(deq, item_count, items);

    // FREE all items, and the items container.
    for (uint32_t i = 0; i < item_count; i++)
    {
        // We free the item copy made by the enqueue function. The user-specified item pointer
        // is never used directly.
        if (items[i].item != NULL)
        {
            // log_trace("free item #%d in proc #%d", i, proc_idx);
            FREE(items[i].item);
        }
    }
    FREE(items);
}



void dvz_deq_wait(DvzDeq* deq, uint32_t proc_idx)
{
    ANN(deq);

    ASSERT(proc_idx < deq->proc_count);
    DvzDeqProc* proc = &deq->procs[proc_idx];
    log_trace("start waiting for proc #%d", proc_idx);

    while (_deq_size(deq, proc->queue_count, proc->queue_indices) > 0 ||
           dvz_atomic_get(proc->is_processing))
    {
        dvz_sleep(1);
    }
    log_trace("finished waiting for empty queues");
}



static char* _strcat(char* dest, char* src)
{
    while (*dest)
        dest++;
    while ((*dest++ = *src++))
        ;
    return --dest;
}

void dvz_deq_stats(DvzDeq* deq)
{
    ANN(deq);

    char s[1024] = {0};
    char sn[8] = {0};
    for (uint32_t i = 0; i < deq->queue_count; i++)
    {
        snprintf(sn, sizeof(sn), "%d", dvz_fifo_size(deq->queues[i]));
        _strcat(s, sn);
        if (i < deq->queue_count - 1)
            _strcat(s, ", ");
    }
    log_info("queue sizes: %s", s);
}



void dvz_deq_destroy(DvzDeq* deq)
{
    ANN(deq);
    log_trace("destroy deq");

    // Empty the queues before destryoing them, to ensure all item copies are FREE-ed.
    DvzDeqItem item = {0};
    for (uint32_t i = 0; i < deq->proc_count; i++)
    {
        while (true)
        {
            item = dvz_deq_dequeue_return(deq, i, false);
            if (item.item == NULL)
                break;
            else
                FREE(item.item);
        }
    }

    for (uint32_t i = 0; i < deq->proc_count; i++)
    {
        dvz_mutex_destroy(&deq->procs[i].lock);
        dvz_cond_destroy(&deq->procs[i].cond);
        dvz_atomic_destroy(deq->procs[i].is_processing);
    }

    for (uint32_t i = 0; i < deq->queue_count; i++)
        dvz_fifo_destroy(deq->queues[i]);

    FREE(deq);
}
