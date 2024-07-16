import json
from textwrap import dedent
from pathlib import Path


ROOT_DIR = Path(__file__).parent.parent
TYPES = set()
ENUMS = set()

HEADER = """'''
WARNING: DO NOT EDIT: automatically-generated file
'''

# ===============================================================================
# Imports
# ===============================================================================

import ctypes
import faulthandler
import os
import pathlib
import platform

from enum import IntEnum


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

# Development paths: the libraries are in build/ and libs/
if not DATOVIZ_DIR.exists():
    DATOVIZ_DIR = (FILE_DIR / "../build/").resolve()

if not LIB_DIR.exists():
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


"""


def map_ctype(dtype, enum_int=False):
    """Map C type from JSON to ctypes."""
    type_mappings = {
        "int": "c_int",

        "uint8_t": "c_uint8",
        "uint16_t": "c_uint16",
        "uint32_t": "c_uint32",
        "uint64_t": "c_uint64",

        "bool": "c_bool",
        "float": "c_float",
        "double": "c_double",

        "vec2": "c_float * 2",
        "vec3": "c_float * 3",
        "vec4": "c_float * 4",
        "mat4": "c_float * 16",

        "cvec2": "c_int8 * 2",
        "cvec3": "c_int8 * 3",
        "cvec4": "c_int8 * 4",

        "uvec2": "c_uint32 * 2",
        "uvec3": "c_uint32 * 3",
        "uvec4": "c_uint32 * 4",

        "dvec2": "c_double * 2",
        "dvec3": "c_double * 3",
        "dvec4": "c_double * 4",

        "vec3*": "POINTER(ctypes.c_float)",
        "cvec4*": "POINTER(ctypes.c_uint8)",
        "vec4*": "POINTER(ctypes.c_float)",

        "DvzId": "c_uint64",
        "DvzIndex*": "POINTER(ctypes.c_uint32)",

        "char*": "c_char_p",
    }

    if "void*" in dtype:
        return "ctypes.c_void_p"

    # TODO HACK: fix callbacks
    if dtype.endswith("Callback") or dtype == "DvzAppGui":
        return "ctypes.c_void_p"

    if enum_int and dtype in ENUMS:
        return "ctypes.c_int32"

    if dtype == "void":
        return "None"

    if dtype in type_mappings:
        return f"ctypes.{type_mappings.get(dtype)}"
    elif dtype.endswith('*'):
        base_type = dtype[:-1]
        if dtype.startswith("Dvz"):
            TYPES.add(base_type)
        p = type_mappings.get(base_type, base_type)
        if base_type in type_mappings:
            p = "ctypes." + p
        return f"ctypes.POINTER({p})"
    else:
        return dtype


def generate_ctypes_bindings(headers_json_path, output_path):
    with open(headers_json_path, 'r') as file:
        data = json.load(file)

    out = ""

    def delim(name):
        nonlocal out
        out += dedent(f"""        # {'=' * 79}
        # {name}
        # {'=' * 79}

        """)

    # Handle defines
    delim("DEFINES")
    for fn in data:
        defines = data.get(fn, {}).get("defines", {})
        for define_name, define_value in defines.items():
            out += f'{define_name} = {define_value}\n'
    out += "\n\n"

    # Handle enums
    delim("ENUMERATIONS")
    for fn in data:
        enums = data.get(fn, {}).get("enums", {})
        for enum_name, enum_info in enums.items():
            ENUMS.add(enum_name)
            out += f'class {enum_name}(CtypesEnum):\n'
            for value in enum_info.get('values', []):
                out += f'    {value[0]} = {value[1]}\n'
            out += '\n\n'

    delim("FORWARD DECLARATIONS")
    out += "{forward}"

    # Generate ctypes structures
    delim("STRUCTURES")
    for fn in data:
        structs = data.get(fn, {}).get("structs", {})
        for struct_name, struct_info in structs.items():
            out += f'class {struct_name}(ctypes.Structure):\n'
            out += '    _fields_ = [\n'
            for field in struct_info.get('fields', []):
                dtype = map_ctype(field["dtype"], enum_int=True)
                out += f'        ("{field["name"]}", {dtype}),\n'
            out += '    ]\n\n\n'

    # Generate ctypes function bindings
    delim("FUNCTIONS")
    for fn in data:
        functions = data.get(fn, {}).get("functions", {})
        for func_name, func_info in functions.items():
            orig_name = func_name
            # Remove dvz_ prefix.
            if func_name.startswith("dvz_"):
                func_name = func_name[4:]

            out += f'# Function {orig_name}()\n'
            out += f'{func_name} = dvz.{orig_name}\n'
            out += f'{func_name}.argtypes = [\n'
            for arg in func_info.get('args', []):
                if arg["dtype"] != "void":
                    out += (f'    {map_ctype(arg["dtype"])},  '
                            f'# {arg["dtype"]} {arg["name"]}\n')
            out += ']\n'
            restype = map_ctype(func_info["returns"])
            if restype != "None":
                out += f'{func_name}.restype = {restype}\n'
            out += '\n'

    # Forward declarations.
    forward = ""
    for dtype in sorted(TYPES):
        forward += f"class {dtype}(ctypes.Structure):\n    pass\n\n\n"
    out = out.replace('{forward}', forward)

    with open(output_path, 'w') as file:
        file.write(HEADER)
        file.write(f"{out}")


if __name__ == "__main__":
    headers_json_path = ROOT_DIR / 'tools/headers.json'
    output_path = ROOT_DIR / 'datoviz/ctypes_wrapper.py'

    generate_ctypes_bindings(headers_json_path, output_path)
