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
from pathlib import Path
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
    os.environ["VK_DRIVER_FILES"] = str(LIB_DIR / "MoltenVK_icd.json")


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


class CStringArrayType:
    @classmethod
    def from_param(cls, value):
        if not isinstance(value, list):
            raise TypeError("Expected a list of strings")

        encoded = [s.encode("utf-8") for s in value]
        bufs = [ctypes.create_string_buffer(s) for s in encoded]
        arr = (ctypes.c_char_p * len(bufs))(*
                                            [ctypes.cast(b, ctypes.c_char_p) for b in bufs])

        # keep references alive
        arr._buffers = bufs
        return arr


class CStringBuffer:
    @classmethod
    def from_param(cls, value):
        if isinstance(value, Path):
            value = str(value)
        if not isinstance(value, str):
            raise TypeError("Expected a string")
        buf = ctypes.create_string_buffer(value.encode("utf-8"))
        # keep reference to buffer so it's not GC'd
        buf._keepalive = buf
        return buf


# ===============================================================================
# Out wrapper
# ===============================================================================


class Out:
    _ctype_map = {
        float: ctypes.c_float,
        int: ctypes.c_int,
        bool: ctypes.c_bool,
    }

    def __init__(self, initial):
        py_type = type(initial)
        if py_type not in self._ctype_map:
            raise TypeError(f"Unsupported type: {py_type}")
        self._ctype = self._ctype_map[py_type]
        self._buffer = self._ctype(initial)

    @property
    def value(self):
        return self._buffer.value

    def __ctypes_from_outparam__(self):
        return ctypes.byref(self._buffer)

    @classmethod
    def from_param(cls, obj):
        if not isinstance(obj, cls):
            raise TypeError("Expected an Out instance")
        return ctypes.byref(obj._buffer)


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


# HACK: accept None ndarrays as arguments, see https://stackoverflow.com/a/37664693/1595060
def ndpointer(*args, **kwargs):
    ndim = kwargs.pop("ndim", None)
    ncol = kwargs.pop("ncol", None)
    base = ndpointer_(*args, **kwargs)

    @classmethod
    def from_param(cls, obj):
        if obj is None:
            return obj
        if isinstance(obj, np.ndarray):
            s = f"array <{obj.dtype}>{obj.shape}"
            if ndim and obj.ndim != ndim:
                raise ValueError(
                    f"Wrong ndim {obj.ndim} (expected {ndim}) for {s}")
            if ncol and ncol > 1 and obj.shape[1] != ncol:
                raise ValueError(
                    f"Wrong shape {obj.shape} (expected (*, {ncol})) for {s}")
            out = base.from_param(obj)
        else:
            # NOTE: allow passing ndpointers without change
            out = obj
        return out

    return type(base.__name__, (base,), {"from_param": from_param})


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


# ===============================================================================
# Vec types
# ===============================================================================


class CVectorBase(ctypes.Array):
    _type_ = ctypes.c_float
    _length_ = 0
    _name_ = ""

    def __new__(cls, *values):
        values = list(values) + [0] * (cls._length_ - len(values))
        return super().__new__(cls, *values[: cls._length_])

    def __repr__(self):
        vals = ", ".join(f"{v:.6g}" for v in self)
        return f"{self._name_}({vals})"


def make_vector_type(name, ctype, length):
    return type(
        name,
        (CVectorBase,),
        {
            "_type_": ctype,
            "_length_": length,
            "_name_": name,
        },
    )


vec2 = make_vector_type("vec2", ctypes.c_float, 2)
vec3 = make_vector_type("vec3", ctypes.c_float, 3)
vec4 = make_vector_type("vec4", ctypes.c_float, 4)
dvec2 = make_vector_type("dvec2", ctypes.c_double, 2)
dvec3 = make_vector_type("dvec3", ctypes.c_double, 3)
dvec4 = make_vector_type("dvec4", ctypes.c_double, 4)
cvec4 = make_vector_type("cvec4", ctypes.c_uint8, 4)
uvec2 = make_vector_type("uvec2", ctypes.c_uint32, 2)
uvec3 = make_vector_type("uvec3", ctypes.c_uint32, 3)
uvec4 = make_vector_type("uvec4", ctypes.c_uint32, 4)
ivec2 = make_vector_type("ivec2", ctypes.c_int32, 2)
ivec3 = make_vector_type("ivec3", ctypes.c_int32, 3)
ivec4 = make_vector_type("ivec4", ctypes.c_int32, 4)
mat3 = make_vector_type("mat3", ctypes.c_int32, 3 * 3)
mat4 = make_vector_type("mat4", ctypes.c_int32, 4 * 4)


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


DEFAULT_CLEAR_COLOR = cvec4()
DEFAULT_VIEWPORT = vec2()

from_array = array_pointer
from_pointer = pointer_array


# ===============================================================================
# Helpers
# ===============================================================================

def from_enum(enum_cls, value):
    for name, val in enum_cls.__dict__.items():
        if not name.startswith("_") and isinstance(val, int) and val == value:
            return name
    return None


def key_name(key_code):
    name = from_enum(KeyCode, key_code)
    name = name.replace("DVZ_KEY_", "")
    return name


def button_name(button):
    name = from_enum(MouseButton, button)
    name = name.replace("DVZ_MOUSE_BUTTON_", "")
    return name


def cmap(cm, values):
    values = np.asanyarray(values, dtype=np.float32)
    # shape = values.shape
    n = values.size
    colors = np.full((n, 4), 255, dtype=np.uint8)
    colormap_array(cm, n, values.ravel(), 0, 1, colors)
    return colors


def merge_shapes(shapes):
    return shape_merge(len(shapes), (Shape * len(shapes))(*shapes))
