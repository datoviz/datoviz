"""
Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
Licensed under the MIT license. See LICENSE file in the project root for details.
SPDX-License-Identifier: MIT
"""

# Texture

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

from typing import Tuple

import numpy as np

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
    c_batch : dvz.DvzBatch
        The underlying C app object.
    """

    c_texture: dvz.DvzTexture = None
    c_batch: dvz.DvzApp = None
    ndim: int = None

    def __init__(
        self, c_texture: dvz.DvzTexture, c_batch: dvz.DvzApp = None, ndim: int = None
    ) -> None:
        """
        Initialize a Texture instance.

        Parameters
        ----------
        c_texture : dvz.DvzTexture
            The underlying C texture object.
        c_batch : dvz.DvzBatch
            The underlying C app object.
        ndim : int
            The number of dimensions in the texture.
        """
        assert c_texture is not None
        self.c_texture = c_texture
        self.c_batch = c_batch
        self.ndim = ndim

    def data(self, image: np.ndarray, offset: Tuple[int, int, int] = None):
        """
        Upload an image to the texture with an optional offset.

        Parameters
        ----------
        image : np.ndarray
            The image data to upload.
        offset : Tuple[int, int, int], optional
            The offset at which to upload the image in the texture. Defaults to (0, 0, 0).

        Notes
        -----
        The function determines the width, height, and depth of the image based on its shape.
        It then uploads the image data to the texture using the `dvz.upload_tex` function.
        """
        offset = offset if offset is not None else (0, 0, 0)
        x, y, z = offset

        if self.ndim == 1:
            w, h, d = image.shape[0], 1, 1
        elif self.ndim == 2:
            # WARNING: height and width are reversed to match natural ndarray dimensions
            h, w, d = image.shape[0], image.shape[1], 1
        elif self.ndim == 3:
            h, w, d = image.shape[:3]

        dvz.texture_data(self.c_texture, x, y, z, w, h, d, image.nbytes, image)
