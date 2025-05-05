"""
Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
Licensed under the MIT license. See LICENSE file in the project root for details.
SPDX-License-Identifier: MIT
"""

# Texture

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

from . import _ctypes as dvz

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
