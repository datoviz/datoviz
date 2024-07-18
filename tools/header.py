'''
WARNING: DO NOT EDIT: automatically-generated file
'''

# ===============================================================================
# Imports
# ===============================================================================

import json
import ctypes
from ctypes import POINTER as P_
import faulthandler
import os
import pathlib
import platform

from enum import IntEnum

try:
    import numpy as np
    from numpy import float32
    from numpy.ctypeslib import as_ctypes_type as _ctype
except ImportError:
    float32 = object
    print("NumPy is not available")


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
dvz = ctypes.cdll.LoadLibrary(LIB_PATH)

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


def array_pointer(x, dtype=float32):
    if not isinstance(x, np.ndarray):
        return x
    x = x.astype(dtype)
    return x.ctypes.data_as(P_(_ctype(dtype)))


def _check_struct_sizes(json_path):
    """Check the size of the ctypes structs and unions with respect to the sizes output by
    the CMake process (small executable in tools/struct_sizes.c compiled and executed by CMake).
    """
    with open(json_path, "r") as f:
        sizes = json.load(f)
    for name, size_c in sizes.items():
        obj = globals().get(name)
        assert obj
        size_ctypes = ctypes.sizeof(obj)
        # print(name, size_c, size_ctypes)
        if size_c != size_ctypes:
            raise ValueError(
                f"Mismatch struct/union size error with {name}, "
                f"C struct/union size is {size_c} whereas the ctypes size is {size_ctypes}")
    print(f"Sizes of {len(sizes)} structs/unions successfully checked.")


DvzId = ctypes.c_uint64
