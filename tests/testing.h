/*************************************************************************************************/
/*  Generic testing framework                                                                    */
/*************************************************************************************************/

#ifndef TST_HEADER
#define TST_HEADER



/*************************************************************************************************/
/*  MACROS                                                                                       */
/*************************************************************************************************/

#define TST_DEFAULT_CAPACITY 32

#define AT(x)                                                                                     \
    if (!(x))                                                                                     \
    {                                                                                             \
        log_error("assertion '%s' failed", #x);                                                   \
        return 1;                                                                                 \
    }

#define AEn(n, x, y)                                                                              \
    {                                                                                             \
        for (uint32_t k = 0; k < (n); k++)                                                        \
            AT((x)[k] == (y)[k]);                                                                 \
    }

#define AIN(x, m, M) AT((m) <= (x) && (x) <= (M))

#define AC(x, y, eps) AIN(((x) - (y)), -(eps), +(eps))

#define ACn(n, x, y, eps)                                                                         \
    for (uint32_t i = 0; i < (n); i++)                                                            \
        AC((x)[i], (y)[i], (eps));

#define EPS 1e-6

#define TEST(x) tst_suite_add(&suite, #x, x, NULL);

#define SETUP(x) tst_suite_setup(&suite, x, NULL);

#define TEARDOWN(x) tst_suite_teardown(&suite, x, NULL);

#define PROF_START(num)                                                                           \
    {                                                                                             \
        uint32_t N = num;                                                                         \
        DvzClock clock = dvz_clock();                                                             \
        for (uint32_t i = 0; i < N; i++)                                                          \
        {

#define PROF_END                                                                                  \
    }                                                                                             \
    double elapsed = dvz_clock_get(&clock);                                                       \
    log_info("profiling: %.6f ms per run", 1000 * elapsed / N);                                   \
    }



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "_log.h"
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
/*  Test printing                                                                                */
/*************************************************************************************************/

static void print_start()
{
    printf("--- Starting tests -------------------------------\n"); //
}



static void print_test(int index, const char* name)
{
    printf("- Running test #%03d %28s\n", index, name);
}



static void print_res(int index, const char* name, int res)
{
    printf("%50s", name);
    printf("\x1b[%dm %s\x1b[0m\n", res == 0 ? 32 : 31, res == 0 ? "passed!" : "FAILED!");
}



static void print_end(int index, int res)
{
    printf("--------------------------------------------------\n");
    if (index > 0 && res == 0)
        printf("\x1b[32m%d/%d tests PASSED.\x1b[0m\n", index, index);
    else if (index > 0)
        printf("\x1b[31m%d/%d tests FAILED.\x1b[0m\n", res, index);
    else
        printf("\x1b[31mThere were no tests.\x1b[0m\n");
}



/*************************************************************************************************/
/*  Util functions                                                                               */
/*************************************************************************************************/

static inline bool test_name_matches(TstTest* test, const char* str)
{
    ASSERT(test != NULL);
    ASSERT(str != NULL);
    return strstr(test->name, str) != NULL;
}



/*************************************************************************************************/
/*  Main testing functions                                                                       */
/*************************************************************************************************/

static TstSuite tst_suite(void)
{
    TstSuite suite = {0};
    suite.items = calloc(TST_DEFAULT_CAPACITY, sizeof(TstItem));
    suite.capacity = TST_DEFAULT_CAPACITY;
    suite.n_items = 0;
    return suite;
}



static TstItem* _append(TstSuite* suite, TstItemType type, TstFunction function, void* user_data)
{
    ASSERT(suite != NULL);
    // log_trace(
    //     "append one test item to suite with %d items, capacity %d", //
    //     suite->n_items, suite->capacity);
    // Resize the array if needed.
    if (suite->capacity == suite->n_items)
    {
        log_trace("reallocate memory for test suite items");
        ASSERT(suite->items != NULL);
        ASSERT(suite->n_items > 0);
        REALLOC(suite->items, (size_t)(2 * suite->n_items * sizeof(TstItem)));
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



static void tst_suite_run(TstSuite* suite, const char* match)
{
    ASSERT(suite != NULL);
    TstItem* item = NULL;
    int index = 0;
    int res = 0, cur_res = 0;
    print_start();
    for (uint32_t i = 0; i < suite->n_items; i++)
    {
        item = &suite->items[i];
        switch (item->type)
        {
        case TST_ITEM_SETUP:
        case TST_ITEM_TEARDOWN:
            item->u.f.function(suite);
            break;

        case TST_ITEM_TEST:
            if (match == NULL || test_name_matches(&item->u.t, match))
            {
                item->u.t.res = item->u.t.function(suite);
                cur_res = item->u.t.res;
                print_res(index, item->u.t.name, cur_res);
                res += cur_res == 0 ? 0 : 1;
                index++;
            }
            break;

        default:
            break;
        }
        // TODO: mark as PASS or FAIL depending on the res
    }
    print_end(index, res);
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
