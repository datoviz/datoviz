/*************************************************************************************************/
/*  Testing canvas                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_canvas.h"
#include "_glfw.h"
#include "canvas.h"
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

static DvzGpu* make_gpu(DvzHost* host)
{
    ASSERT(host != NULL);

    DvzGpu* gpu = dvz_gpu_best(host);
    _default_queues(gpu, true);
    dvz_gpu_request_features(gpu, (VkPhysicalDeviceFeatures){.independentBlend = true});

    // HACK: temporarily create a blank window so that we can create a GPU with surface rendering
    // capabilities.
    DvzWindow* window = dvz_window(host, 100, 100);
    ASSERT(window->surface != VK_NULL_HANDLE);
    dvz_gpu_create(gpu, window->surface);
    dvz_window_destroy(window);

    return gpu;
}

int test_canvas_1(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzHost* host = get_host(suite);

    DvzGpu* gpu = make_gpu(host);

    // Create the board.
    DvzCanvas canvas = dvz_canvas(gpu, WIDTH, HEIGHT, 0);
    dvz_canvas_create(&canvas);

    dvz_canvas_loop(&canvas, N_FRAMES);

    dvz_canvas_destroy(&canvas);
    dvz_gpu_destroy(gpu);
    return 0;
}



static void _fill_triangle(DvzCanvas* canvas, DvzCommands* cmds, uint32_t idx)
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

    DvzGpu* gpu = make_gpu(host);

    // Context.
    DvzContext* ctx = dvz_context(gpu);
    ASSERT(ctx != NULL);

    // Create the pipelib.
    DvzPipelib* lib = dvz_pipelib(ctx);

    // Create the board.
    DvzCanvas canvas = dvz_canvas(gpu, WIDTH, HEIGHT, 0);
    dvz_canvas_refill(&canvas, _fill_triangle);
    dvz_canvas_create(&canvas);

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

    dvz_canvas_loop(&canvas, N_FRAMES);

    // Destruction.
    dvz_pipe_destroy(pipe);
    dvz_dat_destroy(dat_vertex);
    dvz_canvas_destroy(&canvas);
    dvz_pipelib_destroy(lib);
    dvz_context_destroy(ctx);
    dvz_gpu_destroy(gpu);
    return 0;
}
