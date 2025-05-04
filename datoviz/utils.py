"""
Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
Licensed under the MIT license. See LICENSE file in the project root for details.
SPDX-License-Identifier: MIT
"""

# Utils

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import ctypes
from ctypes import c_char_p

import numpy as np
from numpy.ctypeslib import as_ctypes_type as _ctype

from . import _constants as cst
from . import _ctypes as dvz
from ._ctypes import P_, __version__, version

# -------------------------------------------------------------------------------------------------
# Array wrappers
# -------------------------------------------------------------------------------------------------


def array_pointer(x, dtype=None):
    if not isinstance(x, np.ndarray):
        return x
    dtype = dtype or x.dtype
    if not x.flags.c_contiguous:
        print(
            'Warning: array is not C contiguous, ensure your array is in row-major (C) order '
            'to avoid potential issues'
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
        return (c_char_p * len(s))(*[c_char_p(str(_).encode('utf-8')) for _ in s])
    return str(s).encode('utf-8')


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


# -------------------------------------------------------------------------------------------------
# Helpers
# -------------------------------------------------------------------------------------------------


def get_version():
    return {
        'ctypes_wrapper': __version__,
        'libdatoviz': version().decode('utf-8'),
    }


def from_enum(enum_cls, value, prettify=True):
    # Find the common prefix for prettify
    if prettify:
        strs = [_ for _ in enum_cls.__dict__.keys() if _.startswith('DVZ_')]
        prefix = ''.join(c[0] for c in zip(*strs) if len(set(c)) == 1)
    for name, val in enum_cls.__dict__.items():
        if not name.startswith('_') and isinstance(val, int) and val == value:
            if prettify:
                name = name.replace(prefix, '').lower()
            return name
    return None


def to_enum(enumstr):
    return getattr(dvz, enumstr.upper())


def key_name(key_code):
    return from_enum(dvz.KeyCode, key_code)


def button_name(button):
    name = from_enum(dvz.MouseButton, button)
    name = name.replace('DVZ_MOUSE_BUTTON_', '')
    return name


def cmap(cm, values, vmin=0.0, vmax=1.0):
    values = np.asanyarray(values, dtype=np.float32)
    n = values.size
    colors = np.full((n, 4), 255, dtype=np.uint8)
    dvz.colormap_array(cm, n, values.ravel(), vmin, vmax, colors)
    return colors


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
    normalized = (arr - vmin) * 1.0 / (vmax - vmin)
    normalized = np.clip(normalized, 0, 1)
    normalized *= 255
    return normalized.astype(np.uint8)


# -------------------------------------------------------------------------------------------------
# Utils
# -------------------------------------------------------------------------------------------------


def get_size(idx, value, total_size=0):
    if isinstance(value, np.ndarray):
        return value.shape[0]
    elif isinstance(idx, slice):
        offset = idx.start or 0
        step = idx.step or 1
        return (idx.stop or total_size - offset) // step
    else:
        return total_size


def dtype_to_format(dtype, n_channels):
    return cst.DTYPE_FORMATS[dtype, n_channels]


def prepare_data_array(name, dtype, shape, value):
    ndim = len(shape)
    pvalue = np.asanyarray(value, dtype=dtype)
    if pvalue.ndim < ndim:
        if pvalue.ndim == 2:
            pvalue = np.atleast_2d(pvalue)
        elif pvalue.ndim == 3:
            pvalue = np.atleast_3d(pvalue)
    elif pvalue.ndim > ndim:
        raise ValueError(
            f'Visual property {name} should have shape {shape} instead of {pvalue.shape}'
        )
    assert ndim == pvalue.ndim
    for dim in range(ndim):
        if shape[dim] > 0 and pvalue.shape[dim] != shape[dim]:
            raise ValueError(f'Incorrect shape {pvalue.shape[dim]} != {shape[dim]}')
    return pvalue


def prepare_data_scalar(name, dtype, size, value):
    if size == 0:
        raise ValueError(f'Property {name} needs to be set after the position')
    pvalue = np.full(size, value, dtype=dtype)
    return pvalue


def mesh_flags(indexed: bool = None, lighting: bool = None, contour: bool = False):
    c_flags = 0
    lighting = lighting if lighting is not None else cst.DEFAULT_LIGHTING
    indexed = indexed if indexed is not None else cst.DEFAULT_MESH_INDEXED
    if indexed:
        c_flags |= dvz.VISUAL_FLAGS_INDEXED
    if lighting:
        c_flags |= dvz.MESH_FLAGS_LIGHTING
    if contour:
        c_flags |= dvz.MESH_FLAGS_CONTOUR
    return c_flags


def get_fixed_params(fixed):
    if fixed is True:
        return (True, True, True)
    elif isinstance(fixed, str):
        return ('x' in fixed, 'y' in fixed, 'z' in fixed)
    return (False, False, False)
