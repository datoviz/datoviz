/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#include "hosted_qt_adapter.h"

#include <QByteArray>
#include <QCloseEvent>
#include <QExposeEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPlatformSurfaceEvent>
#include <QPoint>
#include <QPointF>
#include <QResizeEvent>
#include <QSize>
#include <QWheelEvent>

#include <cmath>
#include <cstdio>

#include "datoviz/window/backend.h"



static void _request_frame_callback(DvzView* win, void* user_data);



static int _mods(Qt::KeyboardModifiers modifiers)
{
    int mods = DVZ_KEY_MODIFIER_NONE;
    if (modifiers.testFlag(Qt::ShiftModifier))
        mods |= DVZ_KEY_MODIFIER_SHIFT;
    if (modifiers.testFlag(Qt::ControlModifier))
        mods |= DVZ_KEY_MODIFIER_CONTROL;
    if (modifiers.testFlag(Qt::AltModifier))
        mods |= DVZ_KEY_MODIFIER_ALT;
    if (modifiers.testFlag(Qt::MetaModifier))
        mods |= DVZ_KEY_MODIFIER_SUPER;
    return mods;
}



static DvzPointerButton _button(Qt::MouseButton button)
{
    switch (button)
    {
    case Qt::LeftButton:
        return DVZ_POINTER_BUTTON_LEFT;
    case Qt::MiddleButton:
        return DVZ_POINTER_BUTTON_MIDDLE;
    case Qt::RightButton:
        return DVZ_POINTER_BUTTON_RIGHT;
    default:
        return DVZ_POINTER_BUTTON_NONE;
    }
}



static QPointF _wheel_steps(QWheelEvent* event)
{
    const QPoint angle_delta = event->angleDelta();
    if (!angle_delta.isNull())
        return QPointF((double)angle_delta.x() / 120.0, (double)angle_delta.y() / 120.0);

    const QPoint pixel_delta = event->pixelDelta();
    return QPointF((double)pixel_delta.x() / 120.0, (double)pixel_delta.y() / 120.0);
}



static DvzKeyCode _key(int key)
{
    if (key >= Qt::Key_A && key <= Qt::Key_Z)
        return (DvzKeyCode)(DVZ_KEY_A + (key - Qt::Key_A));
    if (key >= Qt::Key_0 && key <= Qt::Key_9)
        return (DvzKeyCode)(DVZ_KEY_0 + (key - Qt::Key_0));
    if (key >= Qt::Key_F1 && key <= Qt::Key_F25)
        return (DvzKeyCode)(DVZ_KEY_F1 + (key - Qt::Key_F1));

    switch (key)
    {
    case Qt::Key_Space:
        return DVZ_KEY_SPACE;
    case Qt::Key_Apostrophe:
        return DVZ_KEY_APOSTROPHE;
    case Qt::Key_Comma:
        return DVZ_KEY_COMMA;
    case Qt::Key_Minus:
        return DVZ_KEY_MINUS;
    case Qt::Key_Period:
        return DVZ_KEY_PERIOD;
    case Qt::Key_Slash:
        return DVZ_KEY_SLASH;
    case Qt::Key_Semicolon:
        return DVZ_KEY_SEMICOLON;
    case Qt::Key_Equal:
        return DVZ_KEY_EQUAL;
    case Qt::Key_BracketLeft:
        return DVZ_KEY_LEFT_BRACKET;
    case Qt::Key_Backslash:
        return DVZ_KEY_BACKSLASH;
    case Qt::Key_BracketRight:
        return DVZ_KEY_RIGHT_BRACKET;
    case Qt::Key_QuoteLeft:
        return DVZ_KEY_GRAVE_ACCENT;
    case Qt::Key_Escape:
        return DVZ_KEY_ESCAPE;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        return DVZ_KEY_ENTER;
    case Qt::Key_Tab:
        return DVZ_KEY_TAB;
    case Qt::Key_Backspace:
        return DVZ_KEY_BACKSPACE;
    case Qt::Key_Insert:
        return DVZ_KEY_INSERT;
    case Qt::Key_Delete:
        return DVZ_KEY_DELETE;
    case Qt::Key_Right:
        return DVZ_KEY_RIGHT;
    case Qt::Key_Left:
        return DVZ_KEY_LEFT;
    case Qt::Key_Down:
        return DVZ_KEY_DOWN;
    case Qt::Key_Up:
        return DVZ_KEY_UP;
    case Qt::Key_PageUp:
        return DVZ_KEY_PAGE_UP;
    case Qt::Key_PageDown:
        return DVZ_KEY_PAGE_DOWN;
    case Qt::Key_Home:
        return DVZ_KEY_HOME;
    case Qt::Key_End:
        return DVZ_KEY_END;
    case Qt::Key_CapsLock:
        return DVZ_KEY_CAPS_LOCK;
    case Qt::Key_ScrollLock:
        return DVZ_KEY_SCROLL_LOCK;
    case Qt::Key_NumLock:
        return DVZ_KEY_NUM_LOCK;
    case Qt::Key_Print:
        return DVZ_KEY_PRINT_SCREEN;
    case Qt::Key_Pause:
        return DVZ_KEY_PAUSE;
    case Qt::Key_Shift:
        return DVZ_KEY_LEFT_SHIFT;
    case Qt::Key_Control:
        return DVZ_KEY_LEFT_CONTROL;
    case Qt::Key_Alt:
        return DVZ_KEY_LEFT_ALT;
    case Qt::Key_Meta:
        return DVZ_KEY_LEFT_SUPER;
    case Qt::Key_Menu:
        return DVZ_KEY_MENU;
    default:
        return DVZ_KEY_UNKNOWN;
    }
}



static bool _contains_extension(
    const QVulkanInfoVector<QVulkanExtension>& supported, const char* extension)
{
    return supported.contains(QByteArray(extension));
}



static bool _append_extension(
    const QVulkanInfoVector<QVulkanExtension>& supported, const char* extension,
    std::vector<const char*>* extensions, const char* log_name)
{
    if (!_contains_extension(supported, extension))
    {
        std::fprintf(stderr, "%s: missing Vulkan extension %s\n", log_name, extension);
        return false;
    }
    extensions->push_back(extension);
    return true;
}



static void _request_frame_callback(DvzView* win, void* user_data)
{
    (void)win;
    DvzQtHostedWindow* window = static_cast<DvzQtHostedWindow*>(user_data);
    if (window != nullptr)
        window->schedule_frame();
}



DvzQtHostedWindow::DvzQtHostedWindow(
    DvzApp* app, DvzFigure* figure, DvzPanel* panel, QVulkanInstance* instance,
    const QString& title, const QSize& initial_size)
    : _app(app), _figure(figure), _panel(panel), _instance(instance)
{
    setSurfaceType(QSurface::VulkanSurface);
    setVulkanInstance(_instance);
    setTitle(title);
    resize(initial_size);
}



DvzQtHostedWindow::~DvzQtHostedWindow()
{
    release_surface();
}



void DvzQtHostedWindow::schedule_frame()
{
    _repaint_requested = true;
    _request_count++;
    requestUpdate();
}



void DvzQtHostedWindow::request_scene_frame()
{
    if (_view != nullptr)
        dvz_view_request_frame(_view);
    else
        requestUpdate();
}



void DvzQtHostedWindow::release_surface()
{
    if (_view == nullptr)
        return;

    (void)dvz_view_release_external_surface(_view);

    _view = nullptr;
    _app = nullptr;
}



void DvzQtHostedWindow::set_wheel_scale(float scale)
{
    _wheel_scale = scale > 0.0f ? scale : 1.0f;
}



DvzView* DvzQtHostedWindow::view() const
{
    return _view;
}



uint32_t DvzQtHostedWindow::request_count() const
{
    return _request_count;
}



bool DvzQtHostedWindow::event(QEvent* event)
{
    if (event->type() == QEvent::PlatformSurface)
    {
        QPlatformSurfaceEvent* surface_event = static_cast<QPlatformSurfaceEvent*>(event);
        if (surface_event->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
            release_surface();
    }
    if (event->type() == QEvent::UpdateRequest)
    {
        _render_once();
        return true;
    }
    return QWindow::event(event);
}



void DvzQtHostedWindow::exposeEvent(QExposeEvent* event)
{
    (void)event;
    if (isExposed())
        requestUpdate();
}



void DvzQtHostedWindow::resizeEvent(QResizeEvent* event)
{
    QWindow::resizeEvent(event);
    _emit_resize();
}



void DvzQtHostedWindow::closeEvent(QCloseEvent* event)
{
    release_surface();
    QWindow::closeEvent(event);
}



void DvzQtHostedWindow::mouseMoveEvent(QMouseEvent* event)
{
    _emit_pointer(DVZ_POINTER_EVENT_MOVE, event, DVZ_POINTER_BUTTON_NONE);
}



void DvzQtHostedWindow::mousePressEvent(QMouseEvent* event)
{
    _emit_pointer(DVZ_POINTER_EVENT_PRESS, event, _button(event->button()));
}



void DvzQtHostedWindow::mouseReleaseEvent(QMouseEvent* event)
{
    _emit_pointer(DVZ_POINTER_EVENT_RELEASE, event, _button(event->button()));
}



void DvzQtHostedWindow::wheelEvent(QWheelEvent* event)
{
    if (_view == nullptr)
        return;

    const QPointF pos = event->position();
    const QSize win_size = size();
    const QPointF wheel = _wheel_steps(event) * (double)_wheel_scale;

    (void)dvz_view_emit_wheel(
        _view, (float)pos.x(), (float)pos.y(), (float)win_size.width(),
        (float)win_size.height(), (float)wheel.x(), (float)wheel.y(), _mods(event->modifiers()));
}



void DvzQtHostedWindow::keyPressEvent(QKeyEvent* event)
{
    _emit_key(event->isAutoRepeat() ? DVZ_KEYBOARD_EVENT_REPEAT : DVZ_KEYBOARD_EVENT_PRESS, event);
}



void DvzQtHostedWindow::keyReleaseEvent(QKeyEvent* event)
{
    _emit_key(DVZ_KEYBOARD_EVENT_RELEASE, event);
}



void DvzQtHostedWindow::frame_rendered(int frame_status)
{
    (void)frame_status;
}



uint32_t DvzQtHostedWindow::_positive_u32(int value)
{
    return value > 0 ? (uint32_t)value : 0;
}



DvzWindowExternalSurfaceInfo DvzQtHostedWindow::_surface_info(VkSurfaceKHR surface) const
{
    const double scale = devicePixelRatio();
    const QSize win_size = size();
    const int fb_width = (int)std::lround((double)win_size.width() * scale);
    const int fb_height = (int)std::lround((double)win_size.height() * scale);
    DvzWindowExternalSurfaceInfo info = dvz_window_external_surface_info();
    info.instance = _instance != nullptr ? _instance->vkInstance() : VK_NULL_HANDLE;
    info.surface = surface;
    info.extent.width = _positive_u32(fb_width);
    info.extent.height = _positive_u32(fb_height);
    info.scale_x = scale > 0.0 ? (float)scale : 1.0f;
    info.scale_y = scale > 0.0 ? (float)scale : 1.0f;
    info.owned_by_datoviz = false;
    return info;
}



bool DvzQtHostedWindow::_ensure_initialized()
{
    if (_view != nullptr)
        return true;
    if (_app == nullptr || _figure == nullptr || _instance == nullptr || !_instance->isValid())
        return false;

    VkSurfaceKHR surface = QVulkanInstance::surfaceForWindow(this);
    if (surface == VK_NULL_HANDLE)
        return false;

    DvzWindowExternalSurfaceInfo info = _surface_info(surface);
    _view = dvz_view_external_surface(_app, _figure, &info);
    if (_view == nullptr)
        return false;

    dvz_view_set_request_frame_callback(_view, _request_frame_callback, this);
    if (_panel != nullptr)
    {
        DvzController* controller = dvz_panzoom(dvz_figure_scene(_figure), NULL);
        if (controller != NULL &&
            dvz_panel_bind_controller(_panel, controller, DVZ_DIM_MASK_XY) == 0)
        {
            (void)dvz_panel_connect_input(_panel, dvz_view_input(_view));
        }
    }
    _emit_resize();
    return true;
}



void DvzQtHostedWindow::_emit_resize()
{
    if (_view == nullptr)
        return;

    const double scale = devicePixelRatio();
    const QSize win_size = size();
    const int fb_width = (int)std::lround((double)win_size.width() * scale);
    const int fb_height = (int)std::lround((double)win_size.height() * scale);

    (void)dvz_view_emit_resize(
        _view, _positive_u32(fb_width), _positive_u32(fb_height),
        _positive_u32(win_size.width()), _positive_u32(win_size.height()),
        scale > 0.0 ? (float)scale : 1.0f, scale > 0.0 ? (float)scale : 1.0f);
}



void DvzQtHostedWindow::_emit_pointer(
    DvzPointerEventType type, QMouseEvent* event, DvzPointerButton button)
{
    if (_view == nullptr)
        return;

    const QPointF pos = event->position();
    const QSize win_size = size();
    (void)dvz_view_emit_pointer(
        _view, type, (float)pos.x(), (float)pos.y(), (float)win_size.width(),
        (float)win_size.height(), button, _mods(event->modifiers()));
}



void DvzQtHostedWindow::_emit_key(DvzKeyboardEventType type, QKeyEvent* event)
{
    if (_view == nullptr)
        return;

    const DvzKeyCode key = _key(event->key());
    if (key == DVZ_KEY_UNKNOWN)
        return;
    (void)dvz_view_emit_key(_view, type, key, _mods(event->modifiers()));
}



void DvzQtHostedWindow::_render_once()
{
    if (!isExposed())
        return;
    if (!_ensure_initialized())
    {
        requestUpdate();
        return;
    }

    VkSurfaceKHR surface = QVulkanInstance::surfaceForWindow(this);
    DvzWindowExternalSurfaceInfo info = _surface_info(surface);
    if (dvz_view_update_external_surface(_view, &info) != 0)
    {
        std::fprintf(stderr, "hosted_qt: surface update failed\n");
        close();
        return;
    }

    if (!_repaint_requested)
        return;
    _repaint_requested = false;

    const int rc = dvz_view_render_once(_view);
    if (rc < 0)
    {
        std::fprintf(stderr, "hosted_qt: render failed (%d)\n", rc);
        close();
        return;
    }
    frame_rendered(rc);
}



bool dvz_qt_instance_extensions(
    const QGuiApplication& app, std::vector<const char*>* extensions, const char* log_name)
{
    QVulkanInstance probe;
    const QVulkanInfoVector<QVulkanExtension> supported = probe.supportedExtensions();
    if (!_append_extension(supported, "VK_KHR_surface", extensions, log_name))
        return false;

    const QByteArray platform = app.platformName().toUtf8().toLower();
#if defined(Q_OS_WIN)
    (void)platform;
    return _append_extension(supported, "VK_KHR_win32_surface", extensions, log_name);
#elif defined(Q_OS_MACOS)
    (void)platform;
    if (_contains_extension(supported, "VK_EXT_metal_surface"))
        return _append_extension(supported, "VK_EXT_metal_surface", extensions, log_name);
    return _append_extension(supported, "VK_MVK_macos_surface", extensions, log_name);
#elif defined(Q_OS_LINUX)
    if (platform.startsWith("wayland"))
        return _append_extension(supported, "VK_KHR_wayland_surface", extensions, log_name);
    return _append_extension(supported, "VK_KHR_xcb_surface", extensions, log_name);
#else
    std::fprintf(stderr, "%s: unsupported Qt platform %s\n", log_name, platform.constData());
    return false;
#endif
}
