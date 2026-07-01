#!/usr/bin/env python3
"""Run built C examples sequentially for manual regression checks."""

from __future__ import annotations

import argparse
import os
import platform
import re
import shutil
import subprocess
import sys
from pathlib import Path

import yaml


DEFAULT_MANIFEST = "examples/c/MANIFEST.yaml"


def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run built Datoviz C examples one after another."
    )
    parser.add_argument(
        "filter",
        nargs="?",
        default="",
        help="optional regex/substr filter matched against id, lane, title, source, or group/name",
    )
    parser.add_argument(
        "--ignore",
        action="append",
        default=[],
        help="regex/substr pattern to skip; may be repeated or comma-separated",
    )
    parser.add_argument(
        "--lane",
        action="append",
        default=[],
        help="manifest lane to run; may be repeated or comma-separated",
    )
    parser.add_argument(
        "--stage",
        action="append",
        default=[],
        help="manifest stage to run; may be repeated or comma-separated",
    )
    parser.add_argument(
        "--batch",
        default="",
        help=(
            "manifest review batch name, or number alias such as 3 for review-03-*; "
            "cannot be combined with --lane, --stage, or --all-built"
        ),
    )
    parser.add_argument(
        "--include-lab",
        action="store_true",
        help="include manifest lab entries in the default public-example selection",
    )
    parser.add_argument(
        "--manifest",
        default=DEFAULT_MANIFEST,
        help=f"C example manifest path, default: {DEFAULT_MANIFEST}",
    )
    parser.add_argument(
        "--all-built",
        action="store_true",
        help="ignore the manifest and scan all built C example executables",
    )
    parser.add_argument(
        "--strict",
        action="store_true",
        help="fail when a matching manifest entry has no built executable",
    )
    parser.add_argument(
        "--frames",
        default="",
        help="pass a frame-count argument to each example for bounded runs",
    )
    parser.add_argument(
        "--example-arg",
        action="append",
        default=[],
        help="extra argument passed to each example; may be repeated",
    )
    parser.add_argument(
        "--list",
        action="store_true",
        help="print matching examples without running them",
    )
    parser.add_argument(
        "--code",
        action="store_true",
        help="open matching example source files in VS Code instead of running them",
    )
    parser.add_argument(
        "--code-command",
        default="code",
        help=(
            "VS Code command to run for --code, default: code "
            "(on macOS, the bundled Visual Studio Code.app CLI is used when code is not on PATH)"
        ),
    )
    parser.add_argument(
        "--build-dir",
        default="build",
        help="CMake build directory, default: build",
    )
    args = parser.parse_args()
    if args.batch and args.all_built:
        parser.error("--batch cannot be combined with --all-built")
    if args.batch and args.lane:
        parser.error("--batch cannot be combined with --lane; batch selects explicit manifest IDs")
    if args.batch and args.stage:
        parser.error("--batch cannot be combined with --stage; batch selects explicit manifest IDs")
    return args


def is_executable(path: Path) -> bool:
    return path.is_file() and os.access(path, os.X_OK)


def matches_filter(text: str, filter_text: str) -> bool:
    if filter_text in text:
        return True
    try:
        return re.search(filter_text, text) is not None
    except re.error:
        return False


def split_patterns(patterns: list[str]) -> list[str]:
    return [
        part.strip()
        for pattern in patterns
        for part in pattern.split(",")
        if part.strip()
    ]


def load_manifest(path: Path) -> dict:
    if not path.exists():
        raise FileNotFoundError(f"C example manifest not found: {path}")
    with path.open("r", encoding="utf8") as f:
        return yaml.safe_load(f) or {}


def manifest_public_folders(manifest: dict) -> set[str]:
    folders = manifest.get("folders", {})
    public = folders.get("public", [])
    return {str(folder).rstrip("/") for folder in public}


def manifest_entry_rel(root: Path, entry: dict) -> str:
    source = entry.get("source")
    if not source:
        raise ValueError(f"manifest entry {entry.get('id', '<missing-id>')} has no source")

    source_path = Path(str(source))
    if source_path.is_absolute():
        try:
            source_path = source_path.relative_to(root)
        except ValueError as exc:
            raise ValueError(f"manifest source is outside the repo: {source}") from exc

    try:
        rel = source_path.relative_to("examples/c")
    except ValueError as exc:
        raise ValueError(f"manifest source is not under examples/c: {source}") from exc

    if rel.suffix != ".c":
        raise ValueError(f"manifest source is not a C file: {source}")
    return rel.with_suffix("").as_posix()


def manifest_entry_text(entry: dict, rel: str) -> str:
    fields = [
        str(entry.get("id", "")),
        str(entry.get("title", "")),
        str(entry.get("stage", "")),
        str(entry.get("lane", "")),
        str(entry.get("source", "")),
        rel,
    ]
    return " ".join(fields)


def manifest_entries_by_id(manifest: dict) -> dict[str, dict]:
    entries: dict[str, dict] = {}
    for entry in manifest.get("examples", []):
        example_id = str(entry.get("id", ""))
        if example_id:
            entries[example_id] = entry
    return entries


def resolve_batch_name(manifest: dict, batch_name: str) -> str:
    batches = manifest.get("batches") or {}
    if batch_name in batches:
        return batch_name

    available = sorted(str(name) for name in batches)
    if batch_name.isdigit():
        prefix = f"review-{int(batch_name):02d}-"
        matches = [name for name in available if name.startswith(prefix)]
        if len(matches) == 1:
            return matches[0]
        if len(matches) > 1:
            raise ValueError(
                f"ambiguous C example review batch number {batch_name!r}; "
                f"matches: {', '.join(matches)}"
            )

    lines = [f"unknown C example review batch: {batch_name}"]
    if available:
        lines.append("available batches:")
        lines.extend(f"  - {name}" for name in available)
    else:
        lines.append("no review batches are defined in the manifest")
    raise ValueError("\n".join(lines))


def source_is_public(source: str, public_folders: set[str]) -> bool:
    normalized = source.rstrip("/")
    return any(normalized.startswith(f"{folder}/") for folder in public_folders)


def manifest_batch_examples(
    root: Path,
    examples_root: Path,
    manifest: dict,
    batch_name: str,
    args: argparse.Namespace,
    ignore_patterns: list[str],
) -> tuple[list[tuple[str, Path]], list[str], list[str]]:
    resolved_name = resolve_batch_name(manifest, batch_name)
    args.resolved_batch = resolved_name
    batch_ids = [str(example_id) for example_id in manifest["batches"][resolved_name]]
    entries = manifest_entries_by_id(manifest)
    unknown_ids = [example_id for example_id in batch_ids if example_id not in entries]
    if unknown_ids:
        raise ValueError(
            f"C example review batch {resolved_name!r} contains unknown example IDs: "
            + ", ".join(unknown_ids)
        )

    examples: list[tuple[str, Path]] = []
    ignored: list[str] = []
    missing: list[str] = []
    seen: set[str] = set()

    for example_id in batch_ids:
        if example_id in seen:
            continue
        seen.add(example_id)

        entry = entries[example_id]
        rel = manifest_entry_rel(root, entry)
        text = manifest_entry_text(entry, rel)
        if any(matches_filter(text, pattern) for pattern in ignore_patterns):
            ignored.append(rel)
            continue

        exe = examples_root / rel
        if args.code or is_executable(exe):
            examples.append((rel, exe))
        else:
            missing.append(rel)

    return examples, ignored, missing


def manifest_examples(
    root: Path, examples_root: Path, args: argparse.Namespace, ignore_patterns: list[str]
) -> tuple[list[tuple[str, Path]], list[str], list[str]]:
    manifest = load_manifest(root / args.manifest)
    if args.batch:
        return manifest_batch_examples(root, examples_root, manifest, args.batch, args, ignore_patterns)

    public_folders = manifest_public_folders(manifest)
    lane_filters = set(split_patterns(args.lane))
    stage_filters = set(split_patterns(args.stage))

    examples: list[tuple[str, Path]] = []
    ignored: list[str] = []
    missing: list[str] = []
    seen: set[str] = set()

    for entry in manifest.get("examples", []):
        rel = manifest_entry_rel(root, entry)
        source = str(entry.get("source", ""))
        lane = str(entry.get("lane", ""))
        stage = str(entry.get("stage", ""))

        if lane_filters and lane not in lane_filters:
            continue
        if stage_filters and stage not in stage_filters:
            continue
        if not args.include_lab and not lane_filters and not stage_filters:
            if stage == "lab" or not source_is_public(source, public_folders):
                continue

        text = manifest_entry_text(entry, rel)
        if args.filter and not matches_filter(text, args.filter):
            continue
        if any(matches_filter(text, pattern) for pattern in ignore_patterns):
            ignored.append(rel)
            continue
        if rel in seen:
            continue
        seen.add(rel)

        exe = examples_root / rel
        if args.code or is_executable(exe):
            examples.append((rel, exe))
        else:
            missing.append(rel)

    return examples, ignored, missing


def built_examples(
    examples_root: Path, args: argparse.Namespace, ignore_patterns: list[str]
) -> tuple[list[tuple[str, Path]], list[str]]:
    examples: list[tuple[str, Path]] = []
    ignored: list[str] = []
    if not examples_root.exists():
        return examples, ignored

    for exe in sorted(examples_root.glob("*/*")):
        if not is_executable(exe):
            continue
        rel = exe.relative_to(examples_root).as_posix()
        if args.filter and not matches_filter(rel, args.filter):
            continue
        if any(matches_filter(rel, pattern) for pattern in ignore_patterns):
            ignored.append(rel)
            continue
        examples.append((rel, exe))
    return examples, ignored


def apply_runtime_env(root: Path, env: dict[str, str]) -> None:
    if platform.system() != "Darwin":
        return
    vulkan_sdk = env.get("VULKAN_SDK", "")
    candidates = []
    if vulkan_sdk:
        candidates.append(Path(vulkan_sdk) / "lib")
    candidates.append(root / "libs" / "vulkan" / "macos")
    for candidate in candidates:
        if candidate.is_dir():
            old = env.get("DYLD_FALLBACK_LIBRARY_PATH")
            env["DYLD_FALLBACK_LIBRARY_PATH"] = (
                str(candidate) if not old else f"{candidate}:{old}"
            )
            icd = candidate / "MoltenVK_icd.json"
            if icd.exists() and "VK_DRIVER_FILES" not in env:
                env["VK_DRIVER_FILES"] = str(icd)
            break


def example_source_path(root: Path, rel: str) -> Path:
    return root / "examples" / "c" / f"{rel}.c"


def resolve_code_command(code_command: str) -> str:
    if code_command != "code":
        return code_command

    command = shutil.which(code_command)
    if command:
        return command

    if platform.system() == "Darwin":
        macos_code = Path(
            "/Applications/Visual Studio Code.app/Contents/Resources/app/bin/code"
        )
        if macos_code.is_file():
            return str(macos_code)

    return code_command


def open_code(root: Path, examples: list[tuple[str, Path]], code_command: str) -> int:
    sources = [example_source_path(root, rel) for rel, _ in examples]
    missing = [path for path in sources if not path.is_file()]
    if missing:
        print("Matching examples without source files:", file=sys.stderr)
        for path in missing:
            print(f"  - {path.relative_to(root)}", file=sys.stderr)
        return 1

    resolved_command = resolve_code_command(code_command)
    try:
        result = subprocess.run(
            [resolved_command, "--reuse-window", *[str(path) for path in sources]],
            cwd=root,
            check=False,
        )
    except FileNotFoundError:
        print(
            f"VS Code command not found: {code_command!r}. "
            "Install the 'code' shell command or pass --code-command.",
            file=sys.stderr,
        )
        return 127
    return result.returncode


def main() -> int:
    args = parse_args()
    root = repo_root()
    examples_root = root / args.build_dir / "examples" / "c"

    ignore_patterns = split_patterns(args.ignore)

    if args.all_built:
        examples, ignored = built_examples(examples_root, args, ignore_patterns)
        missing: list[str] = []
    else:
        try:
            examples, ignored, missing = manifest_examples(root, examples_root, args, ignore_patterns)
        except (FileNotFoundError, ValueError, yaml.YAMLError) as exc:
            print(str(exc), file=sys.stderr)
            return 2

    if not examples:
        print(f"No matching C examples found under {examples_root}", file=sys.stderr)
        if missing:
            print("\nMatching manifest entries without built executables:", file=sys.stderr)
            for rel in missing:
                print(f"  - {rel}", file=sys.stderr)
        return 1

    if args.batch:
        resolved_batch = getattr(args, "resolved_batch", args.batch)
        print(f"C example review batch: {resolved_batch} ({len(examples)} examples)")
    action = "open in VS Code" if args.code else "run sequentially"
    print(f"C examples to {action}:")
    for index, (rel, _) in enumerate(examples, 1):
        print(f"  {index:2d}. {rel}")
    if ignored:
        print("\nIgnored examples:")
        for rel in ignored:
            print(f"  - {rel}")
    if missing:
        print("\nManifest entries without built executables:")
        for rel in missing:
            print(f"  - {rel}")
        if args.strict:
            return 1
    if args.list:
        return 0

    if args.code:
        return open_code(root, examples, args.code_command)

    example_args = []
    if args.frames:
        example_args.append(args.frames)
    example_args.extend(args.example_arg)

    if example_args:
        print(f"\nPassing example arguments: {' '.join(example_args)}\n")
    else:
        print("\nClose each example window to continue to the next one.\n")

    env = os.environ.copy()
    apply_runtime_env(root, env)
    for index, (rel, exe) in enumerate(examples, 1):
        print(f"[{index}/{len(examples)}] {rel}")
        result = subprocess.run([str(exe), *example_args], cwd=root, env=env, check=False)
        if result.returncode != 0:
            print(f"Example failed: {rel} exited with {result.returncode}", file=sys.stderr)
            return result.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
