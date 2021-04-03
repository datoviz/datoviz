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
    TEST_FIXTURE_CONTEXT,
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
    DvzContext* context;
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

static void _fixture_context(TestContext* tc)
{
    ASSERT(tc != NULL);
    ASSERT(tc->app != NULL);

    // Select the GPU.
    DvzGpu* gpu = dvz_gpu_best(tc->app);
    ASSERT(gpu != NULL);

    // Ensure the context is created.
    ASSERT(tc->context == NULL);

    // Recreate the GPU and context.
    dvz_gpu_default(gpu, NULL);
    tc->context = dvz_context(gpu);
}

static void _fixture_canvas(TestContext* tc)
{
    ASSERT(tc != NULL);
    ASSERT(tc->app != NULL);

    if (tc->canvas == NULL)
    {
        tc->canvas = dvz_canvas(dvz_gpu_best(tc->app), WIDTH, HEIGHT, 0);
        tc->context = tc->canvas->gpu->context;
        ASSERT(tc->context != NULL);
    }
    else
    {
        ASSERT(tc->canvas->gpu != NULL);
        // dvz_context_reset(tc->canvas->gpu->context);
        dvz_canvas_reset(tc->canvas);
    }
}

static void _fixture_begin(TestContext* tc, TestCase* test_case)
{
    ASSERT(tc != NULL);
    ASSERT(test_case != NULL);

    switch (test_case->fixture)
    {

        // App fixture.
    case TEST_FIXTURE_APP:
        _fixture_app(tc);
        break;

        // Context fixture.
    case TEST_FIXTURE_CONTEXT:
        _fixture_app(tc);
        _fixture_context(tc);
        break;

        // Canvas fixture.
    case TEST_FIXTURE_CANVAS:
        _fixture_app(tc);
        // HACK: offscreen canvas fixture.
        tc->app->backend = DVZ_BACKEND_OFFSCREEN;
        _fixture_canvas(tc);
        break;

    default:
        break;
    }
}

static void _fixture_end(TestContext* tc, TestCase* test_case)
{
    ASSERT(tc != NULL);
    ASSERT(test_case != NULL);

    switch (test_case->fixture)
    {

        // App fixture.
    case TEST_FIXTURE_APP:
        break;

        // Context fixture.
    case TEST_FIXTURE_CONTEXT:
        ASSERT(tc->context);
        dvz_gpu_destroy(tc->context->gpu);
        tc->context = NULL;
        break;

        // Canvas fixture.
    case TEST_FIXTURE_CANVAS:
        tc->app->backend = DVZ_BACKEND_GLFW;
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

    // Begin the fixture.
    _fixture_begin(tc, test_case);

    // Run the test case on the canvas.
    int res = 1;
    res = test_case->function(tc);

    if (tc->app != NULL)
    {
        res += (int)tc->app->n_errors;
    }

    // End the fixture.
    _fixture_end(tc, test_case);

    return res;
}



#endif
