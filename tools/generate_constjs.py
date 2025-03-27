# Parse the C headers.

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import json
from pathlib import Path


# -------------------------------------------------------------------------------------------------
# Constants
# -------------------------------------------------------------------------------------------------

ROOT_DIR = Path(__file__).parent.parent


# -------------------------------------------------------------------------------------------------
# Entry-point
# -------------------------------------------------------------------------------------------------

if __name__ == '__main__':

    with open(ROOT_DIR / "tools/headers.json") as f:
        data = json.load(f)

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

    with open(ROOT_DIR / "build/enums.js", "w") as f:
        f.write("\n".join(lines) + "\n")
