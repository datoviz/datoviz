"""
Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
Licensed under the MIT license. See LICENSE file in the project root for details.
SPDX-License-Identifier: MIT
"""

# Figure

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import typing as tp
from ._panel import Panel
from . import _ctypes as dvz


# -------------------------------------------------------------------------------------------------
# Figure
# -------------------------------------------------------------------------------------------------

class Figure:
    c_figure: dvz.DvzFigure = None

    def __init__(self, c_figure: dvz.DvzFigure):
        assert c_figure
        self.c_figure = c_figure

    def panel(self, offset: tuple = None, size: tuple = None):
        if not offset and not size:
            c_panel = dvz.panel_default(self.c_figure)
        else:
            x, y = offset
            w, h = size
            c_panel = dvz.panel(self.c_figure, x, y, w, h)
        return Panel(c_panel, c_figure=self.c_figure)

    def update(self):
        dvz.figure_update(self.c_figure)

    def figure_id(self):
        return dvz.figure_id(self.c_figure)
