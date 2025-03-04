/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing FIFO                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdio.h>

#include "_thread_utils.h"
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
    // NOTE: this pointer will be FREE-ed by the main thread (as user_data).
    uint8_t* numbers = (uint8_t*)calloc(5, sizeof(uint8_t));
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



int test_fifo_1(TstSuite* suite, TstItem* tstitem)
{
    DvzFifo* fifo = dvz_fifo(8);
    uint8_t item = 12;

    // Enqueue + dequeue in the same thread.
    AT(fifo->is_empty);
    dvz_fifo_enqueue(fifo, &item);
    AT(!_is_empty(fifo));
    AT(fifo->tail == 1);
    AT(fifo->head == 0);

    // Test fifo_get.
    AT(dvz_fifo_get(fifo, 0) == &item);

    uint8_t* data = dvz_fifo_dequeue(fifo, true);
    AT(_is_empty(fifo));
    AT(*data == item);

    // Enqueue in the main thread, dequeue in a background thread.
    DvzThread* thread = dvz_thread(_fifo_thread_1, fifo);
    dvz_fifo_enqueue(fifo, &item);
    AT(!_is_empty(fifo));
    dvz_thread_join(thread);
    ANN(fifo->user_data);
    AT(fifo->user_data == &item);

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



int test_fifo_2(TstSuite* suite, TstItem* tstitem)
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



int test_fifo_resize(TstSuite* suite, TstItem* tstitem)
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



int test_fifo_discard(TstSuite* suite, TstItem* tstitem)
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



int test_fifo_first(TstSuite* suite, TstItem* tstitem)
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

int test_deq_1(TstSuite* suite, TstItem* tstitem)
{
    DvzDeq* deq = dvz_deq(2, sizeof(int));
    DvzDeqItem item = {0};

    int data = 0;
    dvz_deq_callback(deq, 0, 0, _deq_1_callback, &data);
    dvz_deq_proc(deq, 0, 2, (uint32_t[]){0, 1});
    AT(data == 0);

    // Enqueue in the queue with a callback.
    dvz_deq_enqueue(deq, 0, 0, (int[]){2});
    item = dvz_deq_dequeue_return(deq, 0, false);
    AT(item.deq_idx == 0);
    AT(item.type == 0);
    AT(data == 2);
    FREE(item.item);

    // Enqueue in the queue without a callback.
    data = 0;
    dvz_deq_enqueue(deq, 1, 10, (int[]){2});
    item = dvz_deq_dequeue_return(deq, 0, false);
    AT(item.deq_idx == 1);
    AT(item.type == 10);
    AT(data == 0);
    FREE(item.item);

    // Enqueue in the queue with a callback.
    dvz_deq_enqueue(deq, 0, 10, (int[]){3});
    item = dvz_deq_dequeue_return(deq, 0, false);
    AT(item.deq_idx == 0);
    AT(item.type == 10);
    AT(data == 0);
    FREE(item.item);

    dvz_deq_callback(deq, 0, 10, _deq_1_callback, &data);
    dvz_deq_enqueue(deq, 0, 10, (int[]){4});
    item = dvz_deq_dequeue_return(deq, 0, false);
    AT(item.deq_idx == 0);
    AT(item.type == 10);
    AT(item.item != NULL);
    AT(data == 4);
    FREE(item.item);

    // Supbsequent dequeues are empty.
    item = dvz_deq_dequeue_return(deq, 0, false);
    AT(item.item == NULL);

    dvz_deq_destroy(deq);
    return 0;
}



int test_deq_2(TstSuite* suite, TstItem* tstitem)
{
    DvzDeq* deq = dvz_deq(2, sizeof(int));
    dvz_deq_proc(deq, 0, 2, (uint32_t[]){0, 1});
    DvzDeqItem item = {0};

    // Enqueue in the queue with a callback.
    dvz_deq_enqueue(deq, 0, 0, (int[]){1});
    dvz_deq_enqueue(deq, 1, 0, (int[]){2});
    dvz_deq_enqueue(deq, 0, 10, (int[]){3});
    dvz_deq_enqueue(deq, 1, 10, (int[]){4});

    // First queue.
    item = dvz_deq_peek_first(deq, 0);
    ANN(item.item);
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



static void _deq_3_callback_a(DvzDeq* deq, void* item, void* user_data)
{
    ANN(deq);
    ANN(item);
    ANN(user_data);
    int* data = (int*)user_data;
    *data += 2;
}

static void _deq_3_callback_b(DvzDeq* deq, void* item, void* user_data)
{
    ANN(deq);
    ANN(item);
    ANN(user_data);
    int* data = (int*)user_data;
    *data *= 2;
}

int test_deq_3(TstSuite* suite, TstItem* tstitem)
{
    // Test the callback order.
    DvzDeq* deq = dvz_deq(2, sizeof(int));

    int data = 10;
    dvz_deq_callback(deq, 0, 1, _deq_3_callback_a, &data);
    dvz_deq_callback(deq, 0, 1, _deq_3_callback_b, &data);
    dvz_deq_proc(deq, 0, 2, (uint32_t[]){0, 1});

    // Enqueue in the queue with a callback.
    dvz_deq_enqueue(deq, 0, 1, (int[]){0});
    dvz_deq_dequeue_batch(deq, 0);
    AT(data == 24); // (10+2)*2

    data = 10;
    dvz_deq_order(deq, 1, DVZ_DEQ_ORDER_REVERSE);
    dvz_deq_enqueue(deq, 0, 1, (int[]){0});
    dvz_deq_dequeue_batch(deq, 0);
    AT(data == 22); // (10*2)+2

    dvz_deq_destroy(deq);
    return 0;
}
