"""
Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
Licensed under the MIT license. See LICENSE file in the project root for details.
SPDX-License-Identifier: MIT
"""

# Figure

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

from . import _constants as cst
from . import _ctypes as dvz
from ._panel import Panel
from .utils import to_cvec4_array

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

    def panel(
        self,
        offset: tuple[float, float] = None,
        size: tuple[float, float] = None,
        background: tuple[
            tuple[int, int, int, int],
            tuple[int, int, int, int],
            tuple[int, int, int, int],
            tuple[int, int, int, int],
        ] = None,
    ) -> Panel:
        """
        Create a new panel in the figure.

        Parameters
        ----------
        offset : tuple of float, optional
            The (x, y) offset of the panel, in pixels, by default (0, 0).
        size : tuple of float, optional
            The (width, height) size of the panel, by default the entire window size.
        background : tuple or boolean, optional
            If True, show a default gradient background. Otherwise, a tuple of
            four RGBA colors, for each corner of the panel (top-left, top-right, bottom-left,
            bottom-right).

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
        if background:
            if background is True:
                background = cst.DEFAULT_BACKGROUND
            dvz.panel_background(c_panel, to_cvec4_array(background))
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
