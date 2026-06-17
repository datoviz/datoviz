#!/usr/bin/env python3
"""Reject staged runtime payloads and accidental large files."""

from __future__ import annotations

import fnmatch
import os
import subprocess
import sys


MAX_BYTES = int(os.environ.get("DVZ_MAX_STAGED_BYTES", str(1024 * 1024)))
ALLOW_ENV = "DVZ_ALLOW_STAGED_PAYLOADS"

STOP_DIRS = (
    "bin/vulkan/",
    "libs/shaderc/",
    "libs/swiftshader/",
    "libs/vulkan/",
)

STOP_GLOBS = (
    "*.dylib",
    "*.so",
    "*.so.*",
    "*.dll",
    "*.npy",
    "*.npz",
    ".DS_Store",
    "*/.DS_Store",
)


def _git(args: list[str], *, check: bool = True) -> subprocess.CompletedProcess[bytes]:
    return subprocess.run(["git", *args], check=check, stdout=subprocess.PIPE)


def _staged_paths() -> list[str]:
    result = _git(["diff", "--cached", "--name-only", "-z", "--diff-filter=ACMRT"])
    if not result.stdout:
        return []
    return [p.decode("utf-8") for p in result.stdout.split(b"\0") if p]


def _index_mode_and_oid(path: str) -> tuple[str, str] | None:
    result = _git(["ls-files", "-s", "--", path], check=False)
    if result.returncode != 0 or not result.stdout:
        return None
    line = result.stdout.decode("utf-8", errors="replace").splitlines()[0]
    parts = line.split()
    if len(parts) < 2:
        return None
    return parts[0], parts[1]


def _object_size(oid: str) -> int:
    result = _git(["cat-file", "-s", oid])
    return int(result.stdout.decode("ascii").strip())


def _matches_stop_path(path: str) -> bool:
    if any(path == d.rstrip("/") or path.startswith(d) for d in STOP_DIRS):
        return True
    return any(fnmatch.fnmatch(path, pattern) for pattern in STOP_GLOBS)


def main() -> int:
    if os.environ.get(ALLOW_ENV) == "1":
        print(f"warning: staged payload checks bypassed via {ALLOW_ENV}=1", file=sys.stderr)
        return 0

    failures: list[str] = []
    for path in _staged_paths():
        entry = _index_mode_and_oid(path)
        if entry is None:
            continue
        mode, oid = entry

        if path == "data" and mode == "160000":
            failures.append("data: staged submodule gitlink update requires explicit approval")
            continue

        if _matches_stop_path(path):
            failures.append(f"{path}: stop-sign runtime/generated payload")
            continue

        if mode != "160000":
            size = _object_size(oid)
            if size > MAX_BYTES:
                mib = size / 1024 / 1024
                limit = MAX_BYTES / 1024 / 1024
                failures.append(f"{path}: staged file is {mib:.2f} MiB > {limit:.2f} MiB")

    if failures:
        print("Refusing staged payloads:", file=sys.stderr)
        for failure in failures:
            print(f"  - {failure}", file=sys.stderr)
        print(
            f"Set {ALLOW_ENV}=1 only after explicit maintainer approval for these exact paths.",
            file=sys.stderr,
        )
        return 1

    print("staged payload check passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
