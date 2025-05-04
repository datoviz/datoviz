"""
Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
Licensed under the MIT license. See LICENSE file in the project root for details.
SPDX-License-Identifier: MIT
"""

# Base classes

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

from . import _ctypes as dvz

# -------------------------------------------------------------------------------------------------
# Texture
# -------------------------------------------------------------------------------------------------


class Texture:
    c_texture: dvz.DvzTexture = None

    def __init__(self, c_texture: dvz.DvzTexture):
        assert c_texture is not None
        self.c_texture = c_texture
