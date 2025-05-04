"""
Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
Licensed under the MIT license. See LICENSE file in the project root for details.
SPDX-License-Identifier: MIT
"""

# Base classes

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

from . import _constants as cst
from . import _ctypes as dvz
from .interact import Arcball, Camera, Ortho, Panzoom
from .visuals import Visual

# -------------------------------------------------------------------------------------------------
# Panel
# -------------------------------------------------------------------------------------------------


class Panel:
    c_panel: dvz.DvzPanel = None
    c_figure: dvz.DvzFigure = None

    def __init__(self, c_panel: dvz.DvzPanel, c_figure: dvz.DvzFigure = None):
        assert c_panel
        self.c_panel = c_panel
        self.c_figure = c_figure

    def add(self, visual: Visual):
        assert visual
        dvz.panel_visual(self.c_panel, visual.c_visual, 0)

    def update(self):
        dvz.panel_update(self.c_panel)

    def margins(self, top: float = 0, right: float = 0, bottom: float = 0, left: float = 0):
        dvz.panel_margins(self.c_panel, top, right, bottom, left)

    # Interactivity
    # ---------------------------------------------------------------------------------------------

    def panzoom(self, c_flags: int = 0):
        c_panzoom = dvz.panel_panzoom(self.c_panel, c_flags)
        return Panzoom(c_panzoom, self.c_panel)

    def ortho(self, c_flags: int = 0):
        c_ortho = dvz.panel_ortho(self.c_panel, c_flags)
        return Ortho(c_ortho, self.c_panel)

    def arcball(self, initial: cst.Vec3 = None, c_flags: int = 0):
        c_arcball = dvz.panel_arcball(self.c_panel, c_flags)
        if initial is not None:
            dvz.arcball_initial(c_arcball, dvz.vec3(*initial))
            self.update()
        return Arcball(c_arcball, self.c_panel)

    def camera(
        self,
        initial: cst.Vec3 = None,
        initial_lookat: cst.Vec3 = None,
        initial_up: cst.Vec3 = None,
        c_flags: int = 0,
    ):
        c_camera = dvz.panel_camera(self.c_panel, c_flags)
        pos = initial if initial is not None else cst.DEFAULT_CAMERA_POS
        lookat = initial_lookat if initial_lookat is not None else cst.DEFAULT_CAMERA_LOOKAT
        up = initial_up if initial_up is not None else cst.DEFAULT_CAMERA_UP
        if initial is not None:
            dvz.camera_initial(c_camera, dvz.vec3(*pos), dvz.vec3(*lookat), dvz.vec3(*up))
            self.update()
        return Camera(c_camera, self.c_panel)

    # Demo visuals
    # ---------------------------------------------------------------------------------------------

    def demo_2D(self):
        c_visual = dvz.demo_panel_2D(self.c_panel)
        visual = Visual(c_visual, 'demo_2D')
        return visual

    def demo_3D(self):
        c_visual = dvz.demo_panel_3D(self.c_panel)
        visual = Visual(c_visual, 'demo_3D')
        return visual

    # GUI
    # ---------------------------------------------------------------------------------------------

    def gui(self, title: str = None, c_flags: int = 0):
        title = title or 'Panel'
        dvz.panel_gui(self.c_panel, title, c_flags)
