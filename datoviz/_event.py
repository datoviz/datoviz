"""
Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
Licensed under the MIT license. See LICENSE file in the project root for details.
SPDX-License-Identifier: MIT
"""

# Base classes

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

from typing import Any, Optional, tuple

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
    """
    Represents an event in the application, such as mouse or keyboard input.

    Attributes
    ----------
    c_ev : Any
        The underlying C event object.
    event_type : str
        The type of the event (e.g., 'mouse', 'keyboard').
    """

    c_ev: Any = None
    event_type: str = None

    def __init__(self, c_ev: Any, event_type: str) -> None:
        """
        Initialize an Event instance.

        Parameters
        ----------
        c_ev : Any
            The underlying C event object.
        event_type : str
            The type of the event (e.g., 'mouse', 'keyboard').
        """
        assert c_ev
        self.c_ev = c_ev
        self.event_type = event_type

    # Properties
    # ---------------------------------------------------------------------------------------------

    @property
    def type(self) -> int:
        """
        Get the C event type.

        Returns
        -------
        int
            The C event type as an integer.
        """
        return self.c_ev.type

    # Mouse
    # ---------------------------------------------------------------------------------------------

    def is_mouse(self) -> bool:
        """
        Return whether the event is a mouse event.

        Returns
        -------
        bool
            True if the event is a mouse event, False otherwise.
        """
        return self.event_type == 'mouse'

    def mouse_event(self, prettify: bool = True) -> Optional[str]:
        """
        Get the mouse event type.

        Parameters
        ----------
        prettify : bool, optional
            Whether to return a prettified string representation, by default True.

        Returns
        -------
        str or None
            The mouse event type, or None if not a mouse event.
        """
        if self.is_mouse():
            return from_enum(dvz.MouseEventType, self.c_ev.type, prettify=prettify)

    def button(self) -> Optional[int]:
        """
        Get the mouse button C enumeration associated with the event.

        Returns
        -------
        int or None
            The mouse button, or None if not a mouse event.
        """
        if self.is_mouse():
            return self.c_ev.button

    def button_name(self) -> Optional[str]:
        """
        Get the name of the mouse button.

        Returns
        -------
        str or None
            The name of the mouse button (`left`, `right` or `middle`), or None if not a
            mouse event.
        """
        if self.is_mouse():
            return button_name(self.button())

    def pos(self) -> Optional[tuple[float, float]]:
        """
        Get the position of the mouse event.

        Returns
        -------
        tuple of float or None
            The (x, y) position of the mouse event, or None if not a mouse event.
        """
        if self.is_mouse():
            return tuple(self.c_ev.pos)

    def press_pos(self) -> Optional[tuple[float, float]]:
        """
        Get the position where the mouse was pressed during a drag event.

        Returns
        -------
        tuple of float or None
            The (x, y) position of the press event, or None if not applicable.
        """
        if self.is_mouse() and self.mouse_event() in ('drag_start', 'drag_stop', 'drag'):
            return tuple(self.c_ev.content.d.press_pos)

    def wheel(self) -> Optional[float]:
        """
        Get the wheel scroll amount (in vertical direction).

        Returns
        -------
        float or None
            The scroll amount, or None if not a wheel event.
        """
        if self.is_mouse() and self.mouse_event() in ('wheel',):
            return float(self.c_ev.content.w.dir[1])

    # Keyboard
    # ---------------------------------------------------------------------------------------------

    def is_keyboard(self) -> bool:
        """
        Return whether the event is a keyboard event.

        Returns
        -------
        bool
            True if the event is a keyboard event, False otherwise.
        """
        return self.event_type == 'keyboard'

    def key_event(self, prettify: bool = True) -> Optional[str]:
        """
        Get the keyboard event type.

        Parameters
        ----------
        prettify : bool, optional
            Whether to return a prettified string representation, by default True.

        Returns
        -------
        str or None
            The keyboard event type, or None if not a keyboard event.
        """
        if self.is_keyboard():
            return from_enum(dvz.KeyboardEventType, self.c_ev.type, prettify=prettify)

    def key(self) -> Optional[int]:
        """
        Get the key C enumeration associated with the keyboard event.

        Returns
        -------
        int or None
            The key code, or None if not a keyboard event.
        """
        if self.is_keyboard():
            return self.c_ev.key

    def key_name(self) -> Optional[str]:
        """
        Get the name of the key associated with the keyboard event.

        Returns
        -------
        str or None
            The name of the key, or None if not a keyboard event.
        """
        if self.is_keyboard():
            return key_name(self.key())


# -------------------------------------------------------------------------------------------------
# Texture
# -------------------------------------------------------------------------------------------------


class Texture:
    """
    Represents a texture in the application.

    Attributes
    ----------
    c_texture : dvz.DvzTexture
        The underlying C texture object.
    """

    c_texture: dvz.DvzTexture = None

    def __init__(self, c_texture: dvz.DvzTexture) -> None:
        """
        Initialize a Texture instance.

        Parameters
        ----------
        c_texture : dvz.DvzTexture
            The underlying C texture object.
        """
        assert c_texture is not None
        self.c_texture = c_texture


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

    def panzoom(self, c_flags: int = 0) -> Panzoom:
        """
        Add panzoom interactivity to the panel.

        Parameters
        ----------
        c_flags : int, optional
            Datoviz flags for the panzoom interactivity, by default 0.

        Returns
        -------
        Panzoom
            The panzoom interactivity instance.
        """
        c_panzoom = dvz.panel_panzoom(self.c_panel, c_flags)
        return Panzoom(c_panzoom, self.c_panel)

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
        c_ortho = dvz.panel_ortho(self.c_panel, c_flags)
        return Ortho(c_ortho, self.c_panel)

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
        c_arcball = dvz.panel_arcball(self.c_panel, c_flags)
        if initial is not None:
            dvz.arcball_initial(c_arcball, dvz.vec3(*initial))
            self.update()
        return Arcball(c_arcball, self.c_panel)

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
