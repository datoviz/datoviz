#ifndef DVZ_TEST_RUNNER_HEADER
#define DVZ_TEST_RUNNER_HEADER

#include "../include/datoviz/common.h"
// NOTE: do not include tests.h (circular import)



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  800
#define HEIGHT 600



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    TEST_FIXTURE_NONE,
    TEST_FIXTURE_APP,
    // TEST_FIXTURE_GPU_OFFSCREEN,
    // TEST_FIXTURE_GPU_WINDOW,
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
};



struct TestCase
{
    const char* name;
    TestFunction function;
    TestFixture fixture;
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
/*  Test fixtures                                                                                */
/*************************************************************************************************/

static void _fixture_app(TestContext* tc)
{
    ASSERT(tc != NULL);
    if (tc->app == NULL)
    {
        tc->app = dvz_app(DVZ_BACKEND_GLFW);
    }
    else
    {
        // This call resets the context of all GPUs.
        dvz_app_reset(tc->app);
    }
    tc->app->n_errors = 0;
}

// static void _fixture_gpu_offscreen(TestContext* tc)
// {
//     ASSERT(tc != NULL);
//     ASSERT(tc->app != NULL);

//     if (tc->gpu == NULL)
//     {
//         tc->gpu = dvz_gpu_best(tc->app);
//     }

//     if (tc->gpu->context == NULL)
//     {
//         tc->gpu->context = dvz_context(tc->gpu);
//     }
// }

// static void _fixture_gpu_window(TestContext* tc)
// {
//     ASSERT(tc != NULL);
//     ASSERT(tc->app != NULL);

//     if (tc->gpu == NULL)
//         tc->gpu = dvz_gpu_best(tc->app);

//     if (tc->gpu->context != NULL)
//     {
//         dvz_context_reset(tc->gpu->context);
//     }
// }

static void _fixture_canvas(TestContext* tc)
{
    ASSERT(tc != NULL);
    ASSERT(tc->app != NULL);

    if (tc->canvas == NULL)
    {
        // ASSERT(tc->gpu != NULL);
        tc->canvas = dvz_canvas(dvz_gpu_best(tc->app), WIDTH, HEIGHT, 0);
    }
    // ASSERT(tc->gpu != NULL);

    // ASSERT(tc->gpu->context != NULL);
    // dvz_context_reset(tc->gpu->context);
}

static void _set_fixture(TestContext* tc, TestCase* test_case)
{
    ASSERT(tc != NULL);
    ASSERT(test_case != NULL);

    switch (test_case->fixture)
    {

        // App fixture.
    case TEST_FIXTURE_APP:
        _fixture_app(tc);
        break;

        //     // GPU fixture.
        // case TEST_FIXTURE_GPU_OFFSCREEN:
        //     _fixture_app(tc);
        //     _fixture_gpu_offscreen(tc);
        //     break;

        // case TEST_FIXTURE_GPU_WINDOW:
        //     _fixture_app(tc);
        //     _fixture_gpu_window(tc);
        //     break;

        // Canvas fixture.
    case TEST_FIXTURE_CANVAS:
        _fixture_app(tc);
        // _fixture_gpu_window(tc);
        _fixture_canvas(tc);
        break;

    default:
        break;
    }
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
    }

    return res;
}



#endif
