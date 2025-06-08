"""
Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
Licensed under the MIT license. See LICENSE file in the project root for details.
SPDX-License-Identifier: MIT
"""

# Interactivity

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

from typing import Tuple

from . import _ctypes as dvz

# -------------------------------------------------------------------------------------------------
# Interact
# -------------------------------------------------------------------------------------------------


class Panzoom:
    """
    Represents panzoom interactivity for a panel.

    Attributes
    ----------
    c_panzoom : dvz.DvzPanzoom
        The underlying C panzoom object.
    c_panel : dvz.DvzPanel, optional
        The panel to which the panzoom interactivity is attached.
    """

    c_panzoom: dvz.DvzPanzoom = None
    c_panel: dvz.DvzPanel = None

    def __init__(self, c_panzoom: dvz.DvzPanzoom, c_panel: dvz.DvzPanel = None) -> None:
        """
        Initialize a Panzoom instance.

        Parameters
        ----------
        c_panzoom : dvz.DvzPanzoom
            The underlying C panzoom object.
        c_panel : dvz.DvzPanel, optional
            The panel to which the panzoom interactivity is attached, by default None.
        """
        assert c_panzoom
        self.c_panzoom = c_panzoom
        self.c_panel = c_panel

    def zoom(self, zx: float = 1.0, zy: float = 1.0):
        """
        Set the zoom factor.

        Parameters
        ----------
        zx : float
            The zoom factor along the X axis.
        zy : float
            The zoom factor along the Y axis.
        """
        dvz.panzoom_zoom(self.c_panzoom, dvz.vec2(zx, zy))


class Ortho:
    """
    Represents orthographic interactivity for a panel.

    Attributes
    ----------
    c_ortho : dvz.DvzOrtho
        The underlying C ortho object.
    c_panel : dvz.DvzPanel, optional
        The panel to which the ortho interactivity is attached.
    """

    c_ortho: dvz.DvzOrtho = None
    c_panel: dvz.DvzPanel = None

    def __init__(self, c_ortho: dvz.DvzOrtho, c_panel: dvz.DvzPanel = None) -> None:
        """
        Initialize an Ortho instance.

        Parameters
        ----------
        c_ortho : dvz.DvzOrtho
            The underlying C ortho object.
        c_panel : dvz.DvzPanel, optional
            The panel to which the ortho interactivity is attached, by default None.
        """
        assert c_ortho
        self.c_ortho = c_ortho
        self.c_panel = c_panel


class Arcball:
    """
    Represents arcball interactivity for a panel.

    Attributes
    ----------
    c_arcball : dvz.DvzArcball
        The underlying C arcball object.
    c_panel : dvz.DvzPanel, optional
        The panel to which the arcball interactivity is attached.
    """

    c_arcball: dvz.DvzArcball = None
    c_panel: dvz.DvzPanel = None

    def __init__(self, c_arcball: dvz.DvzArcball, c_panel: dvz.DvzPanel = None) -> None:
        """
        Initialize an Arcball instance.

        Parameters
        ----------
        c_arcball : dvz.DvzArcball
            The underlying C arcball object.
        c_panel : dvz.DvzPanel, optional
            The panel to which the arcball interactivity is attached, by default None.
        """
        assert c_arcball
        self.c_arcball = c_arcball
        self.c_panel = c_panel

    def reset(self) -> None:
        """
        Reset the arcball to its initial state.
        """
        dvz.arcball_reset(self.c_arcball)

    def set(self, angles: Tuple[float, float, float]) -> None:
        """
        Set the angles of the arcball.

        Parameters
        ----------
        angles : tuple of float
            The (x, y, z) angles to set.
        """
        dvz.arcball_set(self.c_arcball, dvz.vec3(*angles))
        dvz.panel_update(self.c_panel)

    def get(self) -> Tuple[float, float, float]:
        """
        Get the current angles of the arcball.

        Returns
        -------
        tuple of float
            The (x, y, z) angles of the arcball.
        """
        out_angles = dvz.vec3(0)
        dvz.arcball_angles(self.c_arcball, out_angles)
        return (out_angles[0], out_angles[1], out_angles[2])


class Camera:
    """
    Represents camera interactivity for a panel.

    Attributes
    ----------
    c_camera : dvz.DvzCamera
        The underlying C camera object.
    c_panel : dvz.DvzPanel, optional
        The panel to which the camera interactivity is attached.
    """

    c_camera: dvz.DvzCamera = None
    c_panel: dvz.DvzPanel = None

    def __init__(self, c_camera: dvz.DvzCamera, c_panel: dvz.DvzPanel = None) -> None:
        """
        Initialize a Camera instance.

        Parameters
        ----------
        c_camera : dvz.DvzCamera
            The underlying C camera object.
        c_panel : dvz.DvzPanel, optional
            The panel to which the camera interactivity is attached, by default None.
        """
        assert c_camera
        self.c_camera = c_camera
        self.c_panel = c_panel

    def position(self) -> Tuple[float, float, float]:
        """
        Get the camera position.

        Returns
        -------
        tuple
            The (x, y, z) position of the camera.
        """
        position = dvz.vec3(0)
        dvz.camera_get_position(self.c_camera, position)
        return tuple(position)

    def set(
        self,
        position: Tuple[float, float, float] = None,
        lookat: Tuple[float, float, float] = None,
        up: Tuple[float, float, float] = None,
    ) -> None:
        """
        Set the camera parameters.

        Parameters
        ----------
        position : tuple of float, optional
            The (x, y, z) position of the camera, by default None.
        lookat : tuple of float, optional
            The (x, y, z) position the camera is looking at, by default None.
        up : tuple of float, optional
            The (x, y, z) up vector of the camera, by default None.
        """
        if position is not None:
            position = dvz.vec3(*position)
            dvz.camera_position(self.c_camera, position)

        if lookat is not None:
            lookat = dvz.vec3(*lookat)
            dvz.camera_lookat(self.c_camera, lookat)

        if up is not None:
            up = dvz.vec3(*up)
            dvz.camera_up(self.c_camera, up)

        dvz.panel_update(self.c_panel)


class Fly:
    """
    Fly camera controller.

    Controls:

        - Left mouse drag: Look around (yaw/pitch)
        - Right mouse drag: Orbit around a dynamic center (in front of the camera)
        - Middle mouse drag: Move the camera left/right and up/down
        - Arrow keys: Move in view direction (up/down) or strafe (left/right)

    """

    c_fly: dvz.DvzFly = None
    c_panel: dvz.DvzPanel = None

    def __init__(self, c_fly, c_panel=None):
        """Initialize a fly camera controller."""
        assert c_fly
        self.c_fly = c_fly
        self.c_panel = c_panel

    def reset(self):
        """Reset the fly camera to its initial position and orientation."""
        dvz.fly_reset(self.c_fly)
        dvz.panel_update(self.c_panel)

    def position(self):
        """Get the current camera position."""
        pos = dvz.vec3(0)
        dvz.fly_get_position(self.c_fly, pos)
        return tuple(pos)

    def lookat(self):
        """Get the current camera lookat point."""
        look = dvz.vec3(0)
        dvz.fly_get_lookat(self.c_fly, look)
        return tuple(look)

    def up(self):
        """Get the current camera up vector."""
        up = dvz.vec3(0)
        dvz.fly_get_up(self.c_fly, up)
        return tuple(up)
