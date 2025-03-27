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
CONSTANTS_JS = ROOT_DIR / "build/constants.js"

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


# -------------------------------------------------------------------------------------------------
# Entry-point
# -------------------------------------------------------------------------------------------------

if __name__ == '__main__':

    with open(HEADERS_JSON) as f:
        data = json.load(f)

    # Generate constants.js
    lines = []

    for header, content in data.items():
        # defines
        defines = content.get("defines", {})
        for name, value in defines.items():
            if isinstance(value, (int, float)):
                lines.append(f"const {name} = {value};")

        # enums
        enums = content.get("enums", {})
        for enum_data in enums.values():
            for name, value in enum_data["values"]:
                lines.append(f"const {name} = {value};")

    with open(CONSTANTS_JS, "w") as f:
        f.write("\n".join(lines) + "\n")


    # Generate wrappers.js
    with open(FUNCTIONS_TXT) as f:
        exported = set(line.strip() for line in f if line.strip())

    wrappers = []

    for header in data.values():
        for fn in header.get("functions", {}).values():
            name = fn["name"]
            if name not in exported:
                continue
            ret = map_type(fn["returns"])
            args = [map_type(arg["dtype"]) for arg in fn["args"]]
            short_name = name[4:] if name.startswith('dvz_') else name
            wrappers.append(f"  '{short_name}': Module.cwrap('{name}', {json.dumps(ret)}, {json.dumps(args)}),")

    output = dedent(f"""\
        function wrapAll(Module) {{
        return {{
        {chr(10).join(wrappers)}
        }};
        }}

        if (typeof module !== 'undefined') module.exports = wrapAll;
        """)

    WRAPPERS_JS.parent.mkdir(parents=True, exist_ok=True)
    WRAPPERS_JS.write_text(output)
