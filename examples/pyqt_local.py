"""# PyQt6 local example

This example illustrates how to integrate offscreen Datoviz figures into a PyQt6 application,
using the Datoviz server API which provides a fully offscreen renderer with support for multiple
canvases.

Illustrates:

- Creating a PyQt6 application
- Using the Datoviz server API
- Offscreen rendering with the Datoviz server API

"""

import sys

from PyQt6.QtWidgets import QApplication, QMainWindow, QSplitter
from PyQt6.QtCore import Qt

import datoviz as dvz
from datoviz.backends.pyqt6 import QtServer


WIDTH, HEIGHT = 800, 600


class ExampleWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Example Qt Datoviz window")

        # Create a Qt Datoviz server.
        self.qt_server = QtServer()

        # Create two figures (special Qt widgets with a Datoviz figure).
        w, h = WIDTH // 2, HEIGHT
        self.qt_figure1 = self.qt_server.create_figure(w, h)
        self.qt_figure2 = self.qt_server.create_figure(w, h)

        # Fill the figures with mock data.
        dvz.demo_panel(dvz.panel(self.qt_figure1.figure, 0, 0, w, h))
        dvz.demo_panel(dvz.panel(self.qt_figure2.figure, 0, 0, w, h))

        # Add the two figures in the main window.
        splitter = QSplitter(Qt.Orientation.Horizontal)
        splitter.addWidget(self.qt_figure1)
        splitter.addWidget(self.qt_figure2)
        splitter.setCollapsible(0, False)
        splitter.setCollapsible(1, False)
        self.setCentralWidget(splitter)
        self.resize(WIDTH, HEIGHT)

        # self.setMouseTracking(True)
        # splitter.setMouseTracking(True)
        # self.qt_figure1.setMouseTracking(True)
        # self.qt_figure2.setMouseTracking(True)


if __name__ == "__main__":
    app = QApplication(sys.argv)
    mw = ExampleWindow()
    mw.show()
    sys.exit(app.exec())
