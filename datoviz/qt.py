"""Optional PyQt6 hosting adapter for Datoviz views."""

from __future__ import annotations

import ctypes
import sys
from collections.abc import Callable

try:
    from PyQt6.QtCore import QEvent, QSize, Qt, pyqtSignal
    from PyQt6.QtGui import QCloseEvent, QExposeEvent, QKeyEvent, QMouseEvent
    from PyQt6.QtGui import QPlatformSurfaceEvent, QResizeEvent, QSurface, QVulkanInstance
    from PyQt6.QtGui import QWheelEvent, QWindow
    from PyQt6.QtWidgets import QApplication, QVBoxLayout, QWidget
except ImportError as exc:  # pragma: no cover - depends on optional local Qt build.
    raise ImportError(
        'datoviz.qt requires PyQt6 with Qt Vulkan support. Install a PyQt6 package from '
        'your Python, Conda, or system package manager whose Qt build includes QVulkanInstance.'
    ) from exc

import datoviz.raw as dvz


_KEY_MAP = {
    Qt.Key.Key_Space: dvz.DvzKeyCode.DVZ_KEY_SPACE,
    Qt.Key.Key_Escape: dvz.DvzKeyCode.DVZ_KEY_ESCAPE,
    Qt.Key.Key_Return: dvz.DvzKeyCode.DVZ_KEY_ENTER,
    Qt.Key.Key_Enter: dvz.DvzKeyCode.DVZ_KEY_ENTER,
    Qt.Key.Key_Tab: dvz.DvzKeyCode.DVZ_KEY_TAB,
    Qt.Key.Key_Backspace: dvz.DvzKeyCode.DVZ_KEY_BACKSPACE,
    Qt.Key.Key_Insert: dvz.DvzKeyCode.DVZ_KEY_INSERT,
    Qt.Key.Key_Delete: dvz.DvzKeyCode.DVZ_KEY_DELETE,
    Qt.Key.Key_Right: dvz.DvzKeyCode.DVZ_KEY_RIGHT,
    Qt.Key.Key_Left: dvz.DvzKeyCode.DVZ_KEY_LEFT,
    Qt.Key.Key_Down: dvz.DvzKeyCode.DVZ_KEY_DOWN,
    Qt.Key.Key_Up: dvz.DvzKeyCode.DVZ_KEY_UP,
    Qt.Key.Key_PageUp: dvz.DvzKeyCode.DVZ_KEY_PAGE_UP,
    Qt.Key.Key_PageDown: dvz.DvzKeyCode.DVZ_KEY_PAGE_DOWN,
    Qt.Key.Key_Home: dvz.DvzKeyCode.DVZ_KEY_HOME,
    Qt.Key.Key_End: dvz.DvzKeyCode.DVZ_KEY_END,
    Qt.Key.Key_Shift: dvz.DvzKeyCode.DVZ_KEY_LEFT_SHIFT,
    Qt.Key.Key_Control: dvz.DvzKeyCode.DVZ_KEY_LEFT_CONTROL,
    Qt.Key.Key_Alt: dvz.DvzKeyCode.DVZ_KEY_LEFT_ALT,
    Qt.Key.Key_Meta: dvz.DvzKeyCode.DVZ_KEY_LEFT_SUPER,
}


def _extension_name(extension) -> str:
    name = getattr(extension, 'name', extension)
    if callable(name):
        name = name()
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


def _mods(modifiers) -> int:
    out = int(dvz.DvzKeyboardModifiers.DVZ_KEY_MODIFIER_NONE)
    if modifiers & Qt.KeyboardModifier.ShiftModifier:
        out |= int(dvz.DvzKeyboardModifiers.DVZ_KEY_MODIFIER_SHIFT)
    if modifiers & Qt.KeyboardModifier.ControlModifier:
        out |= int(dvz.DvzKeyboardModifiers.DVZ_KEY_MODIFIER_CONTROL)
    if modifiers & Qt.KeyboardModifier.AltModifier:
        out |= int(dvz.DvzKeyboardModifiers.DVZ_KEY_MODIFIER_ALT)
    if modifiers & Qt.KeyboardModifier.MetaModifier:
        out |= int(dvz.DvzKeyboardModifiers.DVZ_KEY_MODIFIER_SUPER)
    return out


def _button(button) -> int:
    if button == Qt.MouseButton.LeftButton:
        return int(dvz.DvzPointerButton.DVZ_POINTER_BUTTON_LEFT)
    if button == Qt.MouseButton.MiddleButton:
        return int(dvz.DvzPointerButton.DVZ_POINTER_BUTTON_MIDDLE)
    if button == Qt.MouseButton.RightButton:
        return int(dvz.DvzPointerButton.DVZ_POINTER_BUTTON_RIGHT)
    return int(dvz.DvzPointerButton.DVZ_POINTER_BUTTON_NONE)


def _key(event: QKeyEvent) -> int:
    key = Qt.Key(event.key())
    if Qt.Key.Key_A <= key <= Qt.Key.Key_Z:
        return int(dvz.DvzKeyCode.DVZ_KEY_A) + int(key) - int(Qt.Key.Key_A)
    if Qt.Key.Key_0 <= key <= Qt.Key.Key_9:
        return int(dvz.DvzKeyCode.DVZ_KEY_0) + int(key) - int(Qt.Key.Key_0)
    if Qt.Key.Key_F1 <= key <= Qt.Key.Key_F25:
        return int(dvz.DvzKeyCode.DVZ_KEY_F1) + int(key) - int(Qt.Key.Key_F1)
    return int(_KEY_MAP.get(key, dvz.DvzKeyCode.DVZ_KEY_UNKNOWN))


class DatovizWindow(QWindow):
    """Qt `QWindow` that hosts one Datoviz view."""

    def __init__(
        self,
        scene,
        figure,
        *,
        title: str = 'Datoviz',
        size: QSize | tuple[int, int] = (800, 600),
        on_view_ready: Callable[[object], None] | None = None,
    ):
        super().__init__()
        self._scene = scene
        self._figure = figure
        self._on_view_ready = on_view_ready
        self._extension_bytes = _qt_vulkan_extensions(_require_qapplication())
        self._extension_array = (ctypes.c_char_p * len(self._extension_bytes))(
            *self._extension_bytes
        )
        self._app = None
        self._qt_instance = None
        self._view = None
        self._repaint_requested = True
        self._request_callback = dvz.DvzViewRequestFrameCallback(self._request_frame)

        self._create_datoviz_app()
        self._create_qt_instance()

        self.setTitle(title)
        self.setSurfaceType(QSurface.SurfaceType.VulkanSurface)
        self.setVulkanInstance(self._qt_instance)
        if isinstance(size, QSize):
            self.resize(size)
        else:
            self.resize(QSize(int(size[0]), int(size[1])))

    @property
    def view(self):
        """Return the raw `DvzView*`, or `None` until the Qt surface exists."""

        return self._view

    @property
    def app(self):
        """Return the raw `DvzApp*` owned by this host window."""

        return self._app

    def request_frame(self) -> None:
        """Ask Qt to schedule another Datoviz frame."""

        if self._view:
            dvz.dvz_view_request_frame(self._view)
        else:
            self._request_frame(None, None)

    def release(self) -> None:
        """Release Datoviz surface resources and destroy owned app resources."""

        self.release_surface()
        if self._qt_instance is not None:
            self._qt_instance.destroy()
            self._qt_instance = None
        if self._app:
            dvz.dvz_app_destroy(self._app)
            self._app = None

    def release_surface(self) -> None:
        """Release Datoviz resources tied to Qt's current Vulkan surface."""

        if self._view:
            dvz.dvz_view_release_external_surface(self._view)
            self._view = None

    def event(self, event):
        if event.type() == QEvent.Type.PlatformSurface:
            surface_event = event
            if isinstance(surface_event, QPlatformSurfaceEvent) and (
                surface_event.surfaceEventType()
                == QPlatformSurfaceEvent.SurfaceEventType.SurfaceAboutToBeDestroyed
            ):
                self.release_surface()
        if event.type() == QEvent.Type.UpdateRequest:
            self._render_once()
            return True
        return super().event(event)

    def exposeEvent(self, event: QExposeEvent) -> None:
        del event
        if self.isExposed():
            self.requestUpdate()

    def resizeEvent(self, event: QResizeEvent) -> None:
        super().resizeEvent(event)
        self._emit_resize()

    def closeEvent(self, event: QCloseEvent) -> None:
        self.release_surface()
        super().closeEvent(event)

    def mouseMoveEvent(self, event: QMouseEvent) -> None:
        self._emit_pointer(
            dvz.DvzPointerEventType.DVZ_POINTER_EVENT_MOVE,
            event,
            dvz.DvzPointerButton.DVZ_POINTER_BUTTON_NONE,
        )

    def mousePressEvent(self, event: QMouseEvent) -> None:
        self._emit_pointer(
            dvz.DvzPointerEventType.DVZ_POINTER_EVENT_PRESS, event, _button(event.button())
        )

    def mouseReleaseEvent(self, event: QMouseEvent) -> None:
        self._emit_pointer(
            dvz.DvzPointerEventType.DVZ_POINTER_EVENT_RELEASE, event, _button(event.button())
        )

    def wheelEvent(self, event: QWheelEvent) -> None:
        if not self._view:
            return
        pos = event.position()
        size = self.size()
        wheel = event.angleDelta()
        if wheel.isNull():
            wheel = event.pixelDelta()
        dvz.dvz_view_emit_wheel(
            self._view,
            float(pos.x()),
            float(pos.y()),
            float(size.width()),
            float(size.height()),
            float(wheel.x()) / 120.0,
            float(wheel.y()) / 120.0,
            _mods(event.modifiers()),
        )

    def keyPressEvent(self, event: QKeyEvent) -> None:
        event_type = (
            dvz.DvzKeyboardEventType.DVZ_KEYBOARD_EVENT_REPEAT
            if event.isAutoRepeat()
            else dvz.DvzKeyboardEventType.DVZ_KEYBOARD_EVENT_PRESS
        )
        self._emit_key(event_type, event)

    def keyReleaseEvent(self, event: QKeyEvent) -> None:
        self._emit_key(dvz.DvzKeyboardEventType.DVZ_KEYBOARD_EVENT_RELEASE, event)

    def _create_datoviz_app(self) -> None:
        app_config = dvz.dvz_app_config()
        app_config.instance_extension_count = len(self._extension_bytes)
        app_config.instance_extensions = self._extension_array
        app_config.enable_canvas_extensions = True
        app_config.enable_glfw_extensions = False
        self._app = dvz.dvz_app_with_config(self._scene, ctypes.byref(app_config))
        if not self._app:
            raise RuntimeError('dvz_app_with_config() failed')

    def _create_qt_instance(self) -> None:
        instance = dvz.dvz_app_vk_instance(self._app)
        if not instance:
            raise RuntimeError('dvz_app_vk_instance() failed')
        self._qt_instance = QVulkanInstance()
        self._qt_instance.setVkInstance(int(instance))
        if not self._qt_instance.create():
            raise RuntimeError(f'QVulkanInstance.create() failed: {self._qt_instance.errorCode()}')

    def _request_frame(self, _view, _user_data) -> None:
        self._repaint_requested = True
        self.requestUpdate()

    def _surface_tuple(self) -> tuple[int, int, int, int, float]:
        scale = float(self.devicePixelRatio()) or 1.0
        size = self.size()
        framebuffer_width = max(0, round(size.width() * scale))
        framebuffer_height = max(0, round(size.height() * scale))
        surface = int(QVulkanInstance.surfaceForWindow(self) or 0)
        instance = int(dvz.dvz_app_vk_instance(self._app) or 0)
        return instance, surface, framebuffer_width, framebuffer_height, scale

    def _ensure_view(self) -> bool:
        if self._view:
            return True
        instance, surface, width, height, scale = self._surface_tuple()
        if instance == 0 or surface == 0:
            return False
        self._view = dvz.dvz_view_external_surface_ffi(
            self._app,
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
        if self._on_view_ready is not None:
            self._on_view_ready(self._view)
        return True

    def _emit_resize(self) -> None:
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

    def _emit_pointer(self, event_type, event: QMouseEvent, button: int) -> None:
        if not self._view:
            return
        pos = event.position()
        size = self.size()
        dvz.dvz_view_emit_pointer(
            self._view,
            event_type,
            float(pos.x()),
            float(pos.y()),
            float(size.width()),
            float(size.height()),
            button,
            _mods(event.modifiers()),
        )

    def _emit_key(self, event_type, event: QKeyEvent) -> None:
        if not self._view:
            return
        key = _key(event)
        if key == int(dvz.DvzKeyCode.DVZ_KEY_UNKNOWN):
            return
        dvz.dvz_view_emit_key(self._view, event_type, key, _mods(event.modifiers()))

    def _render_once(self) -> None:
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


class DatovizWidget(QWidget):
    """QWidget wrapper around a hosted Datoviz QWindow."""

    view_ready = pyqtSignal(object)

    def __init__(
        self,
        scene,
        figure,
        parent: QWidget | None = None,
        *,
        title: str = 'Datoviz',
        size: QSize | tuple[int, int] = (800, 600),
    ):
        super().__init__(parent)
        self._window = DatovizWindow(
            scene, figure, title=title, size=size, on_view_ready=self.view_ready.emit
        )
        self._container = QWidget.createWindowContainer(self._window, self)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self._container)

    @property
    def window(self) -> DatovizWindow:
        """Return the hosted Datoviz QWindow."""

        return self._window

    @property
    def view(self):
        """Return the raw `DvzView*`, or `None` until the widget is exposed."""

        return self._window.view

    @property
    def app(self):
        """Return the raw `DvzApp*` owned by this widget."""

        return self._window.app

    def request_frame(self) -> None:
        """Ask Qt to schedule another Datoviz frame."""

        self._window.request_frame()

    def release(self) -> None:
        """Release Datoviz and Qt Vulkan resources owned by this widget."""

        self._window.release()

    def resizeEvent(self, event: QResizeEvent) -> None:
        super().resizeEvent(event)
        self._container.setGeometry(self.rect())

    def closeEvent(self, event: QCloseEvent) -> None:
        self.release()
        super().closeEvent(event)


def _require_qapplication() -> QApplication:
    app = QApplication.instance()
    if app is None:
        raise RuntimeError('create a QApplication before constructing DatovizWidget')
    return app


__all__ = ['DatovizWidget', 'DatovizWindow']
