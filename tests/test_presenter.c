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
#include "scene/arcball.h"
#include "scene/camera.h"
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



typedef struct TexStruct TexStruct;
struct TexStruct
{
    DvzRequester* rqr;
    DvzPresenter* prt;

    uint32_t width;
    DvzId tex_id;
    cvec4* tex_data;
};



typedef struct PanzoomStruct PanzoomStruct;
struct PanzoomStruct
{
    DvzRequester* rqr;
    DvzPresenter* prt;
    DvzId mvp_id;
    DvzMVP mvp;
    DvzPanzoom* pz;
};



typedef struct ArcballStruct ArcballStruct;
struct ArcballStruct
{
    DvzRequester* rqr;
    DvzPresenter* prt;
    DvzId mvp_id;
    DvzMVP mvp;
    DvzArcball* arcball;
    DvzCamera* cam;
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

    // Presenter linking the renderer and the client.
    DvzPresenter* prt = dvz_presenter(rd, client, 0);

    // Start.

    // Make a canvas creation request.
    dvz_create_canvas(rqr, WIDTH, HEIGHT, DVZ_DEFAULT_CLEAR_COLOR, 0);

    // Submit a client event with type REQUESTS and with a pointer to the requester.
    // The Presenter will register a REQUESTS callback sending the requests to the underlying
    // renderer.
    dvz_presenter_submit(prt, rqr);

    // Dequeue and process all pending events.
    dvz_client_run(client, N_FRAMES);

    // End.
    dvz_client_destroy(client);
    dvz_presenter_destroy(prt);
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
    log_info("window 0x%" PRIx64 " resized to %dx%d", ev.window_id, width, height);

    CallbackStruct* s = (CallbackStruct*)ev.user_data;
    ANN(s);

    DvzRequester* rqr = s->rqr;
    ANN(rqr);

    // Submit new recording commands to the client.
    dvz_record_begin(rqr, s->canvas_id);
    dvz_record_viewport(rqr, s->canvas_id, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
    dvz_record_draw(rqr, s->canvas_id, s->graphics_id, 0, 3);
    dvz_record_end(rqr, s->canvas_id);
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

        // Canvas id.
        canvas_id = req.id;

        // Create a graphics.
        req = dvz_create_graphics(rqr, DVZ_GRAPHICS_TRIANGLE, 0);
        graphics_id = req.id;

        // Create the vertex buffer dat.
        req = dvz_create_dat(rqr, DVZ_BUFFER_TYPE_VERTEX, 3 * sizeof(DvzVertex), 0);
        dat_id = req.id;

        // Bind the vertex buffer dat to the graphics pipe.
        req = dvz_set_vertex(rqr, graphics_id, dat_id);

        // Upload the triangle data.
        DvzVertex data[] = {
            {{-1, -1, 0}, {255, 0, 0, 255}},
            {{+1, -1, 0}, {0, 255, 0, 255}},
            {{+0, +1, 0}, {0, 0, 255, 255}},
        };
        req = dvz_upload_dat(rqr, dat_id, 0, sizeof(data), data);

        // Binding #0: MVP.
        req = dvz_create_dat(
            rqr, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzMVP), DVZ_DAT_FLAGS_PERSISTENT_STAGING);
        mvp_id = req.id;

        req = dvz_bind_dat(rqr, graphics_id, 0, mvp_id);

        DvzMVP mvp = dvz_mvp_default();
        // dvz_show_base64(sizeof(mvp), &mvp);
        req = dvz_upload_dat(rqr, mvp_id, 0, sizeof(DvzMVP), &mvp);

        // Binding #1: viewport.
        req = dvz_create_dat(rqr, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzViewport), 0);
        viewport_id = req.id;

        req = dvz_bind_dat(rqr, graphics_id, 1, viewport_id);

        DvzViewport viewport = dvz_viewport_default(WIDTH, HEIGHT);
        // dvz_show_base64(sizeof(viewport), &viewport);
        req = dvz_upload_dat(rqr, viewport_id, 0, sizeof(DvzViewport), &viewport);


        // Command buffer.
        req = dvz_record_begin(rqr, canvas_id);

        req = dvz_record_viewport(rqr, canvas_id, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);

        req = dvz_record_draw(rqr, canvas_id, graphics_id, 0, 3);

        req = dvz_record_end(rqr, canvas_id);
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
    dvz_client_destroy(client);
    dvz_presenter_destroy(prt);
    dvz_requester_destroy(rqr);
    dvz_renderer_destroy(rd);
    dvz_gpu_destroy(gpu);

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
    dvz_upload_dat(rqr, wrapper->dat_id, 0, s->n * sizeof(DvzGraphicsPointVertex), wrapper->data);

    // // Update the command buffer with the new n.
    // req = dvz_record_begin(rqr, wrapper->canvas_id);
    //
    // req = dvz_record_viewport(rqr, wrapper->canvas_id, DVZ_DEFAULT_VIEWPORT,
    // DVZ_DEFAULT_VIEWPORT);
    // req = dvz_record_draw(rqr, wrapper->canvas_id, wrapper->graphics_id, 0, s->n);
    //
    // req = dvz_record_end(rqr, wrapper->canvas_id);
    //
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
    graphics_request(rqr, n, &wrapper, 0);
    wrapper.data = calloc(n, sizeof(DvzGraphicsPointVertex));
    _random_data(n, (DvzGraphicsPointVertex*)wrapper.data);

    dvz_upload_dat(rqr, wrapper.dat_id, 0, n * sizeof(DvzGraphicsPointVertex), wrapper.data);

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
    dvz_requester_destroy(rqr);
    dvz_renderer_destroy(rd);
    dvz_gpu_destroy(gpu);

    FREE(wrapper.data);
    return 0;
}



int test_presenter_deserialize(TstSuite* suite)
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

    // Load the requests from requests.dvz.
    dvz_requester_load(rqr, DVZ_DUMP_FILENAME);

    // Submit a client event with type REQUESTS and with a pointer to the requester.
    // The Presenter will register a REQUESTS callback sending the requests to the underlying
    // renderer.
    dvz_presenter_submit(prt, rqr);

    // Dequeue and process all pending events.
    dvz_client_run(client, N_FRAMES);

    // End.
    dvz_client_destroy(client);
    dvz_presenter_destroy(prt);
    dvz_requester_destroy(rqr);
    dvz_renderer_destroy(rd);
    dvz_gpu_destroy(gpu);

    return 0;
}



static inline void _gui_callback_1(DvzGuiWindow* gui_window, void* user_data)
{
    dvz_gui_dialog_begin("Window title", (vec2){100, 100}, (vec2){400, 200}, 0);
    dvz_gui_text("Hello world");

    // NOTE: ImGui code can be called but need C++, unless one uses cimgui and builds it along
    // the executable.

    // dvz_gui_demo();

    TexStruct* tex_struct = (TexStruct*)user_data;
    ANN(tex_struct);

    DvzPresenter* prt = tex_struct->prt;
    ANN(prt);

    DvzRequester* rqr = tex_struct->rqr;
    ANN(rqr);

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
        dvz_colormap(
            DVZ_CMAP_HSV, (i - ((uint32_t)round(t * 128) % width)) * 256 / width, tex_data[i]);

    // Upload the texture data.
    dvz_upload_tex(
        rqr, tex_id, DVZ_ZERO_OFFSET, (uvec3){width, 1, 1}, width * sizeof(cvec4), tex_data);

    // NOTE: this call needs to be explicit when using the presenter API. The app API automatically
    // calls it so the user doesn't need it.
    dvz_presenter_submit(prt, rqr);

    // Display the texture as an image in the GUI.
    DvzTex* tex = dvz_renderer_tex(tex_struct->prt->rd, tex_struct->tex_id);
    ANN(tex);
    dvz_gui_image(tex, 300, 50);

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
    DvzId canvas_id = req.id;

    // Texture.
    const uint32_t width = 256, height = 1;
    req = dvz_create_tex(
        rqr, DVZ_TEX_2D, DVZ_FORMAT_R8G8B8A8_UNORM, (uvec3){width, height, 1},
        DVZ_TEX_FLAGS_PERSISTENT_STAGING);
    DvzId tex_id = req.id;

    // Texture data.
    cvec4* tex_data = (cvec4*)calloc(width * height, sizeof(cvec4));
    for (uint32_t i = 0; i < width; i++)
        dvz_colormap(DVZ_CMAP_HSV, i * 256 / width, tex_data[i]);
    dvz_upload_tex(
        rqr, tex_id, DVZ_ZERO_OFFSET, (uvec3){width, 1, 1}, width * sizeof(cvec4), tex_data);

    // Texture struct.
    TexStruct tex_struct = {
        .prt = prt,
        .rqr = rqr,
        .width = width,
        .tex_id = tex_id,
        .tex_data = tex_data,
    };

    // Submit a client event with type REQUESTS and with a pointer to the requester.
    // The Presenter will register a REQUESTS callback sending the requests to the underlying
    // renderer.
    dvz_presenter_submit(prt, rqr);

    // GUI callback.
    dvz_presenter_gui(prt, canvas_id, _gui_callback_1, &tex_struct);

    // Dequeue and process all pending events.
    dvz_client_run(client, N_FRAMES);

    // End.
    dvz_client_destroy(client);
    dvz_presenter_destroy(prt);
    dvz_requester_destroy(rqr);
    dvz_renderer_destroy(rd);
    dvz_gpu_destroy(gpu);

    FREE(tex_data);

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
        canvas_id_0 = req.id;

        req = dvz_create_graphics(rqr, DVZ_GRAPHICS_TRIANGLE, 0);
        graphics_id_0 = req.id;


        // Canvas #1.

        // Make a canvas creation request.
        req = dvz_create_canvas(rqr, WIDTH / 2, HEIGHT / 2, DVZ_DEFAULT_CLEAR_COLOR, 0);
        canvas_id_1 = req.id;

        req = dvz_create_graphics(rqr, DVZ_GRAPHICS_TRIANGLE, 0);
        graphics_id_1 = req.id;


        // Create the vertex buffer dat.
        req = dvz_create_dat(rqr, DVZ_BUFFER_TYPE_VERTEX, 3 * sizeof(DvzVertex), 0);
        dat_id = req.id;

        // Bind the vertex buffer dat to the graphics pipe.
        req = dvz_set_vertex(rqr, graphics_id_0, dat_id);
        req = dvz_set_vertex(rqr, graphics_id_1, dat_id);

        // Upload the triangle data.
        DvzVertex data[] = {
            {{-1, -1, 0}, {255, 0, 0, 255}},
            {{+1, -1, 0}, {0, 255, 0, 255}},
            {{+0, +1, 0}, {0, 0, 255, 255}},
        };
        req = dvz_upload_dat(rqr, dat_id, 0, sizeof(data), data);

        // Binding #0: MVP.
        req = dvz_create_dat(rqr, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzMVP), 0);
        mvp_id = req.id;

        req = dvz_bind_dat(rqr, graphics_id_0, 0, mvp_id);
        req = dvz_bind_dat(rqr, graphics_id_1, 0, mvp_id);

        DvzMVP mvp = dvz_mvp_default();
        // dvz_show_base64(sizeof(mvp), &mvp);
        req = dvz_upload_dat(rqr, mvp_id, 0, sizeof(DvzMVP), &mvp);

        // Binding #1: viewport.
        req = dvz_create_dat(rqr, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzViewport), 0);
        viewport_id = req.id;

        dvz_bind_dat(rqr, graphics_id_0, 1, viewport_id);
        dvz_bind_dat(rqr, graphics_id_1, 1, viewport_id);

        DvzViewport viewport = dvz_viewport_default(WIDTH / 2, HEIGHT / 2);
        // dvz_show_base64(sizeof(viewport), &viewport);
        dvz_upload_dat(rqr, viewport_id, 0, sizeof(DvzViewport), &viewport);


        // Command buffer.
        dvz_record_begin(rqr, canvas_id_0);
        dvz_record_viewport(rqr, canvas_id_0, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
        dvz_record_draw(rqr, canvas_id_0, graphics_id_0, 0, 3);
        dvz_record_end(rqr, canvas_id_0);
        dvz_record_begin(rqr, canvas_id_1);
        dvz_record_viewport(rqr, canvas_id_1, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
        dvz_record_draw(rqr, canvas_id_1, graphics_id_1, 0, 3);
        dvz_record_end(rqr, canvas_id_1);
    }

    // Submit a client event with type REQUESTS and with a pointer to the requester.
    // The Presenter will register a REQUESTS callback sending the requests to the underlying
    // renderer.
    dvz_presenter_submit(prt, rqr);

    // Dequeue and process all pending events.
    dvz_client_run(client, N_FRAMES);

    // End.
    dvz_client_destroy(client);
    dvz_presenter_destroy(prt);
    dvz_requester_destroy(rqr);
    dvz_renderer_destroy(rd);
    dvz_gpu_destroy(gpu);

    return 0;
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

    // Presenter linking the renderer and the client.
    DvzPresenter* prt = dvz_presenter(rd, client, DVZ_CANVAS_FLAGS_IMGUI);

    // Start.

    // Make a canvas creation request.
    dvz_create_canvas(rqr, WIDTH, HEIGHT, DVZ_DEFAULT_CLEAR_COLOR, DVZ_CANVAS_FLAGS_FPS);

    // Submit a client event with type REQUESTS and with a pointer to the requester.
    // The Presenter will register a REQUESTS callback sending the requests to the underlying
    // renderer.
    dvz_presenter_submit(prt, rqr);

    // Dequeue and process all pending events.
    dvz_client_run(client, N_FRAMES);

    // End.
    dvz_client_destroy(client);
    dvz_presenter_destroy(prt);
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

    DvzMVP* mvp = &ps->mvp;
    ANN(mvp);

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
    *mvp = dvz_mvp_default();
    dvz_panzoom_mvp(pz, mvp);

    // Submit a dat upload request with the new MVP matrices.
    dvz_upload_dat(rqr, mvp_id, 0, sizeof(DvzMVP), mvp);
    dvz_presenter_submit(prt, rqr);
}

static void _scatter_resize(DvzClient* client, DvzClientEvent ev)
{
    ANN(client);

    uint32_t width = ev.content.w.screen_width;
    uint32_t height = ev.content.w.screen_height;
    log_info("window 0x%" PRIx64 " resized to %dx%d", ev.window_id, width, height);

    DvzPanzoom* pz = (DvzPanzoom*)ev.user_data;
    ANN(pz);
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
    DvzPresenter* prt = dvz_presenter(rd, client, DVZ_CANVAS_FLAGS_IMGUI);

    const uint32_t n = 52;
    GraphicsWrapper wrapper = {0};
    graphics_request(rqr, n, &wrapper, DVZ_CANVAS_FLAGS_FPS);
    void* data = graphics_scatter(rqr, wrapper.dat_id, n);

    // Submit a client event with type REQUESTS and with a pointer to the requester.
    // The Presenter will register a REQUESTS callback sending the requests to the underlying
    // renderer.
    dvz_presenter_submit(prt, rqr);


    // Panzoom callback.
    DvzPanzoom* pz = dvz_panzoom(WIDTH, HEIGHT, 0);
    PanzoomStruct ps = {
        .mvp_id = wrapper.mvp_id,
        .prt = prt,
        .pz = pz,
        .rqr = rqr,
    };
    dvz_client_callback(client, DVZ_CLIENT_EVENT_MOUSE, DVZ_CLIENT_CALLBACK_SYNC, _on_mouse, &ps);
    dvz_client_callback(
        client, DVZ_CLIENT_EVENT_WINDOW_RESIZE, DVZ_CLIENT_CALLBACK_SYNC, _scatter_resize, pz);

    // Dequeue and process all pending events.
    dvz_client_run(client, N_FRAMES);

    // End.
    dvz_client_destroy(client);
    dvz_presenter_destroy(prt);
    dvz_requester_destroy(rqr);
    dvz_renderer_destroy(rd);
    dvz_gpu_destroy(gpu);
    dvz_panzoom_destroy(pz);

    FREE(data);
    return 0;
}



static void _on_mouse_arcball(DvzClient* client, DvzClientEvent ev)
{
    ANN(client);

    ArcballStruct* arc = (ArcballStruct*)ev.user_data;
    ANN(arc);

    DvzArcball* arcball = arc->arcball;
    ANN(arcball);

    DvzPresenter* prt = arc->prt;
    ANN(prt);

    DvzRequester* rqr = arc->rqr;
    ANN(rqr);

    DvzMVP* mvp = &arc->mvp;
    ANN(mvp);

    DvzId mvp_id = arc->mvp_id;

    // Dragging: pan.
    if (ev.content.m.type == DVZ_MOUSE_EVENT_DRAG)
    {
        if (ev.content.m.content.d.button == DVZ_MOUSE_BUTTON_LEFT)
        {
            float width = arcball->viewport_size[0];
            float height = arcball->viewport_size[1];

            vec2 cur_pos, last_pos;
            cur_pos[0] = -1 + 2 * ev.content.m.content.d.pos[0] / width;
            cur_pos[1] = +1 - 2 * ev.content.m.content.d.pos[1] / height;
            last_pos[0] = -1 + 2 * ev.content.m.content.d.press_pos[0] / width;
            last_pos[1] = +1 - 2 * ev.content.m.content.d.press_pos[1] / height;

            dvz_arcball_rotate(arcball, cur_pos, last_pos);
        }
        // else if (ev.content.m.content.d.button == DVZ_MOUSE_BUTTON_RIGHT)
        // {
        // }
    }

    // Stop dragging.
    if (ev.content.m.type == DVZ_MOUSE_EVENT_DRAG_STOP)
    {
        dvz_arcball_end(arcball);
    }

    // // Mouse wheel.
    // if (ev.content.m.type == DVZ_MOUSE_EVENT_WHEEL)
    // {
    //     dvz_panzoom_zoom_wheel(pz, ev.content.m.content.w.dir, ev.content.m.content.w.pos);
    // }

    // Double-click
    if (ev.content.m.type == DVZ_MOUSE_EVENT_DOUBLE_CLICK)
    {
        dvz_arcball_reset(arcball);
    }

    // Update the MVP matrices.
    dvz_arcball_mvp(arcball, mvp); // set the model matrix

    // Submit a dat upload request with the new MVP matrices.
    dvz_upload_dat(rqr, mvp_id, 0, sizeof(DvzMVP), mvp);
    dvz_presenter_submit(prt, rqr);
}

static void _arcball_resize(DvzClient* client, DvzClientEvent ev)
{
    ANN(client);

    uint32_t width = ev.content.w.screen_width;
    uint32_t height = ev.content.w.screen_height;
    log_info("window 0x%" PRIx64 " resized to %dx%d", ev.window_id, width, height);

    ANN(client);

    ArcballStruct* arc = (ArcballStruct*)ev.user_data;
    ANN(arc);

    DvzPresenter* prt = arc->prt;
    ANN(prt);

    DvzRequester* rqr = arc->rqr;
    ANN(rqr);

    DvzMVP* mvp = &arc->mvp;
    ANN(mvp);

    DvzId mvp_id = arc->mvp_id;

    DvzCamera* camera = arc->cam;
    ANN(camera);
    dvz_camera_resize(camera, width, height);

    DvzArcball* arcball = arc->arcball;
    ANN(arcball);
    dvz_arcball_resize(arcball, width, height);

    // Update the MVP matrices.
    dvz_camera_mvp(camera, mvp); // set the model matrix

    // Submit a dat upload request with the new MVP matrices.
    dvz_upload_dat(rqr, mvp_id, 0, sizeof(DvzMVP), mvp);
    dvz_presenter_submit(prt, rqr);
}

int test_presenter_arcball(TstSuite* suite)
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
    DvzPresenter* prt = dvz_presenter(rd, client, DVZ_CANVAS_FLAGS_IMGUI);

    const uint32_t n = 1000;
    GraphicsWrapper wrapper = {0};
    graphics_request(rqr, n, &wrapper, DVZ_CANVAS_FLAGS_FPS);
    // void* data = graphics_scatter(rqr, wrapper.dat_id, n);

    // Upload the data.
    DvzGraphicsPointVertex* data =
        (DvzGraphicsPointVertex*)calloc(n, sizeof(DvzGraphicsPointVertex));
    double t = 0;
    // double aspect = WIDTH / (double)HEIGHT;
    for (uint32_t i = 0; i < n; i++)
    {
        t = i / (double)(n);
        data[i].pos[0] = .25 * dvz_rand_normal();
        data[i].pos[1] = .25 * dvz_rand_normal();
        data[i].pos[2] = .25 * dvz_rand_normal();

        glm_vec3_normalize(data[i].pos);

        data[i].size = 4;

        dvz_colormap(DVZ_CMAP_HSV, TO_BYTE(t), data[i].color);
        data[i].color[3] = 128;
    }


    dvz_upload_dat(rqr, wrapper.dat_id, 0, n * sizeof(DvzGraphicsPointVertex), data);

    // Submit a client event with type REQUESTS and with a pointer to the requester.
    // The Presenter will register a REQUESTS callback sending the requests to the underlying
    // renderer.
    dvz_presenter_submit(prt, rqr);

    // Arcball callback.
    DvzArcball* arcball = dvz_arcball(WIDTH, HEIGHT, 0);
    // dvz_arcball_constrain(arcball, (vec3){0, 1, 0});
    DvzCamera* camera = dvz_camera(WIDTH, HEIGHT, 0);
    ArcballStruct arc = {
        .mvp_id = wrapper.mvp_id,
        .prt = prt,
        .arcball = arcball,
        .cam = camera,
        .rqr = rqr,
        .mvp = dvz_mvp_default(),
    };
    dvz_camera_mvp(camera, &arc.mvp); // set the view and proj matrices

    // Submit a dat upload request with the new MVP matrices.
    dvz_upload_dat(rqr, arc.mvp_id, 0, sizeof(DvzMVP), &arc.mvp);
    dvz_presenter_submit(prt, rqr);

    dvz_client_callback(
        client, DVZ_CLIENT_EVENT_MOUSE, DVZ_CLIENT_CALLBACK_SYNC, _on_mouse_arcball, &arc);
    dvz_client_callback(
        client, DVZ_CLIENT_EVENT_WINDOW_RESIZE, DVZ_CLIENT_CALLBACK_SYNC, _arcball_resize, &arc);

    // Dequeue and process all pending events.
    dvz_client_run(client, N_FRAMES);

    // End.

    // Destroying all objects.
    dvz_client_destroy(client);
    dvz_presenter_destroy(prt);
    dvz_requester_destroy(rqr);
    dvz_arcball_destroy(arcball);
    dvz_camera_destroy(camera);
    dvz_renderer_destroy(rd);
    dvz_gpu_destroy(gpu);

    FREE(data);
    return 0;
}
