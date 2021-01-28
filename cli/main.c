#include <unistd.h>
#include <visky/visky.h>

#include "test_array.h"
#include "test_builtin_visuals.h"
#include "test_canvas.h"
#include "test_common.h"
#include "test_graphics.h"
#include "test_interact.h"
#include "test_panel.h"
#include "test_scene.h"
#include "test_transforms.h"
#include "test_visuals.h"
#include "test_vklite.h"
#include "utils.h"



/*************************************************************************************************/
/*  List of tests                                                                                */
/*************************************************************************************************/

static TestCase TEST_CASES[] = {

    // common tests
    CASE_FIXTURE_NONE(test_container),

    // vklite2
    CASE_FIXTURE_NONE(test_vklite_app),            //
    CASE_FIXTURE_NONE(test_vklite_surface),        //
    CASE_FIXTURE_NONE(test_vklite_window),         //
    CASE_FIXTURE_NONE(test_vklite_swapchain),      //
    CASE_FIXTURE_NONE(test_vklite_commands),       //
    CASE_FIXTURE_NONE(test_vklite_buffer_1),       //
    CASE_FIXTURE_NONE(test_vklite_buffer_resize),  //
    CASE_FIXTURE_NONE(test_vklite_compute),        //
    CASE_FIXTURE_NONE(test_vklite_push),           //
    CASE_FIXTURE_NONE(test_vklite_images),         //
    CASE_FIXTURE_NONE(test_vklite_sampler),        //
    CASE_FIXTURE_NONE(test_vklite_barrier),        //
    CASE_FIXTURE_NONE(test_vklite_submit),         //
    CASE_FIXTURE_NONE(test_vklite_blank),          //
    CASE_FIXTURE_NONE(test_vklite_graphics),       //
    CASE_FIXTURE_NONE(test_basic_canvas_1),        //
    CASE_FIXTURE_NONE(test_basic_canvas_triangle), //
    CASE_FIXTURE_NONE(test_shader_compile),        //

    // context
    CASE_FIXTURE_NONE(test_fifo_1),      //
    CASE_FIXTURE_NONE(test_fifo_2),      //
    CASE_FIXTURE_NONE(test_default_app), //

    // canvas
    CASE_FIXTURE_NONE(test_canvas_transfer_buffer),  //
    CASE_FIXTURE_NONE(test_canvas_transfer_texture), //
    CASE_FIXTURE_NONE(test_canvas_1),                //
    CASE_FIXTURE_NONE(test_canvas_2),                //
    CASE_FIXTURE_NONE(test_canvas_3),                //
    CASE_FIXTURE_NONE(test_canvas_4),                //
    CASE_FIXTURE_NONE(test_canvas_5),                //
    CASE_FIXTURE_NONE(test_canvas_6),                //
    CASE_FIXTURE_NONE(test_canvas_7),                //
    CASE_FIXTURE_NONE(test_canvas_8),                //
    CASE_FIXTURE_NONE(test_canvas_depth),            //
    CASE_FIXTURE_NONE(test_canvas_append),           //
    CASE_FIXTURE_NONE(test_canvas_particles),        //
    CASE_FIXTURE_NONE(test_canvas_offscreen),        //
    CASE_FIXTURE_NONE(test_canvas_screencast),       //

    // graphics
    CASE_FIXTURE_NONE(test_graphics_dynamic), //
    CASE_FIXTURE_NONE(test_graphics_3D),      //
    CASE_FIXTURE_NONE(test_graphics_depth),   //

    CASE_FIXTURE_NONE(test_graphics_point),          //
    CASE_FIXTURE_NONE(test_graphics_line),           //
    CASE_FIXTURE_NONE(test_graphics_line_strip),     //
    CASE_FIXTURE_NONE(test_graphics_triangle),       //
    CASE_FIXTURE_NONE(test_graphics_triangle_strip), //
    CASE_FIXTURE_NONE(test_graphics_triangle_fan),   //

    CASE_FIXTURE_NONE(test_graphics_marker),       //
    CASE_FIXTURE_NONE(test_graphics_segment),      //
    CASE_FIXTURE_NONE(test_graphics_text),         //
    CASE_FIXTURE_NONE(test_graphics_image),        //
    CASE_FIXTURE_NONE(test_graphics_volume_1),     //
    CASE_FIXTURE_NONE(test_graphics_volume_slice), //
    CASE_FIXTURE_NONE(test_graphics_mesh),         //

    // transforms
    CASE_FIXTURE_NONE(test_transforms_1), //
    CASE_FIXTURE_NONE(test_transforms_2), //
    CASE_FIXTURE_NONE(test_transforms_3), //
    CASE_FIXTURE_NONE(test_transforms_4), //
    CASE_FIXTURE_NONE(test_transforms_5), //

    // array
    CASE_FIXTURE_NONE(test_array_1),    //
    CASE_FIXTURE_NONE(test_array_2),    //
    CASE_FIXTURE_NONE(test_array_3),    //
    CASE_FIXTURE_NONE(test_array_4),    //
    CASE_FIXTURE_NONE(test_array_5),    //
    CASE_FIXTURE_NONE(test_array_cast), //
    CASE_FIXTURE_NONE(test_array_mvp),  //
    CASE_FIXTURE_NONE(test_array_3D),   //

    // visuals
    CASE_FIXTURE_NONE(test_visuals_1), //
    CASE_FIXTURE_NONE(test_visuals_2), //
    CASE_FIXTURE_NONE(test_visuals_3), //
    CASE_FIXTURE_NONE(test_visuals_4), //
    CASE_FIXTURE_NONE(test_visuals_5), //

    // interact
    CASE_FIXTURE_NONE(test_interact_1),       //
    CASE_FIXTURE_NONE(test_interact_panzoom), //
    CASE_FIXTURE_NONE(test_interact_arcball), //
    CASE_FIXTURE_NONE(test_interact_camera),  //

    // panel
    CASE_FIXTURE_NONE(test_panel_1), //

    // builtin visuals
    CASE_FIXTURE_NONE(test_visuals_point),        //
    CASE_FIXTURE_NONE(test_visuals_marker),       //
    CASE_FIXTURE_NONE(test_visuals_line),         //
    CASE_FIXTURE_NONE(test_visuals_mesh),         //
    CASE_FIXTURE_NONE(test_visuals_volume_slice), //
    CASE_FIXTURE_NONE(test_visuals_axes_2D),      //

    // axes
    CASE_FIXTURE_NONE(test_axes_1), //
    CASE_FIXTURE_NONE(test_axes_2), //
    CASE_FIXTURE_NONE(test_axes_3), //

    // scene
    CASE_FIXTURE_NONE(test_scene_1),        //
    CASE_FIXTURE_NONE(test_scene_mesh),     //
    CASE_FIXTURE_NONE(test_scene_axes),     //
    CASE_FIXTURE_NONE(test_scene_logistic), //

};
static uint32_t N_TESTS = sizeof(TEST_CASES) / sizeof(TestCase);



/*************************************************************************************************/
/*  Tests utils                                                                                  */
/*************************************************************************************************/

static TestCase get_test_case(const char* name)
{
    for (uint32_t i = 0; i < N_TESTS; i++)
    {
        if (strcmp(TEST_CASES[i].name, name) == 0)
        {
            return TEST_CASES[i];
        }
    }
    log_error("test case %s not found!", name);
    return (TestCase){0};
}

static int launcher(TestContext* context, const char* name)
{
    srand(0);

    TestCase test_case = get_test_case(name);
    if (test_case.function == NULL)
        return 1;

    // ASSERT(context != NULL);
    ASSERT(name != NULL);

    // Make sure either the canvas or panel is set up if the test case requires it.
    // _setup(context, test_case.fixture);

    // Run the test case on the canvas.
    int res = 1;
    ASSERT(test_case.function != NULL);
    res = test_case.function(context);

    /*
    // Run the app.
    if (test_case.fixture >= VKY_TEST_FIXTURE_CANVAS)
    {
        ASSERT(context->canvas != NULL);
        run_canvas(context->canvas);
    }

        // Only continue here when offscreen mode.
        if (!context->is_live)
        {
            // If the function passed and needs to be compared with the screenshot, do it.
            if (res == 0 && test_case.save_screenshot)
            {
                // TODO OPTIM: create the screenshot only once, when creating the canvas
                uint8_t* rgb = make_screenshot(context);

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
            test_case.destroy(context);

        // Tear down the fixture (mainly resetting the canvas or destroying the scene).
        _teardown(context, test_case.fixture);
    */

    return res;
}



/*************************************************************************************************/
/*  Main functions                                                                               */
/*************************************************************************************************/

static int test(int argc, char** argv)
{
    // argv: test, <name>, --live
    // bool is_live = argc >= 3 && strcmp(argv[2], "--live") == 0;
    print_start();

    // Create the test context.
    // TestContext context = _create_context(is_live);

    // Start the tests.
    int cur_res = 0;
    int res = 0;
    int index = 0;
    // Loop over all possible tests.
    for (uint32_t i = 0; i < N_TESTS; i++)
    {
        // Run a test only if all tests are requested, or if the requested test matches
        // the current test.
        if (argc == 1 || strstr(TEST_CASES[i].name, argv[1]) != NULL)
        {
            print_case(index, TEST_CASES[i].name);
            cur_res = launcher(NULL, TEST_CASES[i].name);
            print_res(index, TEST_CASES[i].name, cur_res);
            res += cur_res == 0 ? 0 : 1;
            index++;
        }
    }
    print_end(index, res);

    // Destroy the app if needed.
    // _destroy_context(&context);

    return res;
}

static int info(int argc, char** argv)
{
    // VkyGpu gpu = vky_create_device(0, NULL);
    // printf("GPU: %s\n", gpu.device_properties.deviceName);
    // vky_destroy_device(&gpu);
    return 0;
}

static int demo(int argc, char** argv)
{
    // if (argc <= 1)
    // {
    //     log_error("please specify a demo name");
    //     return 1;
    // }
    // TestContext context = _create_context(true);
    // launcher(&context, argv[1]);
    // _destroy_context(&context);
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
