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

#define DVZ_MAX_FIFO_CAPACITY 64



/*************************************************************************************************/
/*  Type definitions                                                                             */
/*************************************************************************************************/

typedef struct DvzFifo DvzFifo;



/*************************************************************************************************/
/*  FIFO queue                                                                                   */
/*************************************************************************************************/

struct DvzFifo
{
    int32_t head, tail;
    int32_t capacity;
    void** items;
    void* user_data;

    pthread_mutex_t lock;
    pthread_cond_t cond;

    atomic(bool, is_processing);
    atomic(bool, is_empty);
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



#ifdef __cplusplus
}
#endif

#endif
