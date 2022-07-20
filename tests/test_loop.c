/*************************************************************************************************/
/*  Testing simple loop                                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_loop.h"
#include "../src/vklite_utils.h"
#include "canvas.h"
#include "loop.h"
#include "pipe.h"
#include "pipelib.h"
#include "test.h"
#include "test_vklite.h"
#include "testing.h"


/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct TestLoopStruct TestLoopStruct;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct TestLoopStruct
{
    DvzPipe* pipe;
    DvzBufferRegions br;
};


/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_loop_1(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzHost* host = get_host(suite);
    ASSERT(host != NULL);

    DvzGpu* gpu = make_gpu(host);
    ASSERT(gpu != NULL);

    DvzLoop* loop = dvz_loop(gpu, WIDTH, HEIGHT, 0);

    dvz_loop_run(loop, N_FRAMES);

    dvz_loop_destroy(loop);
    dvz_gpu_destroy(gpu);
    return 0;
}



static void _fill_triangle(DvzCanvas* canvas, DvzCommands* cmds, uint32_t idx, void* user_data)
{
    ASSERT(canvas != NULL);

    DvzGpu* gpu = canvas->gpu;
    ASSERT(gpu != NULL);

    TestLoopStruct* s = (TestLoopStruct*)user_data;
    ASSERT(s != NULL);

    DvzPipe* pipe = s->pipe;
    triangle_commands(
        cmds, idx, canvas->render.renderpass, &canvas->render.framebuffers, //
        &pipe->u.graphics, &pipe->bindings, s->br);
}

int test_loop_2(TstSuite* suite)
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

    // Create the loop, which creates a canvas and window.
    DvzLoop* loop = dvz_loop(gpu, WIDTH, HEIGHT, 0);
    // DvzCanvas* canvas = &loop->canvas;

    // Create a graphics pipe.
    uvec2 size = {WIDTH, HEIGHT};
    DvzPipe* pipe = dvz_pipelib_graphics(
        lib, ctx, &loop->renderpass, 1, size, DVZ_GRAPHICS_TRIANGLE,
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

    // Pass some information to the refill callbacK.
    TestLoopStruct s = {.pipe = pipe, .br = dat_vertex->br};
    dvz_loop_refill(loop, _fill_triangle, &s);

    // Run the loop.
    dvz_loop_run(loop, N_FRAMES);

    // Destroy objects.
    dvz_loop_destroy(loop);
    dvz_pipe_destroy(pipe);
    dvz_dat_destroy(dat_vertex);
    dvz_pipelib_destroy(lib);
    dvz_context_destroy(ctx);
    dvz_gpu_destroy(gpu);
    return 0;
}
