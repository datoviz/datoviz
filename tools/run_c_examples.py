#!/usr/bin/env python3
"""Run built C examples sequentially for manual regression checks."""

from __future__ import annotations

import argparse
import os
import platform
import re
import subprocess
import sys
from pathlib import Path


DEFAULT_GROUPS = {"visuals", "techniques"}
FILTER_GROUPS = DEFAULT_GROUPS | {"showcase"}


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
        help="optional regex/substr filter matched against group/name",
    )
    parser.add_argument(
        "--ignore",
        action="append",
        default=[],
        help="regex/substr pattern to skip; may be repeated or comma-separated",
    )
    parser.add_argument(
        "--list",
        action="store_true",
        help="print matching examples without running them",
    )
    parser.add_argument(
        "--build-dir",
        default="build",
        help="CMake build directory, default: build",
    )
    return parser.parse_args()


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


def main() -> int:
    args = parse_args()
    root = repo_root()
    examples_root = root / args.build_dir / "examples" / "c"

    ignore_patterns = split_patterns(args.ignore)

    examples: list[tuple[str, Path]] = []
    ignored: list[str] = []
    if examples_root.exists():
        groups = FILTER_GROUPS if args.filter else DEFAULT_GROUPS
        for exe in sorted(examples_root.glob("*/*")):
            if not is_executable(exe):
                continue
            rel = exe.relative_to(examples_root).as_posix()
            group = rel.split("/", 1)[0]
            if group == "tools" and not args.filter.startswith("tools"):
                continue
            if group not in groups and group != "tools":
                continue
            if args.filter and not matches_filter(rel, args.filter):
                continue
            if any(matches_filter(rel, pattern) for pattern in ignore_patterns):
                ignored.append(rel)
                continue
            examples.append((rel, exe))

    if not examples:
        print(f"No matching C examples found under {examples_root}", file=sys.stderr)
        return 1

    print("C examples to run sequentially:")
    for index, (rel, _) in enumerate(examples, 1):
        print(f"  {index:2d}. {rel}")
    if ignored:
        print("\nIgnored examples:")
        for rel in ignored:
            print(f"  - {rel}")
    if args.list:
        return 0
    print("\nClose each example window to continue to the next one.\n")

    env = os.environ.copy()
    apply_runtime_env(root, env)
    for index, (rel, exe) in enumerate(examples, 1):
        print(f"[{index}/{len(examples)}] {rel}")
        result = subprocess.run([str(exe)], cwd=root, env=env, check=False)
        if result.returncode != 0:
            print(f"Example failed: {rel} exited with {result.returncode}", file=sys.stderr)
            return result.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
