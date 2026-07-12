#!/usr/bin/env python3
"""Check mechanically verifiable properties of the How-To code snippets."""

from __future__ import annotations

import ast
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
HOWTO = ROOT / "docs" / "how-to"
FENCE_RE = re.compile(r"^```([^\n]*)\n(.*?)^```", re.MULTILINE | re.DOTALL)


def _public_identifiers() -> tuple[set[str], set[str], set[str]]:
    text = "\n".join(path.read_text(errors="replace") for path in (ROOT / "include").rglob("*.h"))
    functions = set(re.findall(r"\b(dvz_[A-Za-z0-9_]+)\s*\(", text))
    types = set(re.findall(r"\b(Dvz[A-Za-z0-9_]+)\b", text))
    constants = set(re.findall(r"\b(DVZ_[A-Z0-9_]+)\b", text))
    return functions, types, constants


def main() -> int:
    functions, types, constants = _public_identifiers()
    errors: list[str] = []
    counts = {"python": 0, "c": 0}

    for path in sorted(HOWTO.glob("*.md")):
        text = path.read_text()
        for match in FENCE_RE.finditer(text):
            language = match.group(1).strip()
            code = match.group(2)
            line = text.count("\n", 0, match.start(2)) + 1
            if language == "python":
                counts["python"] += 1
                try:
                    ast.parse(code, filename=f"{path}:{line}")
                except SyntaxError as exc:
                    errors.append(f"{path.relative_to(ROOT)}:{line + (exc.lineno or 1) - 1}: {exc.msg}")
            elif language == "c":
                counts["c"] += 1
                checks = (
                    (r"\b(dvz_[A-Za-z0-9_]+)\s*\(", functions, "function"),
                    (r"\b(Dvz[A-Za-z0-9_]+)\b", types, "type"),
                    (r"\b(DVZ_[A-Z0-9_]+)\b", constants, "constant"),
                )
                for pattern, public, kind in checks:
                    for token in sorted(set(re.findall(pattern, code)) - public):
                        errors.append(
                            f"{path.relative_to(ROOT)}:{line}: non-public or unknown C {kind} {token}"
                        )

    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    print(f"How-To snippets: {counts['python']} Python and {counts['c']} C blocks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
