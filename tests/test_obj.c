/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing object                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_obj.h"
#include <stdio.h>

#include "test.h"
#include "test_obj.h"
#include "testing.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct TestObject TestObject;


/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct TestObject
{
    DvzObject obj;
    float x;
};



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_obj_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    uint32_t capacity = 2;

    DvzContainer container = dvz_container(capacity, sizeof(TestObject), 0);
    AT(container.items != NULL);
    AT(container.item_size == sizeof(TestObject));
    AT(container.capacity == capacity);
    AT(container.count == 0);

    // Allocate one object.
    TestObject* a = (TestObject*)dvz_container_alloc(&container);
    AT(a != NULL);
    a->x = 1;
    dvz_obj_created(&a->obj);
    AT(container.items[0] != NULL);
    AT(container.items[0] == a);
    AT(container.items[1] == NULL);
    AT(container.capacity == capacity);
    AT(container.count == 1);

    // Allocate another one.
    TestObject* b = (TestObject*)dvz_container_alloc(&container);
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
    TestObject* c = (TestObject*)dvz_container_alloc(&container);
    AT(c != NULL);
    c->x = 3;
    dvz_obj_created(&c->obj);
    AT(container.items[0] != NULL);
    AT(container.items[0] == c);
    AT(container.capacity == capacity);
    AT(container.count == 2);

    // Allocate another one.
    // Container will be reallocated.
    TestObject* d = (TestObject*)dvz_container_alloc(&container);
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
