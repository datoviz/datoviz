"""
Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
Licensed under the MIT license. See LICENSE file in the project root for details.
SPDX-License-Identifier: MIT
"""

# Properties

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import numpy as np

from ._ctypes import cvec4, ivec2, ivec3, vec2, vec3, vec4

# -------------------------------------------------------------------------------------------------
# Visual properties
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
        'edgecolor': {'type': cvec4},
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
        'cap': {'type': 'enum', 'enum': 'DVZ_CAP'},
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
        'bgcolor': {'type': cvec4},
        'texture': {'type': 'texture'},
    },
    'image': {
        'position': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'size': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 2)},
        'anchor': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 2)},
        'texcoords': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 4)},
        'facecolor': {'type': np.ndarray, 'dtype': np.uint8, 'shape': (-1, 4)},
        'edgecolor': {'type': cvec4},
        'permutation': {'type': ivec2},
        'linewidth': {'type': float},
        'radius': {'type': float},
        'colormap': {'type': 'enum', 'enum': 'DVZ_CMAP'},
        'texture': {'type': 'texture'},
    },
    'wiggle': {
        'edgecolor': {'type': cvec4},
        'xrange': {'type': vec2},
        'scale': {'type': float},
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
        'light_pos': {'type': vec4},
        'light_color': {'type': cvec4},
        'ambient_params': {'type': vec3},
        'diffuse_params': {'type': vec3},
        'specular_params': {'type': vec3},
        'emission_params': {'type': vec3},
        'shine': {'type': float},
        'emit': {'type': float},
        'edgecolor': {'type': cvec4},
        'linewidth': {'type': float},
        'density': {'type': int},
        'texture': {'type': 'texture'},
    },
    'sphere': {
        'position': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'color': {'type': np.ndarray, 'dtype': np.uint8, 'shape': (-1, 4)},
        'size': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1,)},
        'light_pos': {'type': vec4},
        'light_color': {'type': cvec4},
        'ambient_params': {'type': vec3},
        'diffuse_params': {'type': vec3},
        'specular_params': {'type': vec3},
        'emission_params': {'type': vec3},
        'shine': {'type': float},
        'emit': {'type': float},
        'texture': {'type': 'texture'},
    },
    'volume': {
        'permutation': {'type': ivec3},
        'slice': {'type': int},
        'transfer': {'type': vec4},
        'texture': {'type': 'texture'},
    },
    'slice': {
        'position': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'texcoords': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 4)},
        'alpha': {'type': float},
        'texture': {'type': 'texture'},
    },
    'demo_2D': {},
    'demo_3D': {},
}
