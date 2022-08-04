/*************************************************************************************************/
/*  List                                                                                         */
/*************************************************************************************************/

#include "list.h"
#include "_log.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _realloc_if_needed(DvzList* list)
{
    ASSERT(list != NULL);
    ASSERT(list->values != NULL);
    ASSERT(list->capacity > 0);

    if (list->count >= list->capacity)
    {
        list->capacity *= 2;
        REALLOC(list->values, list->capacity * sizeof(int))
    }
    ASSERT(list->count < list->capacity);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzList dvz_list()
{
    DvzList list = {0};
    list.count = 0;
    list.capacity = DVZ_MAX_LIST_CAPACITY;
    list.values = (int*)calloc(list.capacity, sizeof(int));
    return list;
}



void dvz_list_insert(DvzList* list, uint64_t index, int value)
{
    ASSERT(list != NULL);
    ASSERT(list->values != NULL);

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



void dvz_list_append(DvzList* list, int value) { dvz_list_insert(list, list->count, value); }



void dvz_list_remove(DvzList* list, uint64_t index)
{
    ASSERT(list != NULL);
    ASSERT(list->values != NULL);
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
    memset(&list->values[list->count], 0, (list->capacity - list->count) * sizeof(int));
}



int dvz_list_get(DvzList* list, uint64_t index)
{
    ASSERT(list != NULL);
    ASSERT(list->values != NULL);
    // ASSERT(0 <= index);
    ASSERT(index < list->count);
    return list->values[index];
}



uint64_t dvz_list_index(DvzList* list, int value)
{
    ASSERT(list != NULL);
    ASSERT(list->values != NULL);
    for (uint64_t i = 0; i < list->count; i++)
    {
        if (list->values[i] == value)
            return i;
    }
    return UINT64_MAX;
}



bool dvz_list_has(DvzList* list, int value) { return dvz_list_index(list, value) != UINT64_MAX; }



uint64_t dvz_list_count(DvzList* list)
{
    ASSERT(list != NULL);
    return list->count;
}



void dvz_list_destroy(DvzList* list)
{
    ASSERT(list != NULL);
    FREE(list->values);
}
