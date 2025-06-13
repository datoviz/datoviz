"""
Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
Licensed under the MIT license. See LICENSE file in the project root for details.
SPDX-License-Identifier: MIT
"""

# Constants

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

from typing import Tuple

from . import _ctypes as dvz

# -------------------------------------------------------------------------------------------------
# Globals
# -------------------------------------------------------------------------------------------------

DEFAULT_WIDTH = 800
DEFAULT_HEIGHT = 600
DEFAULT_FONT_SIZE = 30
DEFAULT_GLYPH_COLOR = (0, 0, 0, 255)
DEFAULT_INTERPOLATION = 'nearest'
DEFAULT_ADDRESS_MODE = 'clamp_to_border'

DEFAULT_CAMERA_POS = (0, 0, 4)
DEFAULT_CAMERA_LOOKAT = (0, 0, 0)
DEFAULT_CAMERA_UP = (0, 1, 0)

DEFAULT_BACKGROUND = (
    (10, 30, 70, 255),
    (10, 10, 30, 255),
    (10, 10, 20, 255),
    (10, 10, 20, 255),
)

DEFAULT_GRID_COLOR = (230, 230, 230, 230)
DEFAULT_GRID_SCALE = 32
DEFAULT_GRID_SIZE = 64
DEFAULT_GRID_OFFSET = (0, 0, 0)
DEFAULT_GRID_TRANSFORM = (
    (1, 0, 0, 0),
    (0, 0, -1, 0),
    (0, 1, 0, 0),
    (0, 0, 0, 1),
)

# TODO: use the C constants instead of redefining Python constants here
DEFAULT_LIGHTING = False
DEFAULT_LIGHT_DIR = (1.0, -1.0, -1.0, 0.0)
DEFAULT_LIGHT_POS = (-1.0, +1.0, +10.0, 1.0)  # Behind camera.
DEFAULT_LIGHT_COLOR = (255, 255, 255, 255)

DEFAULT_AMBIENT_PARAMS = (0.25, 0.25, 0.25)  # r,g,b values.
DEFAULT_DIFFUSE_PARAMS = (0.75, 0.75, 0.75)  # r,g,b values.
DEFAULT_SPECULAR_PARAMS = (0.5, 0.5, 0.5)  # r,g,b values.
DEFAULT_EMISSION_PARAMS = (0.5, 0.5, 0.5)  # r,g,b values.

DEFAULT_EQUAL_RECTANGULAR = False

DEFAULT_INDEXING = 'earcut'
DEFAULT_CONTOUR = 'joints'
DEFAULT_MESH_INDEXED = True

DEFAULT_IMAGE_UNIT = 'pixels'
DEFAULT_IMAGE_MODE = 'rgba'
DEFAULT_IMAGE_RESCALE = None
DEFAULT_IMAGE_BORDER = False

VOLUME_MODES = ('colormap', 'rgba')
TOPOLOGY_OPTIONS = ('point_list', 'line_list', 'line_strip', 'triangle_list', 'triangle_strip')

NDC = (-1.0, +1.0)


# -------------------------------------------------------------------------------------------------
# Types
# -------------------------------------------------------------------------------------------------

DTYPE_FORMATS = {
    ('uint8', 1): dvz.FORMAT_R8_UNORM,
    ('uint8', 2): dvz.FORMAT_R8G8_UNORM,
    ('uint8', 4): dvz.FORMAT_R8G8B8A8_UNORM,
    ('uint16', 1): dvz.FORMAT_R16_UNORM,
    ('int16', 1): dvz.FORMAT_R16_SNORM,
    ('float32', 1): dvz.FORMAT_R32_SFLOAT,
    ('float32', 2): dvz.FORMAT_R32G32_SFLOAT,
    ('float32', 4): dvz.FORMAT_R32G32B32A32_SFLOAT,
}

Vec3 = Tuple[float, float, float]
