"""
Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
Licensed under the MIT license. See LICENSE file in the project root for details.
SPDX-License-Identifier: MIT
"""

# Interactivity

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import typing as tp

import numpy as np

from . import _ctypes as dvz


# -------------------------------------------------------------------------------------------------
# Interact
# -------------------------------------------------------------------------------------------------

class Panzoom:
    c_panzoom: dvz.DvzPanzoom = None
    c_panel: dvz.DvzPanel = None

    def __init__(self, c_panzoom: dvz.DvzPanzoom, c_panel: dvz.DvzPanel = None):
        assert c_panzoom
        self.c_panzoom = c_panzoom
        self.c_panel = c_panel


class Ortho:
    c_ortho: dvz.DvzOrtho = None
    c_panel: dvz.DvzPanel = None

    def __init__(self, c_ortho: dvz.DvzOrtho, c_panel: dvz.DvzPanel = None):
        assert c_ortho
        self.c_ortho = c_ortho
        self.c_panel = c_panel


class Arcball:
    c_arcball: dvz.DvzArcball = None
    c_panel: dvz.DvzPanel = None

    def __init__(self, c_arcball: dvz.DvzArcball, c_panel: dvz.DvzPanel = None):
        assert c_arcball
        self.c_arcball = c_arcball
        self.c_panel = c_panel

    def reset(self):
        dvz.arcball_reset(self.c_arcball)

    def set(self, angles: tuple):
        dvz.arcball_set(self.c_arcball, dvz.vec3(*angles))
        dvz.panel_update(self.c_panel)

    def get(self):
        out_angles = dvz.vec3(0)
        dvz.arcball_angles(self.c_arcball, out_angles)
        return (out_angles[0], out_angles[1], out_angles[2])


class Camera:
    c_camera: dvz.DvzCamera = None
    c_panel: dvz.DvzPanel = None

    def __init__(self, c_camera: dvz.DvzCamera, c_panel: dvz.DvzPanel = None):
        assert c_camera
        self.c_camera = c_camera
        self.c_panel = c_panel

    def set(self, eye: tuple = None, lookat: tuple = None, up: tuple = None):
        if eye is not None:
            eye = dvz.vec3(*eye)
            dvz.camera_position(self.c_camera, eye)

        if lookat is not None:
            lookat = dvz.vec3(*lookat)
            dvz.camera_lookat(self.c_camera, lookat)

        if up is not None:
            up = dvz.vec3(*up)
            dvz.camera_up(self.c_camera, up)

        dvz.panel_update(self.c_panel)
