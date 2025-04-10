import ctypes
import re
import json
from textwrap import dedent
from pathlib import Path


ROOT_DIR = Path(__file__).parent.parent
TYPES = set()
ENUMS = set()

EXCLUDE_STRUCTS = ('DvzSize', 'DvzColor')
DVZ_COLOR_CVEC4 = 1
DVZ_ALPHA_MAX = 255 if DVZ_COLOR_CVEC4 else 1.0
DvzColor = 'cvec4' if DVZ_COLOR_CVEC4 else 'vec4'
DvzAlpha = 'ctypes.c_uint8' if DVZ_COLOR_CVEC4 else 'ctypes.c_float'


def _extract_int(s):
    try:
        return int(list(filter(str.isdigit, s))[0])
    except IndexError:
        return 1


# Original C type to ctype, no pointers.
def c_to_ctype(type, enum_int=False, unsigned=None):
    assert '*' not in type
    # n = _extract_int(type)
    type_ = type if not type.endswith('_t') else type[:-2]

    if type.startswith('vec') or type[1:].startswith('vec') or type.startswith('mat'):
        return type

    elif type == 'DvzIndex':
        return 'ctypes.c_uint32'

    elif enum_int and type in ENUMS:
        return "ctypes.c_int32"

    elif type == 'char' and unsigned:
        return 'ctypes.c_ubyte'

    elif hasattr(ctypes, f'c_{type_}'):
        return f"ctypes.c_{'u' if unsigned else ''}{type_}"

    elif type == 'void':
        return 'None'

    else:
        return type


# Original C type to np.dtype, no pointers.
def c_to_dtype(type, enum_int=False, unsigned=None):
    import numpy as np
    assert '*' not in type
    n = _extract_int(type)
    type_ = type if not type.endswith('_t') else type[:-2]

    if type.startswith(('vec', 'mat')):
        return 'np.float32', n

    elif type.startswith('cvec'):
        return 'np.uint8', n

    elif type.startswith('uvec'):
        return 'np.uint32', n

    elif type.startswith('dvec'):
        return 'np.double', n

    elif type == 'DvzIndex':
        return 'np.uint32'

    elif type == 'DvzColor':
        if DVZ_COLOR_CVEC4:
            return 'np.uint8', 4
        else:
            return 'np.float', 4

    elif type == 'DvzAlpha':
        if DVZ_COLOR_CVEC4:
            return 'np.uint8'
        else:
            return 'np.float'

    elif type == 'float':
        return 'np.float32'

    elif type == 'bool':
        return 'bool'

    elif type == 'char' and unsigned:
        return 'np.ubyte'

    elif hasattr(np, type):
        return f'np.{type}'

    elif hasattr(np, type_):
        return f'np.{type_}'

    else:
        return None


# Original C pointer to ndpointer.
def cpointer_to_ndpointer(type, unsigned=None, ndpointer=True):
    assert '*' in type
    assert type.endswith('*')
    btype = type[:-1]
    dtype = c_to_dtype(btype, unsigned=unsigned)
    if dtype and ndpointer:
        if isinstance(dtype, tuple):
            dtype, n = dtype
            ndim = 2
        else:
            n = 1
            ndim = 1
        # NOTE: redefined in ctypes_header.py to support None arguments
        return f'ndpointer(dtype={dtype}, ndim={ndim}, ncol={n}, flags="C_CONTIGUOUS")'
    else:
        if btype.startswith("Dvz"):
            TYPES.add(btype)
        ctype = c_to_ctype(btype, enum_int=True) or btype
        return f'ctypes.POINTER({ctype})'


# Final type mapping.
def map_type(type, enum_int=False, unsigned=None, ndpointer=True, out=None, out_type=None, argtypes=None):
    assert type
    if type == 'char*' and not unsigned:
        return 'ctypes.c_char_p' if not argtypes else 'CStringBuffer'

    elif type == 'char**' and not unsigned:
        return 'ctypes.POINTER(ctypes.c_char_p)' if not argtypes else 'CStringArrayType'

    elif type == "void*":
        # NOTE: void* becomes either "ctypes.c_void_p", or an ndpointer if the argument is
        # typically expected to be an ndarray
        if ndpointer:
            return 'ndpointer(dtype=None, ndim=None, flags="C_CONTIGUOUS")'
        else:
            return "ctypes.c_void_p"

    elif type.endswith('*'):
        if not out or out_type == 'array':
            return cpointer_to_ndpointer(type, unsigned=unsigned, ndpointer=ndpointer)
        else:
            # Passing out parameter, normal ctypes Pointer, and caller uses Out(py_var)
            return 'Out'
            # assert '*' in type
            # assert type.endswith('*')
            # btype = type[:-1]
            # ctype = c_to_ctype(btype, enum_int=True) or btype
            # return f'ctypes.POINTER({ctype})'

    else:
        return c_to_ctype(type, enum_int=enum_int, unsigned=unsigned)


def extract_version(version_path):
    with open(version_path, 'r') as f:
        version_contents = f.read()
    major = int(re.compile(
        r"#define DVZ_VERSION_MAJOR ([0-9a-z\.]+)").search(version_contents).group(1))
    minor = int(re.compile(
        r"#define DVZ_VERSION_MINOR ([0-9a-z\.]+)").search(version_contents).group(1))
    patch = int(re.compile(
        r"#define DVZ_VERSION_PATCH ([0-9a-z\.]+)").search(version_contents).group(1))
    dev = re.compile(
        r"#define DVZ_VERSION_DEVEL ([0-9a-z\.\-]+)").search(version_contents)
    version = f"{major}.{minor}.{patch}"
    if dev:
        version += "-dev"
    return version


def convert_javadoc_to_numpy(javadoc_str, func_info):
    # Remove the leading and trailing /** and */
    javadoc_str = javadoc_str.strip().strip('/**').strip('*/')

    # Split the content into lines and strip leading '*'
    lines = [line.strip().lstrip('*').strip()
             for line in javadoc_str.splitlines()]

    # Extract the description (everything before the first @param or @returns)
    description_lines = []
    param_started = False
    for line in lines:
        if line.startswith('@param') or line.startswith('@returns'):
            param_started = True
        if not param_started:
            description_lines.append(line)

    description = ' '.join(description_lines).strip()

    # Find all @param and @returns lines
    params = re.findall(r'@param([\[\]out]*)\s+(\w+)\s+(.*)', javadoc_str)
    returns = re.search(r'@returns?\s+(.*)', javadoc_str)

    # Start building the NumPy-style docstring
    numpy_doc = f"{description}\n\n"

    args = {item['name']: item['dtype'] for item in func_info['args']}
    if params:
        numpy_doc += "Parameters\n----------\n"
        for out, param, desc in params:
            type = args.get(param, 'unknown')
            out = ' (out parameter)' if out else ''
            numpy_doc += f"{param} : {type}{out}\n    {desc}\n"

    if returns:
        numpy_doc += "\nReturns\n-------\n"
        numpy_doc += f"type\n    {returns.group(1)}\n"

    return numpy_doc.strip()


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
    enum_values = []
    for fn in data:
        enums = data.get(fn, {}).get("enums", {})
        for enum_name, enum_info in enums.items():
            ENUMS.add(enum_name)
            out += f'class {enum_name}(CtypesEnum):\n'
            for value in enum_info.get('values', []):
                out += f'    {value[0]} = {value[1]}\n'
                enum_values.append(value)
            out += '\n\n'
    # Aliases without the DVZ_ prefix.
    out += '# Function aliases\n\n'
    for name, value in enum_values:
        if name.startswith('DVZ_'):
            name = name[4:]
        out += f'{name} = {value}\n'
    out += '\n\n'

    # Forward declarations.
    delim("FORWARD DECLARATIONS")
    out += "{forward}"

    # Generate ctypes structures.
    delim("STRUCTURES")
    struct_names = []
    for fn in data:
        structs = data.get(fn, {}).get("structs", {})
        for struct_name, struct_info in structs.items():
            struct_names.append(struct_name)
            is_union = struct_info["type"] == "union"
            cls = "Structure" if not is_union else "Union"
            out += f'class {struct_name}(ctypes.{cls}):\n'
            out += '    _pack_ = 8\n'
            out += '    _fields_ = [\n'
            for field in struct_info.get('fields', []):
                unsigned = field.get("unsigned", None)
                dtype = map_type(
                    field["dtype"], enum_int=True, unsigned=unsigned, ndpointer=False)
                out += f'        ("{field["name"]}", {dtype}),\n'
            out += '    ]\n\n\n'
    # Aliases without the Dvz prefix.
    out += '# Struct aliases\n\n'
    for name in struct_names:
        if name.startswith('Dvz'):
            out += f'{name[3:]} = {name}\n'
    out += '\n\n'

    # Function callbacks.
    delim("FUNCTION CALLBACK TYPES")
    out += dedent("""    gui = DvzAppGuiCallback = ctypes.CFUNCTYPE(None, P_(DvzApp), DvzId, DvzGuiEvent)
    mouse = DvzAppMouseCallback = ctypes.CFUNCTYPE(None, P_(DvzApp), DvzId, DvzMouseEvent)
    keyboard = DvzAppKeyboardCallback = ctypes.CFUNCTYPE(None, P_(DvzApp), DvzId, DvzKeyboardEvent)
    frame = DvzAppFrameCallback = ctypes.CFUNCTYPE(None, P_(DvzApp), DvzId, DvzFrameEvent)
    timer = DvzAppTimerCallback = ctypes.CFUNCTYPE(None, P_(DvzApp), DvzId, DvzTimerEvent)
    resize = DvzAppResizeCallback = ctypes.CFUNCTYPE(None, P_(DvzApp), DvzId, DvzWindowEvent)
    DvzErrorCallback = ctypes.CFUNCTYPE(None, ctypes.c_char_p)

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
            docstring = func_info.get('docstring', '')
            docstring = convert_javadoc_to_numpy(docstring, func_info)
            docstring = docstring.replace('"', '\"')

            out += f'# Function {orig_name}()\n'
            out += f'{func_name} = dvz.{orig_name}\n'
            out += f'{func_name}.__doc__ = """\n{docstring}\n"""\n'
            out += f'{func_name}.argtypes = [\n'

            # annotations = {}
            for arg in func_info.get('args', []):
                if arg["dtype"] != "void":

                    # HACK: when there is "void* data", use a contiguous ndarray Python type
                    if arg["dtype"] == 'void*':
                        ndpointer = arg["name"] == 'data'
                    else:
                        ndpointer = True

                    mtype = map_type(
                        arg["dtype"],
                        ndpointer=ndpointer,
                        out=arg.get("out", None),
                        out_type=arg.get("out_type", None),
                        argtypes=True)
                    out += (f'    {mtype},  '
                            f'# {"out " if arg.get('out', None) else ""}{arg["dtype"]} {arg["name"]}\n')
                    # annotations[arg["name"]] = mtype
            out += ']\n'
            restype = map_type(func_info["returns"])
            if restype and restype != "None":
                out += f'{func_name}.restype = {restype}\n'
                # annotations['return'] = restype

            # out += f'{func_name}.__annotations__ = {str(annotations)}\n'

            out += '\n'

    # Forward declarations.
    forward = ""
    for dtype in sorted(TYPES):
        # Remove the structure from the forward declarations if it is already defined.
        if dtype not in struct_names and dtype not in ENUMS and dtype not in EXCLUDE_STRUCTS:
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

        # Color type.
        file.write(f'DVZ_ALPHA_MAX = {DVZ_ALPHA_MAX}\n')
        file.write(f'DVZ_COLOR_CVEC4 = {DVZ_COLOR_CVEC4}\n')
        file.write(f'DvzColor = {DvzColor}\n')
        file.write(f'DvzAlpha = {DvzAlpha}\n')

        file.write(f"\n\n{out}")

        file.write(f'DVZ_FORMAT_COLOR = FORMAT_R8G8B8A8_UNORM\n')
        _include_py(file, "ctypes_footer.py")


if __name__ == "__main__":
    headers_json_path = ROOT_DIR / 'tools/headers.json'
    output_path = ROOT_DIR / 'datoviz/__init__.py'
    version_path = ROOT_DIR / 'include/datoviz_version.h'

    generate_ctypes_bindings(headers_json_path, output_path, version_path)
