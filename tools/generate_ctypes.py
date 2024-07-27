import ctypes
import re
import json
from textwrap import dedent
from pathlib import Path


ROOT_DIR = Path(__file__).parent.parent
TYPES = set()
ENUMS = set()


def _extract_int(s):
    try:
        return int(list(filter(str.isdigit, s))[0])
    except IndexError:
        return 1


# Original C type to ctype, no pointers.
def c_to_ctype(type, enum_int=False):
    assert '*' not in type
    n = _extract_int(type)
    type_ = type if not type.endswith('_t') else type[:-2]

    if type.startswith('vec'):
        return f'ctypes.c_float * {n}'

    if type.startswith('mat'):
        return f'ctypes.c_float * {n * n}'

    elif type.startswith('cvec'):
        return f'ctypes.c_uint8 * {n}'

    elif type.startswith('uvec'):
        return f'ctypes.c_uint32 * {n}'

    elif type.startswith('dvec'):
        return f'ctypes.c_double * {n}'

    elif type == 'DvzIndex':
        return 'ctypes.c_uint32'

    elif enum_int and type in ENUMS:
        return "ctypes.c_int32"

    elif hasattr(ctypes, f'c_{type_}'):
        return f"ctypes.c_{type_}"

    elif type == 'void':
        return 'None'

    elif type.startswith('Dvz'):
        return type

    else:
        return None


# Original C type to np.dtype, no pointers.
def c_to_dtype(type, enum_int=False):
    import numpy as np
    assert '*' not in type
    type_ = type if not type.endswith('_t') else type[:-2]

    if type.startswith(('vec', 'mat')):
        return 'np.float32'

    elif type.startswith('cvec'):
        return 'np.uint8'

    elif type.startswith('uvec'):
        return 'np.uint32'

    elif type.startswith('dvec'):
        return 'np.double'

    elif type == 'DvzIndex':
        return 'np.uint32'

    elif type == 'float':
        return 'np.float32'

    elif hasattr(np, type):
        return f'np.{type}'

    elif hasattr(np, type_):
        return f'np.{type_}'

    else:
        return None


# Original C pointer to ndpointer.
def cpointer_to_ndpointer(type):
    assert '*' in type
    assert type.endswith('*')
    btype = type[:-1]
    dtype = c_to_dtype(btype)
    if dtype:
        # NOTE: redefined in ctypes_header.py to support None arguments
        return f'ndpointer(dtype={dtype}, flags="C_CONTIGUOUS")'
    else:
        if btype.startswith("Dvz"):
            TYPES.add(btype)
        ctype = c_to_ctype(btype) or btype
        return f'ctypes.POINTER({ctype})'


# Final type mapping.
def map_type(type, enum_int=False):
    assert type
    if type == 'char*':
        return 'ctypes.c_char_p'

    elif type == "void*":
        return "ctypes.c_void_p"

    elif type.endswith('*'):
        return cpointer_to_ndpointer(type)

    else:
        return c_to_ctype(type, enum_int=enum_int)


def extract_version(version_path):
    with open(version_path, 'r') as f:
        version_contents = f.read()
    major = int(re.compile(
        r"#define DVZ_VERSION_MAJOR ([0-9a-z\.]+)").search(version_contents).group(1))
    minor = int(re.compile(
        r"#define DVZ_VERSION_MINOR ([0-9a-z\.]+)").search(version_contents).group(1))
    patch = int(re.compile(
        r"#define DVZ_VERSION_PATCH ([0-9a-z\.]+)").search(version_contents).group(1))
    version = f"{major}.{minor}.{patch}"
    return version


def generate_ctypes_bindings(headers_json_path, output_path, version_path):
    version = extract_version(version_path)

    with open(headers_json_path, 'r') as file:
        data = json.load(file)

    out = ""

    def delim(name):
        nonlocal out
        out += dedent(f"""        # {'=' * 79}
        # {name}
        # {'=' * 79}

        """)

    # Handle defines.
    delim("DEFINES")
    for fn in data:
        defines = data.get(fn, {}).get("defines", {})
        for define_name, define_value in defines.items():
            out += f'{define_name} = {define_value}\n'
    out += "\n\n"

    # Handle enums.
    delim("ENUMERATIONS")
    for fn in data:
        enums = data.get(fn, {}).get("enums", {})
        for enum_name, enum_info in enums.items():
            ENUMS.add(enum_name)
            out += f'class {enum_name}(CtypesEnum):\n'
            for value in enum_info.get('values', []):
                out += f'    {value[0]} = {value[1]}\n'
            out += '\n\n'

    # Forward declarations.
    delim("FORWARD DECLARATIONS")
    out += "{forward}"

    # Generate ctypes structures.
    delim("STRUCTURES")
    for fn in data:
        structs = data.get(fn, {}).get("structs", {})
        for struct_name, struct_info in structs.items():
            is_union = struct_info["type"] == "union"
            cls = "Structure" if not is_union else "Union"
            out += f'class {struct_name}(ctypes.{cls}):\n'
            out += '    _pack_ = 8\n'
            out += '    _fields_ = [\n'
            for field in struct_info.get('fields', []):
                dtype = map_type(field["dtype"], enum_int=True)
                out += f'        ("{field["name"]}", {dtype}),\n'
            out += '    ]\n\n\n'

    # Function callbacks.
    delim("FUNCTION CALLBACK TYPES")
    out += dedent("""    gui = DvzAppGuiCallback = ctypes.CFUNCTYPE(None, P_(DvzApp), DvzId, DvzGuiEvent)
    mouse = DvzAppMouseCallback = ctypes.CFUNCTYPE(None, P_(DvzApp), DvzId, DvzMouseEvent)
    keyboard = DvzAppKeyboardCallback = ctypes.CFUNCTYPE(None, P_(DvzApp), DvzId, DvzKeyboardEvent)
    frame = DvzAppFrameCallback = ctypes.CFUNCTYPE(None, P_(DvzApp), DvzId, DvzFrameEvent)
    timer = DvzAppTimerCallback = ctypes.CFUNCTYPE(None, P_(DvzApp), DvzId, DvzTimerEvent)
    resize = DvzAppResizeCallback = ctypes.CFUNCTYPE(None, P_(DvzApp), DvzId, DvzWindowEvent)


    """)

    # Generate ctypes function bindings.
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
                    out += (f'    {map_type(arg["dtype"])},  '
                            f'# {arg["dtype"]} {arg["name"]}\n')
            out += ']\n'
            restype = map_type(func_info["returns"])
            if restype and restype != "None":
                out += f'{func_name}.restype = {restype}\n'
            out += '\n'

    # Forward declarations.
    forward = ""
    for dtype in sorted(TYPES):
        forward += f"class {dtype}(ctypes.Structure):\n    pass\n\n\n"
    out = out.replace('{forward}', forward)

    # Write the __init__.py file.
    with open(output_path, 'w') as file:
        def _include_py(file, filename, skip=4):
            with open(ROOT_DIR / "tools" / filename, "r") as f:
                for _ in range(skip):
                    f.readline()
                file.write(f.read())

        file.write('"""WARNING: DO NOT EDIT: automatically-generated file"""\n\n')
        file.write(f'__version__ = "{version}"\n')
        _include_py(file, "ctypes_header.py")
        file.write(f"\n\n{out}")
        _include_py(file, "ctypes_footer.py")


if __name__ == "__main__":
    headers_json_path = ROOT_DIR / 'tools/headers.json'
    output_path = ROOT_DIR / 'datoviz/__init__.py'
    version_path = ROOT_DIR / 'include/datoviz_version.h'

    generate_ctypes_bindings(headers_json_path, output_path, version_path)
