#!/usr/bin/env python3
"""
c_to_python_skeleton.py — mechanical C-to-Python skeleton generator for how-to doc snippets.

Usage:
    python3 tools/c_to_python_skeleton.py snippet.c
    python3 tools/c_to_python_skeleton.py < snippet.c

Applies rule-based substitutions to produce a Python skeleton. Non-mechanical parts
(numpy data setup, lifecycle) are marked with # TODO comments for agent completion.

This handles the raw public API pattern used in documentation (quickstart style),
NOT the internal DvzScenarioSpec/DvzScenarioContext runner pattern.
"""

import re
import sys


# ---------------------------------------------------------------------------
# Substitution rules
# ---------------------------------------------------------------------------

# C type prefixes to strip from variable declarations
_TYPE_PREFIXES = (
    r"Dvz\w+\s*\*+\s*",   # DvzFoo* var
    r"Dvz\w+\s+",          # DvzFoo var
    r"const\s+",
    r"static\s+",
    r"uint32_t\s+",
    r"uint8_t\s+",
    r"int32_t\s+",
    r"int64_t\s+",
    r"float\s+",
    r"double\s+",
    r"bool\s+",
    r"int\s+",
    r"char\s+",
    r"void\s*\*+\s*",
)

_DECL_RE = re.compile(
    r"^\s*(?:" + "|".join(_TYPE_PREFIXES) + r")"
    r"(\w+)\s*=\s*(.+)$"
)

# C array declarations that need numpy — e.g. "float pos[N * 3] = {0}"
_ARRAY_DECL_RE = re.compile(
    r"^\s*(?:const\s+)?(?:float|double|uint\w+|int\w+|DvzColor)\s+(\w+)\[.*?\]"
)

# Type cast removal: (DvzFoo*) or (const DvzFoo*)
_CAST_RE = re.compile(r"\(\s*(?:const\s+)?Dvz\w+\s*\*+\s*\)")

# Enum constant namespacing — DVZ_FOO_BAR → dvz.DVZ_FOO_BAR
# (agent must verify the actual Python enum class; this is a best-effort placeholder)
_ENUM_RE = re.compile(r"\bDVZ_[A-Z0-9_]+\b")

# dvz_foo( → dvz.dvz_foo(
_FUNC_RE = re.compile(r"\bdvz_(\w+)\s*\(")

# Lifecycle collapse marker — detect the app/window/run/destroy block
_APP_LINES = {
    "dvz_app(",
    "dvz_view_glfw(",
    "dvz_app_run(",
    "dvz_app_destroy(",
    "dvz_scene_destroy(",
}


def _is_app_lifecycle(line):
    stripped = line.strip()
    return any(stripped.startswith(k) or f" {k}" in stripped for k in _APP_LINES)


def _transform_line(line):
    stripped = line.strip()

    # Skip blank lines from stripped block comments
    if not stripped:
        return ""

    # Skip C line comments
    if stripped.startswith("//"):
        return None

    # Skip includes
    if stripped.startswith("#include"):
        return None

    # Skip preprocessor defines/ifdefs (keep #define N 1000 as a comment hint)
    if stripped.startswith("#define"):
        name_match = re.match(r"#define\s+(\w+)\s+(.*)", stripped)
        if name_match:
            return f"# {name_match.group(1)} = {name_match.group(2)}"
        return None
    if stripped.startswith("#"):
        return None

    # Skip main() signature and its closing brace
    if re.match(r"^\s*int\s+main\s*\(", line):
        return None
    if stripped == "return 0;":
        return "    return 0"

    # C array declaration → numpy TODO
    if _ARRAY_DECL_RE.match(line):
        m = _ARRAY_DECL_RE.match(line)
        return f"    # TODO: numpy — {m.group(1)} = np.zeros(..., dtype=np.float32)"

    # Struct/compound literal initializers on their own line
    if re.match(r"^\s*\{", stripped) or stripped in ("{", "}"):
        return None

    # Variable declaration with assignment
    m = _DECL_RE.match(line)
    if m:
        varname, rhs = m.group(1), m.group(2)
        rhs = _transform_rhs(rhs)
        indent = re.match(r"^(\s*)", line).group(1)
        return f"{indent}{varname} = {rhs}"

    # Plain statement (no type prefix)
    result = _transform_rhs(line)
    # Strip trailing semicolons
    result = re.sub(r";\s*$", "", result)
    return result


def _transform_rhs(text):
    # Remove type casts
    text = _CAST_RE.sub("", text)

    # NULL / true / false
    text = re.sub(r"\bNULL\b", "None", text)
    text = re.sub(r"\btrue\b", "True", text)
    text = re.sub(r"\bfalse\b", "False", text)

    # Enum constants — prefix with dvz. for now
    text = _ENUM_RE.sub(lambda m: f"dvz.{m.group(0)}", text)

    # Function calls: dvz_foo( → dvz.dvz_foo(
    text = _FUNC_RE.sub(lambda m: f"dvz.dvz_{m.group(1)}(", text)

    # Trailing semicolons
    text = re.sub(r";\s*$", "", text)

    return text


def _strip_block_comments(source: str) -> str:
    return re.sub(r"/\*.*?\*/", "", source, flags=re.DOTALL)


def convert(source: str) -> str:
    source = _strip_block_comments(source)
    lines = source.splitlines()
    out = []

    # Detect if source has a main() — add import header if so
    has_main = any(re.match(r"^\s*int\s+main\s*\(", l) for l in lines)

    if has_main:
        out.append("import numpy as np")
        out.append("import datoviz as dvz")
        out.append("")

    # Track lifecycle block
    lifecycle_emitted = False
    in_lifecycle = False
    lifecycle_figure = "figure"  # best-guess figure variable name

    i = 0
    while i < len(lines):
        line = lines[i]

        if _is_app_lifecycle(line):
            if not lifecycle_emitted:
                # Try to find the figure variable from a dvz_view_glfw call
                for ll in lines[i:i+5]:
                    m = re.search(r"dvz_view_glfw\(\s*\w+\s*,\s*(\w+)", ll)
                    if m:
                        lifecycle_figure = m.group(1)
                        break
                indent = re.match(r"^(\s*)", line).group(1)
                out.append(f'{indent}dvz.run(scene, {lifecycle_figure}, title="TODO: title")')
                lifecycle_emitted = True
            i += 1
            continue

        result = _transform_line(line)
        if result is not None:
            out.append(result)
        i += 1

    # Collapse runs of blank lines to at most one
    collapsed = []
    prev_blank = False
    for line in out:
        is_blank = line.strip() == ""
        if is_blank and prev_blank:
            continue
        collapsed.append(line)
        prev_blank = is_blank

    return "\n".join(collapsed)


def main():
    if len(sys.argv) > 1:
        with open(sys.argv[1]) as f:
            source = f.read()
    else:
        source = sys.stdin.read()

    print(convert(source))


if __name__ == "__main__":
    main()
