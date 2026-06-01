#!/usr/bin/env python3
"""Embed a Datoviz view in a normal PyQt6 Widgets application."""

from __future__ import annotations

import ctypes
import random
import sys

from PyQt6.QtCore import Qt
from PyQt6.QtWidgets import QApplication, QHBoxLayout, QLabel, QMainWindow, QPushButton
from PyQt6.QtWidgets import QSlider, QVBoxLayout, QWidget

import datoviz.raw as dvz
from datoviz.qt import DatovizWidget


def _void_p(array: ctypes.Array) -> ctypes.c_void_p:
    return ctypes.cast(array, ctypes.c_void_p)


class ExampleScene:
    def __init__(self):
        self.scene = dvz.dvz_scene()
        if not self.scene:
            raise RuntimeError('dvz_scene() failed')

        self.figure = dvz.dvz_figure(self.scene, 800, 600, 0)
        self.panel = dvz.dvz_panel_full(self.figure)
        self.visual = dvz.dvz_point(self.scene, 0)
        if not self.figure or not self.panel or not self.visual:
            raise RuntimeError('Datoviz scene setup failed')

        self.positions = (ctypes.c_float * 9)(
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
        self.colors = (dvz.DvzColor * 3)(
            dvz.DvzColor(255, 80, 80, 255),
            dvz.DvzColor(80, 220, 120, 255),
            dvz.DvzColor(90, 150, 255, 255),
        )
        self.diameters = (ctypes.c_float * 3)(24.0, 24.0, 24.0)

        self._upload()
        if dvz.dvz_panel_add_visual(self.panel, self.visual, None) != 0:
            raise RuntimeError('dvz_panel_add_visual() failed')
        dvz.dvz_panel_set_background_color(self.panel, 0.05, 0.06, 0.08, 1.0)

        controller = dvz.dvz_panzoom(self.scene, None)
        if not controller:
            raise RuntimeError('dvz_panzoom() failed')
        if dvz.dvz_panel_bind_controller(
            self.panel, controller, dvz.DvzDimMaskFlag.DVZ_DIM_MASK_XY
        ):
            raise RuntimeError('dvz_panel_bind_controller() failed')

    def destroy(self):
        if self.scene:
            dvz.dvz_scene_destroy(self.scene)
            self.scene = None

    def set_point_size(self, size: int):
        for i in range(3):
            self.diameters[i] = float(size)
        if dvz.dvz_visual_set_data(self.visual, b'diameter', _void_p(self.diameters), 3) != 0:
            raise RuntimeError('dvz_visual_set_data(diameter) failed')

    def randomize_colors(self):
        for i in range(3):
            self.colors[i] = dvz.DvzColor(
                random.randint(80, 255),
                random.randint(80, 255),
                random.randint(80, 255),
                255,
            )
        if dvz.dvz_visual_set_data(self.visual, b'color', _void_p(self.colors), 3) != 0:
            raise RuntimeError('dvz_visual_set_data(color) failed')

    def _upload(self):
        if dvz.dvz_visual_set_data(self.visual, b'position', _void_p(self.positions), 3) != 0:
            raise RuntimeError('dvz_visual_set_data(position) failed')
        if dvz.dvz_visual_set_data(self.visual, b'color', _void_p(self.colors), 3) != 0:
            raise RuntimeError('dvz_visual_set_data(color) failed')
        if dvz.dvz_visual_set_data(self.visual, b'diameter', _void_p(self.diameters), 3) != 0:
            raise RuntimeError('dvz_visual_set_data(diameter) failed')


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle('Datoviz PyQt hosting')
        self.example = ExampleScene()
        self.datoviz = DatovizWidget(self.example.scene, self.example.figure, self)

        point_size = QSlider(Qt.Orientation.Horizontal)
        point_size.setRange(8, 64)
        point_size.setValue(24)
        point_size.valueChanged.connect(self._set_point_size)

        randomize = QPushButton('Randomize colors')
        randomize.clicked.connect(self._randomize_colors)

        controls = QWidget()
        controls_layout = QVBoxLayout(controls)
        controls_layout.addWidget(QLabel('Point size'))
        controls_layout.addWidget(point_size)
        controls_layout.addWidget(randomize)
        controls_layout.addStretch(1)

        root = QWidget()
        layout = QHBoxLayout(root)
        layout.addWidget(self.datoviz, 1)
        layout.addWidget(controls)
        self.setCentralWidget(root)
        self.resize(1000, 650)

    def closeEvent(self, event):
        self.datoviz.release()
        self.example.destroy()
        super().closeEvent(event)

    def _set_point_size(self, value: int):
        self.example.set_point_size(value)
        self.datoviz.request_frame()

    def _randomize_colors(self):
        self.example.randomize_colors()
        self.datoviz.request_frame()


def main() -> int:
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    return int(app.exec())


if __name__ == '__main__':
    raise SystemExit(main())
