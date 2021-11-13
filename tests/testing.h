/*************************************************************************************************/
/*  Generic testing framework                                                                    */
/*************************************************************************************************/

#ifndef DVZ_HEADER_TESTING
#define DVZ_HEADER_TESTING



/*************************************************************************************************/
/*  MACROS                                                                                       */
/*************************************************************************************************/

#define TST_DEFAULT_CAPACITY 32



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "_macros.h"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    TST_ITEM_NONE,
    TST_ITEM_TEST,
    TST_ITEM_SETUP,
    TST_ITEM_TEARDOWN,
} TstItemType;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct TstSuite TstSuite;
typedef struct TstTest TstTest;
typedef struct TstFixture TstFixture;
typedef struct TstItem TstItem; // either a test or a fixture
typedef union TstItemUnion TstItemUnion;

typedef int (*TstFunction)(TstSuite*);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct TstTest
{
    const char* name;
    TstFunction function;
    int res;
    void* user_data;
};



struct TstFixture
{
    TstFunction function;
    void* user_data;
};



union TstItemUnion
{
    TstTest t;
    TstFixture f;
};



struct TstItem
{
    TstItemType type;
    TstItemUnion u;
};



struct TstSuite
{
    uint32_t n_items;  // number of items
    uint32_t capacity; // size of the allocated array TstSuite.items
    TstItem* items;    // array of items
    void* context;     // user-specified custom context
};



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

static TstSuite tst_suite(void)
{
    TstSuite suite = {0};
    suite.items = calloc(TST_DEFAULT_CAPACITY, sizeof(TstItem));
    suite.capacity = TST_DEFAULT_CAPACITY;
    return suite;
}



static TstItem* _append(TstSuite* suite, TstItemType type, TstFunction function, void* user_data)
{
    ASSERT(suite != NULL);
    // Resize the array if needed.
    if (suite->capacity == suite->n_items)
    {
        REALLOC(suite->items, 2 * suite->n_items);
        suite->capacity *= 2;
    }
    ASSERT(suite->n_items < suite->capacity);
    TstItem* item = &suite->items[suite->n_items++];
    item->type = type;
    switch (type)
    {
    case TST_ITEM_SETUP:
    case TST_ITEM_TEARDOWN:
        item->u.f.function = function;
        item->u.f.user_data = user_data;
        break;

    case TST_ITEM_TEST:
        item->u.t.function = function;
        item->u.t.user_data = user_data;
        break;

    default:
        break;
    }
    return item;
}



static void tst_suite_setup(TstSuite* suite, TstFunction setup, void* user_data)
{
    ASSERT(suite != NULL);
    _append(suite, TST_ITEM_SETUP, setup, user_data);
}



static void tst_suite_add(TstSuite* suite, const char* name, TstFunction test, void* user_data)
{
    ASSERT(suite != NULL);
    TstItem* item = _append(suite, TST_ITEM_TEST, test, user_data);
    item->u.t.name = name;
}



static void tst_suite_teardown(TstSuite* suite, TstFunction teardown, void* user_data)
{
    ASSERT(suite != NULL);
    _append(suite, TST_ITEM_TEARDOWN, teardown, user_data);
}



static void tst_suite_run(TstSuite* suite)
{
    ASSERT(suite != NULL);
    TstItem* item = NULL;
    bool pass = false;
    // TODO: print run init
    for (uint32_t i = 0; i < suite->n_items; i++)
    {
        item = &suite->items[i];
        // TODO: pattern matching
        switch (item->type)
        {
        case TST_ITEM_SETUP:
        case TST_ITEM_TEARDOWN:
            item->u.f.function(suite);
            break;

        case TST_ITEM_TEST:
            pass = item->u.t.res = item->u.t.function(suite);
            break;

        default:
            break;
        }
        // TODO: mark as PASS or FAIL depending on the res
    }
    // TODO: print run end
}



static void tst_suite_destroy(TstSuite* suite)
{
    ASSERT(suite != NULL);
    ASSERT(suite->items != NULL);
    suite->n_items = 0;
    suite->capacity = 0;
    FREE(suite->items);
}



#endif
