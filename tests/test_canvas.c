/*************************************************************************************************/
/*  Testing canvas                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_canvas.h"
#include "_glfw.h"
#include "canvas.h"
#include "canvas_window.h"
#include "context.h"
#include "pipelib.h"
#include "test.h"
#include "test_resources.h"
#include "test_vklite.h"
#include "testing.h"
#include "window.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct TestCanvasStruct TestCanvasStruct;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct TestCanvasStruct
{
    DvzPipe* pipe;
    DvzBufferRegions br;
};



/*************************************************************************************************/
/*  Canvas tests                                                                                 */
/*************************************************************************************************/

int test_canvas_1(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzHost* host = get_host(suite);
    ASSERT(host != NULL);

    DvzGpu* gpu = make_gpu(host);
    ASSERT(gpu != NULL);

    // Create the window and surface.
    DvzWindow window = dvz_window(host->backend, WIDTH, HEIGHT, 0);
    VkSurfaceKHR surface = dvz_window_surface(host, &window);

    // Create the canvas.
    DvzCanvas canvas = dvz_canvas(gpu, WIDTH, HEIGHT, 0);
    dvz_canvas_create(&canvas, surface);

    dvz_canvas_loop(&canvas, &window, N_FRAMES);

    dvz_canvas_destroy(&canvas);
    dvz_window_destroy(&window);
    dvz_surface_destroy(host, surface);
    dvz_gpu_destroy(gpu);
    return 0;
}



static void _fill_triangle(DvzCanvas* canvas, DvzCommands* cmds, uint32_t idx, void* user_data)
{
    ASSERT(canvas != NULL);
    TestCanvasStruct* s = (TestCanvasStruct*)canvas->user_data;
    ASSERT(s != NULL);
    DvzPipe* pipe = s->pipe;
    triangle_commands(
        cmds, idx, &canvas->render.renderpass, &canvas->render.framebuffers, //
        &pipe->u.graphics, &pipe->bindings, s->br);
}

int test_canvas_triangle(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzHost* host = get_host(suite);
    ASSERT(host != NULL);

    DvzGpu* gpu = make_gpu(host);
    ASSERT(gpu != NULL);

    // Context.
    DvzContext* ctx = dvz_context(gpu);
    ASSERT(ctx != NULL);

    // Create the pipelib.
    DvzPipelib* lib = dvz_pipelib(ctx);

    // Create the window and surface.
    DvzWindow window = dvz_window(host->backend, WIDTH, HEIGHT, 0);
    VkSurfaceKHR surface = dvz_window_surface(host, &window);

    // Create the canvas.
    DvzCanvas canvas = dvz_canvas(gpu, WIDTH, HEIGHT, 0);
    dvz_canvas_refill(&canvas, _fill_triangle, canvas.refill_user_data);
    dvz_canvas_create(&canvas, surface);

    // Create a graphics pipe.
    uvec2 size = {WIDTH, HEIGHT};
    DvzPipe* pipe = dvz_pipelib_graphics(
        lib, ctx, &canvas.render.renderpass, 1, size, DVZ_GRAPHICS_TRIANGLE,
        DVZ_PIPELIB_FLAGS_CREATE_MVP | DVZ_PIPELIB_FLAGS_CREATE_VIEWPORT);

    // Create the vertex buffer dat.
    DvzDat* dat_vertex = dvz_dat(ctx, DVZ_BUFFER_TYPE_VERTEX, 3 * sizeof(DvzVertex), 0);
    ASSERT(dat_vertex != NULL);
    dvz_pipe_vertex(pipe, dat_vertex);

    // Upload the triangle data.
    DvzVertex data[] = {
        {{-1, -1, 0}, {255, 0, 0, 255}},
        {{+1, -1, 0}, {0, 255, 0, 255}},
        {{+0, +1, 0}, {0, 0, 255, 255}},
    };
    dvz_dat_upload(dat_vertex, 0, sizeof(data), data, true);

    // Run the lop.
    TestCanvasStruct s = {.pipe = pipe, .br = dat_vertex->br};
    canvas.user_data = &s;

    dvz_canvas_loop(&canvas, &window, N_FRAMES);

    // Destruction.
    dvz_pipe_destroy(pipe);
    dvz_dat_destroy(dat_vertex);
    dvz_canvas_destroy(&canvas);
    dvz_window_destroy(&window);
    dvz_surface_destroy(host, surface);
    dvz_pipelib_destroy(lib);
    dvz_context_destroy(ctx);
    dvz_gpu_destroy(gpu);
    return 0;
}
