"""
Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
Licensed under the MIT license. See LICENSE file in the project root for details.
SPDX-License-Identifier: MIT
"""

# NOTE: this file is NOT automatically generated, only clib.py is.

import ctypes
from ctypes import c_char_p, byref
import numpy as np
from numpy.ctypeslib import as_ctypes_type as _ctype

from .clib import *
from .wrapper import App


# ===============================================================================
# Array wrappers
# ===============================================================================

def array_pointer(x, dtype=None):
    if not isinstance(x, np.ndarray):
        return x
    dtype = dtype or x.dtype
    if not x.flags.c_contiguous:
        print(
            f"Warning: array is not C contiguous, ensure your array is in row-major (C) order "
            "to avoid potential issues"
        )
    if dtype and x.dtype != dtype:
        x = x.astype(dtype)
    return x.ctypes.data_as(P_(_ctype(dtype)))


def pointer_array(pointer, length, n_components, dtype=np.float32):
    np_array = np.ctypeslib.as_array(pointer, shape=(length,))
    np_array = np_array.view(dtype=dtype).reshape(length, n_components)
    return np_array


def char_pointer(s):
    if isinstance(s, list):
        return (c_char_p * len(s))(*[c_char_p(str(_).encode("utf-8")) for _ in s])
    return str(s).encode("utf-8")


def pointer_image(rgb, width, height, n_channels=3):
    """
    Return a NumPy array of uint8 with shape (height, width, n_channels=3) from an ndpointer
    referring to a C pointer to a buffer of RGB uint8 values.
    """
    c_ptr = ctypes.cast(rgb.value, ctypes.POINTER(ctypes.c_ubyte))
    arr = np.ctypeslib.as_array(c_ptr, shape=(height, width, n_channels))
    return arr


from_array = array_pointer
from_pointer = pointer_array


# ===============================================================================
# Helpers
# ===============================================================================

def get_version():
    return {
        'ctypes_wrapper': __version__,
        'libdatoviz': version().decode('utf-8'),
    }


def from_enum(enum_cls, value):
    for name, val in enum_cls.__dict__.items():
        if not name.startswith("_") and isinstance(val, int) and val == value:
            return name
    return None


def to_enum(enumstr):
    return globals().get(enumstr.upper())


def key_name(key_code):
    name = from_enum(KeyCode, key_code)
    name = name.replace("DVZ_KEY_", "")
    return name


def button_name(button):
    name = from_enum(MouseButton, button)
    name = name.replace("DVZ_MOUSE_BUTTON_", "")
    return name


def cmap(cm, values, vmin=0.0, vmax=10.):
    values = np.asanyarray(values, dtype=np.float32)
    n = values.size
    colors = np.full((n, 4), 255, dtype=np.uint8)
    colormap_array(cm, n, values.ravel(), vmin, vmax, colors)
    return colors


def merge_shapes(shapes):
    merged = shape()
    n = len(shapes)
    array_type = ctypes.POINTER(Shape) * n
    shapes_array = array_type(*(s for s in shapes))
    shape_merge(merged, len(shapes), shapes_array)
    return merged


def to_byte(arr, vmin=None, vmax=None):
    vmin = vmin if vmin is not None else arr.min()
    vmax = vmax if vmax is not None else arr.max()
    assert vmin < vmax

    # # Datoviz version
    # vmin_vmax = vec2(vmin, vmax)
    # arr = np.ascontiguousarray(arr, dtype=np.float32)
    # count = arr.shape[0]
    # normalized = np.empty_like(arr, dtype=np.uint8)
    # normalize_bytes(vmin_vmax, count, arr, normalized)
    # return normalized

    # NumPy version
    normalized = (arr - vmin) * 1. / (vmax - vmin)
    normalized = np.clip(normalized, 0, 1)
    normalized *= 255
    return normalized.astype(np.uint8)
