/*************************************************************************************************/
/*  Standalone, thread-safe, generic FIFO queue                                                  */
/*************************************************************************************************/

#ifndef DVZ_HEADER_FIFO
#define DVZ_HEADER_FIFO


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_atomic.h"
#include "_macros.h"
#include "_thread.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_MAX_FIFO_CAPACITY 256
#define DVZ_DEQ_MAX_QUEUES    8
#define DVZ_DEQ_MAX_PROC_SIZE 4
#define DVZ_DEQ_MAX_PROCS     4
#define DVZ_DEQ_MAX_CALLBACKS 32



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Proc callback position: pre or post.
typedef enum
{
    DVZ_DEQ_PROC_CALLBACK_PRE,
    DVZ_DEQ_PROC_CALLBACK_POST,
} DvzDeqProcCallbackPosition;



// Batch callback position: begin or end.
typedef enum
{
    DVZ_DEQ_PROC_BATCH_BEGIN,
    DVZ_DEQ_PROC_BATCH_END,
} DvzDeqProcBatchPosition;



// Dequeue strategy: breadth-first (default) or depth-first.
typedef enum
{
    DVZ_DEQ_STRATEGY_BREADTH_FIRST,
    DVZ_DEQ_STRATEGY_DEPTH_FIRST,
} DvzDeqStrategy;



/*************************************************************************************************/
/*  Type definitions                                                                             */
/*************************************************************************************************/

typedef struct DvzFifo DvzFifo;
typedef struct DvzDeq DvzDeq;
typedef struct DvzDeqItem DvzDeqItem;
typedef struct DvzDeqItemNext DvzDeqItemNext;
typedef struct DvzDeqProc DvzDeqProc;
typedef struct DvzDeqCallbackRegister DvzDeqCallbackRegister;
typedef struct DvzDeqProcCallbackRegister DvzDeqProcCallbackRegister;
typedef struct DvzDeqProcWaitCallbackRegister DvzDeqProcWaitCallbackRegister;
typedef struct DvzDeqProcBatchCallbackRegister DvzDeqProcBatchCallbackRegister;

typedef void (*DvzDeqCallback)(DvzDeq* deq, void* item, void* user_data);
typedef void (*DvzDeqProcCallback)(
    DvzDeq* deq, uint32_t deq_idx, int type, void* item, void* user_data);
typedef void (*DvzDeqProcWaitCallback)(DvzDeq* deq, void* user_data);
typedef void (*DvzDeqProcBatchCallback)(
    DvzDeq* deq, DvzDeqProcBatchPosition pos, uint32_t item_count, DvzDeqItem* items,
    void* user_data);



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
    bool is_default; // if true, the callback will be discarded if there are other callbacks
};

struct DvzDeqProcWaitCallbackRegister
{
    DvzDeqProcWaitCallback callback;
    void* user_data;
};

struct DvzDeqProcBatchCallbackRegister
{
    DvzDeqProcBatchCallback callback;
    DvzDeqProcBatchPosition pos;
    void* user_data;
};

struct DvzDeqProcCallbackRegister
{
    DvzDeqProcCallback callback;
    DvzDeqProcCallbackPosition pos;
    void* user_data;
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

    // Callbacks called after every dequeue, independently of the deq idx and type, either before
    // or after the item callbacks.
    uint32_t callback_count;
    DvzDeqProcCallbackRegister callbacks[DVZ_DEQ_MAX_CALLBACKS];

    // Callbacks called while waiting with a max_wait delay.
    uint32_t wait_callback_count;
    DvzDeqProcWaitCallbackRegister wait_callbacks[DVZ_DEQ_MAX_CALLBACKS];

    // Callbacks called when dequeuing multiple items at once (batch dequeue).
    uint32_t batch_callback_count;
    DvzDeqProcBatchCallbackRegister batch_callbacks[DVZ_DEQ_MAX_CALLBACKS];

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
    uint32_t queue_count;
    DvzFifo queues[DVZ_DEQ_MAX_QUEUES];

    uint32_t callback_count;
    DvzDeqCallbackRegister callbacks[DVZ_DEQ_MAX_CALLBACKS];

    uint32_t proc_count;
    DvzDeqProc procs[DVZ_DEQ_MAX_PROCS];
    uint32_t q_to_proc[DVZ_DEQ_MAX_QUEUES]; // for each queue, which proc idx is handling it

    bool has_default; // true if there is at least one registered default callback
};



/*************************************************************************************************/
/*  FIFO queue                                                                                   */
/*************************************************************************************************/

EXTERN_C_ON



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

/**
 * Create a Deq structure.
 *
 * A Deq is a set of dequeues, or double-ended queue. One can enqueue items in any of these queues,
 * and dequeue items.
 *
 * A item is defined by a pointer, a queue index, and a type (integer).
 *
 * The Deq is thread-safe.
 *
 * The Deq is multi-producer, single consumer. Multiple threads may enqueue items, but only a
 * single thread is supposed to dequeue items. That thread may also enqueue items.
 *
 * Function callbacks can be registered: they are called every time a item is dequeue. A callback
 * is defined by its queue index, and a item type. It will only be called for items that were
 * dequeued from the specified queue, if these items have the appropriate type.
 *
 * A Proc represents a pair of "processes" (to be understood in the general sense, not OS
 * processes), with a producer and a consumer. It is defined by a subset of the queues, which are
 * supposed to be dequeued from the item dequeueing loop (typically in a dedicated thread).
 *
 * @param capacity the maximum size
 * @returns a Deq
 */
DVZ_EXPORT DvzDeq dvz_deq(uint32_t nq);



/**
 * Define a callback.
 *
 * @param deq the Deq
 * @param deq_idx the queue index
 * @param type the type to register the callback to
 * @param user_data pointer to arbitrary data to be passed to the callback
 */
DVZ_EXPORT void dvz_deq_callback(
    DvzDeq* deq, uint32_t deq_idx, int type, DvzDeqCallback callback, void* user_data);



/**
 * Define a default callback.
 *
 * The default callback is only called when there are no other callbacks. Specifically, we discard
 * a default callback iff there are other callbacks after it in the list of callbacks.
 *
 * @param deq the Deq
 * @param deq_idx the queue index
 * @param type the type to register the callback to
 * @param user_data pointer to arbitrary data to be passed to the callback
 */
DVZ_EXPORT void dvz_deq_callback_default(
    DvzDeq* deq, uint32_t deq_idx, int type, DvzDeqCallback callback, void* user_data);



/**
 * Define a Proc.
 *
 * @param deq the Deq
 * @param proc_idx the Proc index (should be regularly increasing: 0, 1, 2, ...)
 * @param queue_count the number of queues in the Proc
 * @param queue_ids the indices of the queues in the Proc
 */
DVZ_EXPORT void
dvz_deq_proc(DvzDeq* deq, uint32_t proc_idx, uint32_t queue_count, uint32_t* queue_ids);



/**
 * Set a maximum time delay for dequeue waiting, in milliseconds.
 *
 * @param deq the Deq
 * @param proc_idx the Proc index
 * @param delay_ms how many milliseconds to wait before probing for deq size while dequeueing
 */
DVZ_EXPORT void dvz_deq_proc_wait_delay(DvzDeq* deq, uint32_t proc_idx, uint32_t delay_ms);



/**
 * Register a Proc callback.
 *
 * A Proc callback is called in the Deq loop for the associated proc, after every item dequeue.
 *
 * @param deq the Deq
 * @param proc_idx the Proc index (should be regularly increasing: 0, 1, 2, ...)
 * @param pos this callback should be called either before or after the item callbacks
 * @param callback the callback
 * @param user_data pointer to arbitrary data
 */
DVZ_EXPORT void dvz_deq_proc_callback(
    DvzDeq* deq, uint32_t proc_idx, DvzDeqProcCallbackPosition pos, DvzDeqProcCallback callback,
    void* user_data);



/**
 * Register a Proc wait callback.
 *
 * A Proc callback is called in the Deq loop for the associated proc, while waiting for a dequeue.
 * It may be used to raise Timer events.
 *
 * @param deq the Deq
 * @param proc_idx the Proc index (should be regularly increasing: 0, 1, 2, ...)
 * @param callback the callback
 * @param user_data pointer to arbitrary data
 */
DVZ_EXPORT void dvz_deq_proc_wait_callback(
    DvzDeq* deq, uint32_t proc_idx, DvzDeqProcWaitCallback callback, void* user_data);



/**
 * Register a Proc batch callback.
 *
 * A batch callback is called in dvz_deq_dequeue_batch(), before *and* after dequeuing all items in
 * a proc.
 *
 * @param deq the Deq
 * @param proc_idx the Proc index (should be regularly increasing: 0, 1, 2, ...)
 * @param pos called either at the beginning or at the end of the batch dequeue
 * @param callback the callback
 * @param user_data pointer to arbitrary data
 */
DVZ_EXPORT void dvz_deq_proc_batch_callback(
    DvzDeq* deq, uint32_t proc_idx, DvzDeqProcBatchPosition pos, DvzDeqProcBatchCallback callback,
    void* user_data);



/**
 * Enqueue an item.
 *
 * !!! warning
 *     When using the built-in Deq loop, the passed pointer `item` MUST be allocated with malloc()
 *     or equivalent as it will be automatically freed by the Deq loop.
 *
 * @param deq the Deq
 * @param deq_idx the queue index
 * @param type the item type
 * @param item a pointer to the item
 */
DVZ_EXPORT void dvz_deq_enqueue(DvzDeq* deq, uint32_t deq_idx, int type, void* item);



/**
 * Enqueue an item at the first position.
 *
 * @param deq the Deq
 * @param deq_idx the queue index
 * @param type the item type
 * @param item a pointer to the item
 */
DVZ_EXPORT void dvz_deq_enqueue_first(DvzDeq* deq, uint32_t deq_idx, int type, void* item);



/**
 * Create a custom task to be enqueued later. This is used to introduce dependencies between tasks.
 *
 * @param deq_idx the queue index
 * @param type the type
 * @param item the item
 */
DVZ_EXPORT DvzDeqItem* dvz_deq_enqueue_custom(uint32_t deq_idx, int type, void* item);



/**
 * Introduce a dependency between two tasks created by `dvz_deq_enqueue_custom()`.
 *
 * @param deq_item the first task
 * @param next the second task, that will have to be enqueued after the first task's callbacks
 * @param enqueue_first whether the second task need to be enqueued at the start of the queue
 */
DVZ_EXPORT void dvz_deq_enqueue_next(DvzDeqItem* deq_item, DvzDeqItem* next, bool enqueue_first);



/**
 * Enqueue a task created with `dvz_deq_enqueue_custom()`.
 *
 * @param deq the Deq
 * @param deq_item the item to enqueue
 * @param enqueue_first whether to enqueue the task at the start of the queue or not
 */
DVZ_EXPORT void dvz_deq_enqueue_submit(DvzDeq* deq, DvzDeqItem* deq_item, bool enqueue_first);



/**
 * Delete a number of items in a given queue.
 *
 * @param deq the Deq
 * @param deq_idx the queue index
 * @param max_size the maximum number of items to delete
 */
DVZ_EXPORT void dvz_deq_discard(DvzDeq* deq, uint32_t deq_idx, int max_size);



/**
 * Return the first item item in a given queue.
 *
 * @param deq the Deq
 * @param deq_idx the queue index
 * @returns the item
 */
DVZ_EXPORT DvzDeqItem dvz_deq_peek_first(DvzDeq* deq, uint32_t deq_idx);



/**
 * Return thea last item item in a given queue.
 *
 * @param deq the Deq
 * @param deq_idx the queue index
 * @returns the item
 */
DVZ_EXPORT DvzDeqItem dvz_deq_peek_last(DvzDeq* deq, uint32_t deq_idx);



/**
 * Specify the dequeue strategy: breadth-first or depth-first.
 *
 * @param deq the Deq
 * @param proc_idx the Proc index
 * @param strategy the dequeue strategy
 */
DVZ_EXPORT void dvz_deq_strategy(DvzDeq* deq, uint32_t proc_idx, DvzDeqStrategy strategy);



/**
 * Dequeue a single item from one of the queues of a given proc.
 *
 * @param deq the Deq
 * @param proc_idx the Proc index
 * @param wait whether this call should be blocking
 * @returns the dequeue-ed item if a queue was non-empty, or an empty item
 */
DVZ_EXPORT DvzDeqItem dvz_deq_dequeue(DvzDeq* deq, uint32_t proc_idx, bool wait);



/**
 * Start a blocking dequeue loop that will only stop if another thread enqueues an empty item.
 *
 * !!! warning
 *     When using this dequeue loop, all enqueued items must be alloc-ed on the heap because they
 *     will be FREE-ed automatically!
 *
 * @param deq the Deq
 * @param proc_idx the Proc index
 */
DVZ_EXPORT void dvz_deq_dequeue_loop(DvzDeq* deq, uint32_t proc_idx);



/**
 * Immediately dequeue the existing items in batch from all queues in a proc.
 *
 * The registered callbacks will be called as usual for every dequeued item. But in addition to
 * that, batch BEGIN or END callbacks will be also called before and after the dequeues.
 *
 * !!! warning
 *     When using this dequeue loop, all enqueued items must be alloc-ed on the heap because they
 *     will be FREE-ed automatically!
 *
 * @param deq the Deq
 * @param proc_idx the Proc index
 */
DVZ_EXPORT void dvz_deq_dequeue_batch(DvzDeq* deq, uint32_t proc_idx);



/**
 * Wait until all queues within a given Proc are empty.
 *
 * @param deq the Deq
 * @param proc_idx the Proc index
 */
DVZ_EXPORT void dvz_deq_wait(DvzDeq* deq, uint32_t proc_idx);



/**
 * Destroy a Deq.
 *
 * @param deq the Deq
 */
DVZ_EXPORT void dvz_deq_stats(DvzDeq* deq);



/**
 * Destroy a Deq.
 *
 * @param deq the Deq
 */
DVZ_EXPORT void dvz_deq_destroy(DvzDeq* deq);



EXTERN_C_OFF

#endif
