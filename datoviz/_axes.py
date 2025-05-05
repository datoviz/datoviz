"""
Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
Licensed under the MIT license. See LICENSE file in the project root for details.
SPDX-License-Identifier: MIT
"""

# Axes

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import numpy as np

from . import _ctypes as dvz

# -------------------------------------------------------------------------------------------------
# Axes
# -------------------------------------------------------------------------------------------------


class Axes:
    """
    Represents 2D axes.

    Attributes
    ----------
    c_axes : dvz.DvzAxes
        The underlying C axes object.
    c_ref : dvz.DvzRef
        The underlying C ref object.
    c_panzoom : dvz.DvzPanzoom
        The underlying C panzoom object.
    c_panel : dvz.DvzPanel
        The underlying C panel object.
    """

    c_axes: dvz.DvzAxes = None
    c_ref: dvz.DvzRef = None
    c_panzoom: dvz.DvzPanzoom = None
    c_panel: dvz.DvzPanel = None

    def __init__(
        self,
        c_axes: dvz.DvzAxes,
        c_ref: dvz.DvzRef,
        c_panzoom: dvz.DvzPanzoom,
        c_panel: dvz.DvzPanel,
    ) -> None:
        """
        Initialize an Axes instance.

        Parameters
        ----------
        c_axes : dvz.DvzAxes
            The underlying C axes object.
        c_ref : dvz.DvzRef
            The underlying C ref object.
        c_panzoom : dvz.DvzPanzoom
            The underlying C panzoom object.
        c_panel : dvz.DvzPanel
            The underlying C panel object.
        """
        assert c_axes is not None
        assert c_ref is not None
        assert c_panzoom is not None
        assert c_panel is not None

        self.c_axes = c_axes
        self.c_ref = c_ref
        self.c_panzoom = c_panzoom
        self.c_panel = c_panel

    def xlim(self, xmin: float, xmax: float):
        """
        Set the xlim of the axes.

        Parameters
        ----------
        xmin : float
            The minimal x in data coordinates.
        xmax : float
            The maximal x in data coordinates.
        """
        dvz.ref_set(self.c_ref, dvz.DIM_X, xmin, xmax)

    def ylim(self, ymin: float, ymax: float):
        """
        Set the ylim of the axes.

        Parameters
        ----------
        ymin : float
            The minimal y in data coordinates.
        ymax : float
            The maximal y in data coordinates.
        """
        dvz.ref_set(self.c_ref, dvz.DIM_Y, ymin, ymax)

    def bounds(self):
        """
        Return the bounds of the panzoom area.

        This method computes the minimum and maximum bounds for both the x-axis
        and y-axis of the panzoom area.

        Returns
        -------
        tuple of xlim (xmin, xmax) and ylim (ymin, ymax)
        """
        xmin = dvz.Out(0.0, 'double')
        xmax = dvz.Out(0.0, 'double')
        ymin = dvz.Out(0.0, 'double')
        ymax = dvz.Out(0.0, 'double')

        dvz.panzoom_bounds(self.c_panzoom, self.c_ref, xmin, xmax, ymin, ymax)
        xmin, xmax, ymin, ymax = xmin.value, xmax.value, ymin.value, ymax.value
        return (xmin, xmax), (ymin, ymax)

    def normalize(self, x: np.ndarray, y: np.ndarray = None, z: np.ndarray = None):
        if y is None and z is None:
            pos = x
        else:
            x = x.ravel()
            if y is not None:
                y = y.ravel()

            if z is None:
                pos = np.c_[x, y]
            else:
                z = z.ravel()
                pos = np.c_[x, y, z]

        if pos.ndim == 1:
            pos = pos[:, np.newaxis]
        assert pos.ndim == 2
        n, d = pos.shape

        pos_tr = np.zeros((n, 3), dtype=np.float32)

        if d == 1:
            dvz.ref_normalize_1D(self.c_ref, n, pos, pos_tr)
        elif d == 2:
            dvz.ref_normalize_2D(self.c_ref, n, pos, pos_tr)
        elif d == 3:
            dvz.ref_normalize_3D(self.c_ref, n, pos, pos_tr)
        else:
            raise NotImplementedError()

        return pos_tr
