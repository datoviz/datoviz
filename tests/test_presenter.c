/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing presenter                                                                            */
/*  NOTE: resizing is not implemented in these tests except in the second one                    */
/*  Proper resizing support requires registering a RESIZE callback and emitting new requests,    */
/*  notably record requests.                                                                     */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_presenter.h"
#include "_time_utils.h"
#include "backend.h"
#include "client.h"
#include "client_input.h"
#include "datoviz.h"
#include "fps.h"
#include "gui.h"
#include "presenter.h"
#include "scene/arcball.h"
#include "scene/camera.h"
#include "scene/panzoom.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"
#include "timer.h"


/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define BACKEND DVZ_BACKEND_GLFW
#define WIDTH   800
#define HEIGHT  600



/*************************************************************************************************/
/*  Test utils                                                                                   */
/*************************************************************************************************/

typedef struct CallbackStruct CallbackStruct;
struct CallbackStruct
{
    DvzPresenter* prt;
    // DvzBatch* batch;
    DvzId canvas_id;
    DvzId mvp_id;
    DvzId viewport_id;
    DvzId dat_id;
    DvzId graphics_id;
    GraphicsWrapper* graphics_wrapper;
    uint32_t n;
};



typedef struct TexStruct TexStruct;
struct TexStruct
{
    // DvzBatch* batch;
    DvzPresenter* prt;

    uint32_t width;
    DvzId tex_id;
    cvec4* tex_data;
};



typedef struct PanzoomStruct PanzoomStruct;
struct PanzoomStruct
{
    // DvzBatch* batch;
    DvzPresenter* prt;
    DvzId mvp_id;
    DvzMVP mvp;
    DvzPanzoom* pz;
};



typedef struct ArcballStruct ArcballStruct;
struct ArcballStruct
{
    // DvzBatch* batch;
    DvzPresenter* prt;
    DvzId mvp_id;
    DvzMVP mvp;
    DvzArcball* arcball;
    DvzCamera* cam;
};



/*************************************************************************************************/
/*  Presenter tests                                                                              */
/*************************************************************************************************/

int test_presenter_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    // GPU-side.
    DvzHost* host = get_host(suite);

    DvzGpu* gpu = make_gpu(host);
    ANN(gpu);

    // Create a renderer.
    DvzRenderer* rd = dvz_renderer(gpu, 0);

    // Client-side.
    DvzClient* client = dvz_client(BACKEND);
    DvzBatch* batch = dvz_batch();

    // NOTE: we need to manually begin recording the requester, otherwise requests won't be
    // automatically recorded in the requester batch.
    // dvz_requester_begin(batch);


    // Presenter linking the renderer and the client.
    DvzPresenter* prt = dvz_presenter(rd, client, 0);

    // Start.

    // Make a canvas creation request.
    dvz_create_canvas(batch, WIDTH, HEIGHT, DVZ_DEFAULT_CLEAR_COLOR, 0);

    // Submit a client event with type REQUESTS and with a pointer to the requester.
    // The Presenter will register a REQUESTS callback sending the requests to the underlying
    // renderer.
    dvz_presenter_submit(prt, batch);

    // Dequeue and process all pending events.
    dvz_client_run(client, N_FRAMES);

    // NOTE: resizing support is not implemented in this example.

    // End.
    dvz_client_destroy(client);
    dvz_presenter_destroy(prt);
    // dvz_batch_destroy(batch);
    dvz_renderer_destroy(rd);
    dvz_gpu_destroy(gpu);

    return 0;
}



static void _callback_resize(DvzClient* client, DvzClientEvent ev)
{
    ANN(client);

    uint32_t width = ev.content.w.screen_width;
    uint32_t height = ev.content.w.screen_height;
    log_info("window 0x%" PRIx64 " resized to %dx%d", ev.window_id, width, height);

    CallbackStruct* s = (CallbackStruct*)ev.user_data;
    ANN(s);

    // This batch will be destroyed automatically in the event loop by the presenter.
    DvzBatch* batch = dvz_batch();
    ANN(batch);

    // Submit new recording commands to the client.
    dvz_record_begin(batch, s->canvas_id);
    dvz_record_viewport(batch, s->canvas_id, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
    dvz_record_draw(batch, s->canvas_id, s->graphics_id, 0, 3, 0, 1);
    dvz_record_end(batch, s->canvas_id);
    dvz_presenter_submit(s->prt, batch);
}

int test_presenter_2(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    // GPU-side.
    DvzHost* host = get_host(suite);

    DvzGpu* gpu = make_gpu(host);
    ANN(gpu);

    // Create a renderer.
    DvzRenderer* rd = dvz_renderer(gpu, 0);

    // Client-side.
    DvzClient* client = dvz_client(BACKEND);
    DvzBatch* batch = dvz_batch();
    DvzRequest req = {0};

    // Presenter linking the renderer and the client.
    DvzPresenter* prt = dvz_presenter(rd, client, 0);

    // Make rendering requests.

    DvzId canvas_id, graphics_id, dat_id, mvp_id, viewport_id;
    {
        // Make a canvas creation request.
        req = dvz_create_canvas(batch, WIDTH, HEIGHT, DVZ_DEFAULT_CLEAR_COLOR, 0);
        canvas_id = req.id;

        // Create a graphics.
        req = dvz_create_graphics(batch, DVZ_GRAPHICS_TRIANGLE, 0);
        graphics_id = req.id;

        dvz_set_push(
            batch, graphics_id, DVZ_SHADER_VERTEX | DVZ_SHADER_FRAGMENT, 0, sizeof(float));

        // Create the vertex buffer dat.
        req = dvz_create_dat(batch, DVZ_BUFFER_TYPE_VERTEX, 3 * sizeof(DvzVertex), 0);
        dat_id = req.id;

        // Bind the vertex buffer dat to the graphics pipe.
        req = dvz_bind_vertex(batch, graphics_id, 0, dat_id, 0);

        // Upload the triangle data.
        DvzVertex data[] = {
            {{-1, -1, 0}, {255, 0, 0, 255}},
            {{+1, -1, 0}, {0, 255, 0, 255}},
            {{+0, +1, 0}, {0, 0, 255, 255}},
        };
        req = dvz_upload_dat(batch, dat_id, 0, sizeof(data), data, 0);

        // Binding #0: MVP.
        req = dvz_create_dat(
            batch, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzMVP), DVZ_DAT_FLAGS_PERSISTENT_STAGING);
        mvp_id = req.id;
        req = dvz_bind_dat(batch, graphics_id, 0, mvp_id, 0);

        DvzMVP mvp = {0};
        dvz_mvp_default(&mvp);
        // dvz_show_base64(sizeof(mvp), &mvp);
        req = dvz_upload_dat(batch, mvp_id, 0, sizeof(DvzMVP), &mvp, 0);

        // Binding #1: viewport.
        req = dvz_create_dat(batch, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzViewport), 0);
        viewport_id = req.id;
        req = dvz_bind_dat(batch, graphics_id, 1, viewport_id, 0);

        DvzViewport viewport = {0};
        dvz_viewport_default(WIDTH, HEIGHT, &viewport);
        // dvz_show_base64(sizeof(viewport), &viewport);
        req = dvz_upload_dat(batch, viewport_id, 0, sizeof(DvzViewport), &viewport, 0);

        // Command buffer.
        req = dvz_record_begin(batch, canvas_id);
        req = dvz_record_viewport(batch, canvas_id, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
        req = dvz_record_push(
            batch, canvas_id, graphics_id, DVZ_SHADER_VERTEX | DVZ_SHADER_FRAGMENT, 0,
            sizeof(float), (float[]){1});
        req = dvz_record_draw(batch, canvas_id, graphics_id, 0, 3, 0, 1);
        req = dvz_record_end(batch, canvas_id);
    }

    // Resize callback.
    CallbackStruct s = {
        .prt = prt,
        .canvas_id = canvas_id,
        .graphics_id = graphics_id,
        .mvp_id = mvp_id,
        .viewport_id = viewport_id,
        .dat_id = dat_id};
    dvz_client_callback(
        client, DVZ_CLIENT_EVENT_WINDOW_RESIZE, DVZ_CLIENT_CALLBACK_SYNC, _callback_resize, &s);

    // Submit a client event with type REQUESTS and with a pointer to the requester.
    // The Presenter will register a REQUESTS callback sending the requests to the underlying
    // renderer.
    dvz_presenter_submit(prt, batch);

    // Dequeue and process all pending events.
    dvz_client_run(client, N_FRAMES);

    // End.
    dvz_client_destroy(client);
    dvz_presenter_destroy(prt);
    // dvz_batch_destroy(batch);
    dvz_renderer_destroy(rd);
    dvz_gpu_destroy(gpu);

    return 0;
}



// static unsigned int g_seed;

// static inline void fast_srand(int seed) { g_seed = seed; }

// static inline int fast_rand(void)
// {
//     g_seed = (214013 * g_seed + 2531011);
//     return (g_seed >> 16) & 0x7FFF;
// }

static void _random_data(uint32_t n, DvzGraphicsPointVertex* data)
{
    ASSERT(n > 0);
    ANN(data);

    for (uint32_t i = 0; i < n; i++)
    {

        data[i].pos[0] = -1 + 2 * dvz_rand_float(); // 6.1e-5 * fast_rand();
        data[i].pos[1] = -1 + 2 * dvz_rand_float(); // 6.1e-5 * fast_rand();
        data[i].size = 5;
        dvz_colormap(DVZ_CMAP_HSV, i % 256, data[i].color);
        data[i].color[3] = ALPHA_MAX;
    }
}

static void _on_click(DvzClient* client, DvzClientEvent ev)
{
    ANN(client);
    if (ev.content.m.type != DVZ_MOUSE_EVENT_CLICK)
        return;

    CallbackStruct* s = (CallbackStruct*)ev.user_data;
    ANN(s);

    // This batch will be destroyed automatically in the event loop by the presenter.
    DvzBatch* batch = dvz_batch();
    ANN(batch);

    DvzPresenter* prt = s->prt;
    ANN(prt);

    GraphicsWrapper* wrapper = s->graphics_wrapper;
    ANN(wrapper);

    // Update the data.
    _random_data(s->n, (DvzGraphicsPointVertex*)wrapper->data);
    dvz_upload_dat(
        batch, wrapper->dat_id, 0, s->n * sizeof(DvzGraphicsPointVertex), wrapper->data, 0);

    // // Update the command buffer with the new n.
    // req = dvz_record_begin(batch, wrapper->canvas_id);
    //
    // req = dvz_record_viewport(batch, wrapper->canvas_id, DVZ_DEFAULT_VIEWPORT,
    // DVZ_DEFAULT_VIEWPORT);
    // req = dvz_record_draw(batch, wrapper->canvas_id, wrapper->graphics_id, 0, s->n);
    //
    // req = dvz_record_end(batch, wrapper->canvas_id);
    //
    dvz_presenter_submit(prt, batch);
}

int test_presenter_thread(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    // GPU-side.
    DvzHost* host = get_host(suite);

    DvzGpu* gpu = make_gpu(host);
    ANN(gpu);

    // Create a renderer.
    DvzRenderer* rd = dvz_renderer(gpu, 0);

    // Client-side.
    DvzClient* client = dvz_client(BACKEND);
    DvzBatch* batch = dvz_batch();

    // NOTE: we need to manually begin recording the requester, otherwise requests won't be
    // automatically recorded in the requester batch.
    // dvz_requester_begin(batch);

    // Presenter linking the renderer and the client.
    DvzPresenter* prt = dvz_presenter(rd, client, 0);

    const uint32_t n = 256;
    GraphicsWrapper wrapper = {0};
    graphics_request(batch, n, &wrapper, 0);
    wrapper.data = calloc(n, sizeof(DvzGraphicsPointVertex));
    _random_data(n, (DvzGraphicsPointVertex*)wrapper.data);

    dvz_upload_dat(batch, wrapper.dat_id, 0, n * sizeof(DvzGraphicsPointVertex), wrapper.data, 0);

    // Submit a client event with type REQUESTS and with a pointer to the requester.
    // The Presenter will register a REQUESTS callback sending the requests to the underlying
    // renderer.
    dvz_presenter_submit(prt, batch);

    CallbackStruct s = {
        .prt = prt,
        .graphics_wrapper = &wrapper,
        .n = n,
    };
    dvz_client_callback(client, DVZ_CLIENT_EVENT_MOUSE, DVZ_CLIENT_CALLBACK_SYNC, _on_click, &s);

#if OS_MACOS
    dvz_client_run(client, N_FRAMES);
#else
    // Start the client background thread and run an infinite event loop in the thread.
    dvz_client_thread(client, N_FRAMES);

    // Stop the event loop in the thread.
    // dvz_sleep(100);
    // dvz_client_stop(client);

    // Wait until the client event loop is done.
    dvz_client_join(client);
#endif

    // End.
    dvz_client_destroy(client);
    dvz_presenter_destroy(prt);
    // dvz_batch_destroy(batch);
    dvz_renderer_destroy(rd);
    dvz_gpu_destroy(gpu);

    FREE(wrapper.data);
    return 0;
}



static void _on_frame(DvzClient* client, DvzClientEvent ev)
{
    ANN(client);

    DvzTimer* timer = (DvzTimer*)ev.user_data;
    ANN(timer);

    // The timer callbacks are called here.
    dvz_timer_tick(timer, ev.content.f.time);

    log_info("interval %.6f s", ev.content.f.interval);
}

static void _timer_callback(DvzTimer* timer, DvzInternalTimerEvent ev)
{
    ANN(timer);
    ANN(ev.item);

    DvzClient* client = (DvzClient*)ev.user_data;
    ANN(client);

    // Emit a client TIMER event.
    DvzClientEvent cev = {
        .type = DVZ_CLIENT_EVENT_TIMER,
        .content =
            {
                .t =
                    {
                        .timer_item = ev.item,
                        .timer_idx = ev.item->timer_idx,
                        .time = ev.time,
                        .step_idx = ev.item->count,
                    },
            },
    };
    dvz_client_event(client, cev);
}

static void _client_timer_callback(DvzClient* client, DvzClientEvent ev)
{
    ANN(client);

    GraphicsWrapper* wrapper = (GraphicsWrapper*)ev.user_data;
    ANN(wrapper);
    ANN(wrapper->prt);
    ANN(wrapper->data);
    ASSERT(wrapper->dat_id != DVZ_ID_NONE);
    ASSERT(wrapper->n > 0);

    log_info("timer event");

    // This batch will be destroyed automatically in the event loop by the presenter.
    DvzBatch* batch = dvz_batch();
    ANN(batch);

    for (uint32_t i = 0; i < 10; i++)
    {
        _random_data(wrapper->n, (DvzGraphicsPointVertex*)wrapper->data);
        dvz_upload_dat(
            batch, wrapper->dat_id, 0, //
            wrapper->n * sizeof(DvzGraphicsPointVertex), wrapper->data, DVZ_UPLOAD_FLAGS_NOCOPY);
    }
    dvz_presenter_submit(wrapper->prt, batch);
}

int test_presenter_updates(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    // GPU-side.
    DvzHost* host = get_host(suite);

    DvzGpu* gpu = make_gpu(host);
    ANN(gpu);

    // Create a renderer.
    DvzRenderer* rd = dvz_renderer(gpu, 0);

    // Client-side.
    DvzClient* client = dvz_client(BACKEND);
    DvzBatch* batch = dvz_batch();

    // Presenter linking the renderer and the client.
    DvzPresenter* prt = dvz_presenter(rd, client, 0);

    // Batch.
    const uint32_t n = 1024 * 64;
    GraphicsWrapper wrapper = {0};
    wrapper.prt = prt;
    graphics_request(batch, n, &wrapper, 0);
    wrapper.data = calloc(n, sizeof(DvzGraphicsPointVertex));
    _random_data(n, (DvzGraphicsPointVertex*)wrapper.data);
    dvz_upload_dat(batch, wrapper.dat_id, 0, n * sizeof(DvzGraphicsPointVertex), wrapper.data, 0);
    dvz_presenter_submit(prt, batch);


    // Timer.
    DvzTimer* timer = dvz_timer();
    ANN(timer);

    DvzTimerItem* item = dvz_timer_new(timer, 0, .1, 0);

    // Calls timer tick at every frame.
    dvz_client_callback(
        client, DVZ_CLIENT_EVENT_FRAME, DVZ_CLIENT_CALLBACK_SYNC, _on_frame, timer);

    // Push a regular timer event.
    dvz_timer_callback(timer, item, _timer_callback, client);

    // React to the timer event.
    dvz_client_callback(
        client, DVZ_CLIENT_EVENT_TIMER, DVZ_CLIENT_CALLBACK_SYNC, _client_timer_callback,
        &wrapper);


    // Event loop.
    dvz_client_run(client, N_FRAMES);


    // End.
    dvz_client_destroy(client);
    dvz_presenter_destroy(prt);
    // dvz_batch_destroy(batch);
    dvz_renderer_destroy(rd);
    dvz_gpu_destroy(gpu);

    // FREE(wrapper.data);
    return 0;
}



int test_presenter_deserialize(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    // GPU-side.
    DvzHost* host = get_host(suite);

    DvzGpu* gpu = make_gpu(host);
    ANN(gpu);

    // Create a renderer.
    DvzRenderer* rd = dvz_renderer(gpu, 0);

    // Client-side.
    DvzClient* client = dvz_client(BACKEND);
    DvzBatch* batch = dvz_batch();

    // NOTE: we need to manually begin recording the requester, otherwise requests won't be
    // automatically recorded in the requester batch.
    // dvz_requester_begin(batch);

    // Presenter linking the renderer and the client.
    DvzPresenter* prt = dvz_presenter(rd, client, 0);

    // Load the requests from requests.dvz.
    // dvz_requester_load(batch, DVZ_DUMP_FILENAME);

    // Submit a client event with type REQUESTS and with a pointer to the requester.
    // The Presenter will register a REQUESTS callback sending the requests to the underlying
    // renderer.
    dvz_presenter_submit(prt, batch);

    // Dequeue and process all pending events.
    dvz_client_run(client, N_FRAMES);

    // End.
    dvz_client_destroy(client);
    dvz_presenter_destroy(prt);
    // dvz_batch_destroy(batch);
    dvz_renderer_destroy(rd);
    dvz_gpu_destroy(gpu);

    return 0;
}



static inline void _gui_callback_1(DvzGuiWindow* gui_window, void* user_data)
{

    dvz_gui_pos((vec2){100, 100}, DVZ_DIALOG_DEFAULT_PIVOT);
    dvz_gui_size((vec2){400, 200});
    dvz_gui_begin("Window title", 0);
    dvz_gui_text("Hello world");

    // NOTE: ImGui code can be called but need C++, unless one uses cimgui and builds it along
    // the executable.

    // dvz_gui_demo();

    TexStruct* tex_struct = (TexStruct*)user_data;
    ANN(tex_struct);

    DvzPresenter* prt = tex_struct->prt;
    ANN(prt);

    // This batch will be destroyed automatically in the event loop by the presenter.
    DvzBatch* batch = dvz_batch();
    ANN(batch);

    DvzId tex_id = tex_struct->tex_id;
    ASSERT(tex_id != 0);

    uint32_t width = tex_struct->width;
    ASSERT(width > 0);

    cvec4* tex_data = tex_struct->tex_data;
    ANN(tex_data);

    // Elapsed time.
    double t = dvz_clock_get(&prt->client->clock);

    // Update the texture data.
    for (uint32_t i = 0; i < width; i++)
        dvz_colormap_8bit(
            DVZ_CMAP_HSV, (i - ((uint32_t)round(t * 128) % width)) * 256 / width, tex_data[i]);

    // Upload the texture data.
    dvz_upload_tex(
        batch, tex_id, DVZ_ZERO_OFFSET, (uvec3){width, 1, 1}, width * sizeof(cvec4), tex_data, 0);

    // NOTE: this call needs to be explicit when using the presenter API. The app API automatically
    // calls it so the user doesn't need it.
    dvz_presenter_submit(prt, batch);

    // Display the texture as an image in the GUI.
    DvzTex* tex = dvz_renderer_tex(tex_struct->prt->rd, tex_struct->tex_id);
    ANN(tex);
    dvz_gui_image(tex, 300, 50);

    dvz_gui_end();
}

int test_presenter_gui(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    // GPU-side.
    DvzHost* host = get_host(suite);

    DvzGpu* gpu = make_gpu(host);
    ANN(gpu);

    // Create a renderer.
    DvzRenderer* rd = dvz_renderer(gpu, 0);

    // Client-side.
    DvzClient* client = dvz_client(BACKEND);
    DvzBatch* batch = dvz_batch();
    DvzRequest req = {0};

    // NOTE: we need to manually begin recording the requester, otherwise requests won't be
    // automatically recorded in the requester batch.
    // dvz_requester_begin(batch);

    // Presenter linking the renderer and the client.
    DvzPresenter* prt = dvz_presenter(rd, client, DVZ_CANVAS_FLAGS_IMGUI);

    // Start.

    // Make a canvas creation request.
    req = dvz_create_canvas(batch, WIDTH, HEIGHT, DVZ_DEFAULT_CLEAR_COLOR, DVZ_CANVAS_FLAGS_IMGUI);
    DvzId canvas_id = req.id;

    // Texture.
    const uint32_t width = 256, height = 1;
    req = dvz_create_tex(
        batch, DVZ_TEX_2D, DVZ_FORMAT_R8G8B8A8_UNORM, (uvec3){width, height, 1},
        DVZ_TEX_FLAGS_PERSISTENT_STAGING);
    DvzId tex_id = req.id;

    // Texture data.
    cvec4* tex_data = (cvec4*)calloc(width * height, sizeof(cvec4));
    for (uint32_t i = 0; i < width; i++)
        dvz_colormap_8bit(DVZ_CMAP_HSV, i * 256 / width, tex_data[i]);
    dvz_upload_tex(
        batch, tex_id, DVZ_ZERO_OFFSET, (uvec3){width, 1, 1}, width * sizeof(cvec4), tex_data, 0);

    // Texture struct.
    TexStruct tex_struct = {
        .prt = prt,
        .width = width,
        .tex_id = tex_id,
        .tex_data = tex_data,
    };

    // Submit a client event with type REQUESTS and with a pointer to the requester.
    // The Presenter will register a REQUESTS callback sending the requests to the underlying
    // renderer.
    dvz_presenter_submit(prt, batch);

    // GUI callback.
    dvz_presenter_gui(prt, canvas_id, _gui_callback_1, &tex_struct);

    // Timestamps.
    dvz_client_run(client, 50);

    uint64_t seconds[10] = {0};
    uint64_t nanoseconds[10] = {0};
    dvz_canvas_timestamps(dvz_renderer_canvas(rd, canvas_id), 10, &seconds[0], &nanoseconds[0]);
    for (uint32_t i = 0; i < 10; i++)
    {
        printf("%" PRIu64 " s %09" PRIu64 " ns\n", seconds[i], nanoseconds[i]);
        // dvz_time_print((DvzTime[]){{seconds[i], nanoseconds[i]}});
    }

    // Dequeue and process all pending events.
    dvz_client_run(client, N_FRAMES);

    // End.
    dvz_client_destroy(client);
    dvz_presenter_destroy(prt);
    // dvz_batch_destroy(batch);
    dvz_renderer_destroy(rd);
    dvz_gpu_destroy(gpu);

    FREE(tex_data);

    return 0;
}



int test_presenter_multi(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    // GPU-side.
    DvzHost* host = get_host(suite);

    DvzGpu* gpu = make_gpu(host);
    ANN(gpu);

    // Create a renderer.
    DvzRenderer* rd = dvz_renderer(gpu, 0);

    // Client-side.
    DvzClient* client = dvz_client(BACKEND);
    DvzBatch* batch = dvz_batch();
    DvzRequest req = {0};

    // NOTE: we need to manually begin recording the requester, otherwise requests won't be
    // automatically recorded in the requester batch.
    // dvz_requester_begin(batch);

    // Presenter linking the renderer and the client.
    DvzPresenter* prt = dvz_presenter(rd, client, 0);

    // Make rendering requests.

    DvzId canvas_id_0, canvas_id_1, graphics_id_0, graphics_id_1, dat_id, mvp_id, viewport_id;
    {
        // Canvas #0.

        // Make a canvas creation request.
        req = dvz_create_canvas(batch, WIDTH, HEIGHT, DVZ_DEFAULT_CLEAR_COLOR, 0);
        canvas_id_0 = req.id;

        req = dvz_create_graphics(batch, DVZ_GRAPHICS_TRIANGLE, 0);
        graphics_id_0 = req.id;


        // Canvas #1.

        // Make a canvas creation request.
        req = dvz_create_canvas(batch, WIDTH / 2, HEIGHT / 2, DVZ_DEFAULT_CLEAR_COLOR, 0);
        canvas_id_1 = req.id;

        req = dvz_create_graphics(batch, DVZ_GRAPHICS_TRIANGLE, 0);
        graphics_id_1 = req.id;


        // Create the vertex buffer dat.
        req = dvz_create_dat(batch, DVZ_BUFFER_TYPE_VERTEX, 3 * sizeof(DvzVertex), 0);
        dat_id = req.id;

        // Bind the vertex buffer dat to the graphics pipe.
        req = dvz_bind_vertex(batch, graphics_id_0, 0, dat_id, 0);
        req = dvz_bind_vertex(batch, graphics_id_1, 0, dat_id, 0);

        // Upload the triangle data.
        DvzVertex data[] = {
            {{-1, -1, 0}, {255, 0, 0, 255}},
            {{+1, -1, 0}, {0, 255, 0, 255}},
            {{+0, +1, 0}, {0, 0, 255, 255}},
        };
        req = dvz_upload_dat(batch, dat_id, 0, sizeof(data), data, 0);

        // Binding #0: MVP.
        req = dvz_create_dat(batch, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzMVP), 0);
        mvp_id = req.id;

        req = dvz_bind_dat(batch, graphics_id_0, 0, mvp_id, 0);
        req = dvz_bind_dat(batch, graphics_id_1, 0, mvp_id, 0);

        DvzMVP mvp = {0};
        dvz_mvp_default(&mvp);
        // dvz_show_base64(sizeof(mvp), &mvp);
        req = dvz_upload_dat(batch, mvp_id, 0, sizeof(DvzMVP), &mvp, 0);

        // Binding #1: viewport.
        req = dvz_create_dat(batch, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzViewport), 0);
        viewport_id = req.id;

        dvz_bind_dat(batch, graphics_id_0, 1, viewport_id, 0);
        dvz_bind_dat(batch, graphics_id_1, 1, viewport_id, 0);

        DvzViewport viewport = {0};
        dvz_viewport_default(WIDTH / 2, HEIGHT / 2, &viewport);
        // dvz_show_base64(sizeof(viewport), &viewport);
        dvz_upload_dat(batch, viewport_id, 0, sizeof(DvzViewport), &viewport, 0);


        // Command buffer.
        dvz_record_begin(batch, canvas_id_0);
        dvz_record_viewport(batch, canvas_id_0, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
        dvz_record_draw(batch, canvas_id_0, graphics_id_0, 0, 3, 0, 1);
        dvz_record_end(batch, canvas_id_0);
        dvz_record_begin(batch, canvas_id_1);
        dvz_record_viewport(batch, canvas_id_1, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
        dvz_record_draw(batch, canvas_id_1, graphics_id_1, 0, 3, 0, 1);
        dvz_record_end(batch, canvas_id_1);
    }

    // Submit a client event with type REQUESTS and with a pointer to the requester.
    // The Presenter will register a REQUESTS callback sending the requests to the underlying
    // renderer.
    dvz_presenter_submit(prt, batch);

    // Dequeue and process all pending events.
    dvz_client_run(client, N_FRAMES);

    // End.
    dvz_client_destroy(client);
    dvz_presenter_destroy(prt);
    // dvz_batch_destroy(batch);
    dvz_renderer_destroy(rd);
    dvz_gpu_destroy(gpu);

    return 0;
}
