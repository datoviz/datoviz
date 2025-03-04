/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing simple loop                                                                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_loop.h"
#include "_cglm.h"
#include "canvas.h"
#include "datoviz_math.h"
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

int test_loop_1(TstSuite* suite, TstItem* tstitem)
{
    GET_HOST_GPU

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

int test_loop_2(TstSuite* suite, TstItem* tstitem)
{
    GET_HOST_GPU

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
        {{-1, -1, 0}, {ALPHA_MAX, 0, 0, ALPHA_MAX}},
        {{+1, -1, 0}, {0, ALPHA_MAX, 0, ALPHA_MAX}},
        {{+0, +1, 0}, {0, 0, ALPHA_MAX, ALPHA_MAX}},
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

static inline void _load_shader(
    DvzGraphics* graphics, VkShaderStageFlagBits stage, //
    DvzSize size, const unsigned char* buffer)
{
    ANN(graphics);
    ANN(buffer);
    ASSERT(size > 0);
    uint32_t* code = (uint32_t*)calloc(size, 1);
    memcpy(code, buffer, size);
    ASSERT(size % 4 == 0);
    dvz_graphics_shader_spirv(graphics, stage, size, code);
    FREE(code);
}

int test_loop_cube(TstSuite* suite, TstItem* tstitem)
{
    GET_HOST_GPU

    // Context.
    DvzContext* ctx = dvz_context(gpu);
    ANN(ctx);

    // Create the pipelib.
    DvzPipelib* lib = dvz_pipelib(ctx);

    // Create the loop, which creates a canvas and window.
    DvzLoop* loop = dvz_loop(gpu, WIDTH, HEIGHT, 0);
    // DvzCanvas* canvas = &loop->canvas;

    bool use_builtin = false;

    // Create a graphics pipe.
    DvzPipe* pipe = dvz_pipelib_graphics(
        lib, ctx, &loop->renderpass, use_builtin ? DVZ_GRAPHICS_TRIANGLE : DVZ_GRAPHICS_CUSTOM,
        DVZ_PIPELIB_FLAGS_NONE | DVZ_GRAPHICS_FLAGS_DEPTH_TEST);

    DvzGraphics* graphics = &pipe->u.graphics;

    if (!use_builtin)
    {
        // Vertex shader.
        {
            unsigned long size = 0;
            unsigned char* buffer = dvz_resource_shader("graphics_trivial_vert", &size);
            ASSERT(size > 0);
            ANN(buffer);
            _load_shader(graphics, VK_SHADER_STAGE_VERTEX_BIT, size, buffer);
        }

        // Fragment shader.
        {
            unsigned long size = 0;
            unsigned char* buffer = dvz_resource_shader("graphics_trivial_frag", &size);
            ASSERT(size > 0);
            ANN(buffer);
            _load_shader(graphics, VK_SHADER_STAGE_FRAGMENT_BIT, size, buffer);
        }

        // Graphics setting.
        dvz_graphics_renderpass(graphics, &loop->renderpass, 0);
        dvz_graphics_primitive(graphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        dvz_graphics_polygon_mode(graphics, VK_POLYGON_MODE_FILL);
        dvz_graphics_depth_test(graphics, DVZ_DEPTH_TEST_ENABLE);

        // Vertex bindings and attributes.
        dvz_graphics_vertex_binding(graphics, 0, sizeof(DvzVertex), VK_VERTEX_INPUT_RATE_VERTEX);
        dvz_graphics_vertex_attr(
            graphics, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(DvzVertex, pos));
        dvz_graphics_vertex_attr(
            graphics, 0, 1, (VkFormat)DVZ_FORMAT_COLOR, offsetof(DvzVertex, color));

        // Graphics dslots.
        dvz_graphics_slot(graphics, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // MVP
        dvz_graphics_slot(graphics, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // viewport
    }

    // Create a MVP dat manually.
    DvzDat* dat_mvp =
        dvz_dat(ctx, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzMVP), DVZ_DAT_FLAGS_PERSISTENT_STAGING);
    DvzMVP mvp = {0};
    dvz_mvp_default(&mvp);
    dvz_dat_upload(dat_mvp, 0, sizeof(mvp), &mvp, true);
    dvz_pipe_dat(pipe, 0, dat_mvp);

    // Create a viewport dat manually.
    DvzDat* dat_viewport = dvz_dat(ctx, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzViewport), 0);
    DvzViewport viewport = {0};
    dvz_viewport_default(WIDTH, HEIGHT, &viewport);
    dvz_dat_upload(dat_viewport, 0, sizeof(viewport), &viewport, true);
    dvz_pipe_dat(pipe, 1, dat_viewport);

    // NOTE: we now have to create the pipe manually (or automatically when using recorder.c).
    dvz_pipe_create(pipe);

    // Create the vertex buffer dat.
    const uint32_t vertex_count = 36;
    DvzDat* dat_vertex = dvz_dat(ctx, DVZ_BUFFER_TYPE_VERTEX, vertex_count * sizeof(DvzVertex), 0);
    ANN(dat_vertex);
    dvz_pipe_vertex(pipe, 0, dat_vertex, 0);

    // Cube data.
    float x = .5;
    DvzVertex data[] = {
        {{-x, -x, +x}, {ALPHA_MAX, 0, 0, ALPHA_MAX}},         // front
        {{+x, -x, +x}, {ALPHA_MAX, 0, 0, ALPHA_MAX}},         //
        {{+x, +x, +x}, {ALPHA_MAX, 0, 0, ALPHA_MAX}},         //
        {{+x, +x, +x}, {ALPHA_MAX, 0, 0, ALPHA_MAX}},         //
        {{-x, +x, +x}, {ALPHA_MAX, 0, 0, ALPHA_MAX}},         //
        {{-x, -x, +x}, {ALPHA_MAX, 0, 0, ALPHA_MAX}},         //
        {{+x, -x, +x}, {0, ALPHA_MAX, 0, ALPHA_MAX}},         // right
        {{+x, -x, -x}, {0, ALPHA_MAX, 0, ALPHA_MAX}},         //
        {{+x, +x, -x}, {0, ALPHA_MAX, 0, ALPHA_MAX}},         //
        {{+x, +x, -x}, {0, ALPHA_MAX, 0, ALPHA_MAX}},         //
        {{+x, +x, +x}, {0, ALPHA_MAX, 0, ALPHA_MAX}},         //
        {{+x, -x, +x}, {0, ALPHA_MAX, 0, ALPHA_MAX}},         //
        {{-x, +x, -x}, {0, 0, ALPHA_MAX, ALPHA_MAX}},         // back
        {{+x, +x, -x}, {0, 0, ALPHA_MAX, ALPHA_MAX}},         //
        {{+x, -x, -x}, {0, 0, ALPHA_MAX, ALPHA_MAX}},         //
        {{+x, -x, -x}, {0, 0, ALPHA_MAX, ALPHA_MAX}},         //
        {{-x, -x, -x}, {0, 0, ALPHA_MAX, ALPHA_MAX}},         //
        {{-x, +x, -x}, {0, 0, ALPHA_MAX, ALPHA_MAX}},         //
        {{-x, -x, -x}, {0, ALPHA_MAX, ALPHA_MAX, ALPHA_MAX}}, // left
        {{-x, -x, +x}, {0, ALPHA_MAX, ALPHA_MAX, ALPHA_MAX}}, //
        {{-x, +x, +x}, {0, ALPHA_MAX, ALPHA_MAX, ALPHA_MAX}}, //
        {{-x, +x, +x}, {0, ALPHA_MAX, ALPHA_MAX, ALPHA_MAX}}, //
        {{-x, +x, -x}, {0, ALPHA_MAX, ALPHA_MAX, ALPHA_MAX}}, //
        {{-x, -x, -x}, {0, ALPHA_MAX, ALPHA_MAX, ALPHA_MAX}}, //
        {{-x, -x, -x}, {ALPHA_MAX, 0, ALPHA_MAX, ALPHA_MAX}}, // bottom
        {{+x, -x, -x}, {ALPHA_MAX, 0, ALPHA_MAX, ALPHA_MAX}}, //
        {{+x, -x, +x}, {ALPHA_MAX, 0, ALPHA_MAX, ALPHA_MAX}}, //
        {{+x, -x, +x}, {ALPHA_MAX, 0, ALPHA_MAX, ALPHA_MAX}}, //
        {{-x, -x, +x}, {ALPHA_MAX, 0, ALPHA_MAX, ALPHA_MAX}}, //
        {{-x, -x, -x}, {ALPHA_MAX, 0, ALPHA_MAX, ALPHA_MAX}}, //
        {{-x, +x, +x}, {ALPHA_MAX, ALPHA_MAX, 0, ALPHA_MAX}}, // top
        {{+x, +x, +x}, {ALPHA_MAX, ALPHA_MAX, 0, ALPHA_MAX}}, //
        {{+x, +x, -x}, {ALPHA_MAX, ALPHA_MAX, 0, ALPHA_MAX}}, //
        {{+x, +x, -x}, {ALPHA_MAX, ALPHA_MAX, 0, ALPHA_MAX}}, //
        {{-x, +x, -x}, {ALPHA_MAX, ALPHA_MAX, 0, ALPHA_MAX}}, //
        {{-x, +x, +x}, {ALPHA_MAX, ALPHA_MAX, 0, ALPHA_MAX}}, //
    };

    // Upload the vertex data.
    dvz_dat_upload(dat_vertex, 0, sizeof(data), data, true);

    // Pass some information to the refill callbacK.
    TestLoopStruct s = {.pipe = pipe, .br = dat_vertex->br};
    dvz_loop_refill(loop, _fill_cube, &s);

    // View matrix.
    glm_lookat((vec3){0, 0, 4}, (vec3){0}, (vec3){0, 1, 0}, mvp.view);

    // Projection matrix.
    glm_perspective(GLM_PI_4, WIDTH / (float)HEIGHT, .1, 100, mvp.proj);

    // Run the loop.
    for (loop->frame_idx = 0; loop->frame_idx < (DEBUG_TEST ? UINT64_MAX : 5); loop->frame_idx++)
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

    dvz_gui_pos((vec2){100, 100}, DVZ_DIALOG_DEFAULT_PIVOT);
    dvz_gui_size((vec2){400, 400});
    dvz_gui_begin("Hello", 0);
    // dvz_gui_text("Hello");
    dvz_gui_demo();
    dvz_gui_end();
}

int test_loop_gui(TstSuite* suite, TstItem* tstitem)
{
    GET_HOST_GPU

    DvzLoop* loop = dvz_loop(gpu, WIDTH, HEIGHT, DVZ_CANVAS_FLAGS_IMGUI);

    dvz_loop_overlay(loop, _gui_callback, NULL);

    dvz_loop_run(loop, N_FRAMES);

    dvz_loop_destroy(loop);
    dvz_gpu_destroy(gpu);
    return 0;
}
