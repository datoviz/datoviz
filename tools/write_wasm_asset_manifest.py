#!/usr/bin/env python3
"""Write deterministic metadata for the browser WASM runtime payloads."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_WASM_DIR = ROOT / "build-wasm-scene" / "wasm"
OUTPUT_NAME = "datoviz_wasm_scene.assets.json"
ARTIFACT_NAMES = (
    "datoviz_wasm_scene.mjs",
    "datoviz_wasm_scene.wasm",
    "datoviz_wasm_scene.data",
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def build_manifest(wasm_dir: Path) -> dict:
    artifacts = {}
    version_digest = hashlib.sha256()
    for name in ARTIFACT_NAMES:
        path = wasm_dir / name
        if not path.is_file():
            raise FileNotFoundError(f"missing WASM runtime artifact: {path}")
        artifact_hash = sha256(path)
        artifact = {"bytes": path.stat().st_size, "sha256": artifact_hash}
        artifacts[name] = artifact
        version_digest.update(name.encode("utf8"))
        version_digest.update(b"\0")
        version_digest.update(artifact_hash.encode("ascii"))
        version_digest.update(b"\0")
    return {
        "schema": "datoviz.wasm-assets.v1",
        "version": f"sha256-{version_digest.hexdigest()}",
        "artifacts": artifacts,
    }


def encoded_manifest(manifest: dict) -> str:
    return json.dumps(manifest, indent=2, sort_keys=True) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--wasm-dir", type=Path, default=DEFAULT_WASM_DIR)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    output = args.wasm_dir / OUTPUT_NAME
    expected = encoded_manifest(build_manifest(args.wasm_dir))
    if args.check:
        actual = output.read_text(encoding="utf8") if output.is_file() else ""
        if actual != expected:
            print(f"stale WASM asset manifest: {output}")
            return 1
        print(f"WASM asset manifest current: {output}")
        return 0

    if output.is_file() and output.read_text(encoding="utf8") == expected:
        print(f"WASM asset manifest current: {output}")
        return 0
    output.write_text(expected, encoding="utf8")
    print(f"WASM asset manifest updated: {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
