#ifndef VKY_VKLITE2_TEST_HEADER
#define VKY_VKLITE2_TEST_HEADER


#include "utils.h"



/*************************************************************************************************/
/*  vklite2                                                                                      */
/*************************************************************************************************/

int test_app(TestContext* context);
int test_surface(TestContext* context);
int test_window(TestContext* context);
int test_swapchain(TestContext* context);
int test_commands(TestContext* context);
int test_buffer(TestContext* context);
int test_compute(TestContext* context);
int test_push(TestContext* context);
int test_images(TestContext* context);
int test_sampler(TestContext* context);
int test_barrier(TestContext* context);
int test_submit(TestContext* context);
int test_blank(TestContext* context);
int test_graphics(TestContext* context);

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

int test_context_buffer(TestContext* context);
int test_context_texture(TestContext* context);
int test_context_transfer_sync(TestContext* context);
int test_context_copy(TestContext* context);
int test_context_transfer_async_nothread(TestContext* context);
int test_context_transfer_async_thread(TestContext* context);
int test_context_download(TestContext* context);

int test_default_app(TestContext* context);



#endif
