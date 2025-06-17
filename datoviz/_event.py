"""
Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
Licensed under the MIT license. See LICENSE file in the project root for details.
SPDX-License-Identifier: MIT
"""

# Event

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

from typing import Any, Optional, Tuple

from . import _ctypes as dvz
from .utils import button_name, from_enum, key_name

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
            The type of the event (e.g., 'mouse', 'keyboard', 'timer').
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
            The mouse event type (`click`, `double_click`, `drag`, `drag_start`, `drag_stop`,
            `move`, `press`, `release`, `wheel`), or None if not a mouse event.
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

    def pos(self) -> Optional[Tuple[float, float]]:
        """
        Get the position of the mouse event.

        Returns
        -------
        tuple of float or None
            The (x, y) position of the mouse event, or None if not a mouse event.
        """
        if self.is_mouse():
            return tuple(self.c_ev.pos)

    def press_pos(self) -> Optional[Tuple[float, float]]:
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

    # Timer
    # ---------------------------------------------------------------------------------------------

    def is_timer(self) -> bool:
        """
        Return whether the event is a timer event.

        Returns
        -------
        bool
            True if the event is a timer event, False otherwise.
        """
        return self.event_type == 'timer'

    def tick(self):
        """
        Return the current tick index of a timer event.

        Returns
        -------
        int
            The tick index, starting from 0.
        """
        if self.is_timer():
            return self.c_ev.step_idx

    def time(self):
        """
        Return the current time of the event in seconds.

        Returns
        -------
        float
            The time in seconds since the start of the timer.
        """
        return self.c_ev.time

    # Frame
    # ---------------------------------------------------------------------------------------------

    def is_frame(self) -> bool:
        """
        Return whether the event is a frame event.

        Returns
        -------
        bool
            True if the event is a frame event, False otherwise.
        """
        return self.event_type == 'frame'

    def frame_idx(self):
        """
        Return the current frame index of a frame event.

        Returns
        -------
        int
            The frame index, starting from 0.
        """
        if self.is_frame():
            return self.c_ev.frame_idx

    # Resize
    # ---------------------------------------------------------------------------------------------

    def is_resize(self) -> bool:
        """
        Return whether the event is a resize event.

        Returns
        -------
        bool
            True if the event is a resize event, False otherwise.
        """
        return self.event_type == 'resize'

    def screen_width(self):
        """
        Return the current screen width of a resize event.

        Returns
        -------
        int
            The screen width.
        """
        if self.is_resize():
            return self.c_ev.screen_width

    def screen_height(self):
        """
        Return the current screen height of a resize event.

        Returns
        -------
        int
            The screen height.
        """
        if self.is_resize():
            return self.c_ev.screen_height
