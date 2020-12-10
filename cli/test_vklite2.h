#ifndef VKY_VKLITE2_TEST_HEADER
#define VKY_VKLITE2_TEST_HEADER


#include "utils.h"



/*************************************************************************************************/
/*  vklite2                                                                                      */
/*************************************************************************************************/

int test_app(VkyTestContext* context);
int test_surface(VkyTestContext* context);
int test_window(VkyTestContext* context);
int test_swapchain(VkyTestContext* context);
int test_commands(VkyTestContext* context);
int test_buffer(VkyTestContext* context);
int test_compute(VkyTestContext* context);
int test_push(VkyTestContext* context);
int test_images(VkyTestContext* context);
int test_sampler(VkyTestContext* context);
int test_barrier(VkyTestContext* context);
int test_submit(VkyTestContext* context);
int test_blank(VkyTestContext* context);
int test_graphics(VkyTestContext* context);

int test_basic_canvas_1(VkyTestContext* context);
int test_basic_canvas_triangle(VkyTestContext* context);
int test_shader_compile(VkyTestContext* context);


/*************************************************************************************************/
/*  FIFO queue                                                                                   */
/*************************************************************************************************/

int test_fifo(VkyTestContext* context);



/*************************************************************************************************/
/*  Context                                                                                   */
/*************************************************************************************************/

int test_context_buffer(VkyTestContext* context);
int test_context_texture(VkyTestContext* context);
int test_context_transfer_sync(VkyTestContext* context);
int test_context_copy(VkyTestContext* context);
int test_context_transfer_async_nothread(VkyTestContext* context);
int test_context_transfer_async_thread(VkyTestContext* context);
int test_context_download(VkyTestContext* context);

int test_default_app(VkyTestContext* context);



#endif
