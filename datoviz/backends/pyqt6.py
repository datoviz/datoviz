"""PyQt6 backend

"""

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import ctypes

try:
    from PyQt6.QtWidgets import QLabel, QWidget, QVBoxLayout
    from PyQt6.QtGui import QImage, QPixmap
    from PyQt6.QtCore import Qt
except:
    from PyQt5.QtWidgets import QLabel, QWidget, QVBoxLayout
    from PyQt5.QtGui import QImage, QPixmap
    from PyQt5.QtCore import Qt

import datoviz as dvz
from datoviz import vec2


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

class QtFigure(QWidget):
    def __init__(self, figure, server=None, scene=None):
        super().__init__()
        self.figure = figure
        self.server = server
        self.scene = scene
        self.mouse = dvz.server_mouse(server)

        self.label = QLabel(self)
        self.label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        layout = QVBoxLayout()
        layout.addWidget(self.label)
        layout.setContentsMargins(0, 0, 0, 0)
        self.setLayout(layout)
        self.setMinimumSize(50, 50)

        # self.resize_timer = QTimer(self)
        # self.resize_timer.setSingleShot(True)
        # self.resize_timer.timeout.connect(self.update_image)

        self.update_image()

    def update_image(self):
        label_size = self.label.size()
        w, h = label_size.width(), label_size.height()
        if w <= 0 or h <= 0:
            return

        dvz.figure_resize(self.figure, w, h)
        dvz.figure_update(self.figure)
        data = dvz.scene_render(self.scene, self.server, self.figure, 0)

        if data is None:
            return

        data_bytes = bytearray(ctypes.string_at(data, w * h * 3))
        self.image = QImage(data_bytes, w, h, 3 * w,
                            QImage.Format.Format_RGB888)
        self.pixmap = QPixmap.fromImage(self.image)
        self.label.setPixmap(self.pixmap)

    def _mouse_button_pos(self, event):
        try:
            button = event.button()
        except:
            button = 0
        dvz_button = BUTTON_MAPPING.get(button, 0)
        try:
            pos = event.pos()
        except:
            pos = event.position()
        x = pos.x()
        y = pos.y()
        return x, y, dvz_button

    def _mouse_move(self, x, y):
        ev = dvz.mouse_move(self.mouse, vec2(x, y), 0)
        dvz.scene_mouse(self.scene, self.figure, ev)

    def _mouse_press(self, dvz_button):
        ev = dvz.mouse_press(self.mouse, dvz_button, 0)
        dvz.scene_mouse(self.scene, self.figure, ev)

    def _mouse_release(self, dvz_button):
        ev = dvz.mouse_release(self.mouse, dvz_button, 0)
        dvz.scene_mouse(self.scene, self.figure, ev)

    def _mouse_wheel(self, dir):
        ev = dvz.mouse_wheel(self.mouse, vec2(0, dir), 0)
        dvz.scene_mouse(self.scene, self.figure, ev)

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
        delta = event.angleDelta().y()
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

class QtServer:
    def __init__(self):
        self.server = dvz.server(0)
        self.scene = dvz.scene(None)

    def create_figure(self, width=None, height=None):
        figure = dvz.figure(self.scene, width or WIDTH, height or HEIGHT, 0)
        return QtFigure(figure, server=self.server, scene=self.scene)

    def __del__(self):
        dvz.server_destroy(self.server)
