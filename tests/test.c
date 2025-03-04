/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing suite                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdio.h>

#include "_thread_utils.h"
#include "fileio.h"
#include "input.h"
#include "scene/test_animation.h"
#include "scene/test_arcball.h"
#include "scene/test_array.h"
#include "scene/test_atlas.h"
#include "scene/test_axes.h"
#include "scene/test_axis.h"
#include "scene/test_baker.h"
#include "scene/test_box.h"
#include "scene/test_camera.h"
#include "scene/test_colormaps.h"
#include "scene/test_dual.h"
#include "scene/test_font.h"
#include "scene/test_graphics.h"
#include "scene/test_mvp.h"
#include "scene/test_ortho.h"
#include "scene/test_panzoom.h"
#include "scene/test_params.h"
#include "scene/test_ref.h"
#include "scene/test_scene.h"
#include "scene/test_sdf.h"
#include "scene/test_shape.h"
#include "scene/test_ticks.h"
#include "scene/test_viewset.h"
#include "scene/test_visual.h"
#include "scene/visuals/test_basic.h"
#include "scene/visuals/test_glyph.h"
#include "scene/visuals/test_image.h"
#include "scene/visuals/test_marker.h"
#include "scene/visuals/test_mesh.h"
#include "scene/visuals/test_monoglyph.h"
#include "scene/visuals/test_path.h"
#include "scene/visuals/test_pixel.h"
#include "scene/visuals/test_point.h"
#include "scene/visuals/test_segment.h"
#include "scene/visuals/test_slice.h"
#include "scene/visuals/test_sphere.h"
#include "scene/visuals/test_volume.h"
#include "scene/visuals/test_wiggle.h"
#include "test.h"
#include "test_alloc.h"
#include "test_app.h"
#include "test_board.h"
#include "test_canvas.h"
#include "test_client.h"
#include "test_client_input.h"
#include "test_datalloc.h"
#include "test_external.h"
#include "test_fifo.h"
#include "test_fileio.h"
#include "test_gui.h"
#include "test_host.h"
#include "test_input.h"
#include "test_keyboard.h"
#include "test_list.h"
#include "test_loop.h"
#include "test_map.h"
#include "test_mouse.h"
#include "test_obj.h"
#include "test_pipe.h"
#include "test_pipelib.h"
#include "test_presenter.h"
#include "test_prng.h"
#include "test_renderer.h"
#include "test_request.h"
#include "test_resources.h"
#include "test_server.h"
#include "test_thread.h"
#include "test_timer.h"
#include "test_transfers.h"
#include "test_vklite.h"
#include "test_window.h"
#include "test_workspace.h"
#include "testing.h"



/*************************************************************************************************/
/*  MACROS                                                                                       */
/*************************************************************************************************/

#define TEST(test, tags, setup, teardown, flags)                                                  \
    tst_suite_add(&suite, #test, tags, test, setup, teardown, NULL, flags);

#define TEST_NO_FIXTURE(test) TEST(test, tags, NULL, NULL, TST_ITEM_FLAGS_NONE)

#define TEST_FIXTURE(test) TEST(test, tags, setup, teardown, TST_ITEM_FLAGS_NONE)

#define TEST_STANDALONE(test) TEST(test, tags, setup, teardown, TST_ITEM_FLAGS_STANDALONE)



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int dvz_run_tests(const char* match)
{
    TstSuite suite = tst_suite();
    DvzTestCtx ctx = {0};
    suite.context = &ctx;

    char* tags = NULL;
    TstFunction setup = NULL;
    TstFunction teardown = NULL;



    /*********************************************************************************************/
    /*  Utils                                                                                    */
    /*********************************************************************************************/

    tags = "utils";

    TEST_NO_FIXTURE(test_thread_1)
    TEST_NO_FIXTURE(test_mutex_1)
    TEST_NO_FIXTURE(test_cond_1)
    TEST_NO_FIXTURE(test_atomic_1)
    TEST_NO_FIXTURE(test_prng_1)
    TEST_NO_FIXTURE(test_obj_1)
    TEST_NO_FIXTURE(test_png_1)
    TEST_NO_FIXTURE(test_fifo_1)
    TEST_NO_FIXTURE(test_fifo_2)
    TEST_NO_FIXTURE(test_fifo_resize)
    TEST_NO_FIXTURE(test_fifo_discard)
    TEST_NO_FIXTURE(test_fifo_first)
    TEST_NO_FIXTURE(test_deq_1)
    TEST_NO_FIXTURE(test_deq_2)
    TEST_NO_FIXTURE(test_deq_3)
    TEST_NO_FIXTURE(test_alloc_1)
    TEST_NO_FIXTURE(test_alloc_2)
    TEST_NO_FIXTURE(test_alloc_3)
    TEST_NO_FIXTURE(test_alloc_4)
    TEST_NO_FIXTURE(test_map_1)
    TEST_NO_FIXTURE(test_map_2)
    TEST_NO_FIXTURE(test_list_1)



    /*********************************************************************************************/
    /*  Input                                                                                    */
    /*********************************************************************************************/

    tags = "input";

    TEST_NO_FIXTURE(test_keyboard_1)
    TEST_NO_FIXTURE(test_keyboard_2)
    TEST_NO_FIXTURE(test_mouse_move)
    TEST_NO_FIXTURE(test_mouse_press)
    TEST_NO_FIXTURE(test_mouse_wheel)
    TEST_NO_FIXTURE(test_mouse_drag)
    TEST_NO_FIXTURE(test_timer_1)
    TEST_NO_FIXTURE(test_timer_2)
    TEST_NO_FIXTURE(test_input_mouse)
    TEST_NO_FIXTURE(test_input_keyboard)
    TEST_NO_FIXTURE(test_client_input)



    /*********************************************************************************************/
    /*  Client                                                                                   */
    /*********************************************************************************************/

    tags = "client";

    TEST_NO_FIXTURE(test_window_1)
    TEST_NO_FIXTURE(test_client_1)
    TEST_NO_FIXTURE(test_client_2)
    TEST_NO_FIXTURE(test_client_thread)



    /*********************************************************************************************/
    /*  Request                                                                                  */
    /*********************************************************************************************/

    tags = "request";

    TEST_NO_FIXTURE(test_request_1)
    TEST_NO_FIXTURE(test_requester_1)



    /*********************************************************************************************/
    /*  Scene utils                                                                              */
    /*********************************************************************************************/

    tags = "scene_utils";

    TEST_NO_FIXTURE(test_array_1)
    TEST_NO_FIXTURE(test_array_2)
    TEST_NO_FIXTURE(test_array_3)
    TEST_NO_FIXTURE(test_array_4)
    TEST_NO_FIXTURE(test_array_5)
    TEST_NO_FIXTURE(test_array_6)
    TEST_NO_FIXTURE(test_array_7)
    TEST_NO_FIXTURE(test_array_cast)
    TEST_NO_FIXTURE(test_array_mvp)
    TEST_NO_FIXTURE(test_array_3D)
    TEST_NO_FIXTURE(test_dual_1)
    TEST_NO_FIXTURE(test_dual_2)
    TEST_NO_FIXTURE(test_params_1)
    TEST_NO_FIXTURE(test_baker_1)
    TEST_NO_FIXTURE(test_baker_2)
    TEST_NO_FIXTURE(test_colormaps_default)
    TEST_NO_FIXTURE(test_colormaps_scale)
    TEST_NO_FIXTURE(test_colormaps_array)
    TEST_NO_FIXTURE(test_panzoom_1)
    TEST_NO_FIXTURE(test_panzoom_2)
    TEST_NO_FIXTURE(test_panzoom_3)
    TEST_NO_FIXTURE(test_arcball_1)
    TEST_NO_FIXTURE(test_camera_1)
    TEST_NO_FIXTURE(test_ortho_1)
    TEST_NO_FIXTURE(test_mvp_1)
    TEST_NO_FIXTURE(test_animation_1)
    TEST_NO_FIXTURE(test_shape_1)
    TEST_NO_FIXTURE(test_shape_surface)
    TEST_NO_FIXTURE(test_shape_transform)
    TEST_NO_FIXTURE(test_shape_obj)
    TEST_NO_FIXTURE(test_box_1)
    TEST_NO_FIXTURE(test_box_2)
    TEST_NO_FIXTURE(test_box_3)
    TEST_NO_FIXTURE(test_box_4)
    TEST_NO_FIXTURE(test_box_5)
    TEST_NO_FIXTURE(test_box_6)
    TEST_NO_FIXTURE(test_ticks_1)
    TEST_NO_FIXTURE(test_ticks_labels)
    TEST_NO_FIXTURE(test_ticks_2)
    TEST_NO_FIXTURE(test_ref_1)
    TEST_NO_FIXTURE(test_axis_1)
    TEST_NO_FIXTURE(test_axes_1)
    TEST_NO_FIXTURE(test_atlas_1)
    TEST_NO_FIXTURE(test_sdf_single)
    TEST_NO_FIXTURE(test_sdf_multi)
    TEST_NO_FIXTURE(test_font_1)



    /*********************************************************************************************/
    /*  Vklite                                                                                   */
    /*********************************************************************************************/

    tags = "vklite";
    setup = setup_host;
    teardown = teardown_host;

    TEST_NO_FIXTURE(test_host)

    TEST_FIXTURE(test_vklite_commands)
    TEST_FIXTURE(test_vklite_buffer_1)
    TEST_FIXTURE(test_vklite_buffer_resize)
    TEST_FIXTURE(test_vklite_load_shader)
    TEST_FIXTURE(test_vklite_compute)
    TEST_FIXTURE(test_vklite_push)
    TEST_FIXTURE(test_vklite_images)
    TEST_FIXTURE(test_vklite_sampler)
    TEST_FIXTURE(test_vklite_barrier_buffer)
    TEST_FIXTURE(test_vklite_barrier_image)
    TEST_FIXTURE(test_vklite_submit)
    TEST_FIXTURE(test_vklite_offscreen)
    TEST_FIXTURE(test_vklite_shader)
    TEST_FIXTURE(test_vklite_swapchain)
    TEST_FIXTURE(test_vklite_graphics)
    TEST_FIXTURE(test_vklite_indirect)
    TEST_FIXTURE(test_vklite_indexed)
    TEST_FIXTURE(test_vklite_instanced)
    TEST_FIXTURE(test_vklite_vertex_bindings)
    TEST_FIXTURE(test_vklite_constattr)
    TEST_FIXTURE(test_vklite_specialization)

    // DEBUGGING Vulkan SDK 1.3.275
    // TEST(test_vklite_sync_full)
    // TEST(test_vklite_sync_fail)



    /*********************************************************************************************/
    /*  Present                                                                                  */
    /*********************************************************************************************/

    tags = "present";

    setup = setup_gpu;
    teardown = teardown_gpu;

    TEST_FIXTURE(test_canvas_1)
    // TEST_FIXTURE(test_loop_1)
    // TEST_FIXTURE(test_loop_2)
    // TEST_FIXTURE(test_loop_cube)
    // TEST_FIXTURE(test_loop_gui)
    TEST_FIXTURE(test_gui_1)
    TEST_FIXTURE(test_gui_offscreen)
    TEST_FIXTURE(test_presenter_1)
    TEST_FIXTURE(test_presenter_2)
    TEST_FIXTURE(test_presenter_thread)
    TEST_FIXTURE(test_presenter_updates)
    TEST_FIXTURE(test_presenter_gui)
    TEST_FIXTURE(test_presenter_multi)



    /*********************************************************************************************/
    /*  Renderer                                                                                 */
    /*********************************************************************************************/

    tags = "renderer";

    TEST_FIXTURE(test_resources_1)
    TEST_FIXTURE(test_resources_dat_1)
    TEST_FIXTURE(test_resources_tex_1)
    TEST_FIXTURE(test_datalloc_1)
    TEST_FIXTURE(test_datalloc_2)
    TEST_FIXTURE(test_transfers_buffer_mappable)
    TEST_FIXTURE(test_transfers_buffer_large)
    TEST_FIXTURE(test_transfers_buffer_copy)
    TEST_FIXTURE(test_transfers_image_buffer)
    TEST_FIXTURE(test_transfers_direct_buffer)
    TEST_FIXTURE(test_transfers_direct_image)
    TEST_FIXTURE(test_transfers_dups_util)
    TEST_FIXTURE(test_transfers_dups_upload)
    TEST_FIXTURE(test_transfers_dups_copy)
    TEST_FIXTURE(test_resources_dat_transfers)
    TEST_FIXTURE(test_resources_dat_resize)
    TEST_FIXTURE(test_resources_tex_transfers)
    TEST_FIXTURE(test_resources_tex_resize)
    TEST_FIXTURE(test_board_1)
    TEST_FIXTURE(test_pipe_1)
    TEST_FIXTURE(test_pipelib_1)
    TEST_FIXTURE(test_workspace_1)
    TEST_FIXTURE(test_renderer_1)
    TEST_FIXTURE(test_renderer_graphics)
    TEST_FIXTURE(test_renderer_resize)
    TEST_FIXTURE(test_external_1)
    TEST_FIXTURE(test_server_1)



    /*********************************************************************************************/
    /*  Visuals                                                                                  */
    /*********************************************************************************************/

    tags = "visual";

    TEST_FIXTURE(test_visual_1)
    TEST_FIXTURE(test_viewset_1)
    TEST_FIXTURE(test_viewset_mouse)
    TEST_FIXTURE(test_basic_1)
    TEST_FIXTURE(test_basic_2)
    TEST_FIXTURE(test_monoglyph_1)
    TEST_FIXTURE(test_pixel_1)
    TEST_FIXTURE(test_point_1)
    TEST_FIXTURE(test_marker_code)
    TEST_FIXTURE(test_marker_bitmap)
    TEST_FIXTURE(test_marker_sdf)
    TEST_FIXTURE(test_marker_msdf)
    TEST_FIXTURE(test_marker_rotation)
    TEST_FIXTURE(test_segment_1)
    TEST_FIXTURE(test_path_1)
    TEST_FIXTURE(test_path_2)
    TEST_FIXTURE(test_path_closed)
    TEST_FIXTURE(test_glyph_1)
    TEST_FIXTURE(test_mesh_1)
    TEST_FIXTURE(test_mesh_2)
    TEST_FIXTURE(test_mesh_polygon)
    TEST_FIXTURE(test_mesh_edgecolor)
    TEST_FIXTURE(test_mesh_contour)
    TEST_FIXTURE(test_mesh_surface)
    TEST_FIXTURE(test_mesh_obj)
    TEST_FIXTURE(test_mesh_geo)
    TEST_FIXTURE(test_volume_1)
    TEST_FIXTURE(test_volume_2)
    TEST_FIXTURE(test_image_1)
    TEST_FIXTURE(test_image_2)
    TEST_FIXTURE(test_slice_1)
    TEST_FIXTURE(test_sphere_1)



    /*********************************************************************************************/
    /*  Scene                                                                                    */
    /*********************************************************************************************/

    tags = "scene";

    TEST_NO_FIXTURE(test_scene_1)
    TEST_NO_FIXTURE(test_scene_2)
    TEST_NO_FIXTURE(test_scene_3)
    TEST_NO_FIXTURE(test_scene_offscreen)
    TEST_NO_FIXTURE(test_scene_gui)



    /*********************************************************************************************/
    /*  App                                                                                      */
    /*********************************************************************************************/

    tags = "app";

    // TEST_FIXTURE(test_app_scatter)
    // TEST_FIXTURE(test_app_arcball)
    // TEST_FIXTURE(test_app_anim)
    // TEST_FIXTURE(test_app_pixel)
    // TEST_FIXTURE(test_app_viewset)

    tst_suite_run(&suite, match);
    tst_suite_destroy(&suite);
    return 0;
}
