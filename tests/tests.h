#ifndef DVZ_TEST_HEADER
#define DVZ_TEST_HEADER

#include "runner.h"



/*************************************************************************************************/
/*  Test macros                                                                                  */
/*************************************************************************************************/

#define AT(x)                                                                                     \
    if (!(x))                                                                                     \
    {                                                                                             \
        log_error("assertion '%s' failed", #x);                                                   \
        return 1;                                                                                 \
    }

#define AIN(x, m, M) AT((m) <= (x) && (x) <= (M))

#define AC(x, y, eps) AIN(((x) - (y)), -(eps), +(eps))
#define ACn(n, x, y, eps)                                                                         \
    for (uint32_t i = 0; i < (n); i++)                                                            \
        AC((x)[i], (y)[i], (eps));

#define WIDTH  800
#define HEIGHT 600
#define EPS    1e-6



/*************************************************************************************************/
/*  Test declarations                                                                            */
/*************************************************************************************************/

// Test utils.
int test_utils_container(TestContext*);
int test_utils_fifo_1(TestContext*);
int test_utils_fifo_2(TestContext*);
int test_utils_fifo_3(TestContext*);
int test_utils_array_1(TestContext*);
int test_utils_array_2(TestContext*);
int test_utils_array_3(TestContext*);
int test_utils_array_4(TestContext*);
int test_utils_array_5(TestContext*);
int test_utils_array_6(TestContext*);
int test_utils_array_7(TestContext*);
int test_utils_array_cast(TestContext*);
int test_utils_array_mvp(TestContext*);
int test_utils_array_3D(TestContext*);
int test_utils_transforms_1(TestContext*);
int test_utils_transforms_2(TestContext*);
int test_utils_transforms_3(TestContext*);
int test_utils_transforms_4(TestContext*);
// int test_utils_transforms_5(TestContext*);

// Test vklite.
int test_vklite_app(TestContext*);
int test_vklite_commands(TestContext*);
int test_vklite_buffer_1(TestContext*);
int test_vklite_buffer_resize(TestContext*);
int test_vklite_compute(TestContext*);
int test_vklite_push(TestContext*);
int test_vklite_images(TestContext*);
int test_vklite_sampler(TestContext*);
int test_vklite_barrier(TestContext*);
int test_vklite_submit(TestContext*);
int test_vklite_offscreen(TestContext*);
int test_vklite_shader(TestContext*);
int test_vklite_surface(TestContext*);
int test_vklite_window(TestContext*);
int test_vklite_swapchain(TestContext*);
int test_vklite_graphics(TestContext*);
int test_vklite_canvas_blank(TestContext*);
int test_vklite_canvas_triangle(TestContext*);

// Test canvas.
int test_canvas_blank(TestContext*);
int test_canvas_multiple(TestContext*);
int test_canvas_events(TestContext*);
int test_canvas_triangle_1(TestContext*);
int test_canvas_triangle_push(TestContext*);
int test_canvas_triangle_upload(TestContext*);
int test_canvas_triangle_uniform(TestContext*);



/*************************************************************************************************/
/*  List of tests                                                                                */
/*************************************************************************************************/

#define CASE_FIXTURE_NONE(func)                                                                   \
    {                                                                                             \
        .name = #func, .function = func,                                                          \
    }

#define CASE_FIXTURE_APP(func)                                                                    \
    {                                                                                             \
        .name = #func, .function = func, .fixture = TEST_FIXTURE_APP                              \
    }

#define CASE_FIXTURE_CANVAS(func)                                                                 \
    {                                                                                             \
        .name = #func, .function = func, .fixture = TEST_FIXTURE_CANVAS                           \
    }

static TestCase TEST_CASES[] = {
    // Utils.
    CASE_FIXTURE_NONE(test_utils_container),    //
    CASE_FIXTURE_NONE(test_utils_fifo_1),       //
    CASE_FIXTURE_NONE(test_utils_fifo_2),       //
    CASE_FIXTURE_NONE(test_utils_fifo_3),       //
    CASE_FIXTURE_NONE(test_utils_array_1),      //
    CASE_FIXTURE_NONE(test_utils_array_2),      //
    CASE_FIXTURE_NONE(test_utils_array_3),      //
    CASE_FIXTURE_NONE(test_utils_array_4),      //
    CASE_FIXTURE_NONE(test_utils_array_5),      //
    CASE_FIXTURE_NONE(test_utils_array_6),      //
    CASE_FIXTURE_NONE(test_utils_array_7),      //
    CASE_FIXTURE_NONE(test_utils_array_cast),   //
    CASE_FIXTURE_NONE(test_utils_array_mvp),    //
    CASE_FIXTURE_NONE(test_utils_array_3D),     //
    CASE_FIXTURE_NONE(test_utils_transforms_1), //
    CASE_FIXTURE_NONE(test_utils_transforms_2), //
    CASE_FIXTURE_NONE(test_utils_transforms_3), //
    CASE_FIXTURE_NONE(test_utils_transforms_4), //
    // CASE_FIXTURE_NONE(test_utils_transforms_5), //

    // vklite.
    CASE_FIXTURE_NONE(test_vklite_app),             //
    CASE_FIXTURE_NONE(test_vklite_commands),        //
    CASE_FIXTURE_NONE(test_vklite_buffer_1),        //
    CASE_FIXTURE_NONE(test_vklite_buffer_resize),   //
    CASE_FIXTURE_NONE(test_vklite_compute),         //
    CASE_FIXTURE_NONE(test_vklite_push),            //
    CASE_FIXTURE_NONE(test_vklite_images),          //
    CASE_FIXTURE_NONE(test_vklite_sampler),         //
    CASE_FIXTURE_NONE(test_vklite_barrier),         //
    CASE_FIXTURE_NONE(test_vklite_submit),          //
    CASE_FIXTURE_NONE(test_vklite_offscreen),       //
    CASE_FIXTURE_NONE(test_vklite_shader),          //
    CASE_FIXTURE_NONE(test_vklite_surface),         //
    CASE_FIXTURE_NONE(test_vklite_window),          //
    CASE_FIXTURE_NONE(test_vklite_swapchain),       //
    CASE_FIXTURE_NONE(test_vklite_graphics),        //
    CASE_FIXTURE_NONE(test_vklite_canvas_blank),    //
    CASE_FIXTURE_NONE(test_vklite_canvas_triangle), //

    // Canvas.
    CASE_FIXTURE_APP(test_canvas_blank),            //
    CASE_FIXTURE_APP(test_canvas_multiple),         //
    CASE_FIXTURE_APP(test_canvas_events),           //
    CASE_FIXTURE_APP(test_canvas_triangle_1),       //
    CASE_FIXTURE_APP(test_canvas_triangle_push),    //
    CASE_FIXTURE_APP(test_canvas_triangle_upload),  //
    CASE_FIXTURE_APP(test_canvas_triangle_uniform), //

};

static uint32_t N_TESTS = sizeof(TEST_CASES) / sizeof(TestCase);



/*************************************************************************************************/
/* Test context                                                                                  */
/*************************************************************************************************/

static TestContext _test_context()
{
    TestContext tc = {0};
    tc.n_tests = N_TESTS;
    tc.cases = TEST_CASES;
    return tc;
}

static void _test_context_destroy(TestContext* tc)
{
    ASSERT(tc != NULL);
    if (tc->canvas != NULL)
    {
        dvz_canvas_destroy(tc->canvas);
        tc->canvas = NULL;
    }
    if (tc->app != NULL)
    {
        dvz_app_destroy(tc->app);
        tc->app = NULL;
    }
}



#endif
