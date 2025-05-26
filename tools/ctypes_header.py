"""
This file is automatically included at the top of _ctypes.py during the ctypes wrapper generation.
This is handled by `just ctypes`.
"""


# ===============================================================================
# Imports
# ===============================================================================

import ctypes
import faulthandler
import os
import pathlib
import platform
from collections.abc import Iterable
from ctypes import POINTER as P_  # noqa
from ctypes import byref  # noqa
from enum import IntEnum
from pathlib import Path

try:
    import numpy as np
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


class CStringArrayType:
    @classmethod
    def from_param(cls, value):
        if not isinstance(value, Iterable) or isinstance(value, (str, bytes)):
            raise TypeError("Expected a list of strings")

        encoded = [s.encode("utf-8") for s in value]
        bufs = [ctypes.create_string_buffer(s) for s in encoded]
        arr = (ctypes.c_char_p * len(bufs))(*
                                            [ctypes.cast(b, ctypes.c_char_p) for b in bufs])

        # keep references alive
        arr._buffers = bufs
        return arr


class CStringBuffer:
    """
    A tiny helper for C `char*` arguments.

    • You can still pass a plain Python `str` — a temporary buffer will be
      created exactly like before (read‑only use‑case).

    • Or allocate an *in/out* buffer yourself:

          buf = CStringBuffer("initial text", size=256)
          lib.dvz_gui_textbox(b"Label", buf, buf.size, 0)
          print("New text is:", buf.value)

      After the C call,   buf.value   holds the updated UTF‑8 string.
    """

    # -------- allocate an explicit, reusable buffer -----------------
    def __init__(self, initial: str = "", size: int | None = 64):
        if isinstance(initial, Path):
            initial = str(initial)
        if not isinstance(initial, str):
            raise TypeError("Expected a string")

        raw = initial.encode("utf-8")
        size = (len(raw) + 1) if size is None else size
        if size < len(raw) + 1:
            raise ValueError("Buffer too small for initial contents")

        self._buf = ctypes.create_string_buffer(size)
        ctypes.memmove(self._buf, raw, len(raw))   # copy text, keep final NUL

    # -------- read the current value --------------------------------
    @property
    def value(self) -> str:
        """Return the UTF‑8 text currently stored in the buffer."""
        return self._buf.value.decode("utf‑8")

    # convenience: expose total capacity (bytes, inc. NUL)
    @property
    def size(self) -> int:
        return ctypes.sizeof(self._buf)

    # -------- interface for ctypes ----------------------------------
    @classmethod
    def from_param(cls, value):
        """
        Called automatically by ctypes when this object (or a str/Path)
        appears in `argtypes`.
        """
        # 1) already a CStringBuffer → pass its internal buffer
        if isinstance(value, cls):
            return value._buf

        # 2) plain Path or str (read‑only one‑shot)
        if isinstance(value, Path):
            value = str(value)
        if isinstance(value, str):
            buf = ctypes.create_string_buffer(value.encode("utf‑8"))
            buf._keepalive = buf           # prevent premature GC
            return buf

        raise TypeError("Expected CStringBuffer or str")


# ===============================================================================
# Out wrapper
# ===============================================================================

class Out:
    _ctype_map = {
        float: ctypes.c_float,
        int: ctypes.c_int,
        bool: ctypes.c_bool,
    }

    def __init__(self, initial, ctype=None):
        if ctype:
            self._ctype = getattr(ctypes, f'c_{ctype}') if isinstance(ctype, str) else ctype
        else:
            py_type = type(initial)
            if py_type not in self._ctype_map:
                raise TypeError(f"Unsupported type: {py_type}")
            self._ctype = self._ctype_map[py_type]
        self._buffer = self._ctype(initial)

    @property
    def value(self):
        return self._buffer.value

    @value.setter
    def value(self, new_value):
        self._buffer.value = new_value

    def __ctypes_from_outparam__(self):
        return ctypes.byref(self._buffer)

    @classmethod
    def from_param(cls, obj):
        if not isinstance(obj, cls):
            raise TypeError("Expected an Out instance")
        return ctypes.byref(obj._buffer)

    def __format__(self, format_spec):
        return format(self.value, format_spec)

    def __str__(self):
        return f'Out({self.value})'


# ===============================================================================
# Array wrappers
# ===============================================================================

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


VEC_TYPES = []


def make_vector_type(name, ctype, length):
    t = type(
        name,
        (CVectorBase,),
        {
            "_type_": ctype,
            "_length_": length,
            "_name_": name,
        },
    )
    VEC_TYPES.append(t)
    return t


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
