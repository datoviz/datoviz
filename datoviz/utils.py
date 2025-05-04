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


def array_pointer(x: np.ndarray, dtype: np.dtype = None) -> ctypes.POINTER:
    """
    Convert a NumPy array to a C pointer.

    Parameters
    ----------
    x : np.ndarray
        The NumPy array to convert.
    dtype : np.dtype, optional
        The desired data type, by default None.

    Returns
    -------
    ctypes.POINTER
        A C pointer to the array data.
    """
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


def pointer_array(
    pointer: ctypes.POINTER, length: int, n_components: int, dtype: np.dtype = np.float32
) -> np.ndarray:
    """
    Convert a C pointer to a NumPy array.

    Parameters
    ----------
    pointer : ctypes.POINTER
        The C pointer to convert.
    length : int
        The length of the array.
    n_components : int
        The number of components per element.
    dtype : np.dtype, optional
        The desired data type, by default np.float32.

    Returns
    -------
    np.ndarray
        The resulting NumPy array.
    """
    np_array = np.ctypeslib.as_array(pointer, shape=(length,))
    np_array = np_array.view(dtype=dtype).reshape(length, n_components)
    return np_array


def char_pointer(s: str | list[str]) -> ctypes.POINTER:
    """
    Convert a string or list of strings to a C char pointer.

    Parameters
    ----------
    s : str or list of str
        The string or list of strings to convert.

    Returns
    -------
    ctypes.POINTER
        A C char pointer.
    """
    if isinstance(s, list):
        return (c_char_p * len(s))(*[c_char_p(str(_).encode('utf-8')) for _ in s])
    return str(s).encode('utf-8')


def pointer_image(rgb: ctypes.POINTER, width: int, height: int, n_channels: int = 3) -> np.ndarray:
    """
    Convert a C pointer to an image buffer into a NumPy array.

    Parameters
    ----------
    rgb : ctypes.POINTER
        The C pointer to the image buffer.
    width : int
        The width of the image.
    height : int
        The height of the image.
    n_channels : int, optional
        The number of channels in the image, by default 3.

    Returns
    -------
    np.ndarray
        A NumPy array representing the image.
    """
    c_ptr = ctypes.cast(rgb.value, ctypes.POINTER(ctypes.c_ubyte))
    arr = np.ctypeslib.as_array(c_ptr, shape=(height, width, n_channels))
    return arr


from_array = array_pointer
from_pointer = pointer_array


# -------------------------------------------------------------------------------------------------
# Helpers
# -------------------------------------------------------------------------------------------------


def get_version() -> dict[str, str]:
    """
    Get the version information for the library.

    Returns
    -------
    dict of str
        A dictionary containing the version of the ctypes wrapper and the Datoviz library.
    """
    return {
        'ctypes_wrapper': __version__,
        'libdatoviz': version().decode('utf-8'),
    }


def from_enum(enum_cls: type, value: int, prettify: bool = True) -> str | None:
    """
    Convert an enum value to its string representation.

    Parameters
    ----------
    enum_cls : type
        The enum class.
    value : int
        The enum value.
    prettify : bool, optional
        Whether to prettify the string representation, by default True.

    Returns
    -------
    str or None
        The string representation of the enum value, or None if not found.
    """
    if prettify:
        strs = [_ for _ in enum_cls.__dict__.keys() if _.startswith('DVZ_')]
        prefix = ''.join(c[0] for c in zip(*strs) if len(set(c)) == 1)
    for name, val in enum_cls.__dict__.items():
        if not name.startswith('_') and isinstance(val, int) and val == value:
            if prettify:
                name = name.replace(prefix, '').lower()
            return name
    return None


def to_enum(enumstr: str) -> int:
    """
    Convert an enum string to its corresponding value.

    Parameters
    ----------
    enumstr : str
        The enum string.

    Returns
    -------
    int
        The enum value.
    """
    return getattr(dvz, enumstr.upper())


def key_name(key_code: int) -> str:
    """
    Get the name of a key from its key code.

    Parameters
    ----------
    key_code : int
        The key code.

    Returns
    -------
    str
        The name of the key.
    """
    return from_enum(dvz.KeyCode, key_code)


def button_name(button: int) -> str:
    """
    Get the name of a mouse button from its button code.

    Parameters
    ----------
    button : int
        The button code.

    Returns
    -------
    str
        The name of the mouse button.
    """
    name = from_enum(dvz.MouseButton, button)
    name = name.replace('DVZ_MOUSE_BUTTON_', '')
    return name


def cmap(cm: int, values: np.ndarray, vmin: float = 0.0, vmax: float = 1.0) -> np.ndarray:
    """
    Apply a colormap to an array of values.

    Parameters
    ----------
    cm : int
        The colormap identifier.
    values : np.ndarray
        The array of values to map.
    vmin : float, optional
        The minimum value for normalization, by default 0.0.
    vmax : float, optional
        The maximum value for normalization, by default 1.0.

    Returns
    -------
    np.ndarray
        An array of RGBA colors.
    """
    values = np.asanyarray(values, dtype=np.float32)
    n = values.size
    colors = np.full((n, 4), 255, dtype=np.uint8)
    dvz.colormap_array(cm, n, values.ravel(), vmin, vmax, colors)
    return colors


def to_byte(arr: np.ndarray, vmin: float = None, vmax: float = None) -> np.ndarray:
    """
    Normalize an array to the range [0, 255] and convert to uint8.

    Parameters
    ----------
    arr : np.ndarray
        The array to normalize.
    vmin : float, optional
        The minimum value for normalization, by default None.
    vmax : float, optional
        The maximum value for normalization, by default None.

    Returns
    -------
    np.ndarray
        The normalized array as uint8.
    """
    vmin = vmin if vmin is not None else arr.min()
    vmax = vmax if vmax is not None else arr.max()
    assert vmin < vmax

    normalized = (arr - vmin) * 1.0 / (vmax - vmin)
    normalized = np.clip(normalized, 0, 1)
    normalized *= 255
    return normalized.astype(np.uint8)


def get_size(idx: slice | int, value: np.ndarray, total_size: int = 0) -> int:
    """
    Get the size of a slice or array.

    Parameters
    ----------
    idx : slice or int
        The slice or index.
    value : np.ndarray
        The array.
    total_size : int, optional
        The total size of the array, by default 0.

    Returns
    -------
    int
        The size of the slice or array.
    """
    if isinstance(value, np.ndarray):
        return value.shape[0]
    elif isinstance(idx, slice):
        offset = idx.start or 0
        step = idx.step or 1
        return (idx.stop or total_size - offset) // step
    else:
        return total_size


def dtype_to_format(dtype: str, n_channels: int) -> int:
    """
    Get the format identifier for a given data type and number of channels.

    Parameters
    ----------
    dtype : str
        The data type.
    n_channels : int
        The number of channels.

    Returns
    -------
    int
        The format identifier.
    """
    return cst.DTYPE_FORMATS[dtype, n_channels]


def prepare_data_array(
    name: str, dtype: str, shape: tuple[int, ...], value: np.ndarray
) -> np.ndarray:
    """
    Prepare a data array for use in a visual.

    Parameters
    ----------
    name : str
        The name of the property.
    dtype : str
        The data type of the array.
    shape : tuple of int
        The expected shape of the array.
    value : np.ndarray
        The array to prepare.

    Returns
    -------
    np.ndarray
        The prepared array.

    Raises
    ------
    ValueError
        If the array shape is incorrect.
    """
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


def prepare_data_scalar(name: str, dtype: str, size: int, value: float) -> np.ndarray:
    """
    Prepare a scalar value for use in a visual.

    Parameters
    ----------
    name : str
        The name of the property.
    dtype : str
        The data type of the scalar.
    size : int
        The size of the array to create.
    value : float
        The scalar value.

    Returns
    -------
    np.ndarray
        An array filled with the scalar value.

    Raises
    ------
    ValueError
        If the size is zero.
    """
    if size == 0:
        raise ValueError(f'Property {name} needs to be set after the position')
    pvalue = np.full(size, value, dtype=dtype)
    return pvalue


def mesh_flags(indexed: bool = None, lighting: bool = None, contour: bool = False) -> int:
    """
    Compute the C mesh flags based on the given options.

    Parameters
    ----------
    indexed : bool, optional
        Whether the mesh is indexed, by default None.
    lighting : bool, optional
        Whether lighting is enabled, by default None.
    contour : bool, optional
        Whether contour is enabled, by default False.

    Returns
    -------
    int
        The computed mesh flags.
    """
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


def get_fixed_params(fixed: bool | str) -> tuple[bool, bool, bool]:
    """
    Get the fixed parameters for a visual on all three dimensions.

    Parameters
    ----------
    fixed : bool or str
        The fixed parameters.

    Returns
    -------
    tuple of bool
        A tuple indicating whether each axis is fixed.
    """
    if fixed is True:
        return (True, True, True)
    elif isinstance(fixed, str):
        return ('x' in fixed, 'y' in fixed, 'z' in fixed)
    return (False, False, False)
