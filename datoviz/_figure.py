"""
Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
Licensed under the MIT license. See LICENSE file in the project root for details.
SPDX-License-Identifier: MIT
"""

# Figure

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

from typing import TYPE_CHECKING, Optional, Tuple

if TYPE_CHECKING:
    from ._app import App

from . import _constants as cst
from . import _ctypes as dvz
from ._panel import Panel
from .utils import to_cvec4_array, to_enum

# -------------------------------------------------------------------------------------------------
# Figure
# -------------------------------------------------------------------------------------------------


class Colorbar:
    """
    Represents a colorbar associated with a panel.

    Attributes
    ----------
    c_colorbar : dvz.DvzColorbar
        The underlying C colorbar object.
    colorbar_panel : Panel
        The panel in which the colorbar is displayed.
    """

    c_colorbar: dvz.DvzColorbar = None
    colorbar_panel: 'Panel' = None

    def __init__(self, c_colorbar: dvz.DvzColorbar, colorbar_panel: 'Panel'):
        """
        Initialize a Colorbar instance.

        Parameters
        ----------
        c_colorbar : dvz.DvzColorbar
            The underlying C colorbar object.
        colorbar_panel : Panel
            The panel to attach the colorbar to.
        """
        self.c_colorbar = c_colorbar
        self.colorbar_panel = colorbar_panel

    def set_cmap(self, cmap: str):
        """
        Set the colormap of the colorbar.

        Parameters
        ----------
        cmap : str
            The name of the colormap to use.
        """
        c_cmap = to_enum(f'cmap_{cmap}')
        dvz.colorbar_cmap(self.c_colorbar, c_cmap)
        dvz.colorbar_update(self.c_colorbar)

    def set_range(self, dmin: float, dmax: float):
        """
        Set the data range of the colorbar.

        Parameters
        ----------
        dmin : float
            Minimum data value.
        dmax : float
            Maximum data value.
        """
        dvz.colorbar_range(self.c_colorbar, dmin, dmax)

    def destroy(self):
        """
        Destroy the colorbar.
        """
        dvz.colorbar_destroy(self.c_colorbar)


class Figure:
    """
    Represents a figure, which is a container for panels.

    Attributes
    ----------
    c_figure : dvz.DvzFigure
        The underlying C figure object.
    """

    c_figure: dvz.DvzFigure = None
    _app: 'App' = None
    colorbar: Colorbar = None

    def __init__(self, c_figure: dvz.DvzFigure, app: Optional['App'] = None) -> None:
        """
        Initialize a Figure instance.

        Parameters
        ----------
        c_figure : dvz.DvzFigure
            The underlying C figure object.
        app : App
            The App instance.
        """
        assert c_figure
        self.c_figure = c_figure
        self._app = app

    def size(self):
        """
        Get the size of the figure in pixels.

        Returns
        -------
        tuple of int
            The (width, height) of the figure.
        """
        return (dvz.figure_width(self.c_figure), dvz.figure_height(self.c_figure))

    def set_fullscreen(self, fullscreen: bool) -> None:
        """
        Set figure to fullscreen mode.

        Parameters
        ----------
        is_fullscreen : True for fullscreen mode, False for window mode.
        """
        dvz.app_fullscreen(self._app.c_app, self.figure_id(), fullscreen)

    def panel(
        self,
        offset: Tuple[float, float] = None,
        size: Tuple[float, float] = None,
        background: Tuple[
            Tuple[int, int, int, int],
            Tuple[int, int, int, int],
            Tuple[int, int, int, int],
            Tuple[int, int, int, int],
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
        return Panel(c_panel, figure=self)

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

    def _create_colorbar_panel(self):
        """
        Create and return a panel for the colorbar.

        Returns
        -------
        Panel
            The panel used for displaying the colorbar.
        """
        w, h = self.size()
        cw, ch = 80, h - 40
        m = 20
        colorbar_panel = self.panel((w - cw - m, m), (cw, ch))
        colorbar_panel.margins(m // 2, m // 2, m // 2, m // 2)
        return colorbar_panel

    def colorbar(self, cmap: str = 'hsv', dmin: float = 0, dmax: float = 1):
        """
        Create a colorbar in the figure.

        Parameters
        ----------
        cmap : str
            The colormap name.
        dmin : float
            Minimum data value.
        dmax : float
            Maximum data value.

        Returns
        -------
        Colorbar
            The created colorbar instance.
        """
        c_cmap = to_enum(f'cmap_{cmap}')
        c_colorbar = dvz.colorbar(self._app.c_batch, c_cmap, dmin, dmax, 0)

        colorbar_panel = self._create_colorbar_panel()
        dvz.colorbar_panel(c_colorbar, colorbar_panel.c_panel)
        return Colorbar(c_colorbar, colorbar_panel)

    def destroy(self) -> None:
        """
        Destroy the figure.
        """
        if self.c_colorbar:
            dvz.colorbar_destroy(self.c_colorbar)
