#include "../include/datoviz/fifo.h"



/*************************************************************************************************/
/*  Thread-safe FIFO queue                                                                       */
/*************************************************************************************************/

DvzFifo dvz_fifo(int32_t capacity)
{
    log_trace("creating generic FIFO queue with a capacity of %d items", capacity);
    ASSERT(capacity >= 2);
    DvzFifo fifo = {0};
    ASSERT(capacity <= DVZ_MAX_FIFO_CAPACITY);
    fifo.capacity = capacity;
    fifo.is_empty = true;
    fifo.items = calloc((uint32_t)capacity, sizeof(void*));

    if (pthread_mutex_init(&fifo.lock, NULL) != 0)
        log_error("mutex creation failed");
    if (pthread_cond_init(&fifo.cond, NULL) != 0)
        log_error("cond creation failed");

    return fifo;
}



void dvz_fifo_enqueue(DvzFifo* fifo, void* item)
{
    ASSERT(fifo != NULL);
    pthread_mutex_lock(&fifo->lock);

    // Old size
    int size = fifo->tail - fifo->head;
    if (size < 0)
        size += fifo->capacity;

    // Old capacity
    int old_cap = fifo->capacity;

    // Resize if queue is full.
    if ((fifo->tail + 1) % fifo->capacity == fifo->head)
    {
        ASSERT(fifo->items != NULL);
        ASSERT(size == fifo->capacity - 1);
        ASSERT(fifo->capacity <= DVZ_MAX_FIFO_CAPACITY);

        fifo->capacity *= 2;
        log_debug("FIFO queue is full, enlarging it to %d", fifo->capacity);
        REALLOC(fifo->items, (uint32_t)fifo->capacity * sizeof(void*));
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
    ASSERT((fifo->tail + 1) % fifo->capacity != fifo->head);
    fifo->items[fifo->tail] = item;
    fifo->tail++;
    if (fifo->tail >= fifo->capacity)
        fifo->tail -= fifo->capacity;
    fifo->is_empty = false;

    ASSERT(0 <= fifo->tail && fifo->tail < fifo->capacity);
    pthread_cond_signal(&fifo->cond);
    pthread_mutex_unlock(&fifo->lock);
}



void* dvz_fifo_dequeue(DvzFifo* fifo, bool wait)
{
    ASSERT(fifo != NULL);
    pthread_mutex_lock(&fifo->lock);

    // Wait until the queue is not empty.
    if (wait)
    {
        log_trace("waiting for the queue to be non-empty");
        while (fifo->tail == fifo->head)
            pthread_cond_wait(&fifo->cond, &fifo->lock);
    }

    // Empty queue.
    if (fifo->tail == fifo->head)
    {
        // log_trace("FIFO queue was empty");
        // Don't forget to unlock the mutex before exiting this function.
        pthread_mutex_unlock(&fifo->lock);
        fifo->is_empty = true;
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
        fifo->is_empty = true;

    pthread_mutex_unlock(&fifo->lock);
    return item;
}



int dvz_fifo_size(DvzFifo* fifo)
{
    ASSERT(fifo != NULL);
    pthread_mutex_lock(&fifo->lock);
    // log_debug("tail %d head %d", fifo->tail, fifo->head);
    int size = fifo->tail - fifo->head;
    if (size < 0)
        size += fifo->capacity;
    ASSERT(0 <= size && size <= fifo->capacity);
    pthread_mutex_unlock(&fifo->lock);
    return size;
}



void dvz_fifo_discard(DvzFifo* fifo, int max_size)
{
    ASSERT(fifo != NULL);
    if (max_size == 0)
        return;
    pthread_mutex_lock(&fifo->lock);
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
    pthread_mutex_unlock(&fifo->lock);
}



void dvz_fifo_reset(DvzFifo* fifo)
{
    ASSERT(fifo != NULL);
    pthread_mutex_lock(&fifo->lock);
    fifo->tail = 0;
    fifo->head = 0;
    pthread_cond_signal(&fifo->cond);
    pthread_mutex_unlock(&fifo->lock);
}



void dvz_fifo_destroy(DvzFifo* fifo)
{
    ASSERT(fifo != NULL);
    pthread_mutex_destroy(&fifo->lock);
    pthread_cond_destroy(&fifo->cond);

    ASSERT(fifo->items != NULL);
    FREE(fifo->items);
}



/*************************************************************************************************/
/*  Dequeues                                                                                     */
/*************************************************************************************************/

static DvzFifo* _deq_fifo(DvzDeq* deq, uint32_t deq_idx)
{
    ASSERT(deq != NULL);
    ASSERT(deq_idx < deq->queue_count);

    DvzFifo* fifo = &deq->queues[deq_idx];
    ASSERT(fifo != NULL);
    ASSERT(fifo->capacity > 0);
    return fifo;
}



// Call all callback functions registered with a deq_idx and type on a deq item.
static void _deq_callbacks(DvzDeq* deq, DvzDeqItem item_s)
{
    ASSERT(deq != NULL);
    ASSERT(item_s.item != NULL);
    DvzDeqCallbackRegister* reg = NULL;
    for (uint32_t i = 0; i < deq->callback_count; i++)
    {
        reg = &deq->callbacks[i];
        ASSERT(reg != NULL);
        if (reg->deq_idx == item_s.deq_idx && reg->type == item_s.type)
        {
            reg->callback(deq, item_s.item, reg->user_data);
        }
    }
}



DvzDeq dvz_deq(uint32_t nq)
{
    DvzDeq deq = {0};
    deq.queue_count = nq;
    for (uint32_t i = 0; i < nq; i++)
        deq.queues[i] = dvz_fifo(DVZ_MAX_FIFO_CAPACITY);
    return deq;
}



void dvz_deq_callback(
    DvzDeq* deq, uint32_t deq_idx, int type, DvzDeqCallback callback, void* user_data)
{
    ASSERT(deq != NULL);
    ASSERT(callback != NULL);

    DvzDeqCallbackRegister* reg = &deq->callbacks[deq->callback_count++];
    ASSERT(reg != NULL);

    reg->deq_idx = deq_idx;
    reg->type = type;
    reg->callback = callback;
    reg->user_data = user_data;
}



void dvz_deq_enqueue(DvzDeq* deq, uint32_t deq_idx, int type, void* item)
{
    ASSERT(deq != NULL);
    ASSERT(deq_idx < deq->queue_count);
    DvzFifo* fifo = _deq_fifo(deq, deq_idx);
    DvzDeqItem* deq_item = calloc(1, sizeof(DvzDeqItem));
    ASSERT(deq_item != NULL);
    deq_item->deq_idx = deq_idx;
    deq_item->type = type;
    deq_item->item = item;
    dvz_fifo_enqueue(fifo, deq_item);
}



void dvz_deq_enqueue_last(DvzDeq* deq, uint32_t deq_idx, int type, void* item)
{
    ASSERT(deq != NULL);
    ASSERT(deq_idx < deq->queue_count);
    DvzFifo* fifo = _deq_fifo(deq, deq_idx);
    // TODO
    log_error("not implemented yet");
}



void dvz_deq_discard(DvzDeq* deq, uint32_t deq_idx, uint32_t first, uint32_t count)
{
    ASSERT(deq != NULL);
    ASSERT(deq_idx < deq->queue_count);
    DvzFifo* fifo = _deq_fifo(deq, deq_idx);
    // TODO
    log_error("not implemented yet");
}



DvzDeqItem dvz_deq_peek_first(DvzDeq* deq, uint32_t deq_idx)
{
    ASSERT(deq != NULL);
    ASSERT(deq_idx < deq->queue_count);
    DvzFifo* fifo = _deq_fifo(deq, deq_idx);
    return *((DvzDeqItem*)(fifo->items[fifo->head]));
}



DvzDeqItem dvz_deq_peek_last(DvzDeq* deq, uint32_t deq_idx)
{
    ASSERT(deq != NULL);
    ASSERT(deq_idx < deq->queue_count);
    DvzFifo* fifo = _deq_fifo(deq, deq_idx);
    int32_t last = fifo->tail - 1;
    if (last < 0)
        last += fifo->capacity;
    ASSERT(0 <= last && last < fifo->capacity);
    return *((DvzDeqItem*)(fifo->items[last]));
}



DvzDeqItem dvz_deq_dequeue(DvzDeq* deq)
{
    ASSERT(deq != NULL);

    DvzFifo* fifo = NULL;
    DvzDeqItem* deq_item = NULL;
    DvzDeqItem item_s = {0};

    // Find the first non-empty FIFO queue, and dequeue it.
    for (uint32_t deq_idx = 0; deq_idx < deq->queue_count; deq_idx++)
    {
        fifo = _deq_fifo(deq, deq_idx);
        deq_item = dvz_fifo_dequeue(fifo, false);
        if (deq_item != NULL)
        {
            item_s = *deq_item;
            log_trace(
                "dequeue item from FIFO queue #%d with type %d", item_s.deq_idx, item_s.type);
            ASSERT(item_s.item != NULL);
            FREE(deq_item);
            break;
        }
    }

    // Call the associated callbacks automatically.
    if (item_s.item != NULL)
    {
        _deq_callbacks(deq, item_s);
    }

    return item_s;
}



void dvz_deq_destroy(DvzDeq* deq)
{
    ASSERT(deq != NULL);
    for (uint32_t i = 0; i < deq->queue_count; i++)
        dvz_fifo_destroy(&deq->queues[i]);
}
