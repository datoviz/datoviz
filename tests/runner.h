#ifndef DVZ_TEST_RUNNER_HEADER
#define DVZ_TEST_RUNNER_HEADER

#include "../include/datoviz/common.h"
// NOTE: do not include tests.h (circular import)

// #include "../include/datoviz/datoviz.h"
// #include "../src/vklite_utils.h"

// BEGIN_INCL_NO_WARN
// #include "../external/stb_image.h"
// END_INCL_NO_WARN



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

    _set_fixture(tc, test_case);

    // Run the test case on the canvas.
    int res = 1;
    res = test_case->function(tc);

    /*
    // Run the app.
    if (test_case.fixture >= DVZ_TEST_FIXTURE_CANVAS)
    {
        ASSERT(tc->canvas != NULL);
        run_canvas(tc->canvas);
    }

        // Only continue here when offscreen mode.
        if (!tc->is_live)
        {
            // If the function passed and needs to be compared with the screenshot, do it.
            if (res == 0 && test_case.save_screenshot)
            {
                // TODO OPTIM: create the screenshot only once, when creating the canvas
                uint8_t* rgb = make_screenshot(tc);

                // Test fails if image is blank, not even need to compare with screenshot.
                if (is_blank(rgb))
                {
                    log_debug("image was blank, test failed");
                    res = 1;
                }
                else
                {
                    res = compare_images(name, rgb);
                    log_debug("image comparison %s", res == 0 ? "succeeded" : "failed");
                }
                FREE(rgb);
            }
        }

        // Call the test case-specific destruction function if there is one.
        if (test_case.destroy != NULL)
            test_case.destroy(tc);

        // Tear down the fixture (mainly resetting the canvas or destroying the scene).
        _teardown(tc, test_case.fixture);
    */

    return res;
}



#endif
