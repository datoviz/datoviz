/*************************************************************************************************/
/*  Generic testing framework                                                                    */
/*************************************************************************************************/

#ifndef DVZ_HEADER_TESTING
#define DVZ_HEADER_TESTING



/*************************************************************************************************/
/*  MACROS                                                                                       */
/*************************************************************************************************/

#define TST_DEFAULT_N_TESTS 64



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <stdlib.h>

#include "_macros.h"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    TST_FIXTURE_NONE,
    TST_FIXTURE_TEST,
    TST_FIXTURE_SETUP,
    TST_FIXTURE_TEARDOWN,
} TstFixtureType;



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
    TstFixtureType type;
    TstItemUnion u;
};



struct TstSuite
{
    uint32_t n_tests;
    TstItem* items;
    void* context;
};



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

static TstSuite tst_suite(void)
{
    TstSuite suite = {0};
    suite.items = calloc(TST_DEFAULT_N_TESTS, sizeof(TstItem));
    return suite;
}



static void tst_suite_setup(TstSuite* suite, TstFunction setup)
{
    ASSERT(suite != NULL); //
}



static void tst_suite_add(TstSuite* suite, TstFunction test)
{
    ASSERT(suite != NULL); //
}



static void tst_suite_teardown(TstSuite* suite, TstFunction teardown)
{
    ASSERT(suite != NULL); //
}



static void tst_suite_run(TstSuite* suite)
{
    ASSERT(suite != NULL); //
}



static void tst_suite_destroy(TstSuite* suite)
{
    ASSERT(suite != NULL);
    FREE(suite->items);
}



#endif
