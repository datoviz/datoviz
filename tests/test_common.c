#include "../include/datoviz/common.h"
#include "tests.h"



/*************************************************************************************************/
/*  Common tests                                                                                 */
/*************************************************************************************************/

typedef struct TestObject TestObject;
struct TestObject
{
    DvzObject obj;
    float x;
};

int test_container(TestContext* context)
{
    uint32_t capacity = 2;

    DvzContainer container = dvz_container(capacity, sizeof(TestObject), 0);
    AT(container.items != NULL);
    AT(container.item_size == sizeof(TestObject));
    AT(container.capacity == capacity);
    AT(container.count == 0);

    // Allocate one object.
    TestObject* a = dvz_container_alloc(&container);
    AT(a != NULL);
    a->x = 1;
    dvz_obj_created(&a->obj);
    AT(container.items[0] != NULL);
    AT(container.items[0] == a);
    AT(container.items[1] == NULL);
    AT(container.capacity == capacity);
    AT(container.count == 1);

    // Allocate another one.
    TestObject* b = dvz_container_alloc(&container);
    AT(b != NULL);
    b->x = 2;
    dvz_obj_created(&b->obj);
    AT(container.items[1] != NULL);
    AT(container.items[1] == b);
    AT(container.capacity == capacity);
    AT(container.count == 2);

    // Destroy the first object.
    dvz_obj_destroyed(&a->obj);

    // Allocate another one.
    TestObject* c = dvz_container_alloc(&container);
    AT(c != NULL);
    c->x = 3;
    dvz_obj_created(&c->obj);
    AT(container.items[0] != NULL);
    AT(container.items[0] == c);
    AT(container.capacity == capacity);
    AT(container.count == 2);

    // Allocate another one.
    // Container will be reallocated.
    TestObject* d = dvz_container_alloc(&container);
    AT(d != NULL);
    d->x = 4;
    dvz_obj_created(&d->obj);
    AT(container.capacity == 4);
    AT(container.count == 3);
    AT(container.items[2] != NULL);
    AT(container.items[2] == d);
    AT(container.items[3] == NULL);

    for (uint32_t k = 0; k < 10; k++)
    {
        DvzContainerIterator iter = dvz_container_iterator(&container);
        uint32_t i = 0;
        TestObject* obj = NULL;
        // Iterate through items.
        while (iter.item != NULL)
        {
            obj = iter.item;
            AT(obj != NULL);
            // log_info("%d", obj);
            if (i == 0)
                AT(obj->x == 3);
            if (i == 1)
                //     DBG(obj->x);
                AT(obj->x == 2);
            if (i == 2)
                AT(obj->x == 4);
            i++;
            dvz_container_iter(&iter);
        }
        ASSERT(i == 3);
    }

    // Destroy all objects.
    dvz_obj_destroyed(&b->obj);
    dvz_obj_destroyed(&c->obj);
    dvz_obj_destroyed(&d->obj);

    // Free all memory. This function will fail if there is at least one object not destroyed.
    dvz_container_destroy(&container);
    return 0;
}



/*************************************************************************************************/
/*  FIFO queue                                                                                   */
/*************************************************************************************************/

/*
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



int test_fifo_1(TestContext* context)
{
    DvzFifo fifo = dvz_fifo(8);
    uint8_t item = 12;

    // Enqueue + dequeue in the same thread.
    AT(fifo.is_empty);
    dvz_fifo_enqueue(&fifo, &item);
    AT(!fifo.is_empty);
    ASSERT(fifo.head == 1);
    ASSERT(fifo.tail == 0);
    uint8_t* data = dvz_fifo_dequeue(&fifo, true);
    AT(fifo.is_empty);
    ASSERT(*data = item);

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



int test_fifo_2(TestContext* context)
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



int test_fifo_3(TestContext* context)
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
*/
