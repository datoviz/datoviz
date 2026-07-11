#!/usr/bin/env python3
"""
Smoke-test fenced code blocks extracted from documentation markdown files.

Python blocks: dvz.run(...) is patched to dvz.capture(...) so the snippet
runs headlessly without opening a window.

C blocks: dvz_view_glfw(...) is patched to dvz_view_offscreen(...) and
dvz_app_run(app, 0) is patched to a single-frame offscreen run + PNG capture,
then the snippet is compiled against the local build and executed.

Usage:
    python3 tools/doctest.py docs/index.md
    python3 tools/doctest.py --lang python docs/index.md

Quickstart snippets are included from executable fixtures and validated by
``just quickstart-check`` instead.
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build"
INCLUDE = ROOT / "include"

# ---------------------------------------------------------------------------
# Extraction
# ---------------------------------------------------------------------------

_FENCE_RE = re.compile(
    r"^(?P<indent>[ \t]*)```(?P<lang>python|c)\s*\n(?P<body>.*?)(?P=indent)```",
    re.MULTILINE | re.DOTALL,
)


def extract_blocks(path: Path) -> list[tuple[str, str]]:
    """Return [(lang, code), ...] for every Python/C fenced block in path."""
    text = path.read_text()
    results = []
    for m in _FENCE_RE.finditer(text):
        indent = m.group("indent")
        body = m.group("body")
        if indent:
            # Strip the common indentation added by MkDocs tab syntax
            body = re.sub(r"^" + re.escape(indent), "", body, flags=re.MULTILINE)
        results.append((m.group("lang"), body))
    return results


# ---------------------------------------------------------------------------
# Python patching
# ---------------------------------------------------------------------------

# dvz.run(scene, figure, ...) → dvz.capture(scene, figure, path=_OUT)
_PY_RUN_RE = re.compile(r"\bdvz\.run\s*\(([^)]+)\)")


def _patch_python(code: str, out: str) -> str | None:
    """Patch dvz.run(...) to dvz.capture(...). Return None if no run call."""
    if "dvz.run(" not in code:
        return None

    def _replace(m: re.Match) -> str:
        # Keep first two positional args (scene, figure), drop the rest.
        args = [a.strip() for a in m.group(1).split(",")]
        # Strip keyword args like title=...
        positional = [a for a in args if "=" not in a][:2]
        return f'dvz.capture({", ".join(positional)}, path={out!r})'

    return _PY_RUN_RE.sub(_replace, code)


# ---------------------------------------------------------------------------
# C patching
# ---------------------------------------------------------------------------

# dvz_view_glfw(app, figure, W, H, "title") → DvzView* _dv = dvz_view_offscreen(app, figure, W, H)
_C_GLFW_RE = re.compile(
    r'dvz_view_glfw\s*\(\s*(\w+)\s*,\s*(\w+)\s*,\s*(\w+)\s*,\s*(\w+)\s*,[^)]+\)'
)
# dvz_app_run(app, 0) → dvz_app_run(app, 1); dvz_view_capture_png(_dv, PATH)
_C_RUN_RE = re.compile(r"dvz_app_run\s*\(\s*(\w+)\s*,\s*0\s*\)")


def _patch_c(code: str, out: str) -> str | None:
    """Patch the glfw/run lines for headless execution. Return None if not applicable."""
    if "dvz_view_glfw(" not in code:
        return None

    code = _C_GLFW_RE.sub(
        lambda m: (
            f"DvzView* _dv = dvz_view_offscreen("
            f"{m.group(1)}, {m.group(2)}, {m.group(3)}, {m.group(4)})"
        ),
        code,
    )
    code = _C_RUN_RE.sub(
        lambda m: (
            f"dvz_app_run({m.group(1)}, 1);\n"
            f"    dvz_view_capture_png(_dv, {out!r})"
        ),
        code,
    )
    return code


# ---------------------------------------------------------------------------
# Runners
# ---------------------------------------------------------------------------


def run_python(code: str, label: str) -> bool:
    with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as f:
        out_png = f.name
    try:
        patched = _patch_python(code, out_png)
        if patched is None:
            if "doctest: skip" in code:
                print(f"  [skip] {label}: explicitly marked")
                return True
            print(f"  [FAIL] {label}: no dvz.run() call; add 'doctest: skip' if intentional")
            return False
        with tempfile.NamedTemporaryFile(
            suffix=".py", mode="w", delete=False
        ) as f:
            f.write(patched)
            tmp_py = f.name
        try:
            result = subprocess.run(
                [sys.executable, tmp_py],
                capture_output=True,
                text=True,
                cwd=ROOT,
            )
            if result.returncode != 0:
                print(f"  [FAIL] {label}")
                print(result.stderr[-2000:])
                return False
            print(f"  [ok]   {label}")
            return True
        finally:
            os.unlink(tmp_py)
    finally:
        if os.path.exists(out_png):
            os.unlink(out_png)


def run_c(code: str, label: str) -> bool:
    with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as f:
        out_png = f.name
    try:
        patched = _patch_c(code, out_png)
        if patched is None:
            if "doctest: skip" in code:
                print(f"  [skip] {label}: explicitly marked")
                return True
            print(
                f"  [FAIL] {label}: no dvz_view_glfw() call; "
                "add 'doctest: skip' if intentional"
            )
            return False
        with tempfile.NamedTemporaryFile(
            suffix=".c", mode="w", delete=False
        ) as f:
            f.write(patched)
            tmp_c = f.name
        exe = tmp_c.replace(".c", "")
        try:
            compile_result = subprocess.run(
                [
                    "gcc", tmp_c, "-o", exe,
                    f"-I{INCLUDE}",
                    f"-L{BUILD}",
                    f"-Wl,-rpath,{BUILD}",
                    "-lm", "-ldatoviz",
                ],
                capture_output=True,
                text=True,
            )
            if compile_result.returncode != 0:
                print(f"  [FAIL] {label} (compile)")
                print(compile_result.stderr[-2000:])
                return False
            run_result = subprocess.run(
                [exe],
                capture_output=True,
                text=True,
                cwd=ROOT,
            )
            if run_result.returncode != 0:
                print(f"  [FAIL] {label} (run)")
                print(run_result.stderr[-2000:])
                return False
            print(f"  [ok]   {label}")
            return True
        finally:
            os.unlink(tmp_c)
            if os.path.exists(exe):
                os.unlink(exe)
    finally:
        if os.path.exists(out_png):
            os.unlink(out_png)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------


def main() -> int:
    parser = argparse.ArgumentParser(description="Smoke-test doc code blocks")
    parser.add_argument("files", nargs="+", type=Path)
    parser.add_argument(
        "--lang",
        choices=["python", "c", "both"],
        default="both",
        help="Which language blocks to test (default: both)",
    )
    args = parser.parse_args()

    failures = 0
    for path in args.files:
        if not path.is_absolute():
            path = ROOT / path
        print(f"\n{path.relative_to(ROOT)}")
        blocks = extract_blocks(path)
        if not blocks:
            print("  (no Python/C blocks found)")
            continue
        for i, (lang, code) in enumerate(blocks):
            label = f"block {i+1} ({lang})"
            if lang == "python" and args.lang in ("python", "both"):
                if not run_python(code, label):
                    failures += 1
            elif lang == "c" and args.lang in ("c", "both"):
                if not run_c(code, label):
                    failures += 1

    if failures:
        print(f"\n{failures} block(s) failed.")
        return 1
    print("\nAll blocks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
