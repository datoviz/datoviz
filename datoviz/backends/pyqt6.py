"""
PyQt6 backend
"""

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import ctypes
import typing as tp
import warnings

try:
    from PyQt6.QtCore import Qt
    from PyQt6.QtGui import QImage, QPixmap
    from PyQt6.QtWidgets import QLabel, QVBoxLayout, QWidget, QSizePolicy
    _QT_AVAILABLE = True
except Exception:
    try:
        from PyQt5.QtCore import Qt
        from PyQt5.QtGui import QImage, QPixmap
        from PyQt5.QtWidgets import QLabel, QVBoxLayout, QWidget, QSizePolicy
        _QT_AVAILABLE = True
    except Exception:
        Qt = None
        QImage = None
        QPixmap = None
        QLabel = None
        QVBoxLayout = None
        QWidget = None
        QSizePolicy = None
        _QT_AVAILABLE = False
        warnings.warn(
            "PyQt5 or PyQt6 is not available; datoviz.backends.pyqt6 is disabled.",
            RuntimeWarning,
            stacklevel=2,
        )

import datoviz as dvz
from datoviz import vec2
from datoviz import _constants as cst
from datoviz._figure import Figure


if _QT_AVAILABLE:
    # -------------------------------------------------------------------------------------------------
    # Constants
    # -------------------------------------------------------------------------------------------------

    RESIZE_TIMER_DELAY = 10  # ms
    WIDTH, HEIGHT = 800, 600
    CHANNEL_COUNT = 3  # assuming RGB888 format
    BUTTON_MAPPING = {
        Qt.MouseButton.LeftButton: dvz.MOUSE_BUTTON_LEFT,
        Qt.MouseButton.MiddleButton: dvz.MOUSE_BUTTON_MIDDLE,
        Qt.MouseButton.RightButton: dvz.MOUSE_BUTTON_RIGHT,
    }

    # -------------------------------------------------------------------------------------------------
    # Qt figure
    # -------------------------------------------------------------------------------------------------

    class QtFigure(Figure, QWidget):
        def __init__(self, c_figure, c_server=None, c_scene=None):
            QWidget.__init__(self)
            Figure.__init__(self, c_figure)
            self.c_server = c_server
            self.c_scene = c_scene
            self.mouse = dvz.server_mouse(c_server)

            self.label = QLabel(self)
            self.label.setAlignment(Qt.AlignmentFlag.AlignCenter)
            self.label.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)

            layout = QVBoxLayout()
            layout.addWidget(self.label)
            layout.setContentsMargins(0, 0, 0, 0)
            self.setLayout(layout)
            self.setMinimumSize(50, 50)

            self.update_image()

        def update_image(self):
            label_size = self.label.size()
            w, h = label_size.width(), label_size.height()
            if w <= 0 or h <= 0:
                return

            dvz.figure_resize(self.c_figure, w, h)
            dvz.figure_update(self.c_figure)
            dvz.scene_render(self.c_scene, self.c_server)
            data = dvz.server_grab(self.c_server, dvz.figure_id(self.c_figure), 0)

            if data is None:
                return

            data_bytes = bytearray(ctypes.string_at(data, w * h * 3))
            self.image = QImage(data_bytes, w, h, 3 * w, QImage.Format.Format_RGB888)
            self.pixmap = QPixmap.fromImage(self.image)
            self.label.setPixmap(self.pixmap)

        def _mouse_button_pos(self, event):
            try:
                button = event.button()
            except Exception:
                button = 0
            dvz_button = BUTTON_MAPPING.get(button, 0)
            try:
                pos = event.pos()
            except Exception:
                pos = event.position()
            x = pos.x()
            y = pos.y()
            return x, y, dvz_button

        def _mouse_move(self, x, y):
            ev = dvz.mouse_move(self.mouse, vec2(x, y), 0)
            dvz.scene_mouse(self.c_scene, self.c_figure, ev)

        def _mouse_press(self, dvz_button):
            ev = dvz.mouse_press(self.mouse, dvz_button, 0)
            dvz.scene_mouse(self.c_scene, self.c_figure, ev)

        def _mouse_release(self, dvz_button):
            ev = dvz.mouse_release(self.mouse, dvz_button, 0)
            dvz.scene_mouse(self.c_scene, self.c_figure, ev)

        def _mouse_wheel(self, dir):
            ev = dvz.mouse_wheel(self.mouse, vec2(0, dir), 0)
            dvz.scene_mouse(self.c_scene, self.c_figure, ev)

        def resizeEvent(self, event):
            super().resizeEvent(event)
            # self.resize_timer.start(RESIZE_TIMER_DELAY)
            self.update_image()

        def mousePressEvent(self, event):
            super(QtFigure, self).mousePressEvent(event)
            x, y, dvz_button = self._mouse_button_pos(event)
            self._mouse_move(x, y)
            self._mouse_press(dvz_button)
            self.update_image()
            event.accept()

        def mouseReleaseEvent(self, event):
            super(QtFigure, self).mouseReleaseEvent(event)
            x, y, dvz_button = self._mouse_button_pos(event)
            self._mouse_move(x, y)
            self._mouse_release(dvz_button)
            self.update_image()
            event.accept()

        def wheelEvent(self, event):
            super(QtFigure, self).wheelEvent(event)
            x, y, dvz_button = self._mouse_button_pos(event)
            delta = .01 * event.angleDelta().y()
            self._mouse_move(x, y)
            self._mouse_wheel(delta)
            self.update_image()
            event.accept()

        def mouseMoveEvent(self, event):
            super(QtFigure, self).mouseMoveEvent(event)
            x, y, _ = self._mouse_button_pos(event)
            self._mouse_move(x, y)
            self.update_image()
            event.accept()

    # -------------------------------------------------------------------------------------------------
    # Qt offscreen server
    # -------------------------------------------------------------------------------------------------

    class QtServer(dvz.App):
        def __init__(self, background: tp.Optional[str] = None, c_flags: int = 0):

            # HACK: this will change in the next version
            if background == 'white':
                c_flags |= dvz.APP_FLAGS_WHITE_BACKGROUND

            self.c_server = dvz.server(c_flags)

            self.c_scene = dvz.scene(None)
            self.c_batch = dvz.scene_batch(self.c_scene)

            # NOTE: keep a reference to callbacks defined inside functions to avoid them being
            # garbage-collected, resulting in a segfault.
            self._callbacks = []

        def figure(self, width: int = 0, height: int = 0) -> QtFigure:
            c_figure = dvz.figure(self.c_scene, width or WIDTH, height or HEIGHT, 0)
            return QtFigure(c_figure, c_server=self.c_server, c_scene=self.c_scene)

        def __del__(self):
            dvz.server_destroy(self.c_server)
else:

    class QtFigure:  # pragma: no cover - import guard
        def __init__(self, *args, **kwargs):
            raise RuntimeError("PyQt5 or PyQt6 is required for datoviz.backends.pyqt6.")

    class QtServer(dvz.App):  # pragma: no cover - import guard
        def __init__(self, *args, **kwargs):
            raise RuntimeError("PyQt5 or PyQt6 is required for datoviz.backends.pyqt6.")
