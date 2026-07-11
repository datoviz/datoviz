#!/usr/bin/env python3
"""Check that committed example documentation matches its generators."""

from __future__ import annotations

import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
COMMITTED_DIR = ROOT / "docs/examples"


def _run(*args: str) -> None:
    subprocess.run([sys.executable, *args], cwd=ROOT, check=True)


def _generated_files(root: Path) -> set[Path]:
    return {path.relative_to(root) for path in root.rglob("*") if path.is_file()}


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="datoviz-docs-") as tmp:
        generated = Path(tmp) / "examples"
        _run("tools/build_gallery.py", "--docs-dir", str(generated))
        _run("tools/build_examples_manifest.py", "--output", str(generated / "examples.json"))
        _run("tools/build_capabilities.py", "--output", str(generated / "capabilities.json"))

        expected = _generated_files(generated)
        committed_gallery = _generated_files(COMMITTED_DIR / "gallery")
        generated_gallery = {
            path.relative_to("gallery") for path in expected if path.parts[0] == "gallery"
        }
        ok = True
        if committed_gallery != generated_gallery:
            for path in sorted(generated_gallery - committed_gallery):
                print(f"missing generated documentation: docs/examples/gallery/{path}")
            for path in sorted(committed_gallery - generated_gallery):
                print(f"stale generated documentation: docs/examples/gallery/{path}")
            ok = False

        for relative in sorted(expected):
            actual = COMMITTED_DIR / relative
            wanted = generated / relative
            if not actual.exists():
                print(f"missing generated documentation: {actual.relative_to(ROOT)}")
                ok = False
            elif actual.read_bytes() != wanted.read_bytes():
                print(f"generated documentation drift: {actual.relative_to(ROOT)}")
                ok = False

    if not ok:
        print("regenerate with: just docs-generate")
        return 1
    print("generated example documentation is up to date")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
