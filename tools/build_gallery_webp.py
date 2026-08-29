#!/usr/bin/env python3
"""Generate build-local WebP derivatives for gallery screenshots."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import tempfile
from dataclasses import dataclass
from pathlib import Path

import build_gallery
import gallery_media


ROOT = gallery_media.ROOT
DEFAULT_OUTPUT_DIR = ROOT / "build/gallery-webp/v0.4"
DEFAULT_CACHE_DIR = ROOT / "build/gallery-cache/static-webp"
DEFAULT_QUALITY = 90
CACHE_SCHEMA = 1
STATIC_PROFILE = "gallery-static-webp-v1"


@dataclass(frozen=True)
class GalleryWebPResult:
    converted: int = 0
    skipped: int = 0
    missing: int = 0
    animated_skipped: int = 0
    selected: int = 0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=gallery_media.DEFAULT_MANIFEST)
    parser.add_argument("--image-dir", type=Path, default=build_gallery.DEFAULT_IMAGE_DIR)
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT_DIR)
    parser.add_argument("--cache-dir", type=Path, default=DEFAULT_CACHE_DIR)
    parser.add_argument("--quality", type=int, default=DEFAULT_QUALITY)
    parser.add_argument("--id", action="append", default=[], help="example id; repeat or comma-separate")
    parser.add_argument("--lane", action="append", default=[], help="gallery lane")
    parser.add_argument("--dry-run", action="store_true", help="list conversions without writing WebP files")
    parser.add_argument("--force", action="store_true", help="rewrite verified current WebP files")
    parser.add_argument(
        "--animated-fallbacks",
        action="store_true",
        help="create missing still-image fallbacks for animated and video previews",
    )
    parser.add_argument("--strict", action="store_true", help="fail when selected PNG inputs are missing")
    parser.add_argument(
        "--require-image-dir",
        action="store_true",
        help="fail when the source PNG directory is missing",
    )
    parser.add_argument(
        "--quiet-missing",
        action="store_true",
        help="summarize missing PNG inputs instead of printing each path",
    )
    return parser.parse_args()


def split_values(values: list[str]) -> set[str]:
    return gallery_media.split_values(values)


def output_path(example: build_gallery.Example, output_dir: Path) -> Path:
    return gallery_media.gallery_webp_path(example, output_dir)


def fallback_output_path(example: build_gallery.Example, output_dir: Path) -> Path:
    """Return the documentation fallback path for an animated preview."""
    suffix = (
        ".poster.webp"
        if build_gallery.preferred_preview_media(example) == "video-mp4"
        else ".webp"
    )
    return output_dir / example.lane / f"{example.id}{suffix}"


def animated_preview_keys(manifest_data: dict) -> set[tuple[str, str]]:
    return gallery_media.animated_preview_keys(manifest_data)


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as file:
        for chunk in iter(lambda: file.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def image_dimensions(path: Path, expected_format: str) -> tuple[int, int]:
    """Decode an image and return its dimensions after validating its format."""
    try:
        from PIL import Image
    except ImportError as exc:
        raise RuntimeError("Pillow is required to validate gallery WebP products") from exc

    try:
        with Image.open(path) as image:
            if image.format != expected_format:
                raise ValueError(f"expected {expected_format}, got {image.format or 'unknown'}")
            dimensions = image.size
            image.load()
    except (OSError, ValueError) as exc:
        raise RuntimeError(f"invalid {expected_format} image {path}: {exc}") from exc
    return dimensions


def implementation_identity() -> str:
    """Fingerprint implementation inputs that can change the encoded product."""
    digest = hashlib.sha256()
    path = Path(__file__).resolve()
    digest.update(path.name.encode("utf8"))
    digest.update(b"\0")
    digest.update(path.read_bytes())
    return digest.hexdigest()


def cwebp_identity(cwebp: str) -> str:
    """Fingerprint the selected encoder binary and its reported version."""
    result = subprocess.run(
        [cwebp, "-version"],
        check=True,
        capture_output=True,
        text=True,
    )
    executable = Path(cwebp).resolve()
    payload = {
        "path": str(executable),
        "sha256": file_sha256(executable) if executable.is_file() else "",
        "stdout": result.stdout,
        "stderr": result.stderr,
    }
    return hashlib.sha256(
        json.dumps(payload, sort_keys=True, separators=(",", ":")).encode("utf8")
    ).hexdigest()


def cache_path(webp: Path, output_dir: Path, cache_dir: Path) -> Path:
    relative = webp.relative_to(output_dir)
    return cache_dir / relative.parent / f"{relative.stem}.json"


def load_cache(path: Path) -> dict:
    try:
        payload = json.loads(path.read_text(encoding="utf8"))
    except (OSError, json.JSONDecodeError):
        return {}
    return payload if isinstance(payload, dict) else {}


def product_identity(
    *,
    source_hash: str,
    quality: int,
    profile: str,
    encoder_identity: str,
    implementation_hash: str,
) -> dict[str, str | int]:
    return {
        "source_sha256": source_hash,
        "quality": quality,
        "profile": profile,
        "encoder_identity": encoder_identity,
        "implementation_sha256": implementation_hash,
    }


def identity_hash(identity: dict[str, str | int]) -> str:
    return hashlib.sha256(
        json.dumps(identity, sort_keys=True, separators=(",", ":")).encode("utf8")
    ).hexdigest()


def current_cache_hit(
    webp: Path,
    record_path: Path,
    expected_identity: dict[str, str | int],
    expected_dimensions: tuple[int, int],
    *,
    verify_encoder: bool = True,
) -> tuple[bool, str]:
    if not webp.exists():
        return False, "missing output"
    payload = load_cache(record_path)
    if payload.get("schema") != CACHE_SCHEMA:
        return False, "missing or incompatible cache record"
    recorded_identity = payload.get("identity")
    if not isinstance(recorded_identity, dict):
        return False, "invalid cache identity"
    if payload.get("input_hash") != identity_hash(recorded_identity):
        return False, "invalid cache identity hash"
    for key, value in expected_identity.items():
        if key == "encoder_identity" and not verify_encoder:
            continue
        if recorded_identity.get(key) != value:
            return False, f"stale {key}"
    if payload.get("dimensions") != list(expected_dimensions):
        return False, "stale dimensions"
    try:
        actual_dimensions = image_dimensions(webp, "WEBP")
        output_hash = file_sha256(webp)
    except (OSError, RuntimeError):
        return False, "invalid output"
    if actual_dimensions != expected_dimensions:
        return False, "wrong output dimensions"
    if payload.get("webp_sha256") != output_hash:
        return False, "changed outside cache"
    return True, "current"


def atomic_write_json(path: Path, payload: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    file_descriptor, temporary_name = tempfile.mkstemp(
        dir=path.parent, prefix=f".{path.name}.", suffix=".tmp"
    )
    temporary = Path(temporary_name)
    try:
        with os.fdopen(file_descriptor, "w", encoding="utf8") as file:
            json.dump(payload, file, indent=2, sort_keys=True)
            file.write("\n")
            file.flush()
            os.fsync(file.fileno())
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


def encode_atomic(
    cwebp: str,
    png: Path,
    webp: Path,
    quality: int,
    expected_dimensions: tuple[int, int],
) -> str:
    """Encode and validate a temporary product before atomically publishing it."""
    webp.parent.mkdir(parents=True, exist_ok=True)
    file_descriptor, temporary_name = tempfile.mkstemp(
        dir=webp.parent, prefix=f".{webp.name}.", suffix=".tmp"
    )
    os.close(file_descriptor)
    temporary = Path(temporary_name)
    try:
        subprocess.run(
            [cwebp, "-quiet", "-q", str(quality), str(png), "-o", str(temporary)],
            check=True,
        )
        dimensions = image_dimensions(temporary, "WEBP")
        if dimensions != expected_dimensions:
            raise RuntimeError(
                f"encoded WebP dimensions are {dimensions[0]}x{dimensions[1]}, expected "
                f"{expected_dimensions[0]}x{expected_dimensions[1]}"
            )
        output_hash = file_sha256(temporary)
        os.replace(temporary, webp)
        return output_hash
    finally:
        temporary.unlink(missing_ok=True)


def prune_stale_webp(
    output_dir: Path,
    examples: list[build_gallery.Example],
    cache_dir: Path | None = None,
) -> int:
    valid = {(example.lane, example.id) for example in examples}
    removed = 0
    for lane in gallery_media.DOC_LANES:
        lane_dir = output_dir / lane
        if not lane_dir.exists():
            continue
        for webp in lane_dir.glob("*.webp"):
            stem = webp.stem
            if webp.name.endswith(".poster.webp"):
                stem = webp.name[: -len(".poster.webp")]
            if (lane, stem) in valid:
                continue
            webp.unlink()
            if cache_dir is not None:
                cache_path(webp, output_dir, cache_dir).unlink(missing_ok=True)
            removed += 1
    return removed


def select_examples(
    examples: list[build_gallery.Example],
    animated_keys: set[tuple[str, str]],
    ids: set[str],
    lanes: set[str],
    animated_fallbacks: bool,
) -> tuple[list[build_gallery.Example], int]:
    selected: list[build_gallery.Example] = []
    animated_skipped = 0
    for example in examples:
        if "screenshot" not in example.validation:
            continue
        if example.lane not in gallery_media.DOC_LANES:
            continue
        if ids and example.id not in ids:
            continue
        if lanes and example.lane not in lanes:
            continue
        if (example.lane, example.id) in animated_keys:
            if not animated_fallbacks:
                animated_skipped += 1
                continue
        selected.append(example)
    selected.sort(key=lambda item: (item.lane, item.id))
    return selected, animated_skipped


def generate_gallery_webp(
    *,
    manifest: Path = gallery_media.DEFAULT_MANIFEST,
    image_dir: Path = build_gallery.DEFAULT_IMAGE_DIR,
    output_dir: Path = DEFAULT_OUTPUT_DIR,
    cache_dir: Path = DEFAULT_CACHE_DIR,
    quality: int = DEFAULT_QUALITY,
    ids: set[str] | None = None,
    lanes: set[str] | None = None,
    dry_run: bool = False,
    force: bool = False,
    strict: bool = False,
    require_image_dir: bool = False,
    quiet_missing: bool = False,
    prune_stale: bool = True,
    animated_fallbacks: bool = False,
) -> tuple[int, GalleryWebPResult]:
    if not 0 <= quality <= 100:
        print("--quality must be between 0 and 100")
        return 2, GalleryWebPResult()
    if not image_dir.exists():
        print(f"gallery PNG source directory not found: {image_dir}")
        print("Run: git submodule update --init --recursive data")
        return (2 if strict or require_image_dir else 0), GalleryWebPResult()

    cwebp = shutil.which("cwebp")
    if cwebp is None and not dry_run:
        print("cwebp not found; install the WebP tools package or rerun with --dry-run")
        return 2, GalleryWebPResult()
    implementation_hash = implementation_identity()
    encoder_hash = cwebp_identity(cwebp) if cwebp is not None and not dry_run else ""

    manifest_data = gallery_media.load_manifest(manifest)
    all_examples = build_gallery.collect_examples(manifest_data)
    animated_keys = animated_preview_keys(manifest_data)
    ids = ids or set()
    lanes = lanes or set()
    examples, animated_skipped = select_examples(
        all_examples, animated_keys, ids, lanes, animated_fallbacks
    )
    if not examples:
        if animated_skipped:
            print("No static WebP conversions; animated previews are owned by build_gallery_animations.py")
            return 0, GalleryWebPResult(animated_skipped=animated_skipped, selected=animated_skipped)
        print("No matching gallery examples.")
        return 1, GalleryWebPResult()
    can_prune = prune_stale and not dry_run and not ids and not lanes
    valid_for_prune = [
        example
        for example in all_examples
        if "screenshot" in example.validation and example.lane in gallery_media.DOC_LANES
    ]
    removed = prune_stale_webp(output_dir, valid_for_prune, cache_dir) if can_prune else 0

    converted = 0
    skipped = 0
    missing = 0
    for example in examples:
        png = gallery_media.gallery_png_path(example, image_dir)
        is_animated = (example.lane, example.id) in animated_keys
        webp = (
            fallback_output_path(example, output_dir)
            if is_animated and animated_fallbacks
            else output_path(example, output_dir)
        )
        if not png.exists():
            missing += 1
            if not quiet_missing:
                rel_png = png.relative_to(ROOT) if png.is_relative_to(ROOT) else png
                print(f"missing: {example.id} -> {rel_png}")
            continue
        # A richer animation or video pipeline owns existing animated outputs. The documentation
        # fallback only fills missing paths and must never replace those assets, even with --force.
        if is_animated and animated_fallbacks and webp.exists():
            skipped += 1
            continue
        source_hash = file_sha256(png)
        source_dimensions = image_dimensions(png, "PNG")
        profile = (
            f"{STATIC_PROFILE}:documentation-fallback"
            if is_animated and animated_fallbacks
            else STATIC_PROFILE
        )
        record_path = cache_path(webp, output_dir, cache_dir)
        identity = product_identity(
            source_hash=source_hash,
            quality=quality,
            profile=profile,
            encoder_identity=encoder_hash,
            implementation_hash=implementation_hash,
        )
        cache_hit, _ = current_cache_hit(
            webp,
            record_path,
            identity,
            source_dimensions,
            verify_encoder=not dry_run,
        )
        if cache_hit and not force:
            skipped += 1
            continue
        converted += 1
        rel_webp = webp.relative_to(ROOT) if webp.is_relative_to(ROOT) else webp
        if dry_run:
            print(f"would convert: {example.id} -> {rel_webp}")
            continue
        assert cwebp is not None
        output_hash = encode_atomic(cwebp, png, webp, quality, source_dimensions)
        atomic_write_json(
            record_path,
            {
                "schema": CACHE_SCHEMA,
                "input_hash": identity_hash(identity),
                "identity": identity,
                "dimensions": list(source_dimensions),
                "webp_sha256": output_hash,
                "path": str(webp.relative_to(ROOT) if webp.is_relative_to(ROOT) else webp),
            },
        )

    result = GalleryWebPResult(
        converted=converted,
        skipped=skipped,
        missing=missing,
        animated_skipped=animated_skipped,
        selected=len(examples),
    )
    action = "would_convert" if dry_run else "converted"
    print(
        f"gallery webp: {action}={converted} skipped={skipped} "
        f"animated_skipped={animated_skipped} missing={missing}"
    )
    if removed:
        print(f"gallery webp: removed_stale={removed}")
    if missing and strict:
        print("Missing committed gallery PNGs. If this is a fresh clone, run:")
        print("  git submodule update --init --recursive data")
    return (1 if missing and strict else 0), result


def main() -> int:
    args = parse_args()
    ids = split_values(args.id)
    lanes = split_values(args.lane)
    rc, _ = generate_gallery_webp(
        manifest=args.manifest,
        image_dir=args.image_dir,
        output_dir=args.output_dir,
        cache_dir=args.cache_dir,
        quality=args.quality,
        ids=ids,
        lanes=lanes,
        dry_run=args.dry_run,
        force=args.force,
        strict=args.strict,
        require_image_dir=args.require_image_dir,
        quiet_missing=args.quiet_missing,
        animated_fallbacks=args.animated_fallbacks,
    )
    return rc


if __name__ == "__main__":
    raise SystemExit(main())
