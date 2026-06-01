#!/usr/bin/env python3
"""Minimal PyQt6 host-window example using raw ctypes."""

from __future__ import annotations

import ctypes
import sys

try:
    from PyQt6.QtCore import QEvent, QSize
    from PyQt6.QtGui import QCloseEvent, QExposeEvent, QResizeEvent, QSurface, QVulkanInstance
    from PyQt6.QtGui import QWindow
    from PyQt6.QtWidgets import QApplication
except ImportError as exc:  # pragma: no cover - depends on optional local Qt build.
    raise SystemExit(f'PyQt6 with Qt Vulkan support is required: {exc}') from exc

import datoviz.raw as dvz


def _void_p(array: ctypes.Array) -> ctypes.c_void_p:
    return ctypes.cast(array, ctypes.c_void_p)


def _extension_name(extension) -> str:
    name = extension.name() if callable(getattr(extension, 'name', None)) else extension
    if hasattr(name, 'data'):
        name = bytes(name)
    if isinstance(name, bytes):
        return name.decode()
    return str(name)


def _qt_vulkan_extensions(app: QApplication) -> list[bytes]:
    supported = {_extension_name(ext) for ext in QVulkanInstance().supportedExtensions()}
    platform = app.platformName().lower()

    extensions = ['VK_KHR_surface']
    if sys.platform == 'win32':
        extensions.append('VK_KHR_win32_surface')
    elif sys.platform == 'darwin':
        extensions.append(
            'VK_EXT_metal_surface'
            if 'VK_EXT_metal_surface' in supported
            else 'VK_MVK_macos_surface'
        )
    elif platform.startswith('wayland'):
        extensions.append('VK_KHR_wayland_surface')
    else:
        extensions.append('VK_KHR_xcb_surface')

    missing = [extension for extension in extensions if extension not in supported]
    if missing:
        raise RuntimeError(f'Qt Vulkan support is missing: {", ".join(missing)}')
    return [extension.encode() for extension in extensions]


class HostedPyQtWindow(QWindow):
    def __init__(self, app_handle, figure, qt_instance: QVulkanInstance):
        super().__init__()
        self._app_handle = app_handle
        self._figure = figure
        self._qt_instance = qt_instance
        self._view = None
        self._repaint_requested = True
        self._request_callback = dvz.DvzViewRequestFrameCallback(self._request_frame)

        self.setTitle('hosted_pyqt')
        self.setSurfaceType(QSurface.SurfaceType.VulkanSurface)
        self.setVulkanInstance(qt_instance)
        self.resize(QSize(800, 600))

    def event(self, event):
        if event.type() == QEvent.Type.PlatformSurface:
            surface_type = getattr(event, 'surfaceEventType', lambda: None)()
            if str(surface_type).endswith('SurfaceAboutToBeDestroyed'):
                self.release_surface()
        if event.type() == QEvent.Type.UpdateRequest:
            self._render_once()
            return True
        return super().event(event)

    def exposeEvent(self, event: QExposeEvent):
        del event
        if self.isExposed():
            self.requestUpdate()

    def resizeEvent(self, event: QResizeEvent):
        super().resizeEvent(event)
        self._emit_resize()

    def closeEvent(self, event: QCloseEvent):
        self.release_surface()
        super().closeEvent(event)

    def release_surface(self):
        if self._view:
            dvz.dvz_view_release_external_surface(self._view)
            self._view = None

    def _request_frame(self, _view, _user_data):
        self._repaint_requested = True
        self.requestUpdate()

    def _surface_tuple(self) -> tuple[int, int, int, int, float]:
        scale = float(self.devicePixelRatio()) or 1.0
        size = self.size()
        framebuffer_width = max(0, round(size.width() * scale))
        framebuffer_height = max(0, round(size.height() * scale))
        surface = int(QVulkanInstance.surfaceForWindow(self) or 0)
        instance = int(dvz.dvz_app_vk_instance(self._app_handle) or 0)
        return instance, surface, framebuffer_width, framebuffer_height, scale

    def _ensure_view(self) -> bool:
        if self._view:
            return True
        instance, surface, width, height, scale = self._surface_tuple()
        if instance == 0 or surface == 0:
            return False
        self._view = dvz.dvz_view_external_surface_ffi(
            self._app_handle,
            self._figure,
            ctypes.c_void_p(instance),
            surface,
            width,
            height,
            scale,
            scale,
            False,
        )
        if not self._view:
            return False
        dvz.dvz_view_set_request_frame_callback(self._view, self._request_callback, None)
        self._emit_resize()
        return True

    def _emit_resize(self):
        if not self._view:
            return
        _instance, _surface, width, height, scale = self._surface_tuple()
        size = self.size()
        dvz.dvz_view_emit_resize(
            self._view,
            width,
            height,
            max(0, size.width()),
            max(0, size.height()),
            scale,
            scale,
        )

    def _render_once(self):
        if not self.isExposed():
            return
        if not self._ensure_view():
            self.requestUpdate()
            return

        instance, surface, width, height, scale = self._surface_tuple()
        rc = dvz.dvz_view_update_external_surface_ffi(
            self._view, ctypes.c_void_p(instance), surface, width, height, scale, scale, False
        )
        if rc != 0:
            raise RuntimeError('dvz_view_update_external_surface_ffi() failed')

        if not self._repaint_requested:
            return
        self._repaint_requested = False
        rc = dvz.dvz_view_render_once(self._view)
        if rc < 0:
            raise RuntimeError(f'dvz_view_render_once() failed: {rc}')


def _make_scene():
    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError('dvz_scene() failed')

    figure = dvz.dvz_figure(scene, 800, 600, 0)
    panel = dvz.dvz_panel_full(figure)
    visual = dvz.dvz_point(scene, 0)
    if not figure or not panel or not visual:
        raise RuntimeError('scene setup failed')

    positions = (ctypes.c_float * 9)(
        -0.55,
        -0.45,
        0.0,
        +0.55,
        -0.45,
        0.0,
        0.0,
        +0.50,
        0.0,
    )
    colors = (dvz.DvzColor * 3)(
        dvz.DvzColor(255, 80, 80, 255),
        dvz.DvzColor(80, 220, 120, 255),
        dvz.DvzColor(90, 150, 255, 255),
    )
    diameters = (ctypes.c_float * 3)(24.0, 24.0, 24.0)

    dvz.dvz_visual_set_data(visual, b'position', _void_p(positions), 3)
    dvz.dvz_visual_set_data(visual, b'color', _void_p(colors), 3)
    dvz.dvz_visual_set_data(visual, b'diameter', _void_p(diameters), 3)
    dvz.dvz_panel_add_visual(panel, visual, None)
    dvz.dvz_panel_set_background_color(panel, 0.05, 0.06, 0.08, 1.0)
    return scene, figure, (positions, colors, diameters)


def main() -> int:
    qt_app = QApplication(sys.argv)
    extensions = _qt_vulkan_extensions(qt_app)
    extension_array = (ctypes.c_char_p * len(extensions))(*extensions)

    scene = None
    app_handle = None
    qt_instance = None
    window = None
    try:
        scene, figure, _data = _make_scene()
        app_config = dvz.dvz_app_config()
        app_config.instance_extension_count = len(extensions)
        app_config.instance_extensions = extension_array
        app_config.enable_canvas_extensions = True
        app_config.enable_glfw_extensions = False

        app_handle = dvz.dvz_app_with_config(scene, ctypes.byref(app_config))
        if not app_handle:
            raise RuntimeError('dvz_app_with_config() failed')

        instance = dvz.dvz_app_vk_instance(app_handle)
        if not instance:
            raise RuntimeError('dvz_app_vk_instance() failed')

        qt_instance = QVulkanInstance()
        qt_instance.setVkInstance(int(instance))
        if not qt_instance.create():
            raise RuntimeError(f'QVulkanInstance.create() failed: {qt_instance.errorCode()}')

        window = HostedPyQtWindow(app_handle, figure, qt_instance)
        window.show()
        return int(qt_app.exec())
    finally:
        if window is not None:
            window.release_surface()
        if qt_instance is not None:
            qt_instance.destroy()
        if app_handle:
            dvz.dvz_app_destroy(app_handle)
        if scene:
            dvz.dvz_scene_destroy(scene)


if __name__ == '__main__':
    raise SystemExit(main())
