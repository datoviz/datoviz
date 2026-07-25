#!/usr/bin/env python3
"""Generate build-local WebP derivatives for Vulkan tutorial previews."""

from __future__ import annotations

import argparse
import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path

from capture_gallery import png_is_nonblank


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE_DIR = ROOT / "data" / "tutorials" / "vulkan"
DEFAULT_OUTPUT_DIR = ROOT / "build" / "tutorial-webp" / "vulkan"
DEFAULT_QUALITY = 90
EXPECTED_PREVIEWS = (
    "first-triangle",
    "shaders-and-pipeline",
    "vertex-buffers",
)


@dataclass(frozen=True)
class TutorialMediaResult:
    converted: int = 0
    skipped: int = 0
    missing: int = 0
    invalid: int = 0


def _needs_update(source: Path, output: Path, force: bool) -> bool:
    return force or not output.exists() or source.stat().st_mtime_ns > output.stat().st_mtime_ns


def generate_tutorial_media(
    *,
    source_dir: Path = DEFAULT_SOURCE_DIR,
    output_dir: Path = DEFAULT_OUTPUT_DIR,
    quality: int = DEFAULT_QUALITY,
    force: bool = False,
    strict: bool = False,
) -> tuple[int, TutorialMediaResult]:
    if not 0 <= quality <= 100:
        print("--quality must be between 0 and 100")
        return 2, TutorialMediaResult()
    if not source_dir.is_dir():
        print(f"tutorial PNG source directory not found: {source_dir}")
        print("Run: git submodule update --init --recursive data")
        return (2 if strict else 0), TutorialMediaResult(missing=len(EXPECTED_PREVIEWS))

    cwebp = shutil.which("cwebp")
    if cwebp is None:
        print("cwebp not found; install the WebP tools package")
        return 2, TutorialMediaResult()

    converted = 0
    skipped = 0
    missing = 0
    invalid = 0
    for stem in EXPECTED_PREVIEWS:
        source = source_dir / f"{stem}.png"
        output = output_dir / f"{stem}.webp"
        if not source.is_file():
            print(f"missing tutorial preview: {source}")
            missing += 1
            continue
        valid, detail = png_is_nonblank(source, (800, 600))
        if not valid:
            print(f"invalid tutorial preview {source}: {detail}")
            invalid += 1
            continue
        if not _needs_update(source, output, force):
            skipped += 1
            continue
        output.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(
            [cwebp, "-quiet", "-q", str(quality), str(source), "-o", str(output)],
            check=True,
        )
        converted += 1

    result = TutorialMediaResult(
        converted=converted,
        skipped=skipped,
        missing=missing,
        invalid=invalid,
    )
    print(
        f"tutorial webp: converted={converted} skipped={skipped} "
        f"missing={missing} invalid={invalid}"
    )
    return (1 if strict and (missing or invalid) else 0), result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-dir", type=Path, default=DEFAULT_SOURCE_DIR)
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT_DIR)
    parser.add_argument("--quality", type=int, default=DEFAULT_QUALITY)
    parser.add_argument("--force", action="store_true")
    parser.add_argument("--strict", action="store_true")
    args = parser.parse_args()
    rc, _ = generate_tutorial_media(
        source_dir=args.source_dir,
        output_dir=args.output_dir,
        quality=args.quality,
        force=args.force,
        strict=args.strict,
    )
    return rc


if __name__ == "__main__":
    raise SystemExit(main())
