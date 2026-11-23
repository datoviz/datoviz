#!/usr/bin/env python3
"""Verify the actual incremental WASM build graph and record its input/output provenance."""

from __future__ import annotations

import hashlib
import json
import re
import shlex
import subprocess
import sys
from pathlib import Path

from write_wasm_asset_manifest import build_manifest


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "build-wasm-scene/wasm/datoviz_wasm_scene.mjs"
STAMP = ROOT / "build-wasm-scene/wasm/.datoviz_wasm_scene_build.json"
INPUT_ROOTS = (
    ROOT / "CMakeLists.txt",
    ROOT / "cmake",
    ROOT / "include/datoviz",
    ROOT / "src",
    ROOT / "shaders",
    ROOT / "tasks/wasm_build.sh",
    ROOT / "justfiles/webgpu_wasm.just",
    ROOT / "tools/ensure_wasm_scene_build.py",
    ROOT / "tools/example_profiles.py",
    ROOT / "examples/c/MANIFEST.yaml",
)
WASM_EXAMPLE_INPUT_ROOTS = (
    ROOT / "examples/c/composites",
    ROOT / "examples/c/features",
    ROOT / "examples/c/runner",
    ROOT / "examples/c/showcases",
    ROOT / "examples/c/start",
    ROOT / "examples/c/visuals",
)
WASM_EXAMPLE_SHARED_INPUTS = (
    ROOT / "examples/c/example_common.c",
    ROOT / "examples/c/example_common.h",
    ROOT / "examples/c/example_controller_preview.c",
    ROOT / "examples/c/example_controller_preview.h",
    ROOT / "examples/c/example_gui_controls.h",
    ROOT / "examples/c/example_random.c",
    ROOT / "examples/c/example_random.h",
    ROOT / "examples/c/example_style.c",
    ROOT / "examples/c/example_style.h",
    ROOT / "examples/c/example_tuner.h",
)
INPUT_SUFFIXES = {
    ".c",
    ".h",
    ".glsl",
    ".wgsl",
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
    for root in (*INPUT_ROOTS, *WASM_EXAMPLE_INPUT_ROOTS, *WASM_EXAMPLE_SHARED_INPUTS):
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
    for path in sorted(files):
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


def resolved_inputs(build_dir: Path) -> list[Path]:
    """Read actual resource selection and compiler header dependencies from the build tree."""
    paths = set()
    resources = build_dir / "wasm/resolved-inputs.txt"
    if resources.exists():
        paths.update(Path(line) for line in resources.read_text().splitlines() if line)
    for depfile in build_dir.rglob("*.o.d"):
        text = depfile.read_text().replace("\\\n", " ")
        _, separator, dependencies = text.partition(": ")
        if separator:
            paths.update(Path(name) if Path(name).is_absolute() else build_dir / name
                         for name in shlex.split(dependencies))
    return sorted({path.resolve() for path in paths if path.is_file()})


def provenance(build_dir: Path) -> dict:
    """Describe checked build inputs without publishing host paths or command lines."""
    files = sorted(set(iter_inputs()) | set(resolved_inputs(build_dir)))
    inputs = []
    for index, path in enumerate(files):
        label = str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else f"resolved-external/{index}"
        inputs.append({"input": label, "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})
    compiler = subprocess.run(["emcc", "--version"], capture_output=True, text=True, check=True)
    shader_compiler = subprocess.run(
        ["cmake", "-LA", "-N", str(build_dir)], capture_output=True, text=True, check=True)
    # Hash effective commands: they contain private paths, so do not expose their text.
    commands = build_dir / "compile_commands.json"
    flags = commands.read_bytes() if commands.exists() else b""
    payload = {"inputs": inputs, "compiler": compiler.stdout.splitlines()[0],
               "compile_commands_sha256": hashlib.sha256(flags).hexdigest(),
               "cmake_configuration_sha256": hashlib.sha256(shader_compiler.stdout.encode()).hexdigest()}
    compiler_entry = re.search(r"^DVZ_GLSLC_EXECUTABLE:[^=]+=(.+)$", shader_compiler.stdout, re.M)
    if compiler_entry:
        version = subprocess.run([compiler_entry[1], "--version"], capture_output=True,
                                 text=True, check=True)
        payload["shader_compiler_sha256"] = hashlib.sha256(version.stdout.encode()).hexdigest()
    payload["input_hash"] = hashlib.sha256(json.dumps(payload, sort_keys=True).encode()).hexdigest()
    return payload


def write_stamp(digest: str, details: dict | None = None) -> None:
    STAMP.parent.mkdir(parents=True, exist_ok=True)
    STAMP.write_text(json.dumps({"input_hash": digest, **(details or {})}, indent=2) + "\n", encoding="utf8")


def outputs_match(wasm_dir: Path, stamp: dict) -> bool:
    """Validate the complete runtime payload set against the last successful build receipt."""
    try:
        return build_manifest(wasm_dir)["artifacts"] == stamp.get("outputs")
    except (OSError, ValueError):
        return False


def main() -> int:
    # Let the real build dependency graph decide freshness, including resolved resources.
    # A source-tree scanner cannot safely skip compiler/linker dependency checks.
    subprocess.run([sys.executable, "tools/example_profiles.py", "--check"], cwd=ROOT, check=True)
    previous = load_stamp()
    compiler_version = subprocess.run(
        ["emcc", "--version"], capture_output=True, text=True, check=True).stdout.splitlines()[0]
    if previous.get("compiler") and previous["compiler"] != compiler_version:
        # A compiler replaced in place need not change CMake's command strings.
        subprocess.run(["cmake", "--build", str(OUTPUT.parent.parent), "--target", "clean"],
                       cwd=ROOT, check=True)
    if OUTPUT.exists() and not outputs_match(OUTPUT.parent, previous):
        # .wasm/.data are linker side products: a missing or altered one must force a relink.
        OUTPUT.unlink()
    print("wasm scene build: checking incremental compiler/linker dependencies")
    result = subprocess.run(["just", "wasm-scene-build"], cwd=ROOT, check=False)
    if result.returncode != 0:
        return result.returncode
    if not OUTPUT.exists():
        print(f"wasm scene build: expected output missing: {OUTPUT}", file=sys.stderr)
        return 1
    details = provenance(OUTPUT.parent.parent)
    outputs = build_manifest(OUTPUT.parent)
    details["outputs"] = outputs["artifacts"]
    details["source_commit"] = subprocess.run(
        ["git", "rev-parse", "HEAD"], cwd=ROOT, capture_output=True, text=True, check=True).stdout.strip()
    digest = details["input_hash"]
    write_stamp(digest, details)
    public = {key: details[key] for key in ("input_hash", "compiler", "source_commit")}
    public["outputs"] = details["outputs"]
    public["resolved_input_count"] = len(details["inputs"])
    public["working_tree_inputs"] = "content-hashed; source_commit alone is not freshness proof"
    (OUTPUT.parent / "datoviz_wasm_scene.provenance.json").write_text(
        json.dumps(public, indent=2, sort_keys=True) + "\n")
    subprocess.run([sys.executable, "tools/write_wasm_asset_manifest.py"], cwd=ROOT, check=True)
    print("wasm scene build: updated")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
