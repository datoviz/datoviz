/*************************************************************************************************/
/*  Testing presenter */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_presenter.h"
#include "client.h"
#include "client_input.h"
#include "colormaps.h"
#include "fps.h"
#include "glfw_utils.h"
#include "gui.h"
#include "presenter.h"
#include "scene/panzoom.h"
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
    GraphicsWrapper* graphics_wrapper;
    uint32_t n;
};



typedef struct PanzoomStruct PanzoomStruct;
struct PanzoomStruct
{
    DvzRequester* rqr;
    DvzPresenter* prt;
    DvzId mvp_id;
    DvzPanzoom* pz;
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
    dvz_presenter_destroy(prt);

    dvz_client_destroy(client);
    dvz_requester_destroy(rqr);

    dvz_renderer_destroy(rd);
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
    dvz_presenter_destroy(prt);

    dvz_client_destroy(client);
    dvz_requester_destroy(rqr);

    dvz_renderer_destroy(rd);
    dvz_gpu_destroy(gpu);

    return 0;
}



static inline void _gui_callback_1(DvzGuiWindow* gui_window, void* user_data)
{
    dvz_gui_dialog_begin((vec2){100, 100}, (vec2){200, 200}, 0);
    dvz_gui_text("Hello world");
    // NOTE: ImGui code can be called but need C++, unless one uses cimgui and builds it along
    // the executable.

    // dvz_gui_demo();

    // DvzTex* tex = (DvzTex*)user_data;
    // ANN(tex);
    // {
    //     const uint32_t width = tex->shape[0];

    //     cvec4* img = (cvec4*)calloc(width, 4);
    //     for (uint32_t i = 0; i < width; i++)
    //         dvz_colormap(DVZ_CMAP_HSV, i * 256 / (width), img[i]);

    //     dvz_tex_upload(
    //         tex, DVZ_ZERO_OFFSET, (uvec3){width, 1, 1}, width * sizeof(cvec4), img, true);
    //     FREE(img);
    // }
    // dvz_gui_image(tex, 300, 50);

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

    // Submit a client event with type REQUESTS and with a pointer to the requester.
    // The Presenter will register a REQUESTS callback sending the requests to the underlying
    // renderer.
    dvz_presenter_submit(prt, rqr);

    // // Texture.
    // DvzTex* tex = dvz_tex(
    //     rd->ctx, DVZ_TEX_2D, (uvec3){256, 1, 1}, DVZ_FORMAT_R8G8B8A8_UNORM,
    //     DVZ_TEX_FLAGS_PERSISTENT_STAGING);

    // GUI callback.
    dvz_presenter_gui(prt, req.id, _gui_callback_1, NULL);

    // Dequeue and process all pending events.
    dvz_client_run(client, N_FRAMES);

    // End.

    // Destroying all objects.
    dvz_presenter_destroy(prt);

    dvz_client_destroy(client);
    dvz_requester_destroy(rqr);

    dvz_renderer_destroy(rd);
    dvz_gpu_destroy(gpu);

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
    dvz_presenter_destroy(prt);

    dvz_client_destroy(client);
    dvz_requester_destroy(rqr);

    dvz_renderer_destroy(rd);
    dvz_gpu_destroy(gpu);

    return 0;
}



static inline void _gui_callback_fps(DvzGuiWindow* gui_window, void* user_data)
{
    ANN(gui_window);
    DvzFps* fps = (DvzFps*)user_data;
    ANN(fps);

    dvz_gui_dialog_begin((vec2){100, 100}, (vec2){200, 200}, DVZ_DIALOG_FLAGS_FPS);

    dvz_fps_tick(fps);
    dvz_fps_histogram(fps);

    dvz_gui_dialog_end();
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

    // Make a canvas creation request.
    req = dvz_create_canvas(rqr, WIDTH, HEIGHT, DVZ_DEFAULT_CLEAR_COLOR, DVZ_CANVAS_FLAGS_FPS);
    dvz_requester_add(rqr, req);

    // Submit a client event with type REQUESTS and with a pointer to the requester.
    // The Presenter will register a REQUESTS callback sending the requests to the underlying
    // renderer.
    dvz_presenter_submit(prt, rqr);

    // FPS callback.
    DvzFps fps = dvz_fps();
    dvz_presenter_gui(prt, req.id, _gui_callback_fps, &fps);

    // Dequeue and process all pending events.
    dvz_client_run(client, N_FRAMES);

    // End.


    // Destroying all objects.
    dvz_fps_destroy(&fps);
    dvz_presenter_destroy(prt);

    dvz_client_destroy(client);
    dvz_requester_destroy(rqr);

    dvz_renderer_destroy(rd);
    dvz_gpu_destroy(gpu);

    return 0;
}



static void _on_mouse(DvzClient* client, DvzClientEvent ev)
{
    ANN(client);

    PanzoomStruct* ps = (PanzoomStruct*)ev.user_data;
    ANN(ps);

    DvzPanzoom* pz = ps->pz;
    ANN(pz);

    DvzPresenter* prt = ps->prt;
    ANN(prt);

    DvzRequester* rqr = ps->rqr;
    ANN(rqr);

    DvzId mvp_id = ps->mvp_id;

    // Dragging: pan.
    if (ev.content.m.type == DVZ_MOUSE_EVENT_DRAG)
    {
        if (ev.content.m.content.d.button == DVZ_MOUSE_BUTTON_LEFT)
        {
            dvz_panzoom_pan_shift(pz, ev.content.m.content.d.shift, (vec2){0});
        }
        else if (ev.content.m.content.d.button == DVZ_MOUSE_BUTTON_RIGHT)
        {
            dvz_panzoom_zoom_shift(
                pz, ev.content.m.content.d.shift, ev.content.m.content.d.press_pos);
        }
    }

    // Stop dragging.
    if (ev.content.m.type == DVZ_MOUSE_EVENT_DRAG_STOP)
    {
        dvz_panzoom_end(pz);
    }

    // Mouse wheel.
    if (ev.content.m.type == DVZ_MOUSE_EVENT_WHEEL)
    {
        dvz_panzoom_zoom_wheel(pz, ev.content.m.content.w.dir, ev.content.m.content.w.pos);
    }

    // Double-click
    if (ev.content.m.type == DVZ_MOUSE_EVENT_DOUBLE_CLICK)
    {
        dvz_panzoom_reset(pz);
    }

    // Update the MVP matrices.
    DvzMVP* mvp = dvz_panzoom_mvp(pz);

    // Submit a dat upload request with the new MVP matrices.
    DvzRequest req = dvz_upload_dat(rqr, mvp_id, 0, sizeof(DvzMVP), mvp);
    dvz_requester_add(rqr, req);
    dvz_presenter_submit(prt, rqr);
}

static void _scatter_resize(DvzClient* client, DvzClientEvent ev)
{
    ANN(client);

    uint32_t width = ev.content.w.screen_width;
    uint32_t height = ev.content.w.screen_height;
    log_info("window %x resized to %dx%d", ev.window_id, width, height);

    DvzPanzoom* pz = (DvzPanzoom*)ev.user_data;
    dvz_panzoom_resize(pz, width, height);
}

int test_presenter_scatter(TstSuite* suite)
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

    // Presenter linking the renderer and the client.
    DvzPresenter* prt = dvz_presenter(rd, client, 0);

    const uint32_t n = 50;
    GraphicsWrapper wrapper = {0};
    graphics_request(rqr, n, &wrapper);
    void* data = graphics_scatter(rqr, wrapper.dat_id, n);

    // Submit a client event with type REQUESTS and with a pointer to the requester.
    // The Presenter will register a REQUESTS callback sending the requests to the underlying
    // renderer.
    dvz_presenter_submit(prt, rqr);


    // Panzoom callback.
    DvzPanzoom pz = dvz_panzoom(WIDTH, HEIGHT, 0);
    PanzoomStruct ps = {
        .mvp_id = wrapper.mvp_id,
        .prt = prt,
        .pz = &pz,
        .rqr = rqr,
    };
    dvz_client_callback(client, DVZ_CLIENT_EVENT_MOUSE, DVZ_CLIENT_CALLBACK_SYNC, _on_mouse, &ps);
    dvz_client_callback(
        client, DVZ_CLIENT_EVENT_WINDOW_RESIZE, DVZ_CLIENT_CALLBACK_SYNC, _scatter_resize, &pz);

    // Dequeue and process all pending events.
    dvz_client_run(client, N_FRAMES);

    // End.

    // Destroying all objects.
    dvz_presenter_destroy(prt);

    dvz_client_destroy(client);
    dvz_panzoom_destroy(&pz);
    dvz_requester_destroy(rqr);

    dvz_renderer_destroy(rd);
    dvz_gpu_destroy(gpu);

    FREE(data);
    return 0;
}



static void _random_data(uint32_t n, DvzGraphicsPointVertex* data)
{
    ASSERT(n > 0);
    ANN(data);

    for (uint32_t i = 0; i < n; i++)
    {
        data[i].pos[0] = -1 + 2 * dvz_rand_float();
        data[i].pos[1] = -1 + 2 * dvz_rand_float();
        data[i].size = 5;
        dvz_colormap(DVZ_CMAP_HSV, i % 256, data[i].color);
        data[i].color[3] = 224;
    }
}

static void _on_click(DvzClient* client, DvzClientEvent ev)
{
    ANN(client);
    if (ev.content.m.type != DVZ_MOUSE_EVENT_CLICK)
        return;

    CallbackStruct* s = (CallbackStruct*)ev.user_data;
    ANN(s);

    DvzRequester* rqr = s->rqr;
    ANN(rqr);

    DvzPresenter* prt = s->prt;
    ANN(prt);

    GraphicsWrapper* wrapper = s->graphics_wrapper;
    ANN(wrapper);

    // Update the data.
    _random_data(s->n, (DvzGraphicsPointVertex*)wrapper->data);
    DvzRequest req = dvz_upload_dat(
        rqr, wrapper->dat_id, 0, s->n * sizeof(DvzGraphicsPointVertex), wrapper->data);
    dvz_requester_add(rqr, req);

    // // Update the command buffer with the new n.
    // req = dvz_record_begin(rqr, wrapper->canvas_id);
    // dvz_requester_add(rqr, req);

    // req = dvz_record_viewport(rqr, wrapper->canvas_id, DVZ_DEFAULT_VIEWPORT,
    // DVZ_DEFAULT_VIEWPORT); dvz_requester_add(rqr, req);

    // req = dvz_record_draw(rqr, wrapper->canvas_id, wrapper->graphics_id, 0, s->n);
    // dvz_requester_add(rqr, req);

    // req = dvz_record_end(rqr, wrapper->canvas_id);
    // dvz_requester_add(rqr, req);

    dvz_presenter_submit(prt, rqr);
}

int test_presenter_thread(TstSuite* suite)
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

    // Presenter linking the renderer and the client.
    DvzPresenter* prt = dvz_presenter(rd, client, 0);

    const uint32_t n = 256;
    GraphicsWrapper wrapper = {0};
    graphics_request(rqr, n, &wrapper);
    wrapper.data = calloc(n, sizeof(DvzGraphicsPointVertex));
    _random_data(n, (DvzGraphicsPointVertex*)wrapper.data);
    DvzRequest req =
        dvz_upload_dat(rqr, wrapper.dat_id, 0, n * sizeof(DvzGraphicsPointVertex), wrapper.data);
    dvz_requester_add(rqr, req);

    // Submit a client event with type REQUESTS and with a pointer to the requester.
    // The Presenter will register a REQUESTS callback sending the requests to the underlying
    // renderer.
    dvz_presenter_submit(prt, rqr);

    CallbackStruct s = {
        .prt = prt,
        .rqr = rqr,
        .graphics_wrapper = &wrapper,
        .n = n,
    };
    dvz_client_callback(client, DVZ_CLIENT_EVENT_MOUSE, DVZ_CLIENT_CALLBACK_SYNC, _on_click, &s);

    // Start the client background thread and run an infinite event loop in the thread.
    dvz_client_thread(client, N_FRAMES);

    // Stop the event loop in the thread.
    // dvz_sleep(100);
    // dvz_client_stop(client);

    // Wait until the client event loop is done.
    dvz_client_join(client);

    // End.

    // Destroying all objects.
    dvz_presenter_destroy(prt);

    dvz_client_destroy(client);
    dvz_requester_destroy(rqr);

    dvz_renderer_destroy(rd);
    dvz_gpu_destroy(gpu);

    FREE(wrapper.data);
    return 0;
}
