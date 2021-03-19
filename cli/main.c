#include <datoviz/datoviz.h>
#include <unistd.h>

#include "../tests/runner.h"
#include "../tests/tests.h"
#include "main.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define SWITCH_CLI_ARG(arg)                                                                       \
    if (argc >= 1 && strcmp(argv[1], #arg) == 0)                                                  \
        res = arg(argc - 1, &argv[1]);



/*************************************************************************************************/
/*  Main functions                                                                               */
/*************************************************************************************************/

static int test(int argc, char** argv)
{
    // argv: test, <name>, --live
    // bool is_live = argc >= 3 && strcmp(argv[2], "--live") == 0;
    print_start();

    // Create the test context.
    TestContext tc = _test_context();

    // Start the tests.
    int cur_res = 0;
    int res = 0;
    int index = 0;
    // Loop over all possible tests.
    for (uint32_t i = 0; i < tc.n_tests; i++)
    {
        // Run a test only if all tests are requested, or if the requested test matches
        // the current test.
        if (argc == 1 || strstr(tc.cases[i].name, argv[1]) != NULL)
        {
            print_case(index, tc.cases[i].name);
            cur_res = run_test_case(&tc, &tc.cases[i]);
            print_res(index, tc.cases[i].name, cur_res);
            res += cur_res == 0 ? 0 : 1;
            index++;
        }
    }
    print_end(index, res);

    // Destroy the test context.
    _test_context_destroy(&tc);

    return res;
}



static int info(int argc, char** argv)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    for (uint32_t i = 0; i < app->gpus.count; i++)
    {
        DvzGpu* gpu = dvz_gpu(app, i);
        printf("GPU #%u: %s\n", i, gpu->name);
    }
    dvz_app_destroy(app);
    return 0;
}



static int demo(int argc, char** argv)
{
    if (strcmp(argv[argc - 1], "gui") == 0)
    {
        dvz_demo_gui();
        return 0;
    }

    const int32_t N = 50000;
    dvec3* pos = calloc((uint32_t)N, sizeof(dvec3));
    for (int32_t i = 0; i < N; i++)
    {
        pos[i][0] = .25 * dvz_rand_normal();
        pos[i][1] = .25 * dvz_rand_normal();
        pos[i][2] = .25 * dvz_rand_normal();
    }
    dvz_demo_scatter(N, pos);
    FREE(pos);
    return 0;
}



int main(int argc, char** argv)
{
    log_set_level_env();
    if (argc <= 1)
    {
        log_error("specify a command: info, demo, test");
        return 1;
    }
    ASSERT(argc >= 2);
    int res = 0;
    SWITCH_CLI_ARG(info)
    SWITCH_CLI_ARG(test)
    SWITCH_CLI_ARG(demo)
    return res;
}
