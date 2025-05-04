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
from ._constants import Vec3
from .interact import Arcball, Camera, Ortho, Panzoom
from .utils import button_name, from_enum, key_name
from .visuals import Visual

# -------------------------------------------------------------------------------------------------
# Event
# -------------------------------------------------------------------------------------------------


class Event:
    c_ev = None
    event_type: str = None

    def __init__(self, c_ev, event_type: str):
        assert c_ev
        self.c_ev = c_ev
        self.event_type = event_type

    # Properties
    # ---------------------------------------------------------------------------------------------

    @property
    def type(self):
        return self.c_ev.type

    # Mouse
    # ---------------------------------------------------------------------------------------------

    def is_mouse(self):
        return self.event_type == 'mouse'

    def mouse_event(self, prettify: bool = True):
        if self.is_mouse():
            return from_enum(dvz.MouseEventType, self.c_ev.type, prettify=prettify)

    def button(self):
        if self.is_mouse():
            return self.c_ev.button

    def button_name(self):
        if self.is_mouse():
            return button_name(self.button())

    def pos(self):
        if self.is_mouse():
            return tuple(self.c_ev.pos)

    def press_pos(self):
        if self.is_mouse() and self.mouse_event() in ('drag_start', 'drag_stop', 'drag'):
            return tuple(self.c_ev.content.d.press_pos)

    def wheel(self):
        if self.is_mouse() and self.mouse_event() in ('wheel',):
            return float(self.c_ev.content.w.dir[1])

    # Keyboard
    # ---------------------------------------------------------------------------------------------

    def is_keyboard(self):
        return self.event_type == 'keyboard'

    def key_event(self, prettify: bool = True):
        if self.is_keyboard():
            return from_enum(dvz.KeyboardEventType, self.c_ev.type, prettify=prettify)

    def key(self):
        if self.is_keyboard():
            return self.c_ev.key

    def key_name(self):
        if self.is_keyboard():
            return key_name(self.key())


# -------------------------------------------------------------------------------------------------
# Texture
# -------------------------------------------------------------------------------------------------


class Texture:
    c_texture: dvz.DvzTexture = None

    def __init__(self, c_texture: dvz.DvzTexture):
        assert c_texture is not None
        self.c_texture = c_texture


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

    def arcball(self, initial: Vec3 = None, c_flags: int = 0):
        c_arcball = dvz.panel_arcball(self.c_panel, c_flags)
        if initial is not None:
            dvz.arcball_initial(c_arcball, dvz.vec3(*initial))
            self.update()
        return Arcball(c_arcball, self.c_panel)

    def camera(
        self,
        initial: Vec3 = None,
        initial_lookat: Vec3 = None,
        initial_up: Vec3 = None,
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
