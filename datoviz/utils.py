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
from collections.abc import Iterable
from ctypes import c_char_p, c_uint8
from typing import Dict, List, Optional, Tuple, Union

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


def char_pointer(s: Union[str, List[str]]) -> ctypes.POINTER:
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


def to_cvec4_array(colors):
    """
    Convert a list of RGBA colors to a C array of cvec4.
    """
    cvec4 = c_uint8 * 4
    cvec4_array = cvec4 * len(colors)
    return cvec4_array(*(cvec4(*rgba) for rgba in colors))


# -------------------------------------------------------------------------------------------------
# Misc
# -------------------------------------------------------------------------------------------------


def get_version() -> Dict[str, str]:
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


def is_enumerable(x) -> bool:
    """Return whether a variable is an enumerable."""
    return isinstance(x, Iterable) and not isinstance(x, (str, bytes))


# -------------------------------------------------------------------------------------------------
# Enumerations
# -------------------------------------------------------------------------------------------------


def from_enum(enum_cls: type, value: int, prettify: bool = True) -> Optional[str]:
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


def to_enum(enumstr: Union[str, int]) -> int:
    """
    Convert an enum string to its corresponding value.

    Parameters
    ----------
    enumstr : str or int
        The enum string or value (no-op).

    Returns
    -------
    int
        The enum value.
    """
    return getattr(dvz, enumstr.upper(), 0 if isinstance(enumstr, str) else enumstr)


# -------------------------------------------------------------------------------------------------
# User events
# -------------------------------------------------------------------------------------------------


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


# -------------------------------------------------------------------------------------------------
# Color maps
# -------------------------------------------------------------------------------------------------


def cmap(
    cm: Union[str, int], values: np.ndarray, vmin: float = 0.0, vmax: float = 1.0
) -> np.ndarray:
    """
    Apply a colormap to an array of values.

    Parameters
    ----------
    cm : str or int
        The colormap identifier, either as a string name or an integer.
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
    if isinstance(cm, str):
        cm = to_enum(f'cmap_{cm}')
    assert isinstance(cm, int)
    values = np.asanyarray(values, dtype=np.float32)
    n = values.size
    colors = np.full((n, 4), 255, dtype=np.uint8)
    dvz.colormap_array(cm, n, values.ravel(), vmin, vmax, colors)
    return colors


# -------------------------------------------------------------------------------------------------
# Data preparation
# -------------------------------------------------------------------------------------------------


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


def get_size(idx: Union[slice, int], value: np.ndarray, total_size: int = 0) -> int:
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
    if isinstance(value, list):
        return len(value)
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
    name: str, dtype: str, shape: Tuple[int, ...], value: np.ndarray
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
            raise ValueError(f'Incorrect shape for {name}: {pvalue.shape[dim]} != {shape[dim]}')
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


# -------------------------------------------------------------------------------------------------
# Visual helpers
# -------------------------------------------------------------------------------------------------


def image_flags(
    unit: str = None,
    mode: str = None,
    rescale: str = None,
    border: bool = None,
) -> int:
    """
    Compute the image flags for rendering based on the provided options.

    Parameters
    ----------
    unit : str, optional
        Specifies the unit for the image size. Can be:
        - `pixels` (default): Image size is specified in pixels.
        - `ndc`: Image size depends on the normalized device coordinates (NDC) of the panel.
    mode : str, optional
        Specifies the image mode. Can be:
        - `rgba` (default): RGBA image mode.
        - `colormap`: Single-channel image with a colormap applied.
        - `fill`: Uniform color fill mode.
    rescale : str, optional
        Specifies how the image should be rescaled with transformations. Can be:
        - `None` (default): No rescaling.
        - `rescale`: Rescale the image with the panel size.
        - `keep_ratio`: Rescale the image while maintaining its aspect ratio.
    border : bool, optional
        Indicates whether to display a border around the image. Defaults to `False`.

    Returns
    -------
    int
        The computed image flags as an integer bitmask.
    """
    c_flags = 0

    # Image size unit.
    unit = unit if unit is not None else cst.DEFAULT_IMAGE_UNIT
    c_flags |= to_enum(f'image_flags_size_{unit}')

    # Image mode.
    mode = mode if mode is not None else cst.DEFAULT_IMAGE_MODE
    c_flags |= to_enum(f'image_flags_mode_{mode}')

    # Image rescaling.
    rescale = rescale if rescale is not None else cst.DEFAULT_IMAGE_RESCALE
    if rescale is True or rescale == 'rescale':
        c_flags |= dvz.IMAGE_FLAGS_RESCALE
    elif rescale == 'keep_ratio':
        c_flags |= dvz.IMAGE_FLAGS_RESCALE_KEEP_RATIO

    # Border.
    border = border if border is not None else cst.DEFAULT_IMAGE_BORDER
    if border:
        c_flags |= dvz.IMAGE_FLAGS_BORDER

    return c_flags


def mesh_flags(
    indexed: bool = None,
    textured: bool = None,
    lighting: bool = None,
    contour: bool = None,
    isoline: bool = None,
) -> int:
    """
    Compute the C mesh flags based on the given options.

    Parameters
    ----------
    indexed : bool
        Whether the mesh is indexed.
    textured : bool
        Whether to use a texture for the mesh.
    lighting : bool
        Whether lighting is enabled.
    contour : bool
        Whether contour is enabled.
    isoline : bool
        Whether to show isolines.

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
    if textured:
        c_flags |= dvz.MESH_FLAGS_TEXTURED
    if lighting:
        c_flags |= dvz.MESH_FLAGS_LIGHTING
    if contour:
        c_flags |= dvz.MESH_FLAGS_CONTOUR
    if isoline:
        c_flags |= dvz.MESH_FLAGS_ISOLINE
    return c_flags


def sphere_flags(
    textured: bool = None,
    equal_rectangular: bool = None,
    lighting: bool = None,
    size_pixels: bool = None,
) -> int:
    """
    Compute the C mesh flags based on the given options.

    Parameters
    ----------
    textured : bool
        Whether to use a texture for the sphere.
    equal_rectangular : bool
        Whether texture is equal rectangular or front/back tiled.
    lighting : bool
        Whether lighting is enabled.
    size_pixels : bool
        Whether to specify the sphere size in pixels rather than NDC.

    Returns
    -------
    int
        The computed sphere flags.
    """
    c_flags = 0
    lighting = lighting if (lighting is not None) else cst.DEFAULT_LIGHTING
    equal_rectangular = (
        equal_rectangular if (equal_rectangular is not None) else cst.DEFAULT_EQUAL_RECTANGULAR
    )
    if textured:
        c_flags |= dvz.SPHERE_FLAGS_TEXTURED
    if equal_rectangular:
        c_flags |= dvz.SPHERE_FLAGS_EQUAL_RECTANGULAR
    if lighting:
        c_flags |= dvz.SPHERE_FLAGS_LIGHTING
    if size_pixels:
        c_flags |= dvz.SPHERE_FLAGS_SIZE_PIXELS
    return c_flags


def get_fixed_flag(fixed: Union[bool, str]) -> int:
    """
    Get the fixed parameters for a visual on all three dimensions.

    Parameters
    ----------
    fixed : bool or str
        The fixed parameters.

    Returns
    -------
    int
        The visual fixed flag.
    """
    if fixed is True:
        return dvz.VISUAL_FLAGS_FIXED_ALL
    elif isinstance(fixed, str):
        c_flags = 0
        if 'x' in fixed:
            c_flags |= dvz.VISUAL_FLAGS_FIXED_X
        if 'y' in fixed:
            c_flags |= dvz.VISUAL_FLAGS_FIXED_Y
        if 'z' in fixed:
            c_flags |= dvz.VISUAL_FLAGS_FIXED_Z
        return c_flags
    return 0
