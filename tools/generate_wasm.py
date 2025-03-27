# Parse the C headers.

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import json
from pathlib import Path
from textwrap import dedent


# -------------------------------------------------------------------------------------------------
# Constants
# -------------------------------------------------------------------------------------------------

ROOT_DIR = Path(__file__).parent.parent
HEADERS_JSON = ROOT_DIR / "tools/headers.json"
FUNCTIONS_TXT = ROOT_DIR / "wasm_functions.txt"
WRAPPERS_JS = ROOT_DIR / "build/wrappers.js"

TYPE_MAP = {
    "void": None,
    "int": "number",
    "float": "number",
    "double": "number",
    "char*": "string",
    "const char*": "string",
    "bool": "number",
    "size_t": "number",
}


# -------------------------------------------------------------------------------------------------
# Utils
# -------------------------------------------------------------------------------------------------

def map_type(dtype):
    dtype = dtype.strip()
    if dtype in TYPE_MAP:
        return TYPE_MAP[dtype]
    if dtype.endswith("*"):
        return "number"
    return "number"

def strip_prefix(name):
    return name[4:] if name.startswith("DVZ_") else name


# -------------------------------------------------------------------------------------------------
# Constant resolution
# -------------------------------------------------------------------------------------------------

def resolve_constants(raw_constants):
    resolved = {}

    def resolve(name):
        if name in resolved:
            return resolved[name]
        if name not in raw_constants:
            return name
        val = raw_constants[name]
        if isinstance(val, (int, float)):
            resolved[name] = val
        elif isinstance(val, str):
            # resolve references recursively
            resolved[name] = resolve(val)
        else:
            raise ValueError(f"Unsupported constant value: {name} = {val}")
        return resolved[name]

    for name in raw_constants:
        resolve(name)

    return resolved


# -------------------------------------------------------------------------------------------------
# Entry-point
# -------------------------------------------------------------------------------------------------

if __name__ == '__main__':

    with open(HEADERS_JSON) as f:
        data = json.load(f)

    with open(FUNCTIONS_TXT) as f:
        exported = set(line.strip() for line in f if line.strip())

    wrappers = []
    raw_constants = {}

    for header in data.values():

        # Functions
        for fn in header.get("functions", {}).values():
            name = fn["name"]
            if name not in exported:
                continue
            ret = map_type(fn["returns"])
            args = [map_type(arg["dtype"]) for arg in fn["args"]]
            short_name = name[4:] if name.startswith('dvz_') else name
            wrappers.append(
                f"    '{short_name}': Module.cwrap('{name}', {json.dumps(ret)}, {json.dumps(args)}),"
            )

        # Defines
        for name, value in header.get("defines", {}).items():
            raw_constants[name] = value

        # Enums
        for enum in header.get("enums", {}).values():
            for name, value in enum.get("values", []):
                raw_constants[name] = value

    # resolve all constants to final numeric values
    resolved_constants = resolve_constants(raw_constants)

    constants = [
        f"    {strip_prefix(name)}: {val},"
        for name, val in sorted(resolved_constants.items())
        if isinstance(val, (int, float))
    ]

    output = dedent(f"""\
        function wrapAll(Module) {{
          return {{
{chr(10).join(sorted(wrappers) + sorted(constants))}
          }};
        }}

        if (typeof module !== 'undefined') module.exports = wrapAll;
    """)

    WRAPPERS_JS.parent.mkdir(parents=True, exist_ok=True)
    WRAPPERS_JS.write_text(output)
