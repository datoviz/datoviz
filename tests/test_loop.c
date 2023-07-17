/*************************************************************************************************/
/*  Testing simple loop                                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_loop.h"
#include "canvas.h"
#include "gui.h"
#include "loop.h"
#include "pipe.h"
#include "pipelib.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"


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
    ANN(suite);
    DvzHost* host = get_host(suite);
    ANN(host);

    DvzGpu* gpu = make_gpu(host);
    ANN(gpu);

    DvzLoop* loop = dvz_loop(gpu, WIDTH, HEIGHT, 0);

    dvz_loop_run(loop, N_FRAMES);

    dvz_loop_destroy(loop);
    dvz_gpu_destroy(gpu);
    return 0;
}



static void _fill_triangle(DvzCanvas* canvas, DvzCommands* cmds, uint32_t idx, void* user_data)
{
    ANN(canvas);

    DvzGpu* gpu = canvas->gpu;
    ANN(gpu);

    TestLoopStruct* s = (TestLoopStruct*)user_data;
    ANN(s);

    DvzPipe* pipe = s->pipe;
    triangle_commands(
        cmds, idx, canvas->render.renderpass, &canvas->render.framebuffers, //
        &pipe->u.graphics, &pipe->descriptors, s->br);
}

int test_loop_2(TstSuite* suite)
{
    ANN(suite);
    DvzHost* host = get_host(suite);
    ANN(host);

    DvzGpu* gpu = make_gpu(host);
    ANN(gpu);

    // Context.
    DvzContext* ctx = dvz_context(gpu);
    ANN(ctx);

    // Create the pipelib.
    DvzPipelib* lib = dvz_pipelib(ctx);

    // Create the loop, which creates a canvas and window.
    DvzLoop* loop = dvz_loop(gpu, WIDTH, HEIGHT, 0);
    // DvzCanvas* canvas = &loop->canvas;

    // Create a graphics pipe.
    DvzPipe* pipe = dvz_pipelib_graphics(
        lib, ctx, &loop->renderpass, DVZ_GRAPHICS_TRIANGLE,
        DVZ_PIPELIB_FLAGS_CREATE_MVP | DVZ_PIPELIB_FLAGS_CREATE_VIEWPORT);

    // NOTE: we now have to create the pipe manually (or automatically when using recorder.c).
    dvz_pipe_create(pipe);

    // Create the vertex buffer dat.
    DvzDat* dat_vertex = dvz_dat(ctx, DVZ_BUFFER_TYPE_VERTEX, 3 * sizeof(DvzVertex), 0);
    ANN(dat_vertex);
    dvz_pipe_vertex(pipe, 0, dat_vertex, 0);

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



static void _fill_cube(DvzCanvas* canvas, DvzCommands* cmds, uint32_t idx, void* user_data)
{
    ANN(canvas);

    DvzGpu* gpu = canvas->gpu;
    ANN(gpu);

    TestLoopStruct* s = (TestLoopStruct*)user_data;
    ANN(s);

    DvzPipe* pipe = s->pipe;

    uint32_t width = canvas->render.framebuffers.attachments[0]->shape[0];
    uint32_t height = canvas->render.framebuffers.attachments[0]->shape[1];
    uint32_t n_vertices = 36;

    ASSERT(width > 0);
    ASSERT(height > 0);

    // Commands.
    dvz_cmd_begin(cmds, idx);
    dvz_cmd_begin_renderpass(cmds, idx, canvas->render.renderpass, &canvas->render.framebuffers);
    dvz_cmd_viewport(cmds, idx, (VkViewport){0, 0, (float)width, (float)height, 0, 1});
    dvz_cmd_bind_vertex_buffer(cmds, idx, 1, (DvzBufferRegions[]){s->br}, (DvzSize[]){0});
    dvz_cmd_bind_descriptors(cmds, idx, &pipe->descriptors, 0);
    dvz_cmd_bind_graphics(cmds, idx, &pipe->u.graphics);
    dvz_cmd_draw(cmds, idx, 0, n_vertices, 0, 1);
    dvz_cmd_end_renderpass(cmds, idx);
    dvz_cmd_end(cmds, idx);
}

int test_loop_cube(TstSuite* suite)
{
    ANN(suite);
    DvzHost* host = get_host(suite);
    ANN(host);

    DvzGpu* gpu = make_gpu(host);
    ANN(gpu);

    // Context.
    DvzContext* ctx = dvz_context(gpu);
    ANN(ctx);

    // Create the pipelib.
    DvzPipelib* lib = dvz_pipelib(ctx);

    // Create the loop, which creates a canvas and window.
    DvzLoop* loop = dvz_loop(gpu, WIDTH, HEIGHT, 0);
    // DvzCanvas* canvas = &loop->canvas;

    // Create a graphics pipe.
    DvzPipe* pipe = dvz_pipelib_graphics(
        lib, ctx, &loop->renderpass, DVZ_GRAPHICS_TRIANGLE, DVZ_PIPELIB_FLAGS_CREATE_VIEWPORT);

    // Create a MVP dat manually.
    DvzDat* dat_mvp = dvz_dat(ctx, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzMVP), 0);
    DvzMVP mvp = dvz_mvp_default();
    dvz_dat_upload(dat_mvp, 0, sizeof(mvp), &mvp, true);
    dvz_pipe_dat(pipe, 0, dat_mvp);

    // NOTE: we now have to create the pipe manually (or automatically when using recorder.c).
    dvz_pipe_create(pipe);

    // Create the vertex buffer dat.
    const uint32_t vertex_count = 36;
    DvzDat* dat_vertex = dvz_dat(ctx, DVZ_BUFFER_TYPE_VERTEX, vertex_count * sizeof(DvzVertex), 0);
    ANN(dat_vertex);
    dvz_pipe_vertex(pipe, 0, dat_vertex, 0);

    // Upload the triangle data.
    float x = .5;
    DvzVertex data[] = {
        {{-x, -x, +x}, {255, 0, 0, 255}},   // front
        {{+x, -x, +x}, {255, 0, 0, 255}},   //
        {{+x, +x, +x}, {255, 0, 0, 255}},   //
        {{+x, +x, +x}, {255, 0, 0, 255}},   //
        {{-x, +x, +x}, {255, 0, 0, 255}},   //
        {{-x, -x, +x}, {255, 0, 0, 255}},   //
        {{+x, -x, +x}, {0, 255, 0, 255}},   // right
        {{+x, -x, -x}, {0, 255, 0, 255}},   //
        {{+x, +x, -x}, {0, 255, 0, 255}},   //
        {{+x, +x, -x}, {0, 255, 0, 255}},   //
        {{+x, +x, +x}, {0, 255, 0, 255}},   //
        {{+x, -x, +x}, {0, 255, 0, 255}},   //
        {{-x, +x, -x}, {0, 0, 255, 255}},   // back
        {{+x, +x, -x}, {0, 0, 255, 255}},   //
        {{+x, -x, -x}, {0, 0, 255, 255}},   //
        {{+x, -x, -x}, {0, 0, 255, 255}},   //
        {{-x, -x, -x}, {0, 0, 255, 255}},   //
        {{-x, +x, -x}, {0, 0, 255, 255}},   //
        {{-x, -x, -x}, {0, 255, 255, 255}}, // left
        {{-x, -x, +x}, {0, 255, 255, 255}}, //
        {{-x, +x, +x}, {0, 255, 255, 255}}, //
        {{-x, +x, +x}, {0, 255, 255, 255}}, //
        {{-x, +x, -x}, {0, 255, 255, 255}}, //
        {{-x, -x, -x}, {0, 255, 255, 255}}, //
        {{-x, -x, -x}, {255, 0, 255, 255}}, // bottom
        {{+x, -x, -x}, {255, 0, 255, 255}}, //
        {{+x, -x, +x}, {255, 0, 255, 255}}, //
        {{+x, -x, +x}, {255, 0, 255, 255}}, //
        {{-x, -x, +x}, {255, 0, 255, 255}}, //
        {{-x, -x, -x}, {255, 0, 255, 255}}, //
        {{-x, +x, +x}, {255, 255, 0, 255}}, // top
        {{+x, +x, +x}, {255, 255, 0, 255}}, //
        {{+x, +x, -x}, {255, 255, 0, 255}}, //
        {{+x, +x, -x}, {255, 255, 0, 255}}, //
        {{-x, +x, -x}, {255, 255, 0, 255}}, //
        {{-x, +x, +x}, {255, 255, 0, 255}}, //
    };

    dvz_dat_upload(dat_vertex, 0, sizeof(data), data, true);

    // Pass some information to the refill callbacK.
    TestLoopStruct s = {.pipe = pipe, .br = dat_vertex->br};
    dvz_loop_refill(loop, _fill_cube, &s);

    // View matrix.
    glm_lookat((vec3){0, 0, 4}, (vec3){0}, (vec3){0, 1, 0}, mvp.view);

    // Projection matrix.
    glm_perspective(GLM_PI_4, WIDTH / (float)HEIGHT, .1, 100, mvp.proj);

    // Run the loop.
    for (loop->frame_idx = 0;; loop->frame_idx++)
    {
        // Model matrix.
        glm_rotate_y(mvp.model, .001, mvp.model);

        // Upload the MVP struct.
        dvz_dat_upload(dat_mvp, 0, sizeof(mvp), &mvp, true);

        if (dvz_loop_frame(loop))
            break;
    }

    dvz_gpu_wait(loop->gpu);

    // Destroy objects.
    dvz_loop_destroy(loop);
    dvz_pipe_destroy(pipe);
    dvz_dat_destroy(dat_vertex);
    dvz_pipelib_destroy(lib);
    dvz_context_destroy(ctx);
    dvz_gpu_destroy(gpu);
    return 0;
}



static void _gui_callback(DvzLoop* loop, void* user_data)
{
    ANN(loop);

    dvz_gui_dialog_begin("Hello", (vec2){100, 100}, (vec2){400, 400}, 0);
    // dvz_gui_text("Hello");
    dvz_gui_demo();
    dvz_gui_dialog_end();
}

int test_loop_gui(TstSuite* suite)
{
    ANN(suite);
    DvzHost* host = get_host(suite);
    ANN(host);

    DvzGpu* gpu = make_gpu(host);
    ANN(gpu);

    DvzLoop* loop = dvz_loop(gpu, WIDTH, HEIGHT, DVZ_CANVAS_FLAGS_IMGUI);

    dvz_loop_overlay(loop, _gui_callback, NULL);

    dvz_loop_run(loop, N_FRAMES);

    dvz_loop_destroy(loop);
    dvz_gpu_destroy(gpu);
    return 0;
}
