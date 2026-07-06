#!/usr/bin/env python3
"""Build the WebGPU/WASM scene module only when the local output is stale."""

from __future__ import annotations

import hashlib
import json
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "build-wasm-scene/wasm/datoviz_wasm_scene.mjs"
STAMP = ROOT / "build-wasm-scene/wasm/.datoviz_wasm_scene_build.json"
INPUT_ROOTS = (
    ROOT / "CMakeLists.txt",
    ROOT / "cmake",
    ROOT / "include/datoviz",
    ROOT / "src",
    ROOT / "shaders",
    ROOT / "examples/c",
    ROOT / "tasks/wasm_build.sh",
    ROOT / "justfiles/webgpu_wasm.just",
)
INPUT_SUFFIXES = {
    ".c",
    ".h",
    ".glsl",
    ".vert",
    ".frag",
    ".comp",
    ".cmake",
    ".txt",
    ".yaml",
    ".yml",
    ".sh",
    ".just",
}
IGNORED_PARTS = {
    "__pycache__",
    ".cache",
    ".git",
    "build",
    "build-wasm-scene",
    "site",
}


def iter_inputs() -> list[Path]:
    files: list[Path] = []
    for root in INPUT_ROOTS:
        if root.is_file():
            files.append(root)
            continue
        if not root.is_dir():
            continue
        for path in root.rglob("*"):
            if any(part in IGNORED_PARTS for part in path.parts):
                continue
            if path.is_file() and path.suffix in INPUT_SUFFIXES:
                files.append(path)
    return sorted({path.resolve() for path in files})


def input_hash(files: list[Path]) -> str:
    digest = hashlib.sha256()
    for path in files:
        try:
            rel = path.relative_to(ROOT)
            data = path.read_bytes()
        except OSError:
            continue
        digest.update(str(rel).encode("utf8"))
        digest.update(b"\0")
        digest.update(data)
        digest.update(b"\0")
    return digest.hexdigest()


def load_stamp() -> dict:
    try:
        with STAMP.open("r", encoding="utf8") as f:
            payload = json.load(f)
    except (OSError, json.JSONDecodeError):
        return {}
    return payload if isinstance(payload, dict) else {}


def write_stamp(digest: str) -> None:
    STAMP.parent.mkdir(parents=True, exist_ok=True)
    STAMP.write_text(json.dumps({"input_hash": digest}, indent=2) + "\n", encoding="utf8")


def main() -> int:
    files = iter_inputs()
    digest = input_hash(files)
    if OUTPUT.exists() and load_stamp().get("input_hash") == digest:
        print("wasm scene build: current")
        return 0

    reason = "missing" if not OUTPUT.exists() else "stale"
    print(f"wasm scene build: {reason}; running `just wasm-scene-build`")
    result = subprocess.run(["just", "wasm-scene-build"], cwd=ROOT, check=False)
    if result.returncode != 0:
        return result.returncode
    if not OUTPUT.exists():
        print(f"wasm scene build: expected output missing: {OUTPUT}", file=sys.stderr)
        return 1
    write_stamp(digest)
    print("wasm scene build: updated")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
