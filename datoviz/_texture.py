"""
Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
Licensed under the MIT license. See LICENSE file in the project root for details.
SPDX-License-Identifier: MIT
"""

# Base classes

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import typing as tp

import numpy as np

from . import _ctypes as dvz
from . import _constants as cst
from . import visuals as vs
from ._constants import Vec3, PROPS, VEC_TYPES
from .utils import (
    mesh_flags, from_enum, to_enum, key_name, button_name,
    get_size, prepare_data_scalar, prepare_data_array, dtype_to_format)
from .shape_collection import ShapeCollection


# -------------------------------------------------------------------------------------------------
# Texture
# -------------------------------------------------------------------------------------------------

class Texture:
    c_texture: dvz.DvzTexture = None

    def __init__(self, c_texture: dvz.DvzTexture):
        assert c_texture is not None
        self.c_texture = c_texture
