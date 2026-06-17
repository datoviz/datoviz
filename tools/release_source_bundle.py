#!/usr/bin/env python3
"""Create a Datoviz release source bundle with submodule contents."""

from __future__ import annotations

import argparse
import gzip
import hashlib
import os
import subprocess
import tarfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SUBMODULES = (
    "external/cglm",
    "external/cimgui",
    "external/glfw",
    "external/kvazaar",
    "external/mimalloc",
    "external/msdf-atlas-gen",
)


def _run(args: list[str], *, cwd: Path = ROOT) -> str:
    return subprocess.check_output(args, cwd=cwd, text=True)


def _git_files(cwd: Path) -> list[Path]:
    out = subprocess.check_output(["git", "ls-files", "-z"], cwd=cwd)
    return [Path(part.decode()) for part in out.split(b"\0") if part]


def _submodule_paths() -> set[Path]:
    status = _run(["git", "submodule", "status"])
    paths: set[Path] = set()
    for line in status.splitlines():
        fields = line.strip().split()
        if len(fields) >= 2:
            path = Path(fields[1])
            if path != Path("data"):
                paths.add(path)
    return paths


def _add_file(tar: tarfile.TarFile, src: Path, arcname: Path) -> None:
    st = src.stat()
    info = tarfile.TarInfo(os.fspath(arcname))
    info.size = st.st_size
    info.mode = st.st_mode & 0o777
    info.mtime = 0
    info.uid = 0
    info.gid = 0
    info.uname = ""
    info.gname = ""
    with src.open("rb") as f:
        tar.addfile(info, f)


def _source_entries(submodules: tuple[str, ...]) -> list[tuple[Path, Path]]:
    entries: list[tuple[Path, Path]] = []
    submodule_paths = _submodule_paths()

    for rel in _git_files(ROOT):
        if rel == Path("data") or any(rel == sm for sm in submodule_paths):
            continue
        src = ROOT / rel
        if src.is_file():
            entries.append((src, rel))

    for submodule in submodules:
        subroot = ROOT / submodule
        if not (subroot / ".git").exists() and not (subroot / "CMakeLists.txt").exists():
            raise FileNotFoundError(
                f"submodule is not initialized or does not look usable: {submodule}"
            )
        for rel in _git_files(subroot):
            src = subroot / rel
            if src.is_file():
                entries.append((src, Path(submodule) / rel))

    return sorted(entries, key=lambda item: os.fspath(item[1]))


def create_bundle(version: str, output_dir: Path, submodules: tuple[str, ...]) -> Path:
    output_dir.mkdir(parents=True, exist_ok=True)
    archive = output_dir / f"datoviz-{version}-source.tar.gz"
    root_name = Path(f"datoviz-{version}")

    with archive.open("wb") as raw:
        with gzip.GzipFile(filename="", mode="wb", fileobj=raw, compresslevel=9, mtime=0) as gz:
            with tarfile.open(fileobj=gz, mode="w") as tar:
                for src, rel in _source_entries(submodules):
                    _add_file(tar, src, root_name / rel)

    return archive


def sha512(path: Path) -> str:
    h = hashlib.sha512()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("version", help="release version without the leading v, for example 0.4.0")
    parser.add_argument("--output-dir", type=Path, default=ROOT / "dist")
    parser.add_argument(
        "--submodule",
        action="append",
        dest="submodules",
        help="submodule path to include; defaults to all Datoviz-owned external submodules",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    submodules = tuple(args.submodules) if args.submodules else DEFAULT_SUBMODULES
    archive = create_bundle(args.version, args.output_dir.resolve(), submodules)
    digest = sha512(archive)
    print(archive)
    print(f"SHA512 {digest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
