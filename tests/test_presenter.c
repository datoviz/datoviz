/*************************************************************************************************/
/*  Testing presenter */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_presenter.h"
#include "client.h"
#include "glfw_utils.h"
#include "gui.h"
#include "presenter.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"


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
    DvzRequester* rqr;
    DvzId canvas_id;
    DvzId mvp_id;
    DvzId viewport_id;
    DvzId dat_id;
    DvzId graphics_id;
};



/*************************************************************************************************/
/*  Presenter tests                                                                              */
/*************************************************************************************************/

int test_presenter_1(TstSuite* suite)
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
    DvzRequester* rqr = dvz_requester();
    DvzRequest req = {0};

    // Presenter linking the renderer and the client.
    DvzPresenter* prt = dvz_presenter(rd, client, 0);

    // Start.

    // Make a canvas creation request.
    req = dvz_create_canvas(rqr, WIDTH, HEIGHT, DVZ_DEFAULT_CLEAR_COLOR, 0);
    dvz_requester_add(rqr, req);

    // Submit a client event with type REQUESTS and with a pointer to the requester.
    // The Presenter will register a REQUESTS callback sending the requests to the underlying
    // renderer.
    dvz_presenter_submit(prt, rqr);

    // Dequeue and process all pending events.
    dvz_client_run(client, N_FRAMES);

    // End.


    // Destroying all objects.
    dvz_client_destroy(client);
    dvz_requester_destroy(rqr);

    dvz_renderer_destroy(rd);
    dvz_presenter_destroy(prt);
    dvz_gpu_destroy(gpu);

    return 0;
}



static void _callback_resize(DvzClient* client, DvzClientEvent ev)
{
    ANN(client);

    uint32_t width = ev.content.w.screen_width;
    uint32_t height = ev.content.w.screen_height;
    log_info("window %x resized to %dx%d", ev.window_id, width, height);

    CallbackStruct* s = (CallbackStruct*)ev.user_data;
    ANN(s);

    DvzRequester* rqr = s->rqr;
    ANN(rqr);

    // Submit new recording commands to the client.
    DvzRequest req = {0};
    req = dvz_record_begin(rqr, s->canvas_id);
    dvz_requester_add(rqr, req);

    req = dvz_record_viewport(rqr, s->canvas_id, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
    dvz_requester_add(rqr, req);

    req = dvz_record_draw(rqr, s->canvas_id, s->graphics_id, 0, 3);
    dvz_requester_add(rqr, req);

    req = dvz_record_end(rqr, s->canvas_id);
    dvz_requester_add(rqr, req);

    dvz_presenter_submit(s->prt, rqr);
}

int test_presenter_2(TstSuite* suite)
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
    DvzRequester* rqr = dvz_requester();
    DvzRequest req = {0};

    // Presenter linking the renderer and the client.
    DvzPresenter* prt = dvz_presenter(rd, client, 0);

    // Make rendering requests.

    DvzId canvas_id, graphics_id, dat_id, mvp_id, viewport_id;
    {
        // Make a canvas creation request.
        req = dvz_create_canvas(rqr, WIDTH, HEIGHT, DVZ_DEFAULT_CLEAR_COLOR, 0);
        dvz_requester_add(rqr, req);

        // Canvas id.
        canvas_id = req.id;

        // Create a graphics.
        req = dvz_create_graphics(rqr, DVZ_GRAPHICS_TRIANGLE, 0);
        dvz_requester_add(rqr, req);
        graphics_id = req.id;

        // Create the vertex buffer dat.
        req = dvz_create_dat(rqr, DVZ_BUFFER_TYPE_VERTEX, 3 * sizeof(DvzVertex), 0);
        dvz_requester_add(rqr, req);
        dat_id = req.id;

        // Bind the vertex buffer dat to the graphics pipe.
        req = dvz_set_vertex(rqr, graphics_id, dat_id);
        dvz_requester_add(rqr, req);

        // Upload the triangle data.
        DvzVertex data[] = {
            {{-1, -1, 0}, {255, 0, 0, 255}},
            {{+1, -1, 0}, {0, 255, 0, 255}},
            {{+0, +1, 0}, {0, 0, 255, 255}},
        };
        req = dvz_upload_dat(rqr, dat_id, 0, sizeof(data), data);
        dvz_requester_add(rqr, req);

        // Binding #0: MVP.
        req = dvz_create_dat(rqr, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzMVP), 0);
        dvz_requester_add(rqr, req);
        mvp_id = req.id;

        req = dvz_bind_dat(rqr, graphics_id, 0, mvp_id);
        dvz_requester_add(rqr, req);

        DvzMVP mvp = dvz_mvp_default();
        // dvz_show_base64(sizeof(mvp), &mvp);
        req = dvz_upload_dat(rqr, mvp_id, 0, sizeof(DvzMVP), &mvp);
        dvz_requester_add(rqr, req);

        // Binding #1: viewport.
        req = dvz_create_dat(rqr, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzViewport), 0);
        dvz_requester_add(rqr, req);
        viewport_id = req.id;

        req = dvz_bind_dat(rqr, graphics_id, 1, viewport_id);
        dvz_requester_add(rqr, req);

        DvzViewport viewport = dvz_viewport_default(WIDTH, HEIGHT);
        // dvz_show_base64(sizeof(viewport), &viewport);
        req = dvz_upload_dat(rqr, viewport_id, 0, sizeof(DvzViewport), &viewport);
        dvz_requester_add(rqr, req);


        // Command buffer.
        req = dvz_record_begin(rqr, canvas_id);
        dvz_requester_add(rqr, req);

        req = dvz_record_viewport(rqr, canvas_id, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
        dvz_requester_add(rqr, req);

        req = dvz_record_draw(rqr, canvas_id, graphics_id, 0, 3);
        dvz_requester_add(rqr, req);

        req = dvz_record_end(rqr, canvas_id);
        dvz_requester_add(rqr, req);
    }

    // Resize callback.
    CallbackStruct s = {
        .prt = prt,
        .rqr = rqr,
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
    dvz_presenter_submit(prt, rqr);

    // Dequeue and process all pending events.
    dvz_client_run(client, N_FRAMES);

    // End.


    // Destroying all objects.
    dvz_client_destroy(client);
    dvz_requester_destroy(rqr);

    dvz_renderer_destroy(rd);
    dvz_presenter_destroy(prt);
    dvz_gpu_destroy(gpu);

    return 0;
}



static inline void _gui_callback_1(DvzGuiWindow* gui_window, void* user_data)
{
    dvz_gui_dialog_begin((vec2){100, 100}, (vec2){200, 200});
    dvz_gui_text("Hello world");
    // NOTE: ImGui code can be called but need C++, unless one uses cimgui and builds it along
    // the executable.
    dvz_gui_dialog_end();
}

int test_presenter_gui(TstSuite* suite)
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
    DvzRequester* rqr = dvz_requester();
    DvzRequest req = {0};

    // Presenter linking the renderer and the client.
    DvzPresenter* prt = dvz_presenter(rd, client, DVZ_CANVAS_FLAGS_IMGUI);

    // Start.

    // Make a canvas creation request.
    req = dvz_create_canvas(rqr, WIDTH, HEIGHT, DVZ_DEFAULT_CLEAR_COLOR, DVZ_CANVAS_FLAGS_IMGUI);
    dvz_requester_add(rqr, req);
    // DvzId canvas_id = req.id;

    // Submit a client event with type REQUESTS and with a pointer to the requester.
    // The Presenter will register a REQUESTS callback sending the requests to the underlying
    // renderer.
    dvz_presenter_submit(prt, rqr);

    // GUI callback.
    dvz_presenter_gui(prt, req.id, _gui_callback_1, NULL);

    // Dequeue and process all pending events.
    dvz_client_run(client, N_FRAMES);

    // End.


    // Destroying all objects.
    dvz_requester_destroy(rqr);
    dvz_renderer_destroy(rd);
    dvz_presenter_destroy(prt);
    dvz_gpu_destroy(gpu);
    dvz_client_destroy(client);
    return 0;
}



int test_presenter_multi(TstSuite* suite)
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
    DvzRequester* rqr = dvz_requester();
    DvzRequest req = {0};

    // Presenter linking the renderer and the client.
    DvzPresenter* prt = dvz_presenter(rd, client, 0);

    // Make rendering requests.

    DvzId canvas_id_0, canvas_id_1, graphics_id_0, graphics_id_1, dat_id, mvp_id, viewport_id;
    {
        // Canvas #0.

        // Make a canvas creation request.
        req = dvz_create_canvas(rqr, WIDTH, HEIGHT, DVZ_DEFAULT_CLEAR_COLOR, 0);
        dvz_requester_add(rqr, req);
        canvas_id_0 = req.id;

        req = dvz_create_graphics(rqr, DVZ_GRAPHICS_TRIANGLE, 0);
        dvz_requester_add(rqr, req);
        graphics_id_0 = req.id;


        // Canvas #1.

        // Make a canvas creation request.
        req = dvz_create_canvas(rqr, WIDTH / 2, HEIGHT / 2, DVZ_DEFAULT_CLEAR_COLOR, 0);
        dvz_requester_add(rqr, req);
        canvas_id_1 = req.id;

        req = dvz_create_graphics(rqr, DVZ_GRAPHICS_TRIANGLE, 0);
        dvz_requester_add(rqr, req);
        graphics_id_1 = req.id;


        // Create the vertex buffer dat.
        req = dvz_create_dat(rqr, DVZ_BUFFER_TYPE_VERTEX, 3 * sizeof(DvzVertex), 0);
        dvz_requester_add(rqr, req);
        dat_id = req.id;

        // Bind the vertex buffer dat to the graphics pipe.
        req = dvz_set_vertex(rqr, graphics_id_0, dat_id);
        dvz_requester_add(rqr, req);
        req = dvz_set_vertex(rqr, graphics_id_1, dat_id);
        dvz_requester_add(rqr, req);

        // Upload the triangle data.
        DvzVertex data[] = {
            {{-1, -1, 0}, {255, 0, 0, 255}},
            {{+1, -1, 0}, {0, 255, 0, 255}},
            {{+0, +1, 0}, {0, 0, 255, 255}},
        };
        req = dvz_upload_dat(rqr, dat_id, 0, sizeof(data), data);
        dvz_requester_add(rqr, req);

        // Binding #0: MVP.
        req = dvz_create_dat(rqr, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzMVP), 0);
        dvz_requester_add(rqr, req);
        mvp_id = req.id;

        req = dvz_bind_dat(rqr, graphics_id_0, 0, mvp_id);
        dvz_requester_add(rqr, req);
        req = dvz_bind_dat(rqr, graphics_id_1, 0, mvp_id);
        dvz_requester_add(rqr, req);

        DvzMVP mvp = dvz_mvp_default();
        // dvz_show_base64(sizeof(mvp), &mvp);
        req = dvz_upload_dat(rqr, mvp_id, 0, sizeof(DvzMVP), &mvp);
        dvz_requester_add(rqr, req);

        // Binding #1: viewport.
        req = dvz_create_dat(rqr, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzViewport), 0);
        dvz_requester_add(rqr, req);
        viewport_id = req.id;

        req = dvz_bind_dat(rqr, graphics_id_0, 1, viewport_id);
        dvz_requester_add(rqr, req);
        req = dvz_bind_dat(rqr, graphics_id_1, 1, viewport_id);
        dvz_requester_add(rqr, req);

        DvzViewport viewport = dvz_viewport_default(WIDTH / 2, HEIGHT / 2);
        // dvz_show_base64(sizeof(viewport), &viewport);
        req = dvz_upload_dat(rqr, viewport_id, 0, sizeof(DvzViewport), &viewport);
        dvz_requester_add(rqr, req);


        // Command buffer.
        req = dvz_record_begin(rqr, canvas_id_0);
        dvz_requester_add(rqr, req);

        req = dvz_record_viewport(rqr, canvas_id_0, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
        dvz_requester_add(rqr, req);

        req = dvz_record_draw(rqr, canvas_id_0, graphics_id_0, 0, 3);
        dvz_requester_add(rqr, req);

        req = dvz_record_end(rqr, canvas_id_0);
        dvz_requester_add(rqr, req);

        req = dvz_record_begin(rqr, canvas_id_1);
        dvz_requester_add(rqr, req);

        req = dvz_record_viewport(rqr, canvas_id_1, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
        dvz_requester_add(rqr, req);

        req = dvz_record_draw(rqr, canvas_id_1, graphics_id_1, 0, 3);
        dvz_requester_add(rqr, req);

        req = dvz_record_end(rqr, canvas_id_1);
        dvz_requester_add(rqr, req);
    }

    // Submit a client event with type REQUESTS and with a pointer to the requester.
    // The Presenter will register a REQUESTS callback sending the requests to the underlying
    // renderer.
    dvz_presenter_submit(prt, rqr);

    // Dequeue and process all pending events.
    dvz_client_run(client, N_FRAMES);

    // End.


    // Destroying all objects.
    dvz_client_destroy(client);
    dvz_requester_destroy(rqr);

    dvz_renderer_destroy(rd);
    dvz_presenter_destroy(prt);
    dvz_gpu_destroy(gpu);

    return 0;
}


#define DVZ_FPS_MAX_COUNT 2000
#define DVZ_FPS_BINS      100
static double fps_delays[DVZ_FPS_MAX_COUNT] = {0};
static float hist[DVZ_FPS_BINS] = {0};
static DvzClock fps_clock;

static inline void _gui_callback_fps(DvzGuiWindow* gui_window, void* user_data)
{
    ANN(gui_window);

    uint64_t* counter = (uint64_t*)user_data;
    ANN(counter);

    uint64_t counter_mod = (*counter) % DVZ_FPS_MAX_COUNT;
    ASSERT(counter_mod < DVZ_FPS_MAX_COUNT);

    double interval = dvz_clock_interval(&fps_clock);
    fps_delays[counter_mod] = 1. / interval;

    uint32_t count = MIN(DVZ_FPS_MAX_COUNT, *counter);
    double fps = (*counter) >= 2 ? dvz_mean(count, fps_delays) : 1.0;

    dvec2 min_max = {0};
    dvz_range(count, fps_delays, min_max);
    double min = 0; // min_max[0];
    double max = min_max[1];
    double diff = max - min;
    if (diff == 0)
        diff = 1;

    memset(hist, 0, DVZ_FPS_BINS * sizeof(float));
    double bin = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        bin = (fps_delays[i] - min) / diff;
        ASSERT((0 <= bin) && (bin <= 1));
        bin = round(bin * DVZ_FPS_BINS);
        bin = CLIP(bin, 0, DVZ_FPS_BINS - 1);
        ASSERT((0 <= bin) && (bin <= DVZ_FPS_BINS - 1));
        hist[(int)bin]++;
    }

    char str[32] = {0};
    snprintf(str, 32, "FPS: %04.0f/s", fps);

    dvz_gui_dialog_begin((vec2){100, 100}, (vec2){200, 200});
    dvz_gui_histogram(str, DVZ_FPS_BINS, hist);
    // dvz_gui_demo();
    dvz_gui_dialog_end();

    dvz_clock_tick(&fps_clock);
    (*counter)++;
}

int test_presenter_fps(TstSuite* suite)
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
    DvzRequester* rqr = dvz_requester();
    DvzRequest req = {0};

    // Presenter linking the renderer and the client.
    DvzPresenter* prt = dvz_presenter(rd, client, DVZ_CANVAS_FLAGS_IMGUI);

    // Start.

    fps_clock = dvz_clock();

    // Make a canvas creation request.
    req = dvz_create_canvas(rqr, WIDTH, HEIGHT, DVZ_DEFAULT_CLEAR_COLOR, DVZ_CANVAS_FLAGS_FPS);
    dvz_requester_add(rqr, req);
    // DvzId canvas_id = req.id;

    // Submit a client event with type REQUESTS and with a pointer to the requester.
    // The Presenter will register a REQUESTS callback sending the requests to the underlying
    // renderer.
    dvz_presenter_submit(prt, rqr);

    // GUI callback.
    uint64_t counter = 0;
    dvz_presenter_gui(prt, req.id, _gui_callback_fps, &counter);

    // Dequeue and process all pending events.
    dvz_client_run(client, N_FRAMES);

    // End.


    // Destroying all objects.
    dvz_requester_destroy(rqr);
    dvz_renderer_destroy(rd);
    dvz_presenter_destroy(prt);
    dvz_gpu_destroy(gpu);
    dvz_client_destroy(client);
    return 0;
}
