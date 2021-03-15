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

#define AC(x, y, eps) AIN((x - y), -eps, +eps)



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
    CASE_FIXTURE_NONE(test_vklite_app),           //
    CASE_FIXTURE_NONE(test_vklite_commands),      //
    CASE_FIXTURE_NONE(test_vklite_buffer_1),      //
    CASE_FIXTURE_NONE(test_vklite_buffer_resize), //
    CASE_FIXTURE_NONE(test_vklite_compute),       //
    CASE_FIXTURE_NONE(test_vklite_push),          //
    CASE_FIXTURE_NONE(test_vklite_images),        //
    CASE_FIXTURE_NONE(test_vklite_sampler),       //
    CASE_FIXTURE_NONE(test_vklite_barrier),       //
    CASE_FIXTURE_NONE(test_vklite_submit),        //

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



/*
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

    // FIFO queue
    CASE_FIXTURE_NONE(test_fifo_1), //
    CASE_FIXTURE_NONE(test_fifo_2), //
    CASE_FIXTURE_NONE(test_fifo_3), //

    // context
    CASE_FIXTURE_NONE(test_default_app),      //
    CASE_FIXTURE_NONE(test_context_colormap), //

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
    CASE_FIXTURE_NONE(test_canvas_pick),             //
    CASE_FIXTURE_NONE(test_canvas_offscreen),        //
    CASE_FIXTURE_NONE(test_canvas_gui_1),            //
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

// NOTE: macOS does not support triangle_fan primitive
#if !OS_MACOS
    CASE_FIXTURE_NONE(test_graphics_triangle_fan), //
#endif
    CASE_FIXTURE_NONE(test_graphics_marker_1), //

    // generate marker screenshots:
    CASE_FIXTURE_NONE(test_graphics_marker_screenshots), //

    CASE_FIXTURE_NONE(test_graphics_segment),    //
    CASE_FIXTURE_NONE(test_graphics_path),       //
    CASE_FIXTURE_NONE(test_graphics_text),       //
    CASE_FIXTURE_NONE(test_graphics_image_1),    //
    CASE_FIXTURE_NONE(test_graphics_image_cmap), //

    CASE_FIXTURE_NONE(test_graphics_volume_1),     //
    CASE_FIXTURE_NONE(test_graphics_volume_slice), //
    CASE_FIXTURE_NONE(test_graphics_mesh_1),       //
    CASE_FIXTURE_NONE(test_graphics_mesh_2),       //

    // transforms
    CASE_FIXTURE_NONE(test_utils_transforms_1), //
    CASE_FIXTURE_NONE(test_utils_transforms_2), //
    CASE_FIXTURE_NONE(test_utils_transforms_3), //
    CASE_FIXTURE_NONE(test_utils_transforms_4), //
    CASE_FIXTURE_NONE(test_utils_transforms_5), //

    // array
    CASE_FIXTURE_NONE(test_array_1),    //
    CASE_FIXTURE_NONE(test_array_2),    //
    CASE_FIXTURE_NONE(test_array_3),    //
    CASE_FIXTURE_NONE(test_array_4),    //
    CASE_FIXTURE_NONE(test_array_5),    //
    CASE_FIXTURE_NONE(test_array_6),    //
    CASE_FIXTURE_NONE(test_array_7),    //
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
    CASE_FIXTURE_NONE(test_visuals_point),          //
    CASE_FIXTURE_NONE(test_visuals_line),           //
    CASE_FIXTURE_NONE(test_visuals_line_strip),     //
    CASE_FIXTURE_NONE(test_visuals_triangle),       //
    CASE_FIXTURE_NONE(test_visuals_triangle_strip), //
#if !OS_MACOS
    CASE_FIXTURE_NONE(test_visuals_triangle_fan), //
#endif

    CASE_FIXTURE_NONE(test_visuals_marker),         //
    CASE_FIXTURE_NONE(test_visuals_polygon),        //
    CASE_FIXTURE_NONE(test_visuals_path),           //
    CASE_FIXTURE_NONE(test_visuals_image_1),        //
    CASE_FIXTURE_NONE(test_visuals_image_cmap),     //
    CASE_FIXTURE_NONE(test_visuals_axes_2D_1),      //
    CASE_FIXTURE_NONE(test_visuals_axes_2D_update), //

    CASE_FIXTURE_NONE(test_visuals_mesh),         //
    CASE_FIXTURE_NONE(test_visuals_volume_1),     //
    CASE_FIXTURE_NONE(test_visuals_volume_slice), //

    // axes
    CASE_FIXTURE_NONE(test_axes_1), //
    CASE_FIXTURE_NONE(test_axes_2), //
    CASE_FIXTURE_NONE(test_axes_3), //

    // scene
    CASE_FIXTURE_NONE(test_scene_0),        //
    CASE_FIXTURE_NONE(test_scene_1),        //
    CASE_FIXTURE_NONE(test_scene_mesh),     //
    CASE_FIXTURE_NONE(test_scene_axes),     //
    CASE_FIXTURE_NONE(test_scene_logistic), //
*/



#endif
