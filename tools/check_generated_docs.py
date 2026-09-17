#!/usr/bin/env python3
"""Check that committed example documentation matches its generators."""

from __future__ import annotations

import subprocess
import sys
import tempfile
from pathlib import Path

import build_gallery


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
        ok = True
        for dirname in build_gallery.GENERATED_DETAIL_DIRS:
            committed_files = _generated_files(COMMITTED_DIR / dirname)
            generated_files = {
                path.relative_to(dirname) for path in expected if path.parts[0] == dirname
            }
            if committed_files != generated_files:
                for path in sorted(generated_files - committed_files):
                    print(f"missing generated documentation: docs/examples/{dirname}/{path}")
                for path in sorted(committed_files - generated_files):
                    print(f"stale generated documentation: docs/examples/{dirname}/{path}")
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
