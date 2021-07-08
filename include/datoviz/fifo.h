/*************************************************************************************************/
/*  Standalone, thread-safe, generic FIFO queue                                                  */
/*************************************************************************************************/

#ifndef DVZ_FIFO_HEADER
#define DVZ_FIFO_HEADER

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_MAX_FIFO_CAPACITY 256
#define DVZ_DEQ_MAX_QUEUES    8
#define DVZ_DEQ_MAX_CALLBACKS 32



/*************************************************************************************************/
/*  Type definitions                                                                             */
/*************************************************************************************************/

typedef struct DvzFifo DvzFifo;
typedef struct DvzDeq DvzDeq;
typedef struct DvzDeqItem DvzDeqItem;
typedef struct DvzDeqCallbackRegister DvzDeqCallbackRegister;

typedef void (*DvzDeqCallback)(DvzDeq* deq, void* item, void* user_data);



/*************************************************************************************************/
/*  FIFO queue                                                                                   */
/*************************************************************************************************/

struct DvzFifo
{
    int32_t tail, head;
    int32_t capacity;
    void** items;
    void* user_data;

    pthread_mutex_t lock;
    pthread_cond_t cond;

    atomic(bool, is_processing);
    atomic(bool, is_empty);
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
};

struct DvzDeqItem
{
    uint32_t deq_idx;
    int type;
    void* item;
};

struct DvzDeq
{
    uint32_t queue_count;
    DvzFifo queues[DVZ_DEQ_MAX_QUEUES];

    uint32_t callback_count;
    DvzDeqCallbackRegister callbacks[DVZ_DEQ_MAX_CALLBACKS];
};



/*************************************************************************************************/
/*  FIFO queue                                                                                   */
/*************************************************************************************************/

/**
 * Create a FIFO queue.
 *
 * @param capacity the maximum size
 * @returns a FIFO queue
 */
DVZ_EXPORT DvzFifo dvz_fifo(int32_t capacity);

/**
 * Enqueue an object in a queue.
 *
 * @param fifo the FIFO queue
 * @param item the pointer to the object to enqueue
 */
DVZ_EXPORT void dvz_fifo_enqueue(DvzFifo* fifo, void* item);

/**
 * Enqueue an object first in a queue.
 *
 * @param fifo the FIFO queue
 * @param item the pointer to the object to enqueue
 */
DVZ_EXPORT void dvz_fifo_enqueue_first(DvzFifo* fifo, void* item);

/**
 * Dequeue an object from a queue.
 *
 * @param fifo the FIFO queue
 * @param wait whether to return immediately, or wait until the queue is non-empty
 * @returns a pointer to the dequeued object, or NULL if the queue is empty
 */
DVZ_EXPORT void* dvz_fifo_dequeue(DvzFifo* fifo, bool wait);

/**
 * Get the number of items in a queue.
 *
 * @param fifo the FIFO queue
 * @returns the number of elements in the queue
 */
DVZ_EXPORT int dvz_fifo_size(DvzFifo* fifo);

/**
 * Discard old items in a queue.
 *
 * This function will suppress all items in the queue except the `max_size` most recent ones.
 *
 * @param fifo the FIFO queue
 * @param max_size the number of items to keep in the queue.
 */
DVZ_EXPORT void dvz_fifo_discard(DvzFifo* fifo, int max_size);

/**
 * Delete all items in a queue.
 *
 * @param fifo the FIFO queue
 */
DVZ_EXPORT void dvz_fifo_reset(DvzFifo* fifo);

/**
 * Destroy a queue.
 *
 * @param fifo the FIFO queue
 */
DVZ_EXPORT void dvz_fifo_destroy(DvzFifo* fifo);



/*************************************************************************************************/
/*  Dequeues                                                                                     */
/*************************************************************************************************/

DVZ_EXPORT DvzDeq dvz_deq(uint32_t nq);

DVZ_EXPORT void dvz_deq_callback(
    DvzDeq* deq, uint32_t deq_idx, int type, DvzDeqCallback callback, void* user_data);

DVZ_EXPORT void dvz_deq_enqueue(DvzDeq* deq, uint32_t deq_idx, int type, void* item);

DVZ_EXPORT void dvz_deq_enqueue_first(DvzDeq* deq, uint32_t deq_idx, int type, void* item);

DVZ_EXPORT void dvz_deq_discard(DvzDeq* deq, uint32_t deq_idx, int max_size);

DVZ_EXPORT DvzDeqItem dvz_deq_peek_first(DvzDeq* deq, uint32_t deq_idx);

DVZ_EXPORT DvzDeqItem dvz_deq_peek_last(DvzDeq* deq, uint32_t deq_idx);

DVZ_EXPORT DvzDeqItem dvz_deq_dequeue(DvzDeq* deq, bool wait);

DVZ_EXPORT void dvz_deq_destroy(DvzDeq* deq);



#ifdef __cplusplus
}
#endif

#endif
