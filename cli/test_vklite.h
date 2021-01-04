#ifndef VKY_VKLITE2_TEST_HEADER
#define VKY_VKLITE2_TEST_HEADER


#include "utils.h"



/*************************************************************************************************/
/*  vklite2                                                                                      */
/*************************************************************************************************/

int test_vklite_app(TestContext* context);
int test_vklite_surface(TestContext* context);
int test_vklite_window(TestContext* context);
int test_vklite_swapchain(TestContext* context);
int test_vklite_commands(TestContext* context);
int test_vklite_buffer(TestContext* context);
int test_vklite_compute(TestContext* context);
int test_vklite_push(TestContext* context);
int test_vklite_images(TestContext* context);
int test_vklite_sampler(TestContext* context);
int test_vklite_barrier(TestContext* context);
int test_vklite_submit(TestContext* context);
int test_vklite_blank(TestContext* context);
int test_vklite_graphics(TestContext* context);

int test_basic_canvas_1(TestContext* context);
int test_basic_canvas_triangle(TestContext* context);
int test_shader_compile(TestContext* context);


/*************************************************************************************************/
/*  FIFO queue                                                                                   */
/*************************************************************************************************/

int test_fifo(TestContext* context);



/*************************************************************************************************/
/*  Context                                                                                   */
/*************************************************************************************************/

int test_context_buffer_1(TestContext* context);
int test_context_buffer_2(TestContext* context);
int test_context_texture(TestContext* context);
int test_context_transfer_sync(TestContext* context);
int test_context_copy(TestContext* context);
int test_context_transfer_async_nothread(TestContext* context);
int test_context_transfer_async_thread(TestContext* context);
int test_context_download(TestContext* context);

int test_default_app(TestContext* context);



#endif
