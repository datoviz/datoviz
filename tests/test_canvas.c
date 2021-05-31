#include "../external/video.h"
#include "../include/datoviz/canvas.h"
#include "../include/datoviz/controls.h"
#include "proto.h"
#include "tests.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct EventHolder EventHolder;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct EventHolder
{
    DvzEvent init;
    DvzEvent key;
    DvzEvent wheel;
    DvzEvent button;
    DvzEvent move;
    DvzEvent timer;
    DvzEvent frame;
};



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static TestVisual triangle(DvzCanvas* canvas, const char* suffix)
{
    ASSERT(canvas != NULL);
    DvzGpu* gpu = canvas->gpu;
    ASSERT(gpu != NULL);

    TestVisual visual = {0};
    visual.gpu = gpu;
    visual.renderpass = &canvas->renderpass;
    visual.framebuffers = &canvas->framebuffers;

    // Make the graphics.
    visual.graphics = triangle_graphics(gpu, visual.renderpass, suffix);

    return visual;
}

static void triangle_refill(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    // Take the first command buffers, which corresponds to the default canvas render command//
    // buffer.
    ASSERT(ev.u.rf.cmd_count == 1);
    DvzCommands* cmds = ev.u.rf.cmds[0];
    ASSERT(cmds->queue_idx == DVZ_DEFAULT_QUEUE_RENDER);
    uint32_t idx = ev.u.rf.img_idx;

    TestVisual* visual = (TestVisual*)ev.user_data;
    ASSERT(visual != NULL);

    triangle_commands(
        cmds, idx, &canvas->renderpass, &canvas->framebuffers, //
        &visual->graphics, &visual->bindings, visual->br);
}

static void triangle_upload(DvzCanvas* canvas, TestVisual* visual)
{
    ASSERT(canvas != NULL);
    ASSERT(visual != NULL);
    DvzGpu* gpu = visual->gpu;
    ASSERT(gpu != NULL);

    // Create the buffer.
    VkDeviceSize size = 3 * sizeof(TestVertex);
    visual->br = dvz_ctx_buffers(gpu->context, DVZ_BUFFER_TYPE_VERTEX, 1, size);
    TestVertex data[3] = TRIANGLE_VERTICES;
    visual->data = calloc(size, 1);
    memcpy(visual->data, data, size);
    dvz_upload_buffer(gpu->context, visual->br, 0, size, data);
}



/*************************************************************************************************/
/*  Blank canvas                                                                                 */
/*************************************************************************************************/

int test_canvas_blank(TestContext* tc)
{
    DvzApp* app = tc->app;
    DvzGpu* gpu = dvz_gpu_best(app);

    DvzCanvas* canvas = dvz_canvas(gpu, WIDTH, HEIGHT, 0);
    ASSERT(canvas->app != NULL);

    if (app->backend != DVZ_BACKEND_OFFSCREEN)
    {
        ASSERT(canvas->window != NULL);
        ASSERT(canvas->window->app != NULL);
    }

    uvec2 size = {0};

    // Framebuffer size.
    dvz_canvas_size(canvas, DVZ_CANVAS_SIZE_FRAMEBUFFER, size);
    log_debug("canvas framebuffer size is %dx%d", size[0], size[1]);
    ASSERT(size[0] > 0);
    ASSERT(size[1] > 0);

    // Screen size.
    dvz_canvas_size(canvas, DVZ_CANVAS_SIZE_SCREEN, size);
    log_debug("canvas screen size is %dx%d", size[0], size[1]);
    ASSERT(size[0] > 0);
    ASSERT(size[1] > 0);

    dvz_app_run(app, N_FRAMES);

    // Check blank canvas.
    uint8_t* rgb = dvz_screenshot(canvas, false);
    for (uint32_t i = 0; i < size[0] * size[1] * 3 * sizeof(uint8_t); i++)
    {
        AT(rgb[i] == (i % 3 == 0 ? 0 : (i % 3 == 1 ? 8 : 18)))
    }
    FREE(rgb);

    dvz_canvas_destroy(canvas);
    return 0;
}



int test_canvas_multiple(TestContext* tc)
{
    DvzApp* app = tc->app;
    OFFSCREEN_SKIP

    DvzGpu* gpu = dvz_gpu_best(app);

    DvzCanvas* canvas0 = dvz_canvas(gpu, WIDTH, HEIGHT, DVZ_CANVAS_FLAGS_IMGUI);
    DvzCanvas* canvas1 = dvz_canvas(gpu, WIDTH, HEIGHT, DVZ_CANVAS_FLAGS_IMGUI);

    uvec2 size = {0};
    dvz_canvas_size(canvas0, DVZ_CANVAS_SIZE_FRAMEBUFFER, size);

    dvz_canvas_clear_color(canvas0, 1, 0, 0);
    dvz_canvas_clear_color(canvas1, 0, 1, 0);

    dvz_app_run(app, N_FRAMES);

    // Check canvas background color.
    uint8_t* rgb0 = dvz_screenshot(canvas0, false);
    uint8_t* rgb1 = dvz_screenshot(canvas1, false);
    for (uint32_t i = 0; i < size[0] * size[1] * 3 * sizeof(uint8_t); i++)
    {
        AT(rgb0[i] == (i % 3 == 0 ? 255 : 0));
        AT(rgb1[i] == (i % 3 == 1 ? 255 : 0));
    }
    FREE(rgb0);
    FREE(rgb1);

    dvz_canvas_destroy(canvas0);
    dvz_canvas_destroy(canvas1);
    return 0;
}



static void _init_callback(DvzCanvas* canvas, DvzEvent ev)
{
    log_debug("init event for canvas");
    ASSERT(canvas != NULL);
    EventHolder* events = (EventHolder*)ev.user_data;
    ASSERT(events != NULL);
    events->init = ev;
}

static void _wheel_callback(DvzCanvas* canvas, DvzEvent ev)
{
    log_debug("wheel %.3f", ev.u.w.dir[1]);
    ASSERT(canvas != NULL);
    EventHolder* events = (EventHolder*)ev.user_data;
    ASSERT(events != NULL);
    events->wheel = ev;
}

static void _button_callback(DvzCanvas* canvas, DvzEvent ev)
{
    log_debug("clicked %d mods %d", ev.u.b.button, ev.u.b.modifiers);
    ASSERT(canvas != NULL);
    EventHolder* events = (EventHolder*)ev.user_data;
    ASSERT(events != NULL);
    events->button = ev;
}

static void _move_callback(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    EventHolder* events = (EventHolder*)ev.user_data;
    ASSERT(events != NULL);
    events->move = ev;
}

static void _timer_callback(DvzCanvas* canvas, DvzEvent ev)
{
    log_trace("timer callback #%d time %.3f", ev.u.t.idx, ev.u.t.time);
    ASSERT(canvas != NULL);
    EventHolder* events = (EventHolder*)ev.user_data;
    ASSERT(events != NULL);
    events->timer = ev;
}

static void _frame_callback(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    log_debug(
        "canvas #%d, frame callback #%d, time %.6f, interval %.6f", //
        canvas->obj.id, ev.u.f.idx, ev.u.f.time, ev.u.f.interval);
    EventHolder* events = (EventHolder*)ev.user_data;
    ASSERT(events != NULL);
    events->frame = ev;
}

static void _key_callback(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    log_debug("key code %d", ev.u.k.key_code);
    EventHolder* events = (EventHolder*)ev.user_data;
    ASSERT(events != NULL);
    events->key = ev;
}



int test_canvas_events(TestContext* tc)
{
    DvzApp* app = tc->app;
    OFFSCREEN_SKIP

    DvzGpu* gpu = dvz_gpu_best(app);
    DvzCanvas* canvas = dvz_canvas(gpu, WIDTH, HEIGHT, DVZ_CANVAS_FLAGS_FPS);

    EventHolder events = {0};
    dvz_event_callback( //
        canvas, DVZ_EVENT_INIT, 0, DVZ_EVENT_MODE_SYNC, _init_callback, &events);
    dvz_event_callback( //
        canvas, DVZ_EVENT_TIMER, .00001, DVZ_EVENT_MODE_SYNC, _timer_callback, &events);
    dvz_event_callback( //
        canvas, DVZ_EVENT_FRAME, 0, DVZ_EVENT_MODE_SYNC, _frame_callback, &events);
    dvz_event_callback( //
        canvas, DVZ_EVENT_KEY_PRESS, 0, DVZ_EVENT_MODE_SYNC, _key_callback, &events);
    dvz_event_callback( //
        canvas, DVZ_EVENT_MOUSE_WHEEL, 0, DVZ_EVENT_MODE_SYNC, _wheel_callback, &events);
    dvz_event_callback( //
        canvas, DVZ_EVENT_MOUSE_MOVE, 0, DVZ_EVENT_MODE_SYNC, _move_callback, &events);
    dvz_event_callback( //
        canvas, DVZ_EVENT_MOUSE_PRESS, 0, DVZ_EVENT_MODE_SYNC, _button_callback, &events);

    dvz_app_run(app, N_FRAMES);

    // Init, timer, frame events.
    AT(events.init.type == DVZ_EVENT_INIT);
    AT(events.timer.u.t.idx > 0);
    AT(events.frame.u.f.idx == 4);

    // Key press.
    dvz_event_key_press(canvas, DVZ_KEY_A, 0);
    // dvz_app_run(app, 3);
    AT(events.key.u.k.key_code == DVZ_KEY_A);

    // Mouse wheel.
    vec2 pos = {10, 10};
    vec2 dir = {0, -2};
    dvz_event_mouse_wheel(canvas, pos, dir, 0);
    // dvz_app_run(app, 3);
    ACn(2, events.wheel.u.w.dir, dir, EPS);

    // Mouse move.
    pos[0] = 20;
    dvz_event_mouse_move(canvas, pos, 0);
    // dvz_app_run(app, 3);
    ACn(2, events.move.u.m.pos, pos, EPS);

    // Mouse press.
    dvz_event_mouse_press(canvas, DVZ_MOUSE_BUTTON_LEFT, 0);
    AT(events.button.u.b.button == DVZ_MOUSE_BUTTON_LEFT);

    // TODO: more events.

    dvz_canvas_destroy(canvas);
    return 0;
}



static void _gui_callback(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    if (ev.u.g.control->type == DVZ_GUI_CONTROL_SLIDER_FLOAT)
    {
        float* value = ev.u.g.control->value;
        log_info("value is %.3f", *value);
    }
    if (ev.u.g.control->type == DVZ_GUI_CONTROL_SLIDER_INT)
    {
        int* value = ev.u.g.control->value;
        log_info("value is %d", *value);
    }
    if (ev.u.g.control->type == DVZ_GUI_CONTROL_CHECKBOX)
    {
        bool* value = ev.u.g.control->value;
        log_info("value is %s", (*value) ? "checked" : "unchecked");
    }
    if (ev.u.g.control->type == DVZ_GUI_CONTROL_BUTTON)
    {
        log_info("button pressed");
    }
}

int test_canvas_gui(TestContext* tc)
{
    DvzApp* app = tc->app;
    OFFSCREEN_SKIP

    DvzGpu* gpu = dvz_gpu_best(app);
    DvzCanvas* canvas = dvz_canvas(gpu, WIDTH, HEIGHT, DVZ_CANVAS_FLAGS_IMGUI);

    // Make a simple GUI.
    DvzGui* gui = dvz_gui(canvas, "Hello world", 0);

    dvz_gui_label(gui, "label", "hello world!");
    dvz_gui_checkbox(gui, "my checkbox", true);
    dvz_gui_slider_float(gui, "my slider 1", 0.0f, 1.0f, .5);
    dvz_gui_slider_float2(gui, "my slider bis", 0.0f, 1.0f, (vec2){.25, .75}, true);
    dvz_gui_slider_int(gui, "my slider 2", 10, 20, 10);
    dvz_gui_input_float(gui, "enter a float", 1, 10, 0);
    dvz_gui_textbox(gui, "textbox", "some text");
    dvz_gui_button(gui, "my button", 0);
    dvz_gui_colormap(gui, DVZ_CMAP_VIRIDIS);

    dvz_event_callback(canvas, DVZ_EVENT_GUI, 0, DVZ_EVENT_MODE_SYNC, _gui_callback, NULL);

    dvz_app_run(app, N_FRAMES);
    int res = check_canvas(canvas, "test_canvas_gui");

    dvz_canvas_destroy(canvas);
    return res;
}



static void _frame_screencast_callback(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    cvec4 color = {0};
    dvz_colormap(DVZ_CMAP_HSV, (ev.u.f.idx / 3) % 256, color);
    dvz_canvas_clear_color(canvas, color[0] / 255.0, color[1] / 255.0, color[2] / 255.0);
}

static void _screencast_callback(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL); //
    bool ok = false;
    cvec4 color = {0};
    dvz_colormap(DVZ_CMAP_HSV, (canvas->frame_idx / 3) % 256, color);

    // log_info("%d %d %d %d", color[0], color[1], color[2], color[3]);
    // log_info("%d %d %d %d", ev.u.sc.rgba[0], ev.u.sc.rgba[1], ev.u.sc.rgba[2], ev.u.sc.rgba[3]);

    ok =
        (abs((int)color[0] - (int)ev.u.sc.rgba[0]) <= 16 &&
         abs((int)color[1] - (int)ev.u.sc.rgba[1]) <= 16 &&
         abs((int)color[2] - (int)ev.u.sc.rgba[2]) <= 16);

    FREE(ev.u.sc.rgba);
    if (((int*)ev.user_data)[0] == 0)
        ((int*)ev.user_data)[0] = ok ? 0 : 1;
}

int test_canvas_screencast(TestContext* tc)
{
    DvzApp* app = tc->app;

    // TODO: fix screencast/video in offscreen mode
    OFFSCREEN_SKIP

    DvzGpu* gpu = dvz_gpu_best(app);
    DvzCanvas* canvas = dvz_canvas(gpu, WIDTH, HEIGHT, 0);
    dvz_canvas_clear_color(canvas, 0, 1, 0);

    dvz_event_callback(
        canvas, DVZ_EVENT_FRAME, 0, DVZ_EVENT_MODE_SYNC, _frame_screencast_callback, NULL);

    int res = 0;
    dvz_event_callback(
        canvas, DVZ_EVENT_SCREENCAST, 0, DVZ_EVENT_MODE_SYNC, _screencast_callback, &res);
    AT(res == 0);

    dvz_screencast(canvas, 1.0 / 30.0, false);

    dvz_app_run(app, 60);

    dvz_canvas_destroy(canvas);
    return res;
}



static void _video_callback(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    cvec4 color = {0};
    dvz_colormap(DVZ_CMAP_HSV, ev.u.t.idx % 256, color);
    dvz_canvas_clear_color(canvas, color[0] / 255.0, color[1] / 255.0, color[2] / 255.0);
}

static void _video_read_callback(int frame_idx, int width, int height, int wrap, uint8_t* image)
{
    log_info("%d", frame_idx);
}

int test_canvas_video(TestContext* tc)
{
    DvzApp* app = tc->app;

    // TODO: fix screencast/video in offscreen mode
    OFFSCREEN_SKIP

    DvzGpu* gpu = dvz_gpu_best(app);
    DvzCanvas* canvas = dvz_canvas(gpu, WIDTH, HEIGHT, 0);
    dvz_canvas_clear_color(canvas, 1, 0, 0);

    dvz_event_callback(canvas, DVZ_EVENT_TIMER, 0.01, DVZ_EVENT_MODE_SYNC, _video_callback, NULL);

#if HAS_FFMPEG
    char path[1024];
    snprintf(path, sizeof(path), "%s/test_canvas_video.mp4", ARTIFACTS_DIR);
    dvz_canvas_video(canvas, 30, 10000000, path, true);

    dvz_app_run(app, 120);
    dvz_canvas_stop(canvas);

    AT(file_exists(path))
    AT(file_size(path) > 4096)

    // // Try to open the video file.
    // Video* video = read_video(path);
    // read_frames(video, _video_read_callback);
    // close_video(video);

#else
    log_warn("skipping ffmpeg video, datoviz was not compiled with ffmpeg support");
#endif

    dvz_canvas_destroy(canvas);
    return 0;
}



/*************************************************************************************************/
/*  Canvas with triangle                                                                         */
/*************************************************************************************************/

int test_canvas_triangle_1(TestContext* tc)
{
    DvzApp* app = tc->app;
    DvzGpu* gpu = dvz_gpu_best(app);
    DvzCanvas* canvas = dvz_canvas(gpu, WIDTH, HEIGHT, 0);
    TestVisual visual = triangle(canvas, "");

    // Bindings and graphics pipeline.
    visual.bindings = dvz_bindings(&visual.graphics.slots, 1);
    dvz_bindings_update(&visual.bindings);
    dvz_graphics_create(&visual.graphics);

    // Triangle data.
    triangle_upload(canvas, &visual);

    // Run.
    dvz_event_callback(canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, triangle_refill, &visual);
    dvz_app_run(app, N_FRAMES);

    // Check screenshot.
    int res = check_canvas(canvas, "test_canvas_triangle_1");

    // Destroy.
    destroy_visual(&visual);
    dvz_canvas_destroy(canvas);

    return res;
}



int test_canvas_triangle_resize(TestContext* tc)
{
    DvzApp* app = tc->app;

    OFFSCREEN_SKIP

    DvzGpu* gpu = dvz_gpu_best(app);
    DvzCanvas* canvas = dvz_canvas(gpu, WIDTH, HEIGHT, 0);
    TestVisual visual = triangle(canvas, "");

    // Bindings and graphics pipeline.
    visual.bindings = dvz_bindings(&visual.graphics.slots, 1);
    dvz_bindings_update(&visual.bindings);
    dvz_graphics_create(&visual.graphics);

    // Triangle data.
    triangle_upload(canvas, &visual);

    // Run.
    dvz_event_callback(canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, triangle_refill, &visual);
    dvz_app_run(app, N_FRAMES);

    // Resize to a smaller size.
    dvz_canvas_resize(canvas, WIDTH / 2, HEIGHT / 2);
    dvz_app_run(app, 20);
    int res = check_canvas(canvas, "test_canvas_triangle_resize_1");

    // Resize to a larger size.
    dvz_app_wait(canvas->app);
    dvz_canvas_resize(canvas, 1000, 1000);
    dvz_app_run(app, 20);
    res = res || check_canvas(canvas, "test_canvas_triangle_resize_2");

    // Destroy.
    destroy_visual(&visual);
    dvz_canvas_destroy(canvas);

    return res;
}



int test_canvas_triangle_offscreen(TestContext* tc)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_OFFSCREEN);
    DvzGpu* gpu = dvz_gpu_best(app);
    DvzCanvas* canvas = dvz_canvas(gpu, WIDTH, HEIGHT, 0);
    TestVisual visual = triangle(canvas, "");

    // Bindings and graphics pipeline.
    visual.bindings = dvz_bindings(&visual.graphics.slots, 1);
    dvz_bindings_update(&visual.bindings);
    dvz_graphics_create(&visual.graphics);

    // Triangle data.
    triangle_upload(canvas, &visual);

    // Run.
    dvz_event_callback(canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, triangle_refill, &visual);
    dvz_app_run(app, N_FRAMES);

    // Check screenshot.
    int res = check_canvas(canvas, "test_canvas_triangle_offscreen");

    // Destroy.
    destroy_visual(&visual);
    dvz_canvas_destroy(canvas);
    dvz_app_destroy(app);

    return res;
}



static void _push_cursor_callback(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    uvec2 size = {0};
    dvz_canvas_size(canvas, DVZ_CANVAS_SIZE_SCREEN, size);
    double x = ev.u.m.pos[0] / (double)size[0];
    double y = ev.u.m.pos[1] / (double)size[1];
    float* push_vec = (float*)canvas->user_data;
    push_vec[0] = x;
    push_vec[1] = y;
    push_vec[2] = 1;
    dvz_canvas_to_refill(canvas);
}

int test_canvas_triangle_push(TestContext* tc)
{
    DvzApp* app = tc->app;
    DvzGpu* gpu = dvz_gpu_best(app);
    DvzCanvas* canvas = dvz_canvas(gpu, WIDTH, HEIGHT, 0);
    dvz_mouse_toggle(&canvas->mouse, false);
    TestVisual visual = triangle(canvas, "_push");

    // Bindings and graphics pipeline.
    dvz_graphics_push(&visual.graphics, 0, sizeof(vec3), VK_SHADER_STAGE_VERTEX_BIT);
    visual.bindings = dvz_bindings(&visual.graphics.slots, 1);
    dvz_bindings_update(&visual.bindings);
    dvz_graphics_create(&visual.graphics);

    // Triangle data.
    triangle_upload(canvas, &visual);

    // Push constant.
    vec3 push_vec = {0};
    visual.graphics.user_data = &push_vec;
    canvas->user_data = &push_vec;

    // Run.
    dvz_event_callback(canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, triangle_refill, &visual);
    dvz_event_callback(
        canvas, DVZ_EVENT_MOUSE_MOVE, 0, DVZ_EVENT_MODE_SYNC, _push_cursor_callback, NULL);
    dvz_app_run(app, N_FRAMES);

    // Move mouse.
    vec2 pos = {WIDTH / 2, HEIGHT / 2};
    dvz_event_mouse_move(canvas, pos, 0);
    dvz_app_run(app, N_FRAMES);

    // Check screenshot.
    int res = check_canvas(canvas, "test_canvas_triangle_push");

    // Destroy.
    destroy_visual(&visual);
    dvz_canvas_destroy(canvas);
    return res;
}



static void _vertex_cursor_callback(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    uvec2 size = {0};
    dvz_canvas_size(canvas, DVZ_CANVAS_SIZE_SCREEN, size);
    double x = ev.u.m.pos[0] / (double)size[0];
    double y = ev.u.m.pos[1] / (double)size[1];

    TestVisual* visual = ev.user_data;
    ASSERT(visual != NULL);

    TestVertex* data = (TestVertex*)visual->data;
    ASSERT(data != NULL);

    for (uint32_t i = 0; i < 3; i++)
    {
        data[i].color[0] = x;
        data[i].color[1] = y;
        data[i].color[2] = 1;
    }
    dvz_upload_buffer(
        canvas->gpu->context->gpu->context, visual->br, 0, 3 * sizeof(TestVertex), data);
}

int test_canvas_triangle_upload(TestContext* tc)
{
    DvzApp* app = tc->app;
    DvzGpu* gpu = dvz_gpu_best(app);
    DvzCanvas* canvas = dvz_canvas(gpu, WIDTH, HEIGHT, 0);
    dvz_mouse_toggle(&canvas->mouse, false);
    TestVisual visual = triangle(canvas, "");

    // Bindings and graphics pipeline.
    visual.bindings = dvz_bindings(&visual.graphics.slots, 1);
    dvz_bindings_update(&visual.bindings);
    dvz_graphics_create(&visual.graphics);

    // Triangle data.
    triangle_upload(canvas, &visual);

    // Run.
    dvz_event_callback(canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, triangle_refill, &visual);
    dvz_event_callback(
        canvas, DVZ_EVENT_MOUSE_MOVE, 0, DVZ_EVENT_MODE_SYNC, _vertex_cursor_callback, &visual);
    dvz_app_run(app, N_FRAMES);

    // Move mouse.
    vec2 pos = {WIDTH / 2, HEIGHT / 2};
    dvz_event_mouse_move(canvas, pos, 0);
    dvz_app_run(app, N_FRAMES);

    // Check screenshot.
    int res = check_canvas(canvas, "test_canvas_triangle_upload");

    // Destroy
    destroy_visual(&visual);
    dvz_canvas_destroy(canvas);
    return res;
}



static void _uniform_cursor_callback(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    uvec2 size = {0};
    dvz_canvas_size(canvas, DVZ_CANVAS_SIZE_SCREEN, size);
    double x = ev.u.m.pos[0] / (double)size[0];
    double y = ev.u.m.pos[1] / (double)size[1];

    float* vec = canvas->user_data;
    ASSERT(vec != NULL);
    vec[0] = x;
    vec[1] = y;
    vec[2] = 1;
    vec[3] = 1;
}

static void _uniform_frame_callback(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    TestVisual* visual = ev.user_data;
    ASSERT(visual != NULL);

    float* vec = canvas->user_data;
    ASSERT(vec != NULL);

    dvz_canvas_buffers(canvas, visual->br_u, 0, sizeof(vec4), vec);
}

int test_canvas_triangle_uniform(TestContext* tc)
{
    DvzApp* app = tc->app;
    DvzGpu* gpu = dvz_gpu_best(app);
    DvzCanvas* canvas = dvz_canvas(gpu, WIDTH, HEIGHT, 0);
    dvz_mouse_toggle(&canvas->mouse, false);
    TestVisual visual = triangle(canvas, "_ubo");

    // Uniform buffer.
    vec4 vec = {1, 0, 1, 1};
    canvas->user_data = (void*)vec;
    dvz_graphics_slot(&visual.graphics, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    visual.br_u = dvz_ctx_buffers(
        gpu->context, DVZ_BUFFER_TYPE_UNIFORM_MAPPABLE, canvas->swapchain.img_count, sizeof(vec4));
    ASSERT(visual.br_u.aligned_size >= visual.br_u.size);

    // Bindings and graphics pipeline.
    visual.bindings = dvz_bindings(&visual.graphics.slots, canvas->swapchain.img_count);
    dvz_bindings_buffer(&visual.bindings, 0, visual.br_u);
    dvz_bindings_update(&visual.bindings);
    dvz_graphics_create(&visual.graphics);

    // Triangle data.
    triangle_upload(canvas, &visual);

    // Run.
    dvz_event_callback(canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, triangle_refill, &visual);
    dvz_event_callback(
        canvas, DVZ_EVENT_MOUSE_MOVE, 0, DVZ_EVENT_MODE_SYNC, _uniform_cursor_callback, &visual);
    dvz_event_callback(
        canvas, DVZ_EVENT_FRAME, 0, DVZ_EVENT_MODE_SYNC, _uniform_frame_callback, &visual);
    dvz_app_run(app, N_FRAMES);

    // Move mouse.
    vec2 pos = {WIDTH / 2, HEIGHT / 2};
    dvz_event_mouse_move(canvas, pos, 0);
    dvz_app_run(app, N_FRAMES);

    // Check screenshot.
    int res = check_canvas(canvas, "test_canvas_triangle_uniform");

    // Destroy.
    destroy_visual(&visual);
    dvz_canvas_destroy(canvas);
    return res;
}



// dir=0: read then write
// dir=1: write then read
static void _buffer_barrier(TestVisual* visual, DvzCommands* cmds, uint32_t idx, int dir)
{
    VkPipelineStageFlags stages[] = {
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT};
    VkPipelineStageFlags bef = stages[dir];
    VkPipelineStageFlags aft = stages[1 - dir];

    VkAccessFlags access[] = {VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_MEMORY_WRITE_BIT};
    VkAccessFlags src = access[dir];
    VkAccessFlags dst = access[1 - dir];

    DvzBarrier barrier = dvz_barrier(visual->gpu);
    dvz_barrier_stages(&barrier, bef, aft);
    dvz_barrier_buffer(&barrier, visual->br);
    dvz_barrier_buffer_access(&barrier, src, dst);
    dvz_cmd_barrier(cmds, idx, &barrier);
}

static void triangle_refill_compute(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(ev.u.rf.cmd_count == 1);
    DvzCommands* cmds = ev.u.rf.cmds[0];
    ASSERT(cmds->queue_idx == DVZ_DEFAULT_QUEUE_RENDER);
    uint32_t idx = ev.u.rf.img_idx;

    TestVisual* visual = (TestVisual*)ev.user_data;
    ASSERT(visual != NULL);

    dvz_cmd_begin(cmds, idx);

    _buffer_barrier(visual, cmds, idx, 0);
    dvz_cmd_compute(cmds, idx, visual->compute, (uvec3){3, 1, 1});
    _buffer_barrier(visual, cmds, idx, 1);

    dvz_cmd_begin_renderpass(cmds, idx, &canvas->renderpass, &canvas->framebuffers);
    dvz_cmd_viewport(cmds, idx, canvas->viewport.viewport);
    dvz_cmd_bind_vertex_buffer(cmds, idx, visual->br, 0);
    dvz_cmd_bind_graphics(cmds, idx, &visual->graphics, &visual->bindings, 0);
    dvz_cmd_draw(cmds, idx, 0, 3);
    dvz_cmd_end_renderpass(cmds, idx);

    dvz_cmd_end(cmds, idx);
}

int test_canvas_triangle_compute(TestContext* tc)
{
    DvzApp* app = tc->app;
    DvzGpu* gpu = dvz_gpu_best(app);
    DvzCanvas* canvas = dvz_canvas(gpu, WIDTH, HEIGHT, 0);
    dvz_mouse_toggle(&canvas->mouse, false);
    TestVisual visual = triangle(canvas, "");

    // Bindings and graphics pipeline.
    visual.bindings = dvz_bindings(&visual.graphics.slots, 1);
    dvz_bindings_update(&visual.bindings);
    dvz_graphics_create(&visual.graphics);

    // Triangle data.
    triangle_upload(canvas, &visual);

    // Create compute object.
    DvzBindings bindings = {0};
    {
        char path[1024];
        snprintf(path, sizeof(path), "%s/test_triangle.comp.spv", SPIRV_DIR);
        visual.compute = dvz_ctx_compute(gpu->context, path);
        dvz_compute_slot(visual.compute, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        bindings = dvz_bindings(&visual.compute->slots, 1);
        dvz_bindings_buffer(&bindings, 0, visual.br);
        dvz_bindings_update(&bindings);
        dvz_compute_bindings(visual.compute, &bindings);
        dvz_compute_create(visual.compute);
    }

    // Run.
    dvz_event_callback(
        canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, triangle_refill_compute, &visual);
    dvz_app_run(app, 20);

    // Check screenshot.
    int res = check_canvas(canvas, "test_canvas_triangle_compute");

    // Destroy.
    dvz_bindings_destroy(&bindings);
    destroy_visual(&visual);
    dvz_canvas_destroy(canvas);
    return res;
}



static void _triangle_click(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);

    // Mouse coordinates.
    uvec2 pos = {0};
    pos[0] = (uint32_t)round(ev.u.c.pos[0]);
    pos[1] = (uint32_t)round(ev.u.c.pos[1]);

    ASSERT(canvas->user_data != NULL);
    dvz_canvas_pick(canvas, pos, canvas->user_data);
    int32_t* picked = canvas->user_data;
    log_info("picked %d %d %d %d", picked[0], picked[1], picked[2], picked[3]);
}

int test_canvas_triangle_pick(TestContext* tc)
{
    DvzApp* app = tc->app;
    OFFSCREEN_SKIP

    DvzGpu* gpu = dvz_gpu_best(app);
    DvzCanvas* canvas =
        dvz_canvas(gpu, WIDTH, HEIGHT, DVZ_CANVAS_FLAGS_FPS | DVZ_CANVAS_FLAGS_PICK);
    dvz_mouse_toggle(&canvas->mouse, false);
    TestVisual visual = triangle(canvas, "_pick");

    // Bindings and graphics pipeline.
    visual.bindings = dvz_bindings(&visual.graphics.slots, 1);
    dvz_bindings_update(&visual.bindings);
    dvz_graphics_pick(&visual.graphics, true);
    dvz_graphics_create(&visual.graphics);

    // Triangle data.
    triangle_upload(canvas, &visual);

    // Run.
    ivec4 picked = {0};
    canvas->user_data = (void*)picked;
    dvz_event_callback(canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, triangle_refill, &visual);
    dvz_event_callback(
        canvas, DVZ_EVENT_MOUSE_CLICK, 0, DVZ_EVENT_MODE_SYNC, _triangle_click, &visual);
    dvz_app_run(app, N_FRAMES);

    // Test picking.
    // ivec4 exp = {0};
    float w = (float)canvas->swapchain.images[0].width;
    float h = (float)canvas->swapchain.images[0].height;
    ASSERT(w > 0 && h > 0);

    // TODO: multiple clicks (doesn't work for now)

    dvz_event_mouse_click(canvas, (vec2){w / 2, h / 2}, DVZ_MOUSE_BUTTON_LEFT, 0);
    AIN(picked[1], 60, 68);
    AIN(picked[2], 60, 68);
    AIN(picked[3], 123, 131);

    // Destroy.
    destroy_visual(&visual);
    dvz_canvas_destroy(canvas);
    return 0;
}



static void triangle_append(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    TestVisual* visual = ev.user_data;
    ASSERT(visual != NULL);

    if (ev.u.f.idx >= 8)
        return;

    const uint32_t N = 3 + ev.u.f.idx;
    visual->n_vertices = 3 * N;
    TestVertex* data = calloc(visual->n_vertices, sizeof(TestVertex));
    float t = 0, t2 = 0;
    for (uint32_t i = 0; i < N; i++)
    {
        t = i / (float)N;
        t2 = (i + 1) / (float)N;

        data[3 * i + 0].color[0] = 1;
        data[3 * i + 0].color[3] = 1;

        data[3 * i + 1].color[1] = 1;
        data[3 * i + 1].color[3] = 1;
        data[3 * i + 1].pos[0] = .5 * cos(M_2PI * t);
        data[3 * i + 1].pos[1] = .5 * sin(M_2PI * t);

        data[3 * i + 2].color[2] = 1;
        data[3 * i + 2].color[3] = 1;
        data[3 * i + 2].pos[0] = .5 * cos(M_2PI * t2);
        data[3 * i + 2].pos[1] = .5 * sin(M_2PI * t2);
    }
    FREE(visual->data);
    visual->data = data;
    VkDeviceSize size = visual->n_vertices * sizeof(TestVertex);
    visual->br = dvz_ctx_buffers(canvas->gpu->context, DVZ_BUFFER_TYPE_VERTEX, 1, size);
    dvz_upload_buffer(canvas->gpu->context, visual->br, 0, size, data);

    // NOTE: important, we need to refill the canvas after the vertex count has changed.
    dvz_canvas_to_refill(canvas);
}

int test_canvas_triangle_append(TestContext* tc)
{
    DvzApp* app = tc->app;
    DvzGpu* gpu = dvz_gpu_best(app);
    DvzCanvas* canvas = dvz_canvas(gpu, WIDTH, HEIGHT, 0);
    TestVisual visual = triangle(canvas, "");
    visual.n_vertices = 3;

    // Bindings and graphics pipeline.
    visual.bindings = dvz_bindings(&visual.graphics.slots, 1);
    dvz_bindings_update(&visual.bindings);
    dvz_graphics_create(&visual.graphics);

    // Triangle data.
    triangle_upload(canvas, &visual);

    // Run.
    dvz_event_callback(canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, triangle_refill, &visual);
    dvz_event_callback(canvas, DVZ_EVENT_FRAME, 0, DVZ_EVENT_MODE_SYNC, triangle_append, &visual);

    dvz_app_run(app, 30);

    // Check screenshot.
    int res = check_canvas(canvas, "test_canvas_triangle_append");

    // Destroy.
    destroy_visual(&visual);
    dvz_canvas_destroy(canvas);
    return res;
}
