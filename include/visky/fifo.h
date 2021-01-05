#ifndef VKL_FIFO_HEADER
#define VKL_FIFO_HEADER

#include "common.h"



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
    void* items[VKL_MAX_FIFO_CAPACITY];
    void* user_data;

    pthread_mutex_t lock;
    pthread_cond_t cond;

    atomic(bool, is_processing);
    atomic(bool, is_empty);
};



/*************************************************************************************************/
/*  FIFO queue                                                                                   */
/*************************************************************************************************/

VKY_EXPORT VklFifo vkl_fifo(int32_t capacity);

VKY_EXPORT void vkl_fifo_enqueue(VklFifo* fifo, void* item);

VKY_EXPORT void* vkl_fifo_dequeue(VklFifo* fifo, bool wait);

VKY_EXPORT int vkl_fifo_size(VklFifo* fifo);

// Discard all but max_size items in the queue (only keep the most recent ones)
VKY_EXPORT void vkl_fifo_discard(VklFifo* fifo, int max_size);

VKY_EXPORT void vkl_fifo_reset(VklFifo* fifo);

VKY_EXPORT void vkl_fifo_destroy(VklFifo* fifo);



#endif
