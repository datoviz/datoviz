"""
Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
Licensed under the MIT license. See LICENSE file in the project root for details.
SPDX-License-Identifier: MIT
"""

# Panel

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

from typing import TYPE_CHECKING, Optional, Tuple

if TYPE_CHECKING:
    from ._app import App
    from ._figure import Figure

from . import _constants as cst
from . import _ctypes as dvz
from ._axes import Axes
from ._constants import Vec3
from .interact import Arcball, Camera, Fly, Ortho, Panzoom
from .shape_collection import ShapeCollection
from .utils import to_cvec4_array
from .visuals import Point, Sphere, Visual

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
    figure : Figure, optional
        The figure to which the panel belongs.
    """

    c_panel: dvz.DvzPanel = None

    _panzoom: Panzoom = None
    _ortho: Ortho = None
    _arcball: Arcball = None
    _camera: Camera = None
    _fly = None
    _axes: Axes = None
    _app: 'App' = None
    _figure: 'Figure' = None

    def __init__(
        self,
        c_panel: dvz.DvzPanel,
        figure: Optional['Figure'] = None,
    ) -> None:
        """
        Initialize a Panel instance.

        Parameters
        ----------
        c_panel : dvz.DvzPanel
            The underlying C panel object.
        figure : Figure, optional
            The figure to which the panel belongs, by default None.
        """
        assert c_panel
        self.c_panel = c_panel
        self._figure = figure
        self._app = figure._app

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

    def remove(self, visual: Visual) -> None:
        """
        Remove a visual from the panel.

        Parameters
        ----------
        visual : Visual
            The visual to remove.
        """
        assert visual
        dvz.panel_remove(self.c_panel, visual.c_visual)

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

    def background(self, color0: tuple, color1: tuple, color2: tuple, color3: tuple):
        dvz.panel_background(self.c_panel, to_cvec4_array((color0, color1, color2, color3)))

    def link(
        self, target: 'Panel', model: bool = False, view: bool = False, projection: bool = False
    ) -> None:
        """
        Link the current panel to another panel.

        Parameters
        ----------
        target : Panel
            The target panel to link to.
        model : bool, optional
            Whether to link the model matrix, by default False.
        view : bool, optional
            Whether to link the view matrix, by default False.
        projection : bool, optional
            Whether to link the projection matrix, by default False.
        """
        c_flags = 0
        if model:
            c_flags |= dvz.PANEL_LINK_FLAGS_MODEL
        if view:
            c_flags |= dvz.PANEL_LINK_FLAGS_VIEW
        if projection:
            c_flags |= dvz.PANEL_LINK_FLAGS_PROJECTION
        dvz.panel_link(target.c_panel, self.c_panel, c_flags)

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

    def fly(self, c_flags: int = 0) -> 'Fly':
        """
        Add fly camera controller to the panel.

        Similar to first-person camera controls in 3D video games:

        - Left mouse drag: Look around (yaw/pitch)
        - Right mouse drag: Move the camera left/right and up/down
        - Arrow keys: Move in view direction (up/down) or strafe (left/right)

        Parameters
        ----------
        c_flags : int, optional
            Flags for the fly controller, by default 0

            - DVZ_FLY_FLAGS_NONE: No special behavior
            - DVZ_FLY_FLAGS_INVERT_MOUSE: Invert mouse look controls

        Returns
        -------
        Fly
            The fly camera controller instance
        """
        if not self._fly:
            c_fly = dvz.panel_fly(self.c_panel, c_flags)
            self._fly = Fly(c_fly, self.c_panel)
        return self._fly

    # Axes
    # ---------------------------------------------------------------------------------------------

    def axes(self, xlim: Tuple[float, float] = None, ylim: Tuple[float, float] = None):
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
        visual = Point(c_visual)
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
        visual = Sphere(c_visual)
        return visual

    def guizmo(self) -> None:
        """
        Add a 3D guizmo to a 3D panel with an arcball.

        This function displays at the bottom right of the panel three 3D arrows representing
        the X (red), Y (green), and Z (blue) axes, which rotate with the arcball.

        !!! warning

            This feature is still experimental. A known issue is that the gizmo may be obscured
            by other visuals in the scene if they are rendered in front of it. This will be
            fixed in version 0.4.

        """
        w, h = self._figure.size()
        a = 0.25
        m = 0
        offset = (w - w * a - m, h - h * a - m)
        size = (w * a, h * a)

        guizmo_panel = self._figure.panel(offset=offset, size=size)
        guizmo_panel.camera(initial=(0, 0, 3))

        guizmo_sc = ShapeCollection()
        guizmo_sc.add_guizmo()

        guizmo_visual = self._app.mesh(guizmo_sc, lighting=True, depth_test=True)
        guizmo_visual.set_light_pos((2, 2, 5))

        # Diffuse.
        guizmo_visual.set_material_params((0.5,) * 3, idx=0)

        # Ambient.
        guizmo_visual.set_material_params((0.1,) * 3, idx=1)

        # Specular.
        guizmo_visual.set_material_params((0.1,) * 3, idx=2)

        guizmo_panel.add(guizmo_visual)

        self.link(guizmo_panel, model=True)

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
        This functionality is still experimental.

        """
        title = title or 'Panel'
        dvz.panel_gui(self.c_panel, title, c_flags)
