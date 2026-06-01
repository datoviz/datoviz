"""Optional PyQt6 hosting adapter for Datoviz views."""

from __future__ import annotations

import ctypes
import ctypes.util
import os
import sys
from collections.abc import Callable
from pathlib import Path

try:
    from PyQt6.QtCore import QT_VERSION_STR, QEvent, QSize, Qt, pyqtSignal
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


_QTBRIDGE_ABI_VERSION = 1
_QTBRIDGE_ENV = 'DATOVIZ_QTBRIDGE_LIBRARY'
_qt_bridge = None


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


class _QtBridge:
    def __init__(self, lib: ctypes.CDLL, path: str):
        self.lib = lib
        self.path = path

        lib.dvz_qtbridge_abi_version.restype = ctypes.c_uint32
        lib.dvz_qtbridge_qt_version.restype = ctypes.c_char_p
        lib.dvz_qtbridge_qvulkan_set_vk_instance.argtypes = [ctypes.c_size_t, ctypes.c_size_t]
        lib.dvz_qtbridge_qvulkan_set_vk_instance.restype = ctypes.c_int
        lib.dvz_qtbridge_qvulkan_vk_instance.argtypes = [ctypes.c_size_t]
        lib.dvz_qtbridge_qvulkan_vk_instance.restype = ctypes.c_size_t

        self.abi_version = int(lib.dvz_qtbridge_abi_version())
        if self.abi_version != _QTBRIDGE_ABI_VERSION:
            raise RuntimeError(
                f'datoviz_qtbridge ABI mismatch: expected {_QTBRIDGE_ABI_VERSION}, '
                f'got {self.abi_version} from {path}'
            )

    @property
    def qt_version(self) -> str:
        value = self.lib.dvz_qtbridge_qt_version()
        return value.decode() if value else 'unknown'

    def set_vk_instance(self, qvulkan_instance: int, vk_instance: int) -> None:
        rc = int(
            self.lib.dvz_qtbridge_qvulkan_set_vk_instance(
                ctypes.c_size_t(qvulkan_instance),
                ctypes.c_size_t(vk_instance),
            )
        )
        if rc != 0:
            reasons = {
                -1: 'null QVulkanInstance pointer',
                -2: 'null VkInstance handle',
                -3: 'unsupported Qt bridge runtime state',
            }
            reason = reasons.get(rc, f'error code {rc}')
            raise RuntimeError(f'datoviz_qtbridge could not adopt the Vulkan instance: {reason}')

    def vk_instance(self, qvulkan_instance: int) -> int:
        return int(
            self.lib.dvz_qtbridge_qvulkan_vk_instance(ctypes.c_size_t(qvulkan_instance))
        )


def _qt_bridge_names() -> list[str]:
    if sys.platform == 'win32':
        return ['datoviz_qtbridge.dll', 'libdatoviz_qtbridge.dll']
    if sys.platform == 'darwin':
        return ['libdatoviz_qtbridge.dylib', 'datoviz_qtbridge.dylib']
    return ['libdatoviz_qtbridge.so', 'datoviz_qtbridge.so']


def _qt_bridge_candidates() -> list[Path | str]:
    env_path = os.environ.get(_QTBRIDGE_ENV)
    candidates: list[Path | str] = []
    if env_path:
        candidates.append(Path(env_path))

    package_dir = Path(__file__).resolve().parent
    repo_dir = package_dir.parent
    search_dirs = [
        package_dir,
        package_dir / '.libs',
        repo_dir / 'build' / 'qtbridge',
        repo_dir / 'build' / 'lib',
        repo_dir / 'lib',
    ]
    for directory in search_dirs:
        candidates.extend(directory / name for name in _qt_bridge_names())

    found = ctypes.util.find_library('datoviz_qtbridge')
    if found:
        candidates.append(found)

    return candidates


def _load_qt_bridge() -> _QtBridge:
    global _qt_bridge

    if _qt_bridge is not None:
        return _qt_bridge

    errors = []
    env_path = os.environ.get(_QTBRIDGE_ENV)
    for candidate in _qt_bridge_candidates():
        candidate_text = str(candidate)
        if isinstance(candidate, Path) and not candidate.exists():
            if env_path and candidate == Path(env_path):
                errors.append(f'{candidate_text}: file does not exist')
            continue
        try:
            lib = ctypes.CDLL(candidate_text)
            bridge = _QtBridge(lib, candidate_text)
            _check_qt_bridge_runtime(bridge)
            _qt_bridge = bridge
            return _qt_bridge
        except OSError as exc:
            errors.append(f'{candidate_text}: {exc}')

    detail = '; '.join(errors) if errors else 'no candidate library was found'
    raise RuntimeError(
        'PyQt hosting requires the optional datoviz_qtbridge provider. Build Datoviz with '
        f'-DDVZ_ENABLE_QT_BRIDGE=AUTO or ON, or set {_QTBRIDGE_ENV} to the bridge library. '
        f'Lookup detail: {detail}.'
    )


def _check_qt_bridge_runtime(bridge: _QtBridge) -> None:
    pyqt_version = str(QT_VERSION_STR)
    bridge_version = bridge.qt_version
    pyqt_major_minor = '.'.join(pyqt_version.split('.')[:2])
    bridge_major_minor = '.'.join(bridge_version.split('.')[:2])
    if pyqt_major_minor != bridge_major_minor:
        raise RuntimeError(
            'datoviz_qtbridge Qt runtime mismatch: '
            f'PyQt6 uses Qt {pyqt_version}, but {bridge.path} reports Qt {bridge_version}. '
            'Use a bridge built against the same Qt major/minor runtime.'
        )


def _require_pyqt_vulkan_surface() -> None:
    missing = []
    if not hasattr(QWindow, 'setVulkanInstance'):
        missing.append('QWindow.setVulkanInstance')
    if not hasattr(QVulkanInstance, 'surfaceForWindow'):
        missing.append('QVulkanInstance.surfaceForWindow')
    if missing:
        raise RuntimeError(
            'datoviz.qt requires PyQt6 bindings with Qt Vulkan support; missing '
            + ', '.join(missing)
        )


def _unwrap_qvulkan_instance(qt_instance: QVulkanInstance) -> int:
    try:
        from PyQt6 import sip
    except ImportError as exc:  # pragma: no cover - depends on optional PyQt packaging.
        raise RuntimeError('datoviz.qt requires PyQt6.sip.unwrapinstance()') from exc

    unwrap = getattr(sip, 'unwrapinstance', None)
    if unwrap is None:
        raise RuntimeError('datoviz.qt requires PyQt6.sip.unwrapinstance()')

    ptr = int(unwrap(qt_instance) or 0)
    if ptr == 0:
        raise RuntimeError('PyQt6.sip.unwrapinstance(QVulkanInstance) returned a null pointer')
    return ptr


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
        self._released = False
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

        if self._released:
            return
        if self._view:
            dvz.dvz_view_request_frame(self._view)
        else:
            self._request_frame(None, None)

    def release(self) -> None:
        """Release Datoviz surface resources and destroy owned app resources."""

        if self._released:
            return
        self._released = True
        self.release_surface()
        self._destroy_qt_surface()
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

    def _destroy_qt_surface(self) -> None:
        """Destroy Qt-owned native surface objects before the adopted VkInstance dies."""

        self.setVisible(False)
        self.destroy()
        app = QApplication.instance()
        if app is not None:
            app.processEvents()

    def event(self, event):
        if event.type() == QEvent.Type.PlatformSurface:
            surface_event = event
            if isinstance(surface_event, QPlatformSurfaceEvent) and (
                surface_event.surfaceEventType()
                == QPlatformSurfaceEvent.SurfaceEventType.SurfaceAboutToBeDestroyed
            ):
                self.release_surface()
        if event.type() == QEvent.Type.UpdateRequest:
            if self._released:
                return True
            self._render_once()
            return True
        return super().event(event)

    def exposeEvent(self, event: QExposeEvent) -> None:
        del event
        if not self._released and self.isExposed():
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
        _require_pyqt_vulkan_surface()

        self._qt_instance = QVulkanInstance()
        qt_instance_ptr = _unwrap_qvulkan_instance(self._qt_instance)
        vk_instance = int(instance)
        bridge = _load_qt_bridge()
        bridge.set_vk_instance(qt_instance_ptr, vk_instance)
        adopted = bridge.vk_instance(qt_instance_ptr)
        if adopted != vk_instance:
            raise RuntimeError(
                'datoviz_qtbridge Vulkan instance readback mismatch: '
                f'expected 0x{vk_instance:x}, got 0x{adopted:x}'
            )
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
        if self._released or self._app is None:
            return
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
        if self._container is not None:
            self._container.setParent(None)
            self._container.deleteLater()
            self._container = None

    def resizeEvent(self, event: QResizeEvent) -> None:
        super().resizeEvent(event)
        if self._container is not None:
            self._container.setGeometry(self.rect())

    def closeEvent(self, event: QCloseEvent) -> None:
        self.release()
        super().closeEvent(event)


def _require_qapplication() -> QApplication:
    app = QApplication.instance()
    if app is None:
        raise RuntimeError('create a QApplication before constructing DatovizWidget')
    return app


def _probe_qt_bridge() -> dict[str, object]:
    """Return optional Qt bridge diagnostics without constructing a visible window."""

    _require_pyqt_vulkan_surface()
    qt_instance = QVulkanInstance()
    qt_instance_ptr = _unwrap_qvulkan_instance(qt_instance)
    bridge = _load_qt_bridge()
    return {
        'bridge_path': bridge.path,
        'bridge_abi': bridge.abi_version,
        'bridge_qt_version': bridge.qt_version,
        'pyqt_qt_version': str(QT_VERSION_STR),
        'qvulkan_instance_ptr': qt_instance_ptr,
        'qvulkan_instance_readback': bridge.vk_instance(qt_instance_ptr),
        'has_qwindow_set_vulkan_instance': hasattr(QWindow, 'setVulkanInstance'),
        'has_qvulkan_surface_for_window': hasattr(QVulkanInstance, 'surfaceForWindow'),
    }


def _main() -> int:
    try:
        info = _probe_qt_bridge()
    except Exception as exc:  # pragma: no cover - diagnostic command.
        print(f'datoviz.qt probe failed: {exc}', file=sys.stderr)
        return 1

    for key, value in info.items():
        print(f'{key}: {value}')
    return 0


__all__ = ['DatovizWidget', 'DatovizWindow']


if __name__ == '__main__':
    raise SystemExit(_main())
