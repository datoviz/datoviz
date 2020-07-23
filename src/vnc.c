#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if HAS_VNC
#include <rfb/rfb.h>
#endif

#include "../include/visky/visky.h"
#include "vnc.h"



#if HAS_VNC
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
    VkyCanvas* canvas = (VkyCanvas*)cl->clientData;
    VkyBackendVNCParams* params = (VkyBackendVNCParams*)canvas->app->backend_params;
    params->mouse_pos[0] = x;
    params->mouse_pos[1] = y;
    params->mouse_button = (uint32_t)button;
}

static void _vnc_keyboard_callback(rfbBool down, rfbKeySym key, rfbClientPtr cl)
{
    // TODO: keyboard support
}

static void _vnc_close_client(rfbClientPtr cl) {}

static enum rfbNewClientAction _vnc_new_client(rfbClientPtr cl)
{
    // TODO: one different canvas per client?
    cl->clientData = cl->screen->screenData;
    cl->clientGoneHook = _vnc_close_client;
    return RFB_CLIENT_ACCEPT;
}

void vky_run_vnc_app(VkyApp* app)
{
    // NOTE: only 1 canvas is supported here.
    VkyCanvas* canvas = app->canvases[0];

    VkyScreenshot* screenshot = vky_create_screenshot(canvas);
    rfbScreenInfoPtr server = rfbGetScreen(
        NULL, NULL, (int)canvas->size.window_width, (int)canvas->size.window_height, 8, 4, 4);
    if (!server)
        return;
    VkyBackendVNCParams* params = (VkyBackendVNCParams*)canvas->app->backend_params;
    if (params == NULL)
    {
        log_trace("create default backend VNC params object");
        params = canvas->app->backend_params = calloc(1, sizeof(VkyBackendVNCParams));
    }
    params->server = server;
    params->screenshot = screenshot;
    server->frameBuffer = (char*)screenshot->image;
    rfbInitServer(server);

    server->screenData = (void*)canvas;
    server->ptrAddEvent = _vnc_mouse_callback;
    server->kbdAddEvent = _vnc_keyboard_callback;
    server->newClientHook = _vnc_new_client;

    vky_fill_command_buffers(canvas);
    vky_offscreen_frame(canvas, VKY_DEFAULT_TIMER);

    // Event loop.
    int usec = server->deferUpdateTime * 1000;
    while (rfbIsActive(server))
    {
        vky_begin_screenshot(screenshot);
        vky_offscreen_frame(canvas, VKY_DEFAULT_TIMER);
        vky_end_screenshot(screenshot);
        rfbMarkRectAsModified(
            server, 0, 0, (int)canvas->size.window_width, (int)canvas->size.window_height);
        rfbProcessEvents(server, usec);
        canvas->frame_count++;
    }
}

// No VNC support

#else

void vky_run_vnc_app(VkyApp* app) { log_error("visky was built without libvncserver"); }

#endif
