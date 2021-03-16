#include "../include/datoviz/canvas.h"
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



/*************************************************************************************************/
/*  Blank canvas                                                                                 */
/*************************************************************************************************/

int test_canvas_blank(TestContext* tc)
{
    DvzApp* app = tc->app;
    DvzGpu* gpu = dvz_gpu_best(app);

    DvzCanvas* canvas = dvz_canvas(gpu, WIDTH, HEIGHT, 0);
    ASSERT(canvas->window != NULL);
    ASSERT(canvas->app != NULL);
    ASSERT(canvas->window->app != NULL);

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
    DvzGpu* gpu = dvz_gpu_best(app);

    DvzCanvas* canvas0 = dvz_canvas(gpu, WIDTH, HEIGHT, 0);
    DvzCanvas* canvas1 = dvz_canvas(gpu, WIDTH, HEIGHT, 0);

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
    log_debug(
        "canvas #%d, frame callback #%d, time %.6f, interval %.6f", //
        canvas->obj.id, ev.u.f.idx, ev.u.f.time, ev.u.f.interval);
    ASSERT(canvas != NULL);
    EventHolder* events = (EventHolder*)ev.user_data;
    ASSERT(events != NULL);
    events->frame = ev;
}

static void _key_callback(DvzCanvas* canvas, DvzEvent ev)
{
    log_debug("key code %d", ev.u.k.key_code);
    ASSERT(canvas != NULL);
    EventHolder* events = (EventHolder*)ev.user_data;
    ASSERT(events != NULL);
    events->key = ev;
}



int test_canvas_events(TestContext* tc)
{
    DvzApp* app = tc->app;
    DvzGpu* gpu = dvz_gpu_best(app);
    DvzCanvas* canvas = dvz_canvas(gpu, WIDTH, HEIGHT, 0);

    EventHolder events = {0};
    dvz_event_callback( //
        canvas, DVZ_EVENT_INIT, 0, DVZ_EVENT_MODE_SYNC, _init_callback, &events);
    dvz_event_callback( //
        canvas, DVZ_EVENT_TIMER, .001, DVZ_EVENT_MODE_SYNC, _timer_callback, &events);
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



/*************************************************************************************************/
/*  Canvas with triangle                                                                         */
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
    visual->data = (void*)data;
    dvz_upload_buffers(canvas, visual->br, 0, size, data);
}

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

    // Destroy.
    destroy_visual(&visual);
    dvz_canvas_destroy(canvas);
    return 0;
}



static void _push_cursor_callback(DvzCanvas* canvas, DvzEvent ev)
{
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

    TestVisual visual = triangle_visual(gpu, &canvas->renderpass, &canvas->framebuffers, "_push");
    visual.br.buffer = &visual.buffer;
    visual.br.size = visual.buffer.size;
    visual.br.count = 1;

    vec3 push_vec = {0};
    visual.graphics.user_data = &push_vec;
    canvas->user_data = &push_vec;

    dvz_event_callback(canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, triangle_refill, &visual);
    dvz_event_callback(
        canvas, DVZ_EVENT_MOUSE_MOVE, 0, DVZ_EVENT_MODE_SYNC, _push_cursor_callback, NULL);

    dvz_app_run(app, N_FRAMES);

    destroy_visual(&visual);
    dvz_canvas_destroy(canvas);
    return 0;
}



static void _vertex_cursor_callback(DvzCanvas* canvas, DvzEvent ev)
{
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
    dvz_upload_buffers(canvas, visual->br, 0, 3 * sizeof(TestVertex), data);
}

int test_canvas_triangle_upload(TestContext* tc)
{
    DvzApp* app = tc->app;
    DvzGpu* gpu = dvz_gpu_best(app);
    DvzCanvas* canvas = dvz_canvas(gpu, WIDTH, HEIGHT, 0);

    TestVisual visual = triangle_visual(gpu, &canvas->renderpass, &canvas->framebuffers, "");
    visual.br.buffer = &visual.buffer;
    visual.br.size = visual.buffer.size;
    visual.br.count = 1;

    TestVertex data[] = TRIANGLE_VERTICES;
    visual.data = (void*)data;

    dvz_event_callback(canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, triangle_refill, &visual);
    dvz_event_callback(
        canvas, DVZ_EVENT_MOUSE_MOVE, 0, DVZ_EVENT_MODE_SYNC, _vertex_cursor_callback, &visual);

    dvz_app_run(app, N_FRAMES);

    destroy_visual(&visual);
    dvz_canvas_destroy(canvas);
    return 0;
}



int test_canvas_triangle_uniform(TestContext* tc)
{
    // DvzApp* app = tc->app;
    // DvzGpu* gpu = dvz_gpu_best(app);
    // DvzCanvas* canvas = dvz_canvas(gpu, WIDTH, HEIGHT, 0);

    // TestVisual visual = triangle_visual(gpu, &canvas->renderpass, &canvas->framebuffers,
    // "_ubo"); visual.br.buffer = &visual.buffer; visual.br.size = visual.buffer.size;
    // visual.br.count = 1;

    // TestVertex data[] = TRIANGLE_VERTICES;
    // visual.data = (void*)data;

    // dvz_event_callback(canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, triangle_refill,
    // &visual); dvz_event_callback(
    //     canvas, DVZ_EVENT_MOUSE_MOVE, 0, DVZ_EVENT_MODE_SYNC, _vertex_cursor_callback, &visual);

    // dvz_app_run(app, N_FRAMES);

    // destroy_visual(&visual);
    // dvz_canvas_destroy(canvas);
    return 0;
}
