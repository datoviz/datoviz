"""
Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
Licensed under the MIT license. See LICENSE file in the project root for details.
SPDX-License-Identifier: MIT
"""

# Figure

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

from . import _ctypes as dvz
from ._panel import Panel

# -------------------------------------------------------------------------------------------------
# Figure
# -------------------------------------------------------------------------------------------------


class Figure:
    """
    Represents a figure, which is a container for panels.

    Attributes
    ----------
    c_figure : dvz.DvzFigure
        The underlying C figure object.
    """

    c_figure: dvz.DvzFigure = None

    def __init__(self, c_figure: dvz.DvzFigure) -> None:
        """
        Initialize a Figure instance.

        Parameters
        ----------
        c_figure : dvz.DvzFigure
            The underlying C figure object.
        """
        assert c_figure
        self.c_figure = c_figure

    def panel(self, offset: tuple[float, float] = None, size: tuple[float, float] = None) -> Panel:
        """
        Create a new panel in the figure.

        Parameters
        ----------
        offset : tuple of float, optional
            The (x, y) offset of the panel, in pixels, by default (0, 0).
        size : tuple of float, optional
            The (width, height) size of the panel, by default the entire window size.

        Returns
        -------
        Panel
            The created panel instance.
        """
        if not offset and not size:
            c_panel = dvz.panel_default(self.c_figure)
        else:
            x, y = offset
            w, h = size
            c_panel = dvz.panel(self.c_figure, x, y, w, h)
        return Panel(c_panel, c_figure=self.c_figure)

    def update(self) -> None:
        """
        Update the figure.
        """
        dvz.figure_update(self.c_figure)

    def figure_id(self) -> int:
        """
        Get the Datoviz internal ID of the figure.

        Returns
        -------
        int
            The ID of the figure.
        """
        return dvz.figure_id(self.c_figure)
