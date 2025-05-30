# ctypes wrapper builder

# Imports
# -------------------------------------------------------------------------------------------------

import ctypes
import json
import re
from pathlib import Path
from textwrap import dedent

# Constants
# -------------------------------------------------------------------------------------------------

ROOT_DIR = Path(__file__).parent.parent
TYPES = set()
ENUMS = set()

HEADER = '''"""
Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
Licensed under the MIT license. See LICENSE file in the project root for details.
SPDX-License-Identifier: MIT
"""

# WARNING: DO NOT EDIT: automatically-generated file
'''.lstrip()

FUNCTION_CALLBACKS = dedent("""

on_gui = DvzAppGuiCallback = ctypes.CFUNCTYPE(None, P_(DvzApp), DvzId, P_(DvzGuiEvent))
on_mouse = DvzAppMouseCallback = ctypes.CFUNCTYPE(None, P_(DvzApp), DvzId, P_(DvzMouseEvent))
on_keyboard = DvzAppKeyboardCallback = ctypes.CFUNCTYPE(None, P_(DvzApp), DvzId, P_(DvzKeyboardEvent))
on_frame = DvzAppFrameCallback = ctypes.CFUNCTYPE(None, P_(DvzApp), DvzId, P_(DvzFrameEvent))
on_timer = DvzAppTimerCallback = ctypes.CFUNCTYPE(None, P_(DvzApp), DvzId, P_(DvzTimerEvent))
on_resize = DvzAppResizeCallback = ctypes.CFUNCTYPE(None, P_(DvzApp), DvzId, P_(DvzWindowEvent))
DvzErrorCallback = ctypes.CFUNCTYPE(None, ctypes.c_char_p)

""")

EXCLUDE_STRUCTS = ('DvzSize', 'DvzColor')
OUT_PARAMS = ('DvzSize', 'DvzBox', 'DvzKeyCode', 'DvzMouseButton', 'DvzTime')
# POINTER_STRUCTS = ('DvzShape',)
DVZ_COLOR_CVEC4 = 1
ALPHA_MAX = 255 if DVZ_COLOR_CVEC4 else 1.0
DvzColor = 'cvec4' if DVZ_COLOR_CVEC4 else 'vec4'
DvzAlpha = 'ctypes.c_uint8' if DVZ_COLOR_CVEC4 else 'ctypes.c_float'

MTYPE_MAPPING = {
    'vec2': 'Tuple[float, float]',
    'vec3': 'Tuple[float, float, float]',
    'vec4': 'Tuple[float, float, float, float]',
    'ivec2': 'Tuple[int, int]',
    'ivec3': 'Tuple[int, int, int]',
    'ivec4': 'Tuple[int, int, int, int]',
    'uvec2': 'Tuple[int, int]',
    'uvec3': 'Tuple[int, int, int]',
    'uvec4': 'Tuple[int, int, int, int]',
    'DvzColor': 'Tuple[int, int, int, int]',
}


# Utils
# -------------------------------------------------------------------------------------------------


def extract_version(version_path):
    with open(version_path) as f:
        version_contents = f.read()
    major = int(
        re.compile(r'#define DVZ_VERSION_MAJOR ([0-9a-z\.]+)').search(version_contents).group(1)
    )
    minor = int(
        re.compile(r'#define DVZ_VERSION_MINOR ([0-9a-z\.]+)').search(version_contents).group(1)
    )
    patch = int(
        re.compile(r'#define DVZ_VERSION_PATCH ([0-9a-z\.]+)').search(version_contents).group(1)
    )
    dev = re.compile(r'#define DVZ_VERSION_DEVEL ([0-9a-z\.\-]+)').search(version_contents)
    version = f'{major}.{minor}.{patch}'
    if dev:
        version += '-dev'
    return version


def _extract_int(s):
    try:
        return int(list(filter(str.isdigit, s))[0])
    except IndexError:
        return 1


# Type mapping
# -------------------------------------------------------------------------------------------------


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
        return 'ctypes.c_int32'

    elif type == 'char' and unsigned:
        return 'ctypes.c_ubyte'

    elif hasattr(ctypes, f'c_{type_}'):
        return f'ctypes.c_{"u" if unsigned else ""}{type_}'

    elif type == 'void':
        return 'None'

    else:
        return type


# Original C type to np.dtype, no pointers.
def c_to_dtype(type, enum_int=False, unsigned=None):
    import numpy as np

    # assert '*' not in type
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

    elif type == 'DvzCapType':
        return 'np.int32'

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
        if btype.startswith('Dvz'):
            TYPES.add(btype)
        ctype = c_to_ctype(btype, enum_int=True) or btype
        return f'ctypes.POINTER({ctype})'


# Final type mapping.
def map_type(
    type, enum_int=False, unsigned=None, ndpointer=True, out=None, out_type=None, argtypes=None
):
    if not type:
        return
    assert type
    if type == 'char*' and not unsigned:
        return 'ctypes.c_char_p' if not argtypes else 'CStringBuffer'

    elif type == 'char**' and not unsigned:
        return 'ctypes.POINTER(ctypes.c_char_p)' if not argtypes else 'CStringArrayType'

    elif type == 'void*':
        # NOTE: void* becomes either "ctypes.c_void_p", or an ndpointer if the argument is
        # typically expected to be an ndarray
        if ndpointer:
            return 'ndpointer(dtype=None, ndim=None, flags="C_CONTIGUOUS")'
        else:
            return 'ctypes.c_void_p'

    elif type == 'DvzShape**':
        return 'ctypes.POINTER(ctypes.POINTER(Shape))'

    elif type.endswith('*'):
        if not out or out_type == 'array':
            return cpointer_to_ndpointer(type, unsigned=unsigned, ndpointer=ndpointer)
        elif type.startswith('Dvz') and type[:-1] not in OUT_PARAMS:
            assert '*' in type
            assert type.endswith('*')
            btype = type[:-1]
            ctype = c_to_ctype(btype, enum_int=True) or btype
            return f'ctypes.POINTER({ctype})'
        else:
            # Passing out parameter, normal ctypes Pointer, and caller uses Out(py_var)
            return 'Out'

    else:
        return c_to_ctype(type, enum_int=enum_int, unsigned=unsigned)


def map_arg_type(arg):
    if arg['dtype'] != 'void':
        # HACK: when there is "void* data", use a contiguous ndarray Python type
        if arg['dtype'] == 'void*':
            ndpointer = arg['name'] == 'data'
        else:
            ndpointer = True

        if arg.get('varargs', False):
            return

        return map_type(
            arg['dtype'],
            ndpointer=ndpointer,
            out=arg.get('out', None),
            out_type=arg.get('out_type', None),
            argtypes=True,
        )


def map_python_type(arg):
    arg = arg.copy()
    d = arg['dtype']

    if arg.pop('out', False):
        arg['dtype'] = arg['dtype'].replace('*', '')
        return f'Out[{map_python_type(arg)}]'
    if '*' not in d:
        if 'int64_t' in d or 'int32_t' in d or 'int16_t' in d:
            return 'int'
        elif d in ('float', 'double'):
            return 'float'
        elif d in MTYPE_MAPPING:
            return MTYPE_MAPPING[d]
    else:
        if d == 'void*':
            return 'np.ndarray'
        elif d == 'char*':
            return 'str'
        elif d == 'char**':
            return 'List[str]'
        elif d[0] != 'D':
            return f'np.ndarray[{arg["dtype"][:-1]}]'
    return d


# Docstrings
# -------------------------------------------------------------------------------------------------


def convert_javadoc_to_numpy(func_info):
    description = func_info['docstring']
    args = [item for item in func_info['args'] if not item.get('varargs', False)]

    # Start building the NumPy-style docstring
    numpy_doc = f'{description}\n\n'

    if args:
        numpy_doc += 'Parameters\n----------\n'
        for arg in args:
            arg_name = arg['name'].strip()
            if not arg_name:
                continue
            # arg_type = arg.get('dtype', None)
            # mtype = map_arg_type(arg)
            arg_desc = arg.get('docstring', '')
            out = ' (out parameter)' if arg.get('out', False) else ''
            arg_type_py = map_python_type(arg)
            numpy_doc += f'{arg_name} : {arg_type_py}{out}\n    {arg_desc}\n'

    returns = func_info.get('returns', None)
    if returns:
        numpy_doc += '\nReturns\n-------\n'
        numpy_doc += (
            f'result : {returns.get("dtype", "unknown")}\n     {returns.get("docstring", "")}\n'
        )

    return numpy_doc.strip()


# Main function
# -------------------------------------------------------------------------------------------------


def generate_ctypes_bindings(headers_json_path, output_path, version_path):
    version = extract_version(version_path)

    with open(headers_json_path) as file:
        data = json.load(file)

    out = ''

    def delim(name):
        nonlocal out
        out += dedent(f"""        # {'=' * 79}
        # {name}
        # {'=' * 79}

        """)

    # Handle defines.
    delim('DEFINES')
    for fn in data:
        defines = data.get(fn, {}).get('defines', {})
        for define_name, define_value in defines.items():
            out += f'{define_name} = {define_value}\n'
    out += '\n\n'

    # Handle enums.
    delim('ENUMERATIONS')
    enum_values = []
    for fn in data:
        enums = data.get(fn, {}).get('enums', {})
        for enum_name, enum_info in enums.items():
            ENUMS.add(enum_name)
            out += f'class {enum_name}(CtypesEnum):\n'
            for value in enum_info.get('values', []):
                out += f'    {value[0]} = {value[1]}\n'
                enum_values.append(value)
            out += '\n\n'
    # Aliases without the DVZ_ prefix.
    out += '# Function aliases\n\n'
    for name in sorted(ENUMS):
        if name.startswith('Dvz'):
            out += f'{name[3:]} = {name}\n'
    out += '\n'
    for name, value in sorted(enum_values):
        if name.startswith('DVZ_'):
            name = name[4:]
        out += f'{name} = {value}\n'
    out += '\n\n'

    # Forward declarations.
    delim('FORWARD DECLARATIONS')
    out += '{forward}'

    # Generate ctypes structures.
    delim('STRUCTURES')
    struct_names = []
    for fn in data:
        structs = data.get(fn, {}).get('structs', {})
        for struct_name, struct_info in structs.items():
            struct_names.append(struct_name)
            is_union = struct_info['type'] == 'union'
            cls = 'Structure' if not is_union else 'Union'
            out += f'class {struct_name}(ctypes.{cls}):\n'
            out += '    _pack_ = 8\n'
            out += '    _fields_ = [\n'
            for field in struct_info.get('fields', []):
                unsigned = field.get('unsigned', None)
                dtype = map_type(field['dtype'], enum_int=True, unsigned=unsigned, ndpointer=False)
                out += f'        ("{field["name"]}", {dtype}),\n'
            out += '    ]\n\n\n'
    # Aliases without the Dvz prefix.
    out += '# Struct aliases\n\n'
    for name in struct_names:
        if name.startswith('Dvz'):
            out += f'{name[3:]} = {name}\n'
    out += '\n\n'

    # Function callbacks.
    delim('FUNCTION CALLBACK TYPES')
    out += FUNCTION_CALLBACKS

    # Generate ctypes function bindings.
    delim('FUNCTIONS')
    for fn in data:
        functions = data.get(fn, {}).get('functions', {})
        for func_name, func_info in functions.items():
            orig_name = func_name
            # Remove dvz_ prefix.
            if func_name.startswith('dvz_'):
                func_name = func_name[4:]
            docstring = func_info.get('docstring', '')
            docstring = convert_javadoc_to_numpy(func_info)
            docstring = docstring.replace('"', '"')

            out += '# ' + ('-' * 97) + '\n'
            # out += f'# Function {orig_name}()\n'
            out += f'{func_name} = dvz.{orig_name}\n'
            out += f'{func_name}.__doc__ = """\n{docstring}\n"""\n'
            out += f'{func_name}.argtypes = [\n'

            for arg in func_info.get('args', []):
                mtype = map_arg_type(arg)
                if not mtype:
                    continue
                out += (
                    f'    {mtype},  '
                    f'# {"out " if arg.get("out", None) else ""}{arg["dtype"]} {arg["name"]}\n'
                )
            out += ']\n'
            restype = map_type(func_info.get('returns', {}).get('dtype', None))
            if restype and restype != 'None':
                out += f'{func_name}.restype = {restype}\n'
            out += '\n\n'

    # Forward declarations.
    forward = ''
    for dtype in sorted(TYPES):
        # Remove the structure from the forward declarations if it is already defined.
        if dtype not in struct_names and dtype not in ENUMS and dtype not in EXCLUDE_STRUCTS:
            forward += f'class {dtype}(ctypes.Structure):\n    pass\n\n\n'
    out = out.replace('{forward}', forward)

    # Write the _ctypes.py file.
    with open(output_path, 'w') as file:

        def _include_py(file, filename, skip=4):
            with open(ROOT_DIR / 'tools' / filename) as f:
                for _ in range(skip):
                    f.readline()
                file.write(f.read())

        file.write(HEADER)
        file.write(
            dedent(f'''
        __version__ = "{version}"
        ''')
        )
        _include_py(file, 'ctypes_header.py')

        # Color type.
        file.write(f'ALPHA_MAX = {ALPHA_MAX}\n')
        file.write(f'DVZ_COLOR_CVEC4 = {DVZ_COLOR_CVEC4}\n')
        file.write(f'DvzColor = {DvzColor}\n')
        file.write(f'DvzAlpha = {DvzAlpha}\n')

        file.write(f'\n\n{out}')

        file.write('DVZ_FORMAT_COLOR = FORMAT_R8G8B8A8_UNORM\n')
        _include_py(file, 'ctypes_footer.py')


if __name__ == '__main__':
    headers_json_path = ROOT_DIR / 'build/headers.json'
    output_path = ROOT_DIR / 'datoviz/_ctypes.py'
    version_path = ROOT_DIR / 'include/datoviz_version.h'

    # with open(headers_json_path) as file:
    #     data = json.load(file)
    # for fn in data:
    #     functions = data.get(fn, {}).get('functions', {})
    #     for func_name, func_info in functions.items():
    #         print('---------------')
    #         print(func_name)
    #         print('---------------')
    #         for arg in func_info.get('args', []):
    #             print(arg.get('name', ''), arg.get('dtype', ''), map_arg_type(arg))
    #         print()

    generate_ctypes_bindings(headers_json_path, output_path, version_path)
