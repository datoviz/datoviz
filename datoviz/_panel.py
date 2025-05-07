"""
Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
Licensed under the MIT license. See LICENSE file in the project root for details.
SPDX-License-Identifier: MIT
"""

# Panel

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

from typing import Optional

from . import _constants as cst
from . import _ctypes as dvz
from ._axes import Axes
from ._constants import Vec3
from .interact import Arcball, Camera, Ortho, Panzoom
from .visuals import Visual

# -------------------------------------------------------------------------------------------------
# Panel
# -------------------------------------------------------------------------------------------------


class Panel:
    """
    Represents a panel in a figure, which can contain visuals and interactivity.

    Attributes
    ----------
    c_panel : dvz.DvzPanel
        The underlying C panel object.
    c_figure : dvz.DvzFigure, optional
        The figure to which the panel belongs.
    """

    c_panel: dvz.DvzPanel = None
    c_figure: Optional[dvz.DvzFigure] = None

    _panzoom: Panzoom = None
    _ortho: Ortho = None
    _arcball: Arcball = None
    _camera: Camera = None
    _axes: Axes = None

    def __init__(self, c_panel: dvz.DvzPanel, c_figure: Optional[dvz.DvzFigure] = None) -> None:
        """
        Initialize a Panel instance.

        Parameters
        ----------
        c_panel : dvz.DvzPanel
            The underlying C panel object.
        c_figure : dvz.DvzFigure, optional
            The figure to which the panel belongs, by default None.
        """
        assert c_panel
        self.c_panel = c_panel
        self.c_figure = c_figure

    def add(self, visual: Visual) -> None:
        """
        Add a visual to the panel.

        Parameters
        ----------
        visual : Visual
            The visual to add.
        """
        assert visual
        dvz.panel_visual(self.c_panel, visual.c_visual, 0)

    def update(self) -> None:
        """
        Update the panel.
        """
        dvz.panel_update(self.c_panel)

    def margins(
        self, top: float = 0, right: float = 0, bottom: float = 0, left: float = 0
    ) -> None:
        """
        Set the margins of the panel.

        Parameters
        ----------
        top : float, optional
            Top margin in pixels, by default 0.
        right : float, optional
            Right margin in pixels, by default 0.
        bottom : float, optional
            Bottom margin in pixels, by default 0.
        left : float, optional
            Left margin in pixels, by default 0.
        """
        dvz.panel_margins(self.c_panel, top, right, bottom, left)

    # Interactivity
    # ---------------------------------------------------------------------------------------------

    def panzoom(self, c_flags: int = 0, fixed: Optional[str] = None) -> Panzoom:
        """
        Add panzoom interactivity to the panel.

        Parameters
        ----------
        c_flags : int, optional
            Datoviz flags for the panzoom interactivity, by default 0.
        fixed : str, optional
            'x' or 'y' to fix the panzoom interactivity along a given axis.

        Returns
        -------
        Panzoom
            The panzoom interactivity instance.
        """
        if not self._panzoom:
            if fixed is not None:
                if 'x' in fixed:
                    c_flags |= dvz.PANZOOM_FLAGS_FIXED_X
                if 'y' in fixed:
                    c_flags |= dvz.PANZOOM_FLAGS_FIXED_Y
            c_panzoom = dvz.panel_panzoom(self.c_panel, c_flags)
            self._panzoom = Panzoom(c_panzoom, self.c_panel)
        return self._panzoom

    def ortho(self, c_flags: int = 0) -> Ortho:
        """
        Add orthographic interactivity to the panel.

        Parameters
        ----------
        c_flags : int, optional
            Datoviz flags for the orthographic interactivity, by default 0.

        Returns
        -------
        Ortho
            The orthographic interactivity instance.
        """
        if not self._ortho:
            c_ortho = dvz.panel_ortho(self.c_panel, c_flags)
            self._ortho = Ortho(c_ortho, self.c_panel)
        return self._ortho

    def arcball(self, initial: Optional[Vec3] = None, c_flags: int = 0) -> Arcball:
        """
        Add arcball interactivity to the panel.

        Parameters
        ----------
        initial : Vec3, optional
            Initial position of the arcball, by default None.
        c_flags : int, optional
            Datoviz flags for the arcball interactivity, by default 0.

        Returns
        -------
        Arcball
            The arcball interactivity instance.
        """
        if not self._arcball:
            c_arcball = dvz.panel_arcball(self.c_panel, c_flags)
            if initial is not None:
                dvz.arcball_initial(c_arcball, dvz.vec3(*initial))
                self.update()
            self._arcball = Arcball(c_arcball, self.c_panel)
        return self._arcball

    def camera(
        self,
        initial: Optional[Vec3] = None,
        initial_lookat: Optional[Vec3] = None,
        initial_up: Optional[Vec3] = None,
        c_flags: int = 0,
    ) -> Camera:
        """
        Add 3D camera interactivity to the panel.

        Parameters
        ----------
        initial : Vec3, optional
            Initial camera position, by default None.
        initial_lookat : Vec3, optional
            Initial look-at position, by default None.
        initial_up : Vec3, optional
            Initial up vector, by default None.
        c_flags : int, optional
            Datoviz flags for the camera interactivity, by default 0.

        Returns
        -------
        Camera
            The camera interactivity instance.
        """
        if not self._camera:
            c_camera = dvz.panel_camera(self.c_panel, c_flags)
            pos = initial if initial is not None else cst.DEFAULT_CAMERA_POS
            lookat = initial_lookat if initial_lookat is not None else cst.DEFAULT_CAMERA_LOOKAT
            up = initial_up if initial_up is not None else cst.DEFAULT_CAMERA_UP
            if initial is not None:
                dvz.camera_initial(c_camera, dvz.vec3(*pos), dvz.vec3(*lookat), dvz.vec3(*up))
                self.update()
            self._camera = Camera(c_camera, self.c_panel)
        return self._camera

    # Axes
    # ---------------------------------------------------------------------------------------------

    def axes(self, xlim: tuple[float, float] = None, ylim: tuple[float, float] = None):
        if self._axes is None:
            xlim = xlim or cst.NDC
            ylim = ylim or cst.NDC
            xmin, xmax = xlim
            ymin, ymax = ylim

            c_axes = dvz.panel_axes_2D(self.c_panel, xmin, xmax, ymin, ymax)
            c_ref = dvz.panel_ref(self.c_panel)
            c_panzoom = dvz.panel_panzoom(self.c_panel, 0)

            self._axes = Axes(c_axes, c_ref, c_panzoom, self.c_panel)
        return self._axes

    # Demo visuals
    # ---------------------------------------------------------------------------------------------

    def demo_2D(self) -> Visual:
        """
        Add a 2D demo visual to the panel.

        Returns
        -------
        Visual
            The 2D demo visual instance.
        """
        c_visual = dvz.demo_panel_2D(self.c_panel)
        visual = Visual(c_visual, 'demo_2D')
        return visual

    def demo_3D(self) -> Visual:
        """
        Add a 3D demo visual to the panel.

        Returns
        -------
        Visual
            The 3D demo visual instance.
        """
        c_visual = dvz.demo_panel_3D(self.c_panel)
        visual = Visual(c_visual, 'demo_3D')
        return visual

    # GUI
    # ---------------------------------------------------------------------------------------------

    def gui(self, title: Optional[str] = None, c_flags: int = 0) -> None:
        """
        Convert a standard Datoviz panel to a movable GUI panel.

        Parameters
        ----------
        title : str, optional
            Title of the GUI, by default 'Panel'.
        c_flags : int, optional
            Datoviz flags for the GUI, by default 0.

        Warnings
        --------
        .. warning::
            This functionality is still experimental.

        """
        title = title or 'Panel'
        dvz.panel_gui(self.c_panel, title, c_flags)
