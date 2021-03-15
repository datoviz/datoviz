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



/*************************************************************************************************/
/*  Test canvas                                                                                  */
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
        canvas, DVZ_EVENT_TIMER, .05, DVZ_EVENT_MODE_SYNC, _timer_callback, &events);
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

    return 0;
}