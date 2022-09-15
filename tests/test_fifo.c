/*************************************************************************************************/
/*  Testing FIFO                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdio.h>

#include "_thread.h"
#include "fifo.h"
#include "test.h"
#include "test_fifo.h"
#include "testing.h"



/*************************************************************************************************/
/*  Utilstests                                                                                   */
/*************************************************************************************************/

static bool _is_empty(DvzFifo* fifo)
{
    ANN(fifo);
    return dvz_atomic_get(fifo->is_empty) == 1;
}



/*************************************************************************************************/
/*  FIFO tests                                                                                   */
/*************************************************************************************************/

static int _fifo_thread_1(void* arg)
{
    DvzFifo* fifo = arg;
    uint8_t* data = dvz_fifo_dequeue(fifo, true);
    ASSERT(*data == 12);
    // Signal to the caller thread that the dequeue was successfull.
    fifo->user_data = data;
    return 0;
}



static int _fifo_thread_2(void* arg)
{
    DvzFifo* fifo = arg;
    uint8_t* numbers = calloc(5, sizeof(uint8_t));
    fifo->user_data = numbers;
    for (uint32_t i = 0; i < 5; i++)
    {
        numbers[i] = i;
        dvz_fifo_enqueue(fifo, &numbers[i]);
        dvz_sleep(10);
    }
    dvz_fifo_enqueue(fifo, NULL);
    return 0;
}



int test_fifo_1(TstSuite* suite)
{
    DvzFifo* fifo = dvz_fifo(8);
    uint8_t item = 12;

    // Enqueue + dequeue in the same thread.
    AT(fifo->is_empty);
    dvz_fifo_enqueue(fifo, &item);
    AT(!_is_empty(fifo));
    ASSERT(fifo->tail == 1);
    ASSERT(fifo->head == 0);
    uint8_t* data = dvz_fifo_dequeue(fifo, true);
    AT(_is_empty(fifo));
    ASSERT(*data == item);

    // Enqueue in the main thread, dequeue in a background thread.
    DvzThread* thread = dvz_thread(_fifo_thread_1, fifo);
    dvz_fifo_enqueue(fifo, &item);
    AT(!_is_empty(fifo));
    dvz_thread_join(thread);
    ANN(fifo->user_data);
    ASSERT(fifo->user_data == &item);

    // Multiple enqueues in the background thread, dequeue in the main thread.
    thread = dvz_thread(_fifo_thread_2, fifo);
    uint8_t* dequeued = NULL;
    uint32_t i = 0;
    do
    {
        dequeued = dvz_fifo_dequeue(fifo, true);
        if (dequeued == NULL)
            break;
        AT(*dequeued == i);
        i++;
    } while (dequeued != NULL);
    dvz_thread_join(thread);
    FREE(fifo->user_data);

    dvz_fifo_destroy(fifo);
    return 0;
}



int test_fifo_2(TstSuite* suite)
{
    DvzFifo* fifo = dvz_fifo(8);
    uint32_t numbers[64] = {0};
    for (uint32_t i = 0; i < 64; i++)
    {
        numbers[i] = i;
        dvz_fifo_enqueue(fifo, &numbers[i]);
    }
    uint32_t* res = NULL;
    for (uint32_t i = 0; i < 64; i++)
    {
        AT(!_is_empty(fifo));
        res = dvz_fifo_dequeue(fifo, false);
        AT(*res == i);
    }
    AT(_is_empty(fifo));
    dvz_fifo_destroy(fifo);
    return 0;
}



int test_fifo_resize(TstSuite* suite)
{
    DvzFifo* fifo = dvz_fifo(8);
    uint32_t numbers[256] = {0};
    uint32_t i = 0;
    for (i = 0; i < 64; i++)
    {
        numbers[i] = i;
        dvz_fifo_enqueue(fifo, &numbers[i]);
        if (i % 2 == 0)
            dvz_fifo_dequeue(fifo, false);
    }

    i = 0;
    uint32_t* n = 0;
    while (dvz_fifo_size(fifo) > 0)
    {
        n = dvz_fifo_dequeue(fifo, false);
        AT(*n == i + 32);
        i++;
    }
    dvz_fifo_destroy(fifo);
    return 0;
}



int test_fifo_discard(TstSuite* suite)
{
    DvzFifo* fifo = dvz_fifo(8);
    uint32_t numbers[8] = {0};
    for (uint32_t i = 0; i < 7; i++)
    {
        numbers[i] = i;
        dvz_fifo_enqueue(fifo, &numbers[i]);
    }
    AT(fifo->capacity == 8);

    // First item is 0.
    AT(dvz_fifo_size(fifo) == 7);
    AT(*((int*)dvz_fifo_dequeue(fifo, false)) == 0);

    // Discard 2 elements (from size 7 to 5).
    dvz_fifo_discard(fifo, 5);

    // First item is 2.
    AT(dvz_fifo_size(fifo) == 5);
    AT(*((int*)dvz_fifo_dequeue(fifo, false)) == 2);

    dvz_fifo_destroy(fifo);
    return 0;
}



int test_fifo_first(TstSuite* suite)
{
    DvzFifo* fifo = dvz_fifo(8);
    dvz_fifo_enqueue(fifo, (int[]){1});
    dvz_fifo_enqueue(fifo, (int[]){2});
    dvz_fifo_enqueue(fifo, (int[]){3});

    AT(*((int*)dvz_fifo_dequeue(fifo, false)) == 1);
    dvz_fifo_enqueue_first(fifo, (int[]){4});
    AT(*((int*)dvz_fifo_dequeue(fifo, false)) == 4);

    dvz_fifo_destroy(fifo);
    return 0;
}



/*************************************************************************************************/
/*  Deq tests                                                                                    */
/*************************************************************************************************/

static void _deq_1_callback(DvzDeq* deq, void* item, void* user_data)
{
    ANN(deq);
    ANN(item);
    ANN(user_data);
    int* data = (int*)user_data;
    *data = *((int*)item);
}

int test_deq_1(TstSuite* suite)
{
    DvzDeq* deq = dvz_deq(2);
    DvzDeqItem item = {0};

    int data = 0;
    dvz_deq_callback(deq, 0, 0, _deq_1_callback, &data);
    dvz_deq_proc(deq, 0, 2, (uint32_t[]){0, 1});
    AT(data == 0);

    // Enqueue in the queue with a callback.
    dvz_deq_enqueue(deq, 0, 0, (int[]){2});
    item = dvz_deq_dequeue(deq, 0, false);
    AT(item.deq_idx == 0);
    AT(item.type == 0);
    AT(data == 2);

    // Enqueue in the queue without a callback.
    data = 0;
    dvz_deq_enqueue(deq, 1, 10, (int[]){2});
    item = dvz_deq_dequeue(deq, 0, false);
    AT(item.deq_idx == 1);
    AT(item.type == 10);
    AT(data == 0);

    // Enqueue in the queue with a callback.
    dvz_deq_enqueue(deq, 0, 10, (int[]){3});
    item = dvz_deq_dequeue(deq, 0, false);
    AT(item.deq_idx == 0);
    AT(item.type == 10);
    AT(data == 0);

    dvz_deq_callback(deq, 0, 10, _deq_1_callback, &data);
    dvz_deq_enqueue(deq, 0, 10, (int[]){4});
    item = dvz_deq_dequeue(deq, 0, false);
    AT(item.deq_idx == 0);
    AT(item.type == 10);
    AT(item.item != NULL);
    AT(data == 4);

    // Supbsequent dequeues are empty.
    item = dvz_deq_dequeue(deq, 0, false);
    AT(item.item == NULL);

    dvz_deq_destroy(deq);
    return 0;
}



int test_deq_2(TstSuite* suite)
{
    DvzDeq* deq = dvz_deq(2);
    dvz_deq_proc(deq, 0, 2, (uint32_t[]){0, 1});
    DvzDeqItem item = {0};

    // Enqueue in the queue with a callback.
    dvz_deq_enqueue(deq, 0, 0, (int[]){1});
    dvz_deq_enqueue(deq, 1, 0, (int[]){2});
    dvz_deq_enqueue(deq, 0, 10, (int[]){3});
    dvz_deq_enqueue(deq, 1, 10, (int[]){4});

    // First queue.
    item = dvz_deq_peek_first(deq, 0);
    AT(item.type == 0);
    AT(*(int*)(item.item) == 1);
    item = dvz_deq_peek_last(deq, 0);
    AT(item.type == 10);
    AT(*(int*)(item.item) == 3);

    // Second queue.
    item = dvz_deq_peek_first(deq, 1);
    AT(item.type == 0);
    AT(*(int*)(item.item) == 2);
    item = dvz_deq_peek_last(deq, 1);
    AT(item.type == 10);
    AT(*(int*)(item.item) == 4);

    dvz_deq_destroy(deq);
    return 0;
}



static void _deq_dep_callback_1(DvzDeq* deq, void* item, void* user_data)
{
    ANN(deq);
    dvz_sleep(50);
}

static void _deq_dep_callback_2(DvzDeq* deq, void* item, void* user_data)
{
    ANN(deq);
    int* res = (int*)user_data;
    ANN(res);
    *res = 42;
}

static int _dep_thread_1(void* user_data)
{
    DvzDeq* deq = (DvzDeq*)user_data;
    ANN(deq);
    dvz_deq_dequeue_loop(deq, 0);
    return 0;
}

static int _dep_thread_2(void* user_data)
{
    DvzDeq* deq = (DvzDeq*)user_data;
    ANN(deq);
    dvz_deq_dequeue_loop(deq, 1);
    return 0;
}

int test_deq_dependencies(TstSuite* suite)
{
    DvzDeq* deq = dvz_deq(3);
    int res = 0;
    dvz_deq_proc(deq, 0, 1, (uint32_t[]){0});
    dvz_deq_proc(deq, 1, 1, (uint32_t[]){1});
    dvz_deq_proc(deq, 2, 1, (uint32_t[]){2});
    dvz_deq_callback(deq, 0, 0, _deq_dep_callback_1, NULL);
    dvz_deq_callback(deq, 1, 10, _deq_dep_callback_2, &res);

    // Need to allocate on the heap because the dequeue loop will free these items.
    int* one = calloc(1, sizeof(int));
    *one = 1;
    int* two = calloc(1, sizeof(int));
    *two = 2;
    int* three = calloc(1, sizeof(int));
    *three = 3;

    // First enqueue 0, 0, {1}, and then, after the callbacks, enqueue 1, 10, {2}.
    DvzDeqItem* deq_item = dvz_deq_enqueue_custom(0, 0, one);
    DvzDeqItem* next_item = dvz_deq_enqueue_custom(1, 10, two);
    DvzDeqItem* last_item = dvz_deq_enqueue_custom(2, 0, three);

    { // Put a dependency between the two tasks.
        dvz_deq_enqueue_next(deq_item, next_item, false);
        dvz_deq_enqueue_next(next_item, last_item, false);
        dvz_deq_enqueue_submit(deq, deq_item, false);

        // DEBUG: if using two submissions in parallel, the test will fail. The dependency is
        // required for the test to succeed.
        // dvz_deq_enqueue_submit(deq, deq_item, false);
        // dvz_deq_enqueue_submit(deq, next_item, false);
    }
    // should return immediately because that queue is still empty at this point. It won't be after
    // the 2 other tasks have finished, because of the dependencies between them.
    dvz_deq_wait(deq, 2);

    // Dequeue in a thread.
    DvzThread* thread1 = dvz_thread(_dep_thread_1, deq);
    DvzThread* thread2 = dvz_thread(_dep_thread_2, deq);

    // After 20 ms, the first item is still being processed. The second item is NOT in the queue
    // and is not being processed, because it will only be enqueued after the first item's callback
    // have finished running.
    dvz_sleep(20);
    AT(res == 0);
    dvz_sleep(80);
    // If we wait long enough, the first item will have finished being processed, the second item
    // will have been enqueued, its callback will have modified the value.
    AT(res == 42);

    dvz_deq_wait(deq, 0);
    dvz_deq_wait(deq, 1);

    // Dequeue the last item.
    dvz_deq_dequeue(deq, 2, true);
    dvz_deq_wait(deq, 2);

    // End the threads.
    dvz_deq_enqueue(deq, 0, 0, NULL);
    dvz_deq_enqueue(deq, 1, 0, NULL);
    dvz_thread_join(thread1);
    dvz_thread_join(thread2);
    dvz_deq_destroy(deq);
    return 0;
}



// static void _proc_callback(DvzDeq* deq, uint32_t deq_idx, int type, void* item, void* user_data)
// {
//     ANN(deq);
//     ANN(item);
//     ANN(user_data);

//     uvec3* v = (uvec3*)user_data;
//     v[0][0] = deq_idx;
//     v[0][1] = (uint32_t)type;
//     v[0][2] = *((uint32_t*)item);
// }

int test_deq_proc(TstSuite* suite)
{
    DvzDeq* deq = dvz_deq(3);
    dvz_deq_proc(deq, 0, 2, (uint32_t[]){0, 1});
    dvz_deq_proc(deq, 1, 1, (uint32_t[]){2});

    uvec3 v = {0};
    // dvz_deq_proc_callback(deq, 0, DVZ_DEQ_PROC_CALLBACK_PRE, _proc_callback, &v);

    DvzDeqItem item = {0};

    // Enqueue in the queue with a callback.
    dvz_deq_enqueue(deq, 1, 0, (int[]){1});
    dvz_deq_enqueue(deq, 2, 0, (int[]){2});

    // Dequeue the first proc.
    item = dvz_deq_dequeue(deq, 0, true);
    AT(*((uint8_t*)item.item) == 1);
    AT(v[0] == 1);
    AT(v[1] == 0);
    AT(v[2] == 1);

    // Here the queue #2 is non-empty, but the wait still returns because only for proc 0 that does
    // not contain queue #2.
    dvz_deq_wait(deq, 0);

    // Dequeue the second proc.
    item = dvz_deq_dequeue(deq, 1, true);
    AT(*((uint8_t*)item.item) == 2);
    dvz_deq_wait(deq, 1);

    dvz_deq_destroy(deq);
    return 0;
}



int test_deq_circular(TstSuite* suite)
{
    DvzDeq* deq = dvz_deq(2);
    dvz_deq_proc(deq, 0, 2, (uint32_t[]){0, 1});

    // Enqueue in the queue with a callback.
    dvz_deq_enqueue(deq, 0, 0, (int[]){(int)0});
    dvz_deq_enqueue(deq, 0, 0, (int[]){(int)1});
    dvz_deq_enqueue(deq, 0, 0, (int[]){(int)2});
    dvz_deq_enqueue(deq, 1, 0, (int[]){42});

    // Default is breadth-first dequeue strategy.
    // Item 42 in queue #1 should be obtained at the second dequeue() call thanks to the circular
    // dequeueing logic.
    int expected[4] = {0, 42, 1, 2};
    for (uint32_t i = 0; i < 4; i++)
        AT(*(int*)(dvz_deq_dequeue(deq, 0, false).item) == expected[i]);

    // Now, switch to depth-first dequeue strategy.
    dvz_deq_strategy(deq, 0, DVZ_DEQ_STRATEGY_DEPTH_FIRST);

    dvz_deq_enqueue(deq, 0, 0, (int[]){(int)0});
    dvz_deq_enqueue(deq, 1, 0, (int[]){42});
    dvz_deq_enqueue(deq, 0, 0, (int[]){(int)1});
    dvz_deq_enqueue(deq, 0, 0, (int[]){(int)2});
    expected[0] = 0;
    expected[1] = 1;
    expected[2] = 2;
    expected[3] = 42;
    for (uint32_t i = 0; i < 4; i++)
        AT(*(int*)(dvz_deq_dequeue(deq, 0, false).item) == expected[i]);

    dvz_deq_destroy(deq);
    return 0;
}



// static int _proc_thread(void* user_data)
// {
//     DvzDeq* deq = (DvzDeq*)user_data;
//     ANN(deq);
//     dvz_deq_dequeue_loop(deq, 0);
//     return 0;
// }

// static void _proc_wait(DvzDeq* deq, void* user_data)
// {
//     ANN(deq);
//     int* count = (int*)user_data;
//     ANN(count);
//     (*count)++;
//     log_debug("wait iter %d", *count);
// }

// int test_deq_wait(TstSuite* suite)
// {
//     DvzDeq* deq = dvz_deq(1);
//     dvz_deq_proc(deq, 0, 1, (uint32_t[]){0});
//     dvz_deq_proc_wait_delay(deq, 0, 10);
//     int count = 0;
//     dvz_deq_proc_wait_callback(deq, 0, _proc_wait, &count);

//     DvzThread* thread = dvz_thread(_proc_thread, deq);

//     dvz_sleep(100);
//     AT(count >= 3);

//     int* item = calloc(1, sizeof(int));
//     *item = 1;
//     dvz_deq_enqueue(
//         deq, 0, 0, item); // will be FREEd by the dequeue proc in dvz_deq_dequeue_loop()

//     dvz_sleep(20);
//     AT(count >= 4);

//     dvz_deq_enqueue(deq, 0, 0, NULL);
//     dvz_thread_join(thread);
//     dvz_deq_destroy(deq);
//     return 0;
// }



// static void _proc_batch(
//     DvzDeq* deq, DvzDeqProcBatchPosition pos, uint32_t item_count, DvzDeqItem* items,
//     void* user_data)
// {
//     ANN(deq);
//     int* res = (int*)user_data;
//     ANN(res);
//     if (pos == DVZ_DEQ_PROC_BATCH_BEGIN)
//     {
//         log_debug("begin batch, %d item(s) to be dequeued", item_count);
//         ASSERT(item_count == 5);
//         ASSERT(items == NULL);
//     }
//     else
//     {
//         log_debug("end batch, %d item(s) processed", item_count);
//         ASSERT(item_count == 5);
//         // Compute the sum of all dequeued items in the batch.
//         for (uint32_t i = 0; i < item_count; i++)
//         {
//             *res += *(int*)items[i].item;
//         }
//     }
// }

// int test_deq_batch(TstSuite* suite)
// {
//     DvzDeq* deq = dvz_deq(2);
//     dvz_deq_proc(deq, 0, 1, (uint32_t[]){0});
//     dvz_deq_proc(deq, 1, 1, (uint32_t[]){1});

//     int res = 0;
//     dvz_deq_proc_batch_callback(deq, 1, DVZ_DEQ_PROC_BATCH_BEGIN, _proc_batch, &res);
//     dvz_deq_proc_batch_callback(deq, 1, DVZ_DEQ_PROC_BATCH_END, _proc_batch, &res);

//     for (uint32_t i = 0; i < 10; i++)
//     {
//         int* item = calloc(1, sizeof(int)); // will be FREE-ed by dequeue_batch
//         *item = (int)i;

//         // NOTE: the type should not be taken into account by the batch callbacks.
//         dvz_deq_enqueue(deq, i % 2, (int)i, item);
//     }

//     dvz_deq_dequeue_batch(deq, 1);

//     // 1+3+5+7+9 because we batch-dequeue the second proc only.
//     AT(res == 25);

//     dvz_deq_destroy(deq);
//     return 0;
// }
