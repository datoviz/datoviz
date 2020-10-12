#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/visky/visky.h"

// VNC callbacks
#if HAS_VNC
#include <rfb/rfb.h>

static vec2 mouse_pos;
static int mouse_button;

static void _vnc_mouse_callback(int button, int x, int y, rfbClientPtr cl)
{
    switch (button)
    {
    case 1:
        button = VKY_MOUSE_BUTTON_LEFT;
        break;
    case 4:
        button = VKY_MOUSE_BUTTON_RIGHT;
        break;
    default:
        button = 0;
        break;
    }
    mouse_pos[0] = x;
    mouse_pos[1] = y;
    mouse_button = button;
}

static void _vnc_keyboard_callback(rfbBool down, rfbKeySym key, rfbClientPtr cl)
{
    // TODO
}

static void _vnc_close_client(rfbClientPtr cl) {}

static enum rfbNewClientAction _vnc_new_client(rfbClientPtr cl)
{
    cl->clientData = cl->screen->screenData;
    cl->clientGoneHook = _vnc_close_client;
    return RFB_CLIENT_ACCEPT;
}

static void mouse_input(VkyCanvas* canvas)
{
    vky_update_mouse_state(canvas->event_controller->mouse, mouse_pos, mouse_button);
}

static void run_vnc(VkyCanvas* canvas)
{
    VkyScreenshot* screenshot = vky_create_screenshot(canvas);
    rfbScreenInfoPtr server = rfbGetScreen(
        NULL, NULL, (int)canvas->size.window_width, (int)canvas->size.window_height, 8, 4, 4);
    if (!server)
        return;
    server->frameBuffer = (char*)screenshot->image;
    rfbInitServer(server);

    server->screenData = (void*)canvas;
    server->ptrAddEvent = _vnc_mouse_callback;
    server->kbdAddEvent = _vnc_keyboard_callback;
    server->newClientHook = _vnc_new_client;

    vky_add_mock_input_callback(canvas, mouse_input);

    vky_fill_command_buffers(canvas);
    vky_offscreen_frame(canvas, VKY_TIME);

    // Event loop.
    int usec = server->deferUpdateTime * 1000;
    while (rfbIsActive(server))
    {
        vky_begin_screenshot(screenshot);
        vky_offscreen_frame(canvas, VKY_TIME);
        vky_end_screenshot(screenshot);
        rfbMarkRectAsModified(
            server, 0, 0, (int)canvas->size.window_width, (int)canvas->size.window_height);
        rfbProcessEvents(server, usec);
        canvas->frame_count++;
    }

    vky_destroy_screenshot(screenshot);
}

#endif

int main()
{
    log_set_level_env();

#if HAS_VNC
    VkyApp* app = vky_create_app(VKY_BACKEND_OFFSCREEN, NULL);
#else
    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    log_error("libvnc not available, VNC backend unavailable and replaced by default backend");
#endif

    VkyCanvas* canvas = vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_BLACK, 1, 1);

    // Set the panel controllers.
    VkyPanel* panel = vky_get_panel(scene, 0, 0);
    vky_set_controller(panel, VKY_CONTROLLER_PANZOOM, NULL);

    // Create the visual.
    VkyVisual* visual = vky_visual(panel->scene, VKY_VISUAL_MESH_RAW, NULL, NULL);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Upload the data.
    float x = .5;
    VkyVertex vertices[] = {
        {{-x, -x, 0}, {{255, 0, 0}, 255}},
        {{+x, -x, 0}, {{0, 255, 0}, 255}},
        {{0, x, 0}, {{0, 0, 255}, 255}},
    };
    vky_visual_data_raw(visual, (VkyData){0, NULL, 3, vertices, 0, NULL});

#if HAS_VNC
    run_vnc(canvas);
#else
    vky_run_app(app);
#endif

    vky_destroy_app(app);
    return 0;
}
