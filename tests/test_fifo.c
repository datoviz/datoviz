/*************************************************************************************************/
/*  Testing FIFO                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdio.h>

#include "fifo.h"
#include "test.h"
#include "test_fifo.h"
#include "testing.h"



/*************************************************************************************************/
/*  FIFO tests                                                                                   */
/*************************************************************************************************/

static void* _fifo_thread_1(void* arg)
{
    DvzFifo* fifo = arg;
    uint8_t* data = dvz_fifo_dequeue(fifo, true);
    ASSERT(*data == 12);
    // Signal to the caller thread that the dequeue was successfull.
    fifo->user_data = data;
    return NULL;
}



static void* _fifo_thread_2(void* arg)
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
    return NULL;
}



int test_utils_fifo_1(TstSuite* suite)
{
    DvzFifo fifo = dvz_fifo(8);
    uint8_t item = 12;

    // Enqueue + dequeue in the same thread.
    AT(fifo.is_empty);
    dvz_fifo_enqueue(&fifo, &item);
    AT(!fifo.is_empty);
    ASSERT(fifo.tail == 1);
    ASSERT(fifo.head == 0);
    uint8_t* data = dvz_fifo_dequeue(&fifo, true);
    AT(fifo.is_empty);
    ASSERT(*data == item);

    // Enqueue in the main thread, dequeue in a background thread.
    pthread_t thread = {0};
    ASSERT(fifo.user_data == NULL);
    pthread_create(&thread, NULL, _fifo_thread_1, &fifo);
    dvz_fifo_enqueue(&fifo, &item);
    AT(!fifo.is_empty);
    pthread_join(thread, NULL);
    ASSERT(fifo.user_data != NULL);
    ASSERT(fifo.user_data == &item);

    // Multiple enqueues in the background thread, dequeue in the main thread.
    pthread_create(&thread, NULL, _fifo_thread_2, &fifo);
    uint8_t* dequeued = NULL;
    uint32_t i = 0;
    do
    {
        dequeued = dvz_fifo_dequeue(&fifo, true);
        if (dequeued == NULL)
            break;
        AT(*dequeued == i);
        i++;
    } while (dequeued != NULL);
    pthread_join(thread, NULL);
    FREE(fifo.user_data);

    dvz_fifo_destroy(&fifo);
    return 0;
}



int test_utils_fifo_2(TstSuite* suite)
{
    DvzFifo fifo = dvz_fifo(8);
    uint32_t numbers[64] = {0};
    for (uint32_t i = 0; i < 64; i++)
    {
        numbers[i] = i;
        dvz_fifo_enqueue(&fifo, &numbers[i]);
    }
    uint32_t* res = NULL;
    for (uint32_t i = 0; i < 64; i++)
    {
        AT(!fifo.is_empty);
        res = dvz_fifo_dequeue(&fifo, false);
        AT(*res == i);
    }
    AT(fifo.is_empty);
    dvz_fifo_destroy(&fifo);
    return 0;
}



int test_utils_fifo_resize(TstSuite* suite)
{
    DvzFifo fifo = dvz_fifo(8);
    uint32_t numbers[256] = {0};
    uint32_t i = 0;
    for (i = 0; i < 64; i++)
    {
        numbers[i] = i;
        dvz_fifo_enqueue(&fifo, &numbers[i]);
        if (i % 2 == 0)
            dvz_fifo_dequeue(&fifo, false);
    }

    i = 0;
    uint32_t* n = 0;
    while (dvz_fifo_size(&fifo) > 0)
    {
        n = dvz_fifo_dequeue(&fifo, false);
        AT(*n == i + 32);
        i++;
    }
    dvz_fifo_destroy(&fifo);
    return 0;
}



int test_utils_fifo_discard(TstSuite* suite)
{
    DvzFifo fifo = dvz_fifo(8);
    uint32_t numbers[8] = {0};
    for (uint32_t i = 0; i < 7; i++)
    {
        numbers[i] = i;
        dvz_fifo_enqueue(&fifo, &numbers[i]);
    }
    AT(fifo.capacity == 8);

    // First item is 0.
    AT(dvz_fifo_size(&fifo) == 7);
    AT(*((int*)dvz_fifo_dequeue(&fifo, false)) == 0);

    // Discard 2 elements (from size 7 to 5).
    dvz_fifo_discard(&fifo, 5);

    // First item is 2.
    AT(dvz_fifo_size(&fifo) == 5);
    AT(*((int*)dvz_fifo_dequeue(&fifo, false)) == 2);

    dvz_fifo_destroy(&fifo);
    return 0;
}



int test_utils_fifo_first(TstSuite* suite)
{
    DvzFifo fifo = dvz_fifo(8);
    dvz_fifo_enqueue(&fifo, (int[]){1});
    dvz_fifo_enqueue(&fifo, (int[]){2});
    dvz_fifo_enqueue(&fifo, (int[]){3});

    AT(*((int*)dvz_fifo_dequeue(&fifo, false)) == 1);
    dvz_fifo_enqueue_first(&fifo, (int[]){4});
    AT(*((int*)dvz_fifo_dequeue(&fifo, false)) == 4);

    dvz_fifo_destroy(&fifo);
    return 0;
}



/*************************************************************************************************/
/*  Deq tests                                                                                    */
/*************************************************************************************************/

static void _deq_1_callback(DvzDeq* deq, void* item, void* user_data)
{
    ASSERT(deq != NULL);
    ASSERT(item != NULL);
    ASSERT(user_data != NULL);
    int* data = (int*)user_data;
    *data = *((int*)item);
}

int test_utils_deq_1(TstSuite* suite)
{
    DvzDeq deq = dvz_deq(2);
    DvzDeqItem item = {0};

    int data = 0;
    dvz_deq_callback(&deq, 0, 0, _deq_1_callback, &data);
    AT(data == 0);

    // Enqueue in the queue with a callback.
    dvz_deq_enqueue(&deq, 0, 0, (int[]){2});
    item = dvz_deq_dequeue(&deq, false);
    AT(item.deq_idx == 0);
    AT(item.type == 0);
    AT(data == 2);

    // Enqueue in the queue without a callback.
    data = 0;
    dvz_deq_enqueue(&deq, 1, 10, (int[]){2});
    item = dvz_deq_dequeue(&deq, false);
    AT(item.deq_idx == 1);
    AT(item.type == 10);
    AT(data == 0);

    // Enqueue in the queue with a callback.
    dvz_deq_enqueue(&deq, 0, 10, (int[]){3});
    item = dvz_deq_dequeue(&deq, false);
    AT(item.deq_idx == 0);
    AT(item.type == 10);
    AT(data == 0);

    dvz_deq_callback(&deq, 0, 10, _deq_1_callback, &data);
    dvz_deq_enqueue(&deq, 0, 10, (int[]){4});
    item = dvz_deq_dequeue(&deq, false);
    AT(item.deq_idx == 0);
    AT(item.type == 10);
    AT(item.item != NULL);
    AT(data == 4);

    // Supbsequent dequeues are empty.
    item = dvz_deq_dequeue(&deq, false);
    AT(item.item == NULL);

    dvz_deq_destroy(&deq);
    return 0;
}



int test_utils_deq_2(TstSuite* suite)
{
    DvzDeq deq = dvz_deq(2);
    DvzDeqItem item = {0};

    // Enqueue in the queue with a callback.
    dvz_deq_enqueue(&deq, 0, 0, (int[]){1});
    dvz_deq_enqueue(&deq, 1, 0, (int[]){2});
    dvz_deq_enqueue(&deq, 0, 10, (int[]){3});
    dvz_deq_enqueue(&deq, 1, 10, (int[]){4});

    // First queue.
    item = dvz_deq_peek_first(&deq, 0);
    AT(item.type == 0);
    AT(*(int*)(item.item) == 1);
    item = dvz_deq_peek_last(&deq, 0);
    AT(item.type == 10);
    AT(*(int*)(item.item) == 3);

    // Second queue.
    item = dvz_deq_peek_first(&deq, 1);
    AT(item.type == 0);
    AT(*(int*)(item.item) == 2);
    item = dvz_deq_peek_last(&deq, 1);
    AT(item.type == 10);
    AT(*(int*)(item.item) == 4);

    dvz_deq_destroy(&deq);
    return 0;
}
