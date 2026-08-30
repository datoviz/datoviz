#!/usr/bin/env python3
"""Validate and stage manifest-declared WebGPU data bundles for the static site."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
from pathlib import Path, PurePosixPath

import yaml


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "examples/c/MANIFEST.yaml"
DEFAULT_OUTPUT = ROOT / "build/webgpu-data"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--check", action="store_true", help="validate without copying bundles")
    parser.add_argument(
        "--include-local",
        action="store_true",
        help="also stage cache-local bundles that must never enter the published site",
    )
    return parser.parse_args()


def safe_relative_path(value: object, label: str) -> PurePosixPath:
    path = PurePosixPath(str(value))
    if path.is_absolute() or not path.parts or any(part in {"", ".", ".."} for part in path.parts):
        raise ValueError(f"{label} is not a safe relative path: {value!r}")
    return path


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def bundle_rows(manifest_path: Path) -> list[tuple[str, dict]]:
    manifest = yaml.safe_load(manifest_path.read_text(encoding="utf8")) or {}
    rows = []
    for entry in manifest.get("examples", []):
        webgpu = entry.get("webgpu") or {}
        if webgpu.get("status") != "webgpu-live":
            continue
        for descriptor in webgpu.get("data_bundles") or []:
            rows.append((str(entry["id"]), descriptor))
    return rows


def local_bundle_rows(manifest_path: Path) -> list[tuple[str, dict]]:
    manifest = yaml.safe_load(manifest_path.read_text(encoding="utf8")) or {}
    rows = []
    for entry in manifest.get("examples", []):
        webgpu = entry.get("webgpu") or {}
        for descriptor in webgpu.get("local_data_bundles") or []:
            rows.append((str(entry["id"]), descriptor))
    return rows


def validate_bundle(example_id: str, descriptor: dict) -> tuple[Path, dict, int]:
    bundle_id = str(descriptor.get("id") or "")
    if not bundle_id:
        raise ValueError(f"{example_id}: data bundle id is required")
    source = descriptor.get("source") or f"data/examples/{bundle_id}/manifest.json"
    source_rel = safe_relative_path(source, f"{example_id}/{bundle_id} source")
    source_path = ROOT.joinpath(*source_rel.parts)
    if not source_path.is_file():
        raise ValueError(f"{example_id}/{bundle_id}: missing committed source manifest {source_rel}")
    payload = json.loads(source_path.read_text(encoding="utf8"))
    if payload.get("schema") != "datoviz.example-data.v1":
        raise ValueError(f"{example_id}/{bundle_id}: unsupported data manifest schema")
    if payload.get("status") != "committed":
        raise ValueError(f"{example_id}/{bundle_id}: data manifest is not committed")
    web = payload.get("web") or {}
    version = str(web.get("version") or "")
    if not version:
        raise ValueError(f"{example_id}/{bundle_id}: data manifest has no web.version")
    if str(descriptor.get("virtual_root") or "") != str(web.get("virtual_root") or ""):
        raise ValueError(f"{example_id}/{bundle_id}: virtual_root differs from data manifest")
    expected_suffix = f"webgpu-data/examples/{bundle_id}/{version}/manifest.json"
    if not str(descriptor.get("url") or "").endswith(expected_suffix):
        raise ValueError(f"{example_id}/{bundle_id}: URL must end with {expected_suffix}")
    max_bytes = web.get("max_bytes")
    if not isinstance(max_bytes, int) or max_bytes < 0:
        raise ValueError(f"{example_id}/{bundle_id}: invalid web.max_bytes")

    seen: set[PurePosixPath] = set()
    total = 0
    root = source_path.parent
    for artifact in payload.get("artifacts") or []:
        rel = safe_relative_path(artifact.get("path"), f"{example_id}/{bundle_id} artifact")
        if rel in seen:
            raise ValueError(f"{example_id}/{bundle_id}: duplicate artifact {rel}")
        seen.add(rel)
        path = root.joinpath(*rel.parts)
        if not path.is_file():
            raise ValueError(f"{example_id}/{bundle_id}: missing artifact {rel}")
        expected_bytes = artifact.get("byte_size", artifact.get("bytes"))
        if not isinstance(expected_bytes, int) or path.stat().st_size != expected_bytes:
            raise ValueError(f"{example_id}/{bundle_id}: byte-size mismatch for {rel}")
        expected_hash = str(artifact.get("sha256") or "")
        if expected_hash and sha256(path) != expected_hash.lower():
            raise ValueError(f"{example_id}/{bundle_id}: SHA-256 mismatch for {rel}")
        total += path.stat().st_size
    if not seen:
        raise ValueError(f"{example_id}/{bundle_id}: bundle has no artifacts")
    if total > max_bytes:
        raise ValueError(f"{example_id}/{bundle_id}: bundle exceeds web.max_bytes")
    return source_path, payload, total


def stage_local_bundle(
    example_id: str, descriptor: dict, output_dir: Path, check: bool = False
) -> tuple[int, int]:
    bundle_id = str(descriptor.get("id") or "")
    if not bundle_id:
        raise ValueError(f"{example_id}: local data bundle id is required")
    metadata_rel = safe_relative_path(
        descriptor.get("metadata_source"), f"{example_id}/{bundle_id} metadata_source"
    )
    artifact_rel = safe_relative_path(
        descriptor.get("artifact_source"), f"{example_id}/{bundle_id} artifact_source"
    )
    output_rel = safe_relative_path(
        descriptor.get("artifact_path"), f"{example_id}/{bundle_id} artifact_path"
    )
    metadata_path = ROOT.joinpath(*metadata_rel.parts)
    artifact_path = ROOT.joinpath(*artifact_rel.parts)
    if not metadata_path.is_file() or not artifact_path.is_file():
        return 0, 0

    metadata = json.loads(metadata_path.read_text(encoding="utf8"))
    artifact = metadata.get("artifact") or {}
    expected_bytes = artifact.get("bytes")
    expected_hash = str(artifact.get("sha256") or "").lower()
    if not isinstance(expected_bytes, int) or artifact_path.stat().st_size != expected_bytes:
        raise ValueError(f"{example_id}/{bundle_id}: local artifact byte-size mismatch")
    if len(expected_hash) != 64 or sha256(artifact_path) != expected_hash:
        raise ValueError(f"{example_id}/{bundle_id}: local artifact SHA-256 mismatch")
    max_bytes = descriptor.get("max_bytes")
    if not isinstance(max_bytes, int) or max_bytes < expected_bytes:
        raise ValueError(f"{example_id}/{bundle_id}: invalid local max_bytes")
    virtual_root = str(descriptor.get("virtual_root") or "")
    safe_relative_path(virtual_root, f"{example_id}/{bundle_id} virtual_root")
    version = f"sha256-{expected_hash[:16]}"
    if check:
        return 1, expected_bytes
    output = output_dir / "examples" / bundle_id / version
    target = output.joinpath(*output_rel.parts)
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(artifact_path, target)
    payload = {
        "schema": "datoviz.example-data.v1",
        "id": bundle_id,
        "status": "committed",
        "title": f"{example_id} local WebGPU bundle",
        "artifacts": [
            {
                "path": output_rel.as_posix(),
                "bytes": expected_bytes,
                "sha256": expected_hash,
            }
        ],
        "web": {
            "version": version,
            "virtual_root": virtual_root,
            "max_bytes": max_bytes,
            "required": True,
        },
        "local_only": True,
    }
    output.mkdir(parents=True, exist_ok=True)
    (output / "manifest.json").write_text(
        json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf8"
    )
    return 1, expected_bytes


def stage_bundles(
    manifest_path: Path = DEFAULT_MANIFEST,
    output_dir: Path = DEFAULT_OUTPUT,
    check: bool = False,
    include_local: bool = False,
) -> int:
    rows = bundle_rows(manifest_path)
    local_rows = local_bundle_rows(manifest_path) if include_local else []
    copied = 0
    total = 0
    if not check:
        shutil.rmtree(output_dir, ignore_errors=True)
    for example_id, descriptor in rows:
        source_path, payload, size = validate_bundle(example_id, descriptor)
        total += size
        if check:
            continue
        bundle_id = str(descriptor["id"])
        version = str(payload["web"]["version"])
        output = output_dir / "examples" / bundle_id / version
        for artifact in payload["artifacts"]:
            rel = safe_relative_path(artifact["path"], f"{example_id}/{bundle_id} artifact")
            target = output.joinpath(*rel.parts)
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source_path.parent.joinpath(*rel.parts), target)
            copied += 1
        output.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source_path, output / "manifest.json")
        for name in ("PROVENANCE.md", "BLOCKERS.md"):
            extra = source_path.parent / name
            if extra.is_file():
                shutil.copy2(extra, output / name)
    local_staged = 0
    for example_id, descriptor in local_rows:
        count, size = stage_local_bundle(example_id, descriptor, output_dir, check=check)
        local_staged += count
        copied += count
        total += size
    action = "validated" if check else "staged"
    print(
        f"WebGPU data bundles: {action} {len(rows)} public bundles, "
        f"{local_staged} local bundles, {copied} artifacts, {total} bytes"
    )
    return 0


def main() -> int:
    args = parse_args()
    return stage_bundles(args.manifest, args.output_dir, args.check, args.include_local)


if __name__ == "__main__":
    raise SystemExit(main())
