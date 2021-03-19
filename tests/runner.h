#ifndef DVZ_TEST_RUNNER_HEADER
#define DVZ_TEST_RUNNER_HEADER

#include "../include/datoviz/common.h"
// NOTE: do not include tests.h (circular import)



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    TEST_FIXTURE_NONE,
    TEST_FIXTURE_APP,
    TEST_FIXTURE_CANVAS,
} TestFixture;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct TestCase TestCase;
typedef struct TestContext TestContext;

// Forward declarations.
typedef struct DvzApp DvzApp;

// Callback functions.
typedef int (*TestFunction)(TestContext*);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct TestContext
{
    uint32_t n_tests;
    TestCase* cases;

    DvzApp* app;
    DvzCanvas* canvas;

    // DvzScreenshot* screenshot;
    // bool is_live;
};



struct TestCase
{
    const char* name;
    TestFunction function;
    TestFixture fixture;
    // TestFunction destroy;
    // bool save_screenshot;
};



/*************************************************************************************************/
/*  Test printing                                                                                */
/*************************************************************************************************/

static void print_start()
{
    printf("--- Starting tests -------------------------------\n"); //
}



static void print_case(int index, const char* name)
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
/*  Test runner                                                                                  */
/*************************************************************************************************/

static TestCase find_test_case(uint32_t n_tests, TestCase* cases, const char* name)
{
    ASSERT(cases != NULL);

    for (uint32_t i = 0; i < n_tests; i++)
    {
        if (strcmp(cases[i].name, name) == 0)
        {
            return cases[i];
        }
    }
    log_error("test case %s not found!", name);
    return (TestCase){0};
}



static void _set_fixture(TestContext* tc, TestCase* test_case)
{
    ASSERT(tc != NULL);
    ASSERT(test_case != NULL);

    switch (test_case->fixture)
    {

    case TEST_FIXTURE_APP:
        if (tc->app == NULL)
            tc->app = dvz_app(DVZ_BACKEND_GLFW);
        tc->app->n_errors = 0;
        break;

    default:
        break;
    }
}



static int run_test_case(TestContext* tc, TestCase* test_case)
{
    ASSERT(tc != NULL);
    ASSERT(test_case != NULL);

    srand(0);

    if (test_case->function == NULL)
        return 1;

    // Make sure either the canvas or panel is set up if the test case requires it.
    // _setup(tc, test_case.fixture);

    // Set the case fixture.
    _set_fixture(tc, test_case);

    // Run the test case on the canvas.
    int res = 1;
    res = test_case->function(tc);

    if (tc->app != NULL)
    {
        res += (int)tc->app->n_errors;
        dvz_app_reset(tc->app);
    }

    return res;
}



#endif
