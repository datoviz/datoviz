/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_TESTS
#define DVZ_HEADER_TESTS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

// #include "../include/datoviz/canvas.h"
// #include "proto.h"
#include "checkimg.h"
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



/*************************************************************************************************/
/*  Test declarations                                                                            */
/*************************************************************************************************/

// Test utils.
int test_utils_container(TestContext*);
int test_utils_thread(TestContext*);
// int test_utils_fifo_1(TestContext*);
// int test_utils_fifo_2(TestContext*);
// int test_utils_fifo_resize(TestContext*);
// int test_utils_fifo_discard(TestContext*);
// int test_utils_fifo_first(TestContext*);
// int test_utils_deq_1(TestContext*);
// int test_utils_deq_2(TestContext*);
// int test_utils_deq_dependencies(TestContext*);
// int test_utils_deq_circular(TestContext*);
// int test_utils_deq_proc(TestContext*);
// int test_utils_deq_wait(TestContext*);
// int test_utils_deq_batch(TestContext*);

// int test_utils_array_1(TestContext*);
// int test_utils_array_2(TestContext*);
// int test_utils_array_3(TestContext*);
// int test_utils_array_4(TestContext*);
// int test_utils_array_5(TestContext*);
// int test_utils_array_6(TestContext*);
// int test_utils_array_7(TestContext*);
// int test_utils_array_cast(TestContext*);
// int test_utils_array_mvp(TestContext*);
// int test_utils_array_3D(TestContext*);

// int test_utils_alloc_1(TestContext*);
// int test_utils_alloc_2(TestContext*);

// // int test_utils_transforms_1(TestContext*);
// // int test_utils_transforms_2(TestContext*);
// // int test_utils_transforms_3(TestContext*);
// // int test_utils_transforms_4(TestContext*);
// // // int test_utils_transforms_5(TestContext*);

// int test_utils_colormap_idx(TestContext*);
// int test_utils_colormap_uv(TestContext*);
// int test_utils_colormap_extent(TestContext*);
// int test_utils_colormap_default(TestContext*);
// int test_utils_colormap_scale(TestContext*);
// int test_utils_colormap_packuv(TestContext*);
// int test_utils_colormap_array(TestContext*);

// int test_utils_ticks_1(TestContext*);
// int test_utils_ticks_2(TestContext*);
// int test_utils_ticks_duplicate(TestContext*);
// int test_utils_ticks_extend(TestContext*);

// // Test vklite.
// int test_vklite_app(TestContext*);
// int test_vklite_commands(TestContext*);
// int test_vklite_buffer_1(TestContext*);
// int test_vklite_buffer_resize(TestContext*);
// int test_vklite_compute(TestContext*);
// int test_vklite_push(TestContext*);
// int test_vklite_images(TestContext*);
// int test_vklite_sampler(TestContext*);
// int test_vklite_barrier_buffer(TestContext*);
// int test_vklite_barrier_image(TestContext*);
// int test_vklite_submit(TestContext*);
// int test_vklite_offscreen(TestContext*);
// int test_vklite_shader(TestContext*);
// int test_vklite_surface(TestContext*);
// int test_vklite_window(TestContext*);
// int test_vklite_swapchain(TestContext*);
// int test_vklite_graphics(TestContext*);
// int test_vklite_canvas_blank(TestContext*);
// int test_vklite_canvas_triangle(TestContext*);

// // Test transfers.
// int test_transfers_buffer_mappable(TestContext*);
// int test_transfers_buffer_large(TestContext*);
// int test_transfers_buffer_copy(TestContext*);
// int test_transfers_image_buffer(TestContext*);
// int test_transfers_direct_buffer(TestContext*);
// int test_transfers_direct_image(TestContext*);
// int test_transfers_dups_util(TestContext* tc);
// int test_transfers_dups_upload(TestContext* tc);
// int test_transfers_dups_copy(TestContext* tc);

// // Test resources.
// int test_ctx_resources_1(TestContext*);
// int test_ctx_datalloc_1(TestContext*);
// int test_ctx_dat_1(TestContext*);
// int test_ctx_dat_resize(TestContext*);
// int test_ctx_tex_1(TestContext*);
// int test_ctx_tex_resize(TestContext*);

// // Test input.
// int test_input_mouse_raw(TestContext*);
// int test_input_mouse_drag(TestContext*);
// int test_input_mouse_click(TestContext*);
// int test_input_keyboard(TestContext*);
// int test_input_timer(TestContext*);

// // Test canvas.
// int test_canvas_window(TestContext*);
// int test_canvas_blank(TestContext*);
// int test_canvas_triangle(TestContext*);

// // Test run.
// int test_run_1(TestContext*);
// int test_run_2(TestContext*);
// int test_run_3(TestContext*);
// int test_run_triangle(TestContext*);
// int test_run_offscreen(TestContext*);
// int test_run_push(TestContext*);
// int test_run_dat(TestContext*);
// int test_run_ubo(TestContext*);
// int test_run_upfill(TestContext*);

// int test_canvas_multiple(TestContext*);
// int test_canvas_events(TestContext*);
// int test_canvas_gui(TestContext*);
// int test_canvas_screencast(TestContext*);
// int test_canvas_video(TestContext*);

// int test_canvas_triangle_1(TestContext*);
// int test_canvas_triangle_resize(TestContext*);
// int test_canvas_triangle_offscreen(TestContext*);
// int test_canvas_triangle_push(TestContext*);
// int test_canvas_triangle_upload(TestContext*);
// int test_canvas_triangle_uniform(TestContext*);
// int test_canvas_triangle_compute(TestContext*);
// int test_canvas_triangle_pick(TestContext*);
// int test_canvas_triangle_append(TestContext*);

// // Test interact.
// int test_interact_panzoom(TestContext*);
// int test_interact_arcball(TestContext*);
// int test_interact_camera(TestContext*);

// // Test graphics.
// int test_graphics_point(TestContext*);
// int test_graphics_line_list(TestContext*);
// int test_graphics_line_strip(TestContext*);
// int test_graphics_triangle_list(TestContext*);
// int test_graphics_triangle_strip(TestContext*);
// int test_graphics_triangle_fan(TestContext*);
// int test_graphics_marker(TestContext*);
// int test_graphics_segment(TestContext*);
// int test_graphics_path(TestContext*);
// int test_graphics_text(TestContext*);
// int test_graphics_image_1(TestContext*);
// int test_graphics_image_cmap(TestContext*);
// int test_graphics_volume_slice(TestContext*);
// int test_graphics_volume_1(TestContext*);
// int test_graphics_mesh(TestContext*);

// // Test visuals.
// int test_visuals_sources(TestContext*);
// int test_visuals_props(TestContext*);
// int test_visuals_update_color(TestContext*);
// int test_visuals_update_pos(TestContext*);
// int test_visuals_partial(TestContext*);
// int test_visuals_append(TestContext*);
// int test_visuals_shared(TestContext*);

// // Test builtin visuals.
// int test_vislib_point(TestContext*);
// int test_vislib_line_list(TestContext*);
// int test_vislib_line_strip(TestContext*);
// int test_vislib_triangle_list(TestContext*);
// int test_vislib_triangle_strip(TestContext*);
// int test_vislib_triangle_fan(TestContext*);
// int test_vislib_marker(TestContext*);
// int test_vislib_polygon(TestContext*);
// int test_vislib_path(TestContext*);
// int test_vislib_image_1(TestContext*);
// int test_vislib_image_cmap(TestContext*);
// int test_vislib_axes_2D_x(TestContext*);
// int test_vislib_axes_2D_y(TestContext*);
// int test_vislib_mesh(TestContext*);
// int test_vislib_volume(TestContext*);
// int test_vislib_volume_slice(TestContext*);

// // Test scene.
// int test_scene_empty(TestContext*);
// int test_scene_empty_visuals(TestContext*);
// int test_scene_single(TestContext*);
// int test_scene_double(TestContext*);
// int test_scene_link(TestContext*);
// int test_scene_different_size(TestContext*);
// int test_scene_different_controllers(TestContext*);
// int test_scene_dynamic_axes(TestContext*);



/*************************************************************************************************/
/*  List of tests                                                                                */
/*************************************************************************************************/

#define CASE_FIXTURE(fixt, func)                                                                  \
    {                                                                                             \
        .name = #func, .function = func, .fixture = TEST_FIXTURE_##fixt                           \
    }

static TestCase TEST_CASES[] = {
    // Utils.
    CASE_FIXTURE(NONE, test_utils_container), //
    CASE_FIXTURE(NONE, test_utils_thread),    //
    // CASE_FIXTURE(NONE, test_utils_fifo_1),           //
    // CASE_FIXTURE(NONE, test_utils_fifo_2),           //
    // CASE_FIXTURE(NONE, test_utils_fifo_resize),      //
    // CASE_FIXTURE(NONE, test_utils_fifo_discard),     //
    // CASE_FIXTURE(NONE, test_utils_fifo_first),       //
    // CASE_FIXTURE(NONE, test_utils_deq_1),            //
    // CASE_FIXTURE(NONE, test_utils_deq_2),            //
    // CASE_FIXTURE(NONE, test_utils_deq_dependencies), //
    // CASE_FIXTURE(NONE, test_utils_deq_circular),     //
    // CASE_FIXTURE(NONE, test_utils_deq_proc),         //
    // CASE_FIXTURE(NONE, test_utils_deq_wait),         //
    // CASE_FIXTURE(NONE, test_utils_deq_batch),        //

    // // Array.
    // CASE_FIXTURE(NONE, test_utils_array_1),    //
    // CASE_FIXTURE(NONE, test_utils_array_2),    //
    // CASE_FIXTURE(NONE, test_utils_array_3),    //
    // CASE_FIXTURE(NONE, test_utils_array_4),    //
    // CASE_FIXTURE(NONE, test_utils_array_5),    //
    // CASE_FIXTURE(NONE, test_utils_array_6),    //
    // CASE_FIXTURE(NONE, test_utils_array_7),    //
    // CASE_FIXTURE(NONE, test_utils_array_cast), //
    // CASE_FIXTURE(NONE, test_utils_array_mvp),  //
    // CASE_FIXTURE(NONE, test_utils_array_3D),   //

    // // Alloc.
    // CASE_FIXTURE(NONE, test_utils_alloc_1), //
    // CASE_FIXTURE(NONE, test_utils_alloc_2), //

    // // CASE_FIXTURE(NONE, test_utils_transforms_1),     //
    // // CASE_FIXTURE(NONE, test_utils_transforms_2),     //
    // // CASE_FIXTURE(NONE, test_utils_transforms_3),     //
    // // CASE_FIXTURE(NONE, test_utils_transforms_4),     //
    // CASE_FIXTURE(NONE, test_utils_colormap_idx),     //
    // CASE_FIXTURE(NONE, test_utils_colormap_uv),      //
    // CASE_FIXTURE(NONE, test_utils_colormap_extent),  //
    // CASE_FIXTURE(NONE, test_utils_colormap_default), //
    // CASE_FIXTURE(NONE, test_utils_colormap_scale),   //
    // CASE_FIXTURE(NONE, test_utils_colormap_packuv),  //
    // CASE_FIXTURE(NONE, test_utils_colormap_array),   //
    // CASE_FIXTURE(NONE, test_utils_ticks_1),          //
    // CASE_FIXTURE(NONE, test_utils_ticks_2),          //
    // CASE_FIXTURE(NONE, test_utils_ticks_duplicate),  //
    // CASE_FIXTURE(NONE, test_utils_ticks_extend),     //

    // // Input.
    // CASE_FIXTURE(NONE, test_input_mouse_raw),   //
    // CASE_FIXTURE(NONE, test_input_mouse_drag),  //
    // CASE_FIXTURE(NONE, test_input_mouse_click), //
    // CASE_FIXTURE(NONE, test_input_keyboard),    //
    // CASE_FIXTURE(NONE, test_input_timer),       //

    // // vklite.
    // CASE_FIXTURE(NONE, test_vklite_app),             //
    // CASE_FIXTURE(NONE, test_vklite_commands),        //
    // CASE_FIXTURE(NONE, test_vklite_buffer_1),        //
    // CASE_FIXTURE(NONE, test_vklite_buffer_resize),   //
    // CASE_FIXTURE(NONE, test_vklite_compute),         //
    // CASE_FIXTURE(NONE, test_vklite_push),            //
    // CASE_FIXTURE(NONE, test_vklite_images),          //
    // CASE_FIXTURE(NONE, test_vklite_sampler),         //
    // CASE_FIXTURE(NONE, test_vklite_barrier_buffer),  //
    // CASE_FIXTURE(NONE, test_vklite_barrier_image),   //
    // CASE_FIXTURE(NONE, test_vklite_submit),          //
    // CASE_FIXTURE(NONE, test_vklite_offscreen),       //
    // CASE_FIXTURE(NONE, test_vklite_shader),          //
    // CASE_FIXTURE(NONE, test_vklite_surface),         //
    // CASE_FIXTURE(NONE, test_vklite_window),          //
    // CASE_FIXTURE(NONE, test_vklite_swapchain),       //
    // CASE_FIXTURE(NONE, test_vklite_graphics),        //
    // CASE_FIXTURE(NONE, test_vklite_canvas_blank),    //
    // CASE_FIXTURE(NONE, test_vklite_canvas_triangle), //

    // // Transfers.
    // CASE_FIXTURE(TRANSFERS, test_transfers_buffer_mappable), //
    // CASE_FIXTURE(TRANSFERS, test_transfers_buffer_large),    //
    // CASE_FIXTURE(TRANSFERS, test_transfers_buffer_copy),     //
    // CASE_FIXTURE(TRANSFERS, test_transfers_image_buffer),    //
    // CASE_FIXTURE(TRANSFERS, test_transfers_direct_buffer),   //
    // CASE_FIXTURE(TRANSFERS, test_transfers_direct_image),    //
    // CASE_FIXTURE(TRANSFERS, test_transfers_dups_util),       //
    // CASE_FIXTURE(TRANSFERS, test_transfers_dups_upload),     //
    // CASE_FIXTURE(TRANSFERS, test_transfers_dups_copy),       //

    // // Resources.
    // CASE_FIXTURE(CONTEXT, test_ctx_resources_1), //
    // CASE_FIXTURE(CONTEXT, test_ctx_datalloc_1),  //
    // CASE_FIXTURE(CONTEXT, test_ctx_dat_1),       //
    // CASE_FIXTURE(CONTEXT, test_ctx_dat_resize),  //
    // CASE_FIXTURE(CONTEXT, test_ctx_tex_1),       //
    // CASE_FIXTURE(CONTEXT, test_ctx_tex_resize),  //

    // // Canvas.
    // CASE_FIXTURE(APP, test_canvas_window),   //
    // CASE_FIXTURE(APP, test_canvas_blank),    //
    // CASE_FIXTURE(APP, test_canvas_triangle), //

    // // Run.
    // CASE_FIXTURE(APP, test_run_1),         //
    // CASE_FIXTURE(APP, test_run_2),         //
    // CASE_FIXTURE(APP, test_run_3),         //
    // CASE_FIXTURE(APP, test_run_triangle),  //
    // CASE_FIXTURE(APP, test_run_offscreen), //
    // CASE_FIXTURE(APP, test_run_push),      //
    // CASE_FIXTURE(APP, test_run_dat),       //
    // CASE_FIXTURE(APP, test_run_ubo),       //
    // CASE_FIXTURE(APP, test_run_upfill),    //



    // CASE_FIXTURE(APP, test_canvas_multiple),           //
    // CASE_FIXTURE(APP, test_canvas_events),             //
    // CASE_FIXTURE(APP, test_canvas_gui),                //
    // CASE_FIXTURE(APP, test_canvas_screencast),         //
    // CASE_FIXTURE(APP, test_canvas_video),              //
    // CASE_FIXTURE(APP, test_canvas_triangle_1),         //
    // CASE_FIXTURE(APP, test_canvas_triangle_resize),    //
    // CASE_FIXTURE(APP, test_canvas_triangle_offscreen), //
    // CASE_FIXTURE(APP, test_canvas_triangle_push),      //
    // CASE_FIXTURE(APP, test_canvas_triangle_upload),    //
    // CASE_FIXTURE(APP, test_canvas_triangle_uniform),   //
    // CASE_FIXTURE(APP, test_canvas_triangle_compute),   //
    // CASE_FIXTURE(APP, test_canvas_triangle_pick),      //
    // CASE_FIXTURE(APP, test_canvas_triangle_append),    //

    // // Interact.
    // CASE_FIXTURE(CANVAS, test_interact_panzoom), //
    // CASE_FIXTURE(CANVAS, test_interact_arcball), //
    // CASE_FIXTURE(CANVAS, test_interact_camera),  //

    // // Graphics.
    // CASE_FIXTURE(CANVAS, test_graphics_point),          //
    // CASE_FIXTURE(CANVAS, test_graphics_line_list),      //
    // CASE_FIXTURE(CANVAS, test_graphics_line_strip),     //
    // CASE_FIXTURE(CANVAS, test_graphics_triangle_list),  //
    // CASE_FIXTURE(CANVAS, test_graphics_triangle_strip), //
    // CASE_FIXTURE(CANVAS, test_graphics_triangle_fan),   //
    // CASE_FIXTURE(CANVAS, test_graphics_marker),         //
    // CASE_FIXTURE(CANVAS, test_graphics_segment),        //
    // CASE_FIXTURE(CANVAS, test_graphics_path),           //
    // CASE_FIXTURE(CANVAS, test_graphics_text),           //
    // CASE_FIXTURE(CANVAS, test_graphics_image_1),        //
    // CASE_FIXTURE(CANVAS, test_graphics_image_cmap),     //
    // CASE_FIXTURE(CANVAS, test_graphics_volume_slice),   //
    // CASE_FIXTURE(CANVAS, test_graphics_volume_1),       //
    // CASE_FIXTURE(CANVAS, test_graphics_mesh),           //

    // // Visuals.
    // CASE_FIXTURE(CANVAS, test_visuals_sources),      //
    // CASE_FIXTURE(CANVAS, test_visuals_props),        //
    // CASE_FIXTURE(CANVAS, test_visuals_update_color), //
    // CASE_FIXTURE(CANVAS, test_visuals_update_pos),   //
    // CASE_FIXTURE(CANVAS, test_visuals_partial),      //
    // CASE_FIXTURE(CANVAS, test_visuals_append),       //
    // CASE_FIXTURE(CANVAS, test_visuals_shared),       //

    // // Builtin visuals.
    // CASE_FIXTURE(CANVAS, test_vislib_point),          //
    // CASE_FIXTURE(CANVAS, test_vislib_line_list),      //
    // CASE_FIXTURE(CANVAS, test_vislib_line_strip),     //
    // CASE_FIXTURE(CANVAS, test_vislib_triangle_list),  //
    // CASE_FIXTURE(CANVAS, test_vislib_triangle_strip), //
    // CASE_FIXTURE(CANVAS, test_vislib_triangle_fan),   //
    // CASE_FIXTURE(CANVAS, test_vislib_marker),         //
    // CASE_FIXTURE(CANVAS, test_vislib_polygon),        //
    // CASE_FIXTURE(CANVAS, test_vislib_path),           //
    // CASE_FIXTURE(CANVAS, test_vislib_image_1),        //
    // CASE_FIXTURE(CANVAS, test_vislib_image_cmap),     //
    // CASE_FIXTURE(CANVAS, test_vislib_axes_2D_x),      //
    // CASE_FIXTURE(CANVAS, test_vislib_axes_2D_y),      //
    // CASE_FIXTURE(CANVAS, test_vislib_mesh),           //
    // CASE_FIXTURE(CANVAS, test_vislib_volume),         //
    // CASE_FIXTURE(CANVAS, test_vislib_volume_slice),   //

    // // Scene.
    // CASE_FIXTURE(CANVAS, test_scene_empty),                 //
    // CASE_FIXTURE(CANVAS, test_scene_empty_visuals),         //
    // CASE_FIXTURE(CANVAS, test_scene_single),                //
    // CASE_FIXTURE(CANVAS, test_scene_double),                //
    // CASE_FIXTURE(CANVAS, test_scene_link),                  //
    // CASE_FIXTURE(CANVAS, test_scene_different_size),        //
    // CASE_FIXTURE(CANVAS, test_scene_different_controllers), //
    // CASE_FIXTURE(CANVAS, test_scene_dynamic_axes),          //

};

static uint32_t N_TESTS = sizeof(TEST_CASES) / sizeof(TestCase);



/*************************************************************************************************/
/* Test context                                                                                  */
/*************************************************************************************************/

static TestContext _test_context()
{
    TestContext tc = {0};
    // tc.n_tests = N_TESTS;
    // tc.cases = TEST_CASES;
    // tc.debug = DEBUG_TEST;
    return tc;
}

static void _test_context_destroy(TestContext* tc)
{
    // ASSERT(tc != NULL);

    // if (tc->canvas != NULL)
    // {
    //     dvz_canvas_destroy(tc->canvas);
    //     tc->canvas = NULL;
    // }

    // // if (tc->context != NULL)
    // // {
    // //     dvz_context_destroy(tc->context);
    // //     tc->context = NULL;
    // // }

    // if (tc->app != NULL)
    // {
    //     dvz_app_destroy(tc->app);
    //     tc->app = NULL;
    // }
}



#endif
