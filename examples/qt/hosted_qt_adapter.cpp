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



static DvzKeyCode _key(quint32 native_scan_code)
{
    const QString platform = QGuiApplication::platformName();
    if (platform == QStringLiteral("xcb"))
    {
        if (native_scan_code < 8)
            return DVZ_KEY_UNKNOWN;
        native_scan_code -= 8;
    }
    else if (platform != QStringLiteral("wayland"))
        return DVZ_KEY_UNKNOWN;

    switch (native_scan_code)
    {
#define DVZ_QT_SCAN(scan, key)                                                              \
    case scan:                                                                               \
        return key
        DVZ_QT_SCAN(1, DVZ_KEY_ESCAPE);
        DVZ_QT_SCAN(2, DVZ_KEY_1);
        DVZ_QT_SCAN(3, DVZ_KEY_2);
        DVZ_QT_SCAN(4, DVZ_KEY_3);
        DVZ_QT_SCAN(5, DVZ_KEY_4);
        DVZ_QT_SCAN(6, DVZ_KEY_5);
        DVZ_QT_SCAN(7, DVZ_KEY_6);
        DVZ_QT_SCAN(8, DVZ_KEY_7);
        DVZ_QT_SCAN(9, DVZ_KEY_8);
        DVZ_QT_SCAN(10, DVZ_KEY_9);
        DVZ_QT_SCAN(11, DVZ_KEY_0);
        DVZ_QT_SCAN(12, DVZ_KEY_MINUS);
        DVZ_QT_SCAN(13, DVZ_KEY_EQUAL);
        DVZ_QT_SCAN(14, DVZ_KEY_BACKSPACE);
        DVZ_QT_SCAN(15, DVZ_KEY_TAB);
        DVZ_QT_SCAN(16, DVZ_KEY_Q);
        DVZ_QT_SCAN(17, DVZ_KEY_W);
        DVZ_QT_SCAN(18, DVZ_KEY_E);
        DVZ_QT_SCAN(19, DVZ_KEY_R);
        DVZ_QT_SCAN(20, DVZ_KEY_T);
        DVZ_QT_SCAN(21, DVZ_KEY_Y);
        DVZ_QT_SCAN(22, DVZ_KEY_U);
        DVZ_QT_SCAN(23, DVZ_KEY_I);
        DVZ_QT_SCAN(24, DVZ_KEY_O);
        DVZ_QT_SCAN(25, DVZ_KEY_P);
        DVZ_QT_SCAN(26, DVZ_KEY_LEFT_BRACKET);
        DVZ_QT_SCAN(27, DVZ_KEY_RIGHT_BRACKET);
        DVZ_QT_SCAN(28, DVZ_KEY_ENTER);
        DVZ_QT_SCAN(29, DVZ_KEY_LEFT_CONTROL);
        DVZ_QT_SCAN(30, DVZ_KEY_A);
        DVZ_QT_SCAN(31, DVZ_KEY_S);
        DVZ_QT_SCAN(32, DVZ_KEY_D);
        DVZ_QT_SCAN(33, DVZ_KEY_F);
        DVZ_QT_SCAN(34, DVZ_KEY_G);
        DVZ_QT_SCAN(35, DVZ_KEY_H);
        DVZ_QT_SCAN(36, DVZ_KEY_J);
        DVZ_QT_SCAN(37, DVZ_KEY_K);
        DVZ_QT_SCAN(38, DVZ_KEY_L);
        DVZ_QT_SCAN(39, DVZ_KEY_SEMICOLON);
        DVZ_QT_SCAN(40, DVZ_KEY_APOSTROPHE);
        DVZ_QT_SCAN(41, DVZ_KEY_GRAVE_ACCENT);
        DVZ_QT_SCAN(42, DVZ_KEY_LEFT_SHIFT);
        DVZ_QT_SCAN(43, DVZ_KEY_BACKSLASH);
        DVZ_QT_SCAN(44, DVZ_KEY_Z);
        DVZ_QT_SCAN(45, DVZ_KEY_X);
        DVZ_QT_SCAN(46, DVZ_KEY_C);
        DVZ_QT_SCAN(47, DVZ_KEY_V);
        DVZ_QT_SCAN(48, DVZ_KEY_B);
        DVZ_QT_SCAN(49, DVZ_KEY_N);
        DVZ_QT_SCAN(50, DVZ_KEY_M);
        DVZ_QT_SCAN(51, DVZ_KEY_COMMA);
        DVZ_QT_SCAN(52, DVZ_KEY_PERIOD);
        DVZ_QT_SCAN(53, DVZ_KEY_SLASH);
        DVZ_QT_SCAN(54, DVZ_KEY_RIGHT_SHIFT);
        DVZ_QT_SCAN(56, DVZ_KEY_LEFT_ALT);
        DVZ_QT_SCAN(57, DVZ_KEY_SPACE);
        DVZ_QT_SCAN(58, DVZ_KEY_CAPS_LOCK);
        DVZ_QT_SCAN(59, DVZ_KEY_F1);
        DVZ_QT_SCAN(60, DVZ_KEY_F2);
        DVZ_QT_SCAN(61, DVZ_KEY_F3);
        DVZ_QT_SCAN(62, DVZ_KEY_F4);
        DVZ_QT_SCAN(63, DVZ_KEY_F5);
        DVZ_QT_SCAN(64, DVZ_KEY_F6);
        DVZ_QT_SCAN(65, DVZ_KEY_F7);
        DVZ_QT_SCAN(66, DVZ_KEY_F8);
        DVZ_QT_SCAN(67, DVZ_KEY_F9);
        DVZ_QT_SCAN(68, DVZ_KEY_F10);
        DVZ_QT_SCAN(87, DVZ_KEY_F11);
        DVZ_QT_SCAN(88, DVZ_KEY_F12);
        DVZ_QT_SCAN(97, DVZ_KEY_RIGHT_CONTROL);
        DVZ_QT_SCAN(100, DVZ_KEY_RIGHT_ALT);
        DVZ_QT_SCAN(102, DVZ_KEY_HOME);
        DVZ_QT_SCAN(103, DVZ_KEY_UP);
        DVZ_QT_SCAN(104, DVZ_KEY_PAGE_UP);
        DVZ_QT_SCAN(105, DVZ_KEY_LEFT);
        DVZ_QT_SCAN(106, DVZ_KEY_RIGHT);
        DVZ_QT_SCAN(107, DVZ_KEY_END);
        DVZ_QT_SCAN(108, DVZ_KEY_DOWN);
        DVZ_QT_SCAN(109, DVZ_KEY_PAGE_DOWN);
        DVZ_QT_SCAN(110, DVZ_KEY_INSERT);
        DVZ_QT_SCAN(111, DVZ_KEY_DELETE);
        DVZ_QT_SCAN(125, DVZ_KEY_LEFT_SUPER);
        DVZ_QT_SCAN(126, DVZ_KEY_RIGHT_SUPER);
        DVZ_QT_SCAN(127, DVZ_KEY_MENU);
#undef DVZ_QT_SCAN
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

    const int mods = _mods(event->modifiers());
    const DvzKeyCode key = _key(event->nativeScanCode());
    (void)dvz_view_emit_key(_view, type, key, mods);
    if (type == DVZ_KEYBOARD_EVENT_RELEASE)
        return;
    QByteArray utf8 = event->text().toUtf8();
    if (!utf8.isEmpty())
        (void)dvz_view_emit_text(_view, utf8.constData(), (uint32_t)utf8.size(), mods);
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
