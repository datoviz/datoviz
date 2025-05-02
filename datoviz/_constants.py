"""
Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
Licensed under the MIT license. See LICENSE file in the project root for details.
SPDX-License-Identifier: MIT
"""

# Constants

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import typing as tp
import numpy as np
from . import _ctypes as dvz


# -------------------------------------------------------------------------------------------------
# Globals
# -------------------------------------------------------------------------------------------------

DEFAULT_WIDTH = 800
DEFAULT_HEIGHT = 600
DEFAULT_FONT_SIZE = 30
DEFAULT_GLYPH_COLOR = (0, 0, 0, 255)
DEFAULT_INTERPOLATION = 'linear'
DEFAULT_ADDRESS_MODE = 'clamp_to_border'

DEFAULT_CAMERA_POS = (0, 0, 4)
DEFAULT_CAMERA_LOOKAT = (0, 0, 0)
DEFAULT_CAMERA_UP = (0, 1, 0)

DEFAULT_LIGHT_DIR = (0.25, -0.25, -1)
DEFAULT_LIGHT_COLOR = (255, 255, 255, 255)
DEFAULT_LIGHT_PARAMS = (.25, .75, .75, 16)


# -------------------------------------------------------------------------------------------------
# Props
# -------------------------------------------------------------------------------------------------

PROPS = {
    'basic': {
        'position': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'color': {'type': np.ndarray, 'dtype': np.uint8, 'shape': (-1, 4)},
        'group': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1,)},
        'size': {'type': float},
    },

    'pixel': {
        'position': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'color': {'type': np.ndarray, 'dtype': np.uint8, 'shape': (-1, 4)},
        'size': {'type': float},
    },

    'point': {
        'position': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'color': {'type': np.ndarray, 'dtype': np.uint8, 'shape': (-1, 4)},
        'size': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1,)},
    },

    'marker': {
        'position': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'size': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1,)},
        'angle': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1,)},
        'color': {'type': np.ndarray, 'dtype': np.uint8, 'shape': (-1, 4)},

        'edgecolor': {'type': dvz.cvec4},
        'linewidth': {'type': float},
        'tex_scale': {'type': float},

        'mode': {'type': 'enum', 'enum': 'DVZ_MARKER_MODE'},
        'aspect': {'type': 'enum', 'enum': 'DVZ_MARKER_ASPECT'},
        'shape': {'type': 'enum', 'enum': 'DVZ_MARKER_SHAPE'},

        'texture': {'type': 'texture'},
    },

    'segment': {
        'position': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'shift': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 4)},
        'color': {'type': np.ndarray, 'dtype': np.uint8, 'shape': (-1, 4)},
        'linewidth': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1,)},
        'cap': {'type': np.ndarray, 'dtype': np.int32, 'shape': (-1,)},
    },

    'path': {
        'position': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'color': {'type': np.ndarray, 'dtype': np.uint8, 'shape': (-1, 4)},
        'linewidth': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1,)},

        'cap': {'type': 'enum', 'enum': 'DVZ_CAP'},
        'join': {'type': 'enum', 'enum': 'DVZ_JOIN'},
    },

    'glyph': {
        'position': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'axis': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'size': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 2)},
        'anchor': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 2)},
        'shift': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 2)},
        'texcoords': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 4)},
        'group_size': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 2)},
        'scale': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1,)},
        'angle': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1,)},
        'color': {'type': np.ndarray, 'dtype': np.uint8, 'shape': (-1, 4)},

        'bgcolor': {'type': dvz.vec4},
        'texture': {'type': 'texture'},
    },

    'image': {
        'position': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'size': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 2)},
        'anchor': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 2)},
        'texcoords': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 4)},
        'facecolor': {'type': np.ndarray, 'dtype': np.uint8, 'shape': (-1, 4)},

        'edgecolor': {'type': dvz.vec4},
        'permutation': {'type': dvz.ivec2},
        'linewidth': {'type': float},
        'radius': {'type': float},
        'colormap': {'type': 'enum', 'enum': 'DVZ_CMAP'},

        'texture': {'type': 'texture'},
    },

    'mesh': {
        'position': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'color': {'type': np.ndarray, 'dtype': np.uint8, 'shape': (-1, 4)},
        'texcoords': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 4)},
        'normal': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'isoline': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1,)},
        'left': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'right': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'contour': {'type': np.ndarray, 'dtype': np.uint8, 'shape': (-1, 4)},
        'index': {'type': np.ndarray, 'dtype': np.uint32, 'shape': (-1,)},

        'light_dir': {'type': dvz.vec3},
        'light_color': {'type': dvz.cvec4},
        'light_params': {'type': dvz.vec4},
        'edgecolor': {'type': dvz.cvec4},
        'linewidth': {'type': float},
        'density': {'type': int},

        'texture': {'type': 'texture'},
    },

    'sphere': {
        'position': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'color': {'type': np.ndarray, 'dtype': np.uint8, 'shape': (-1, 4)},
        'size': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1,)},

        'light_pos': {'type': dvz.vec3},
        'light_params': {'type': dvz.vec4},
    },

    'volume': {
        'permutation': {'type': dvz.ivec3},
        'slice': {'type': int},
        'transfer': {'type': dvz.vec4},
        'texture': {'type': 'texture'},
    },

    'slice': {
        'position': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'texcoords': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 4)},
        'alpha': {'type': float},
        'texture': {'type': 'texture'},
    },

}


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

VEC_TYPES = (dvz.vec3, dvz.vec4, dvz.cvec4)  # TODO: others

Vec3 = tp.Tuple[float, float, float]
