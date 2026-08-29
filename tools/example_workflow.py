#!/usr/bin/env python3
"""Validate or promote one public gallery example through the canonical workflow."""

from __future__ import annotations

import argparse
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import yaml


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "examples/c/MANIFEST.yaml"
MOTION_VALIDATION = {"animation", "interaction", "streaming", "continuous"}
WEBGPU_STATUSES = {
    "webgpu-live",
    "webgpu-planned",
    "webgpu-deferred",
    "native-only",
    "browser-only",
}


@dataclass(frozen=True)
class Workflow:
    commands: tuple[tuple[str, ...], ...]
    warnings: tuple[str, ...]


def _load_entry(manifest_path: Path, example_id: str) -> dict[str, Any]:
    manifest = yaml.safe_load(manifest_path.read_text(encoding="utf8")) or {}
    entries = [entry for entry in manifest.get("examples", []) if entry.get("id") == example_id]
    if len(entries) != 1:
        raise ValueError(f"expected one public manifest entry for {example_id!r}, found {len(entries)}")
    return entries[0]


def _validation_tokens(entry: dict[str, Any]) -> set[str]:
    return {token.strip() for token in str(entry.get("validation") or "").split("+") if token.strip()}


def _preview(entry: dict[str, Any]) -> dict[str, Any]:
    media = entry.get("media") or {}
    preview = media.get("preview") or {}
    return preview if isinstance(preview, dict) else {}


def _validate_policy(entry: dict[str, Any]) -> tuple[str, ...]:
    example_id = str(entry.get("id") or "<unknown>")
    source = str(entry.get("source") or "")
    if not source.startswith("examples/c/") or not source.endswith(".c"):
        raise ValueError(f"{example_id}: public workflow requires one canonical examples/c C source")

    validation = _validation_tokens(entry)
    if "screenshot" not in validation:
        raise ValueError(f"{example_id}: every public gallery example must declare screenshot validation")

    webgpu = entry.get("webgpu") or {}
    status = str(webgpu.get("status") or "")
    if status not in WEBGPU_STATUSES:
        raise ValueError(f"{example_id}: declare an explicit supported WebGPU status")
    if status == "webgpu-live" and not str(webgpu.get("route") or ""):
        raise ValueError(f"{example_id}: webgpu-live requires webgpu.route")

    preview = _preview(entry)
    card = preview.get("card") or {}
    preferred = str(card.get("preferred") or "") if isinstance(card, dict) else ""
    if "video" in validation and preferred != "video-mp4":
        raise ValueError(f"{example_id}: video validation requires a video-mp4 preview card")

    warnings: list[str] = []
    if validation & MOTION_VALIDATION and not preview:
        warnings.append(
            f"{example_id}: interaction or motion is declared but no deterministic preview/video is configured"
        )
    if str(entry.get("portability") or "") == "portable-scenario" and status != "webgpu-live":
        warnings.append(f"{example_id}: portable scenario is explicitly classified {status}, not webgpu-live")
    return tuple(warnings)


def _media_commands(entry: dict[str, Any]) -> list[tuple[str, ...]]:
    example_id = str(entry["id"])
    preview = _preview(entry)
    if not preview:
        return []

    commands: list[tuple[str, ...]] = []
    card = preview.get("card") or {}
    preferred = str(card.get("preferred") or "") if isinstance(card, dict) else ""
    animation = (
        "python3",
        "tools/build_gallery_animations.py",
        "--id",
        example_id,
        "--force",
        "--jobs",
        "1",
        "--capture-jobs",
        "1",
    )
    if preferred == "video-mp4":
        commands.append((*animation, "--include-video-previews"))
        commands.append(
            (
                "python3",
                "tools/compare_gallery_media.py",
                "--id",
                example_id,
                "--site-video-previews",
                "--write-site-assets",
                "--force",
                "--jobs",
                "1",
                "--capture-jobs",
                "1",
            )
        )
    elif str(preview.get("kind") or "") == "animated-webp":
        commands.append(animation)
    return commands


def build_workflow(entry: dict[str, Any], mode: str, approve_data_update: bool) -> Workflow:
    if mode not in {"check", "promote"}:
        raise ValueError(f"unknown workflow mode {mode!r}")
    if mode == "promote" and not approve_data_update:
        raise ValueError("promotion writes canonical data media; rerun with --approve-data-update")

    warnings = _validate_policy(entry)
    example_id = str(entry["id"])
    commands: list[tuple[str, ...]] = [("just", "build")]

    if mode == "check":
        image_dir = f"build/example-check/{example_id}/gallery"
        webp_dir = f"build/example-check/{example_id}/webp"
        commands.extend(
            [
                (
                    "python3",
                    "tools/capture_gallery.py",
                    "--id",
                    example_id,
                    "--image-dir",
                    image_dir,
                    "--force",
                    "--jobs",
                    "1",
                ),
                (
                    "python3",
                    "tools/build_gallery_webp.py",
                    "--id",
                    example_id,
                    "--image-dir",
                    image_dir,
                    "--output-dir",
                    webp_dir,
                    "--strict",
                    "--force",
                ),
            ]
        )
    else:
        commands.extend(
            [
                (
                    "python3",
                    "tools/capture_gallery.py",
                    "--id",
                    example_id,
                    "--force",
                    "--jobs",
                    "1",
                ),
                (
                    "python3",
                    "tools/capture_gallery.py",
                    "--id",
                    example_id,
                    "--cache",
                    "--verify-existing",
                    "--jobs",
                    "1",
                ),
                ("python3", "tools/check_gallery_media.py", "--id", example_id),
                (
                    "python3",
                    "tools/build_gallery_webp.py",
                    "--id",
                    example_id,
                    "--strict",
                    "--force",
                ),
            ]
        )

    commands.extend(_media_commands(entry))
    webgpu = entry.get("webgpu") or {}
    if webgpu.get("status") == "webgpu-live":
        commands.extend(
            [
                ("just", "wasm-scene-smoke"),
                ("just", "webgpu-browser-smoke", f"--route={example_id}"),
                ("python3", "tools/check_wasm_bridge_metadata.py"),
            ]
        )

    if mode == "promote":
        commands.extend([("just", "docs-generate"), ("just", "docs-assets")])
    else:
        docs_dir = f"build/example-check/{example_id}/docs"
        commands.extend(
            [
                ("python3", "tools/build_gallery.py", "--docs-dir", docs_dir),
                (
                    "python3",
                    "tools/build_examples_manifest.py",
                    "--output",
                    f"{docs_dir}/examples.json",
                ),
                (
                    "python3",
                    "tools/build_capabilities.py",
                    "--output",
                    f"{docs_dir}/capabilities.json",
                ),
            ]
        )
    if mode == "check":
        commands.append(
            (
                "python3",
                "tools/check_example_manifests.py",
                "--examples",
                f"{docs_dir}/examples.json",
                "--capabilities",
                f"{docs_dir}/capabilities.json",
                "--gallery-media",
                image_dir,
            )
        )
    else:
        commands.append(("python3", "tools/check_example_manifests.py"))
    commands.extend([("just", "spec-check"), ("git", "diff", "--check")])
    if mode == "promote":
        commands.insert(-2, ("python3", "tools/check_generated_docs.py"))
    return Workflow(commands=tuple(commands), warnings=warnings)


def _run(workflow: Workflow, dry_run: bool) -> None:
    for warning in workflow.warnings:
        print(f"WARNING: {warning}", file=sys.stderr)
    for command in workflow.commands:
        print("+", " ".join(command), flush=True)
        if not dry_run:
            subprocess.run(command, cwd=ROOT, check=True)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("check", "promote"))
    parser.add_argument("id", help="public example id from examples/c/MANIFEST.yaml")
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--approve-data-update", action="store_true")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args(argv)

    try:
        entry = _load_entry(args.manifest, args.id)
        workflow = build_workflow(entry, args.mode, args.approve_data_update)
        _run(workflow, args.dry_run)
    except (OSError, ValueError, subprocess.CalledProcessError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
