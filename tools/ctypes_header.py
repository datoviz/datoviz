"""
This file is automatically included at the top of __init__.py during the ctypes wrapper generation.
This is handled by `just ctypes`.
"""


# ===============================================================================
# Imports
# ===============================================================================

import ctypes
from ctypes import POINTER as P_
from ctypes import c_char_p
import faulthandler
import os
import pathlib
import platform

from enum import IntEnum

try:
    import numpy as np
    from numpy import float32
    from numpy.ctypeslib import as_ctypes_type as _ctype
    from numpy.ctypeslib import ndpointer as ndpointer_
except ImportError:
    float32 = object
    raise ImportError("NumPy is not available")


# ===============================================================================
# Fault handler
# ===============================================================================

faulthandler.enable()


# ===============================================================================
# Global variables
# ===============================================================================

PLATFORMS = {
    "Linux": "linux",
    "Darwin": "macos",
    "Windows": "windows",
}
PLATFORM = PLATFORMS.get(platform.system(), None)

LIB_NAMES = {
    "linux": "libdatoviz.so",
    "macos": "libdatoviz.dylib",
    "windows": "libdatoviz.dll",
}
LIB_NAME = LIB_NAMES.get(PLATFORM, "")

FILE_DIR = pathlib.Path(__file__).parent.resolve()

# Package paths: this Python file is stored alongside the dynamic libraries.
DATOVIZ_DIR = FILE_DIR
LIB_DIR = FILE_DIR
LIB_PATH = DATOVIZ_DIR / LIB_NAME

# Development paths: the libraries are in build/ and libs/
if not LIB_PATH.exists():
    DATOVIZ_DIR = (FILE_DIR / "../build/").resolve()
    LIB_DIR = (FILE_DIR / f"../libs/vulkan/{PLATFORM}/").resolve()
    LIB_PATH = DATOVIZ_DIR / LIB_NAME

if not LIB_PATH.exists():
    raise RuntimeError(f"Unable to find `{LIB_PATH}`.")


# ===============================================================================
# Loading the dynamic library
# ===============================================================================

assert LIB_PATH.exists()
try:
    dvz = ctypes.cdll.LoadLibrary(str(LIB_PATH))
except Exception as e:
    print(f"Error loading library at {LIB_PATH}: {e}")
    exit(1)

# on macOS, we need to set the VK_DRIVER_FILES environment variable to the path to the MoltenVK ICD
if PLATFORM == "macos":
    os.environ['VK_DRIVER_FILES'] = str(LIB_DIR / "MoltenVK_icd.json")


# ===============================================================================
# Util classes
# ===============================================================================

# see https://v4.chriskrycho.com/2015/ctypes-structures-and-dll-exports.html
class CtypesEnum(IntEnum):
    @classmethod
    def from_param(cls, obj):
        return int(obj)


class WrappedValue:
    def __init__(self, initial_value, ctype_type=ctypes.c_float):
        self._value = ctype_type(initial_value)
        self.python_value = initial_value

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.python_value = self._value.value

    @property
    def P_(self):
        return ctypes.byref(self._value)

    @property
    def value(self):
        return self._value.value

    @value.setter
    def value(self, new_value):
        self._value.value = new_value

    def __repr__(self):
        return str(self.value)


def array_pointer(x, dtype=None):
    if not isinstance(x, np.ndarray):
        return x
    dtype = dtype or x.dtype
    if not x.flags.c_contiguous:
        print(
            f"Warning: array is not C contiguous, ensure your array is in row-major (C) order "
            "to avoid potential issues")
    if dtype and x.dtype != dtype:
        x = x.astype(dtype)
    return x.ctypes.data_as(P_(_ctype(dtype)))


def pointer_array(pointer, length, n_components, dtype=np.float32):
    np_array = np.ctypeslib.as_array(pointer, shape=(length,))
    np_array = np_array.view(dtype=dtype).reshape(length, n_components)
    return np_array


# HACK: accept None ndarrays as arguments, see https://stackoverflow.com/a/37664693/1595060
def ndpointer(*args, **kwargs):
    ndim = kwargs.pop('ndim', 1)
    ncol = kwargs.pop('ncol', 1)
    base = ndpointer_(*args, **kwargs)

    @classmethod
    def from_param(cls, obj):
        if obj is None:
            return obj
        if isinstance(obj, np.ndarray):
            s = f"array <{obj.dtype}>{obj.shape}"
            if obj.ndim != ndim:
                raise ValueError(
                    f"Wrong ndim {obj.ndim} (expected {ndim}) for {s}")
            if ncol > 1 and obj.shape[1] != ncol:
                raise ValueError(
                    f"Wrong shape {obj.shape} (expected (*, {ncol})) for {s}")
            out = base.from_param(obj)
        else:
            # NOTE: allow passing ndpointers without change
            out = obj
        return out
    return type(base.__name__, (base,), {'from_param': from_param})


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


def vec2(x: float = 0, y: float = 0):
    return (ctypes.c_float * 2)(x, y)


def vec3(x: float = 0, y: float = 0, z: float = 0):
    return (ctypes.c_float * 3)(x, y, z)


def vec4(x: float = 0, y: float = 0, z: float = 0, w: float = 0):
    return (ctypes.c_float * 4)(x, y, z, w)


def cvec4(r: int = 0, g: int = 0, b: int = 0, a: int = 0):
    return (ctypes.c_uint8 * 4)(r, g, b, a)


def uvec3(x: int = 0, y: int = 0, z: int = 0):
    return (ctypes.c_uint32 * 3)(x, y, z)


def uvec3(x: int = 0, y: int = 0, z: int = 0, a: int = 0):
    return (ctypes.c_uint32 * 4)(x, y, z, a)


def ivec3(r: int = 0, g: int = 0, b: int = 0):
    return (ctypes.c_int32 * 3)(r, g, b)


def ivec4(r: int = 0, g: int = 0, b: int = 0, a: int = 0):
    return (ctypes.c_int32 * 4)(r, g, b, a)



# ===============================================================================
# Aliases
# ===============================================================================

DvzId = ctypes.c_uint64
DvzSize = ctypes.c_uint64
DvzShaderStageFlags = ctypes.c_int32


# HACK: mock structs for Qt/Vulkan wrappers
class QApplication(ctypes.Structure):
    pass

class VkInstance(ctypes.Structure):
    pass

class VkDevice(ctypes.Structure):
    pass

class VkFramebuffer(ctypes.Structure):
    pass

class VkRenderPass(ctypes.Structure):
    pass

class VkCommandBuffer(ctypes.Structure):
    pass



DEFAULT_CLEAR_COLOR = (ctypes.c_ubyte * 4)()
DEFAULT_VIEWPORT = (ctypes.c_float * 2)()

from_array = array_pointer
from_pointer = pointer_array

A_ = array_pointer
S_ = char_pointer
V_ = WrappedValue
