#include "test_common.h"
#include "../include/visky/common.h"



/*************************************************************************************************/
/*  Common tests                                                                                 */
/*************************************************************************************************/

typedef struct TestObject TestObject;
struct TestObject
{
    VklObject obj;
    float x;
};

int test_container(TestContext* context)
{
    uint32_t capacity = 2;

    VklContainer container = vkl_container(capacity, sizeof(TestObject), 0);
    AT(container.items != NULL);
    AT(container.item_size == sizeof(TestObject));
    AT(container.capacity == capacity);
    AT(container.count == 0);

    // Allocate one object.
    TestObject* a = vkl_container_alloc(&container);
    AT(a != NULL);
    a->x = 1;
    obj_created(&a->obj);
    AT(container.items[0] != NULL);
    AT(container.items[0] == a);
    AT(container.items[1] == NULL);
    AT(container.capacity == capacity);
    AT(container.count == 1);

    // Allocate another one.
    TestObject* b = vkl_container_alloc(&container);
    AT(b != NULL);
    b->x = 2;
    obj_created(&b->obj);
    AT(container.items[1] != NULL);
    AT(container.items[1] == b);
    AT(container.capacity == capacity);
    AT(container.count == 2);

    // Destroy the first object.
    obj_destroyed(&a->obj);

    // Allocate another one.
    TestObject* c = vkl_container_alloc(&container);
    AT(c != NULL);
    c->x = 3;
    obj_created(&c->obj);
    AT(container.items[0] != NULL);
    AT(container.items[0] == c);
    AT(container.capacity == capacity);
    AT(container.count == 2);

    // Allocate another one.
    // Container will be reallocated.
    TestObject* d = vkl_container_alloc(&container);
    AT(d != NULL);
    d->x = 4;
    obj_created(&d->obj);
    AT(container.capacity == 4);
    AT(container.count == 3);
    AT(container.items[2] != NULL);
    AT(container.items[2] == d);
    AT(container.items[3] == NULL);

    // Iterate through items.
    TestObject* item = NULL;
    uint32_t i = 0;
    do
    {
        item = vkl_container_iter_get(&container);
        AT(item != NULL);
        if (i == 0)
            AT(item->x == 3);
        if (i == 1)
            AT(item->x == 2);
        if (i == 2)
            AT(item->x == 4);
        i++;
    } while (vkl_container_iter(&container));
    ASSERT(i == 3);

    // Destroy all objects.
    obj_destroyed(&b->obj);
    obj_destroyed(&c->obj);
    obj_destroyed(&d->obj);

    // Free all memory. This function will fail if there is at least one object not destroyed.
    vkl_container_destroy(&container);
    return 0;
}
