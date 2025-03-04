/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  List                                                                                         */
/*************************************************************************************************/

#include "_list.h"
#include "_log.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _realloc_if_needed(DvzList* list)
{
    ANN(list);
    ANN(list->values);
    ASSERT(list->capacity > 0);

    if (list->count >= list->capacity)
    {
        list->capacity *= 2;
        REALLOC(DvzListItem*, list->values, list->capacity * sizeof(DvzListItem))
    }
    ASSERT(list->count < list->capacity);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzList* dvz_list(void)
{
    DvzList* list = (DvzList*)calloc(1, sizeof(DvzList));
    list->count = 0;
    list->capacity = DVZ_MAX_LIST_CAPACITY;
    list->values = (DvzListItem*)calloc(list->capacity, sizeof(DvzListItem));
    return list;
}



void dvz_list_insert(DvzList* list, uint64_t index, DvzListItem value)
{
    ANN(list);
    ANN(list->values);

    _realloc_if_needed(list);

    // NOTE: to append, use index = count
    ASSERT(index <= list->count);

    for (uint32_t i = list->count; i >= index + 1; i--)
    {
        list->values[i] = list->values[i - 1];
    }
    list->values[index] = value;
    list->count++;
}



void dvz_list_append(DvzList* list, DvzListItem value)
{
    ANN(list);
    dvz_list_insert(list, list->count, value);
}



void dvz_list_remove(DvzList* list, uint64_t index)
{
    ANN(list);
    ANN(list->values);
    ASSERT(list->capacity > 0);

    // ASSERT(0 <= index);
    ASSERT(index < list->count);

    // When an element is removed, need to shift all items after the index to the left.
    for (uint32_t i = index; i < list->count - 1; i++)
    {
        list->values[i] = list->values[i + 1];
    }
    list->count--;

    // Reset the unset positions in the array.
    memset(&list->values[list->count], 0, (list->capacity - list->count) * sizeof(DvzListItem));
}



void dvz_list_remove_pointer(DvzList* list, const void* pointer)
{
    ANN(list);
    ANN(list->values);
    ANN(pointer);

    for (uint64_t i = 0; i < list->count; i++)
    {
        if (list->values[i].p == pointer)
        {
            dvz_list_remove(list, i);
        }
    }
}



DvzListItem dvz_list_get(DvzList* list, uint64_t index)
{
    ANN(list);
    if (list->values == NULL)
    {
        log_warn("trying to access data on an empty list");
        return (DvzListItem){0};
    }
    ASSERT(index < list->count);
    return list->values[index];
}



uint64_t dvz_list_index(DvzList* list, int value)
{
    ANN(list);
    ANN(list->values);
    for (uint64_t i = 0; i < list->count; i++)
    {
        if (list->values[i].i == value)
            return i;
    }
    return UINT64_MAX;
}



bool dvz_list_has(DvzList* list, int value) { return dvz_list_index(list, value) != UINT64_MAX; }



void dvz_list_clear(DvzList* list)
{
    ANN(list);
    list->count = 0;
}



uint64_t dvz_list_count(DvzList* list)
{
    ANN(list);
    return list->count;
}



void dvz_list_destroy(DvzList* list)
{
    ANN(list);
    FREE(list->values);
    FREE(list);
}
