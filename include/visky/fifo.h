/*************************************************************************************************/
/*  Standalone, thread-safe, generic FIFO queue                                                  */
/*************************************************************************************************/

#ifndef VKL_FIFO_HEADER
#define VKL_FIFO_HEADER

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define VKL_MAX_FIFO_CAPACITY 64



/*************************************************************************************************/
/*  Type definitions                                                                             */
/*************************************************************************************************/

typedef struct VklFifo VklFifo;



/*************************************************************************************************/
/*  FIFO queue                                                                                   */
/*************************************************************************************************/

struct VklFifo
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
VKY_EXPORT VklFifo vkl_fifo(int32_t capacity);

/**
 * Enqueue an object in a queue.
 *
 * @param fifo the FIFO queue
 * @param item the pointer to the object to enqueue
 */
VKY_EXPORT void vkl_fifo_enqueue(VklFifo* fifo, void* item);

/**
 * Dequeue an object from a queue.
 *
 * @param fifo the FIFO queue
 * @param wait whether to return immediately, or wait until the queue is non-empty
 * @returns a pointer to the dequeued object, or NULL if the queue is empty
 */
VKY_EXPORT void* vkl_fifo_dequeue(VklFifo* fifo, bool wait);

/**
 * Get the number of items in a queue.
 *
 * @param fifo the FIFO queue
 * @returns the number of elements in the queue
 */
VKY_EXPORT int vkl_fifo_size(VklFifo* fifo);

/**
 * Discard old items in a queue.
 *
 * This function will suppress all items in the queue except the `max_size` most recent ones.
 *
 * @param fifo the FIFO queue
 * @param max_size the number of items to keep in the queue.
 */
VKY_EXPORT void vkl_fifo_discard(VklFifo* fifo, int max_size);

/**
 * Delete all items in a queue.
 *
 * @param fifo the FIFO queue
 */
VKY_EXPORT void vkl_fifo_reset(VklFifo* fifo);

/**
 * Destroy a queue.
 *
 * @param fifo the FIFO queue
 */
VKY_EXPORT void vkl_fifo_destroy(VklFifo* fifo);



#ifdef __cplusplus
}
#endif

#endif
