#!/usr/bin/env python3
"""Prepare isolated generated-media stages for MkDocs documentation profiles."""

from __future__ import annotations

import argparse
import io
import os
import shutil
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_GALLERY_PRODUCTS = ROOT / "build/gallery-webp/v0.4"
DEFAULT_MANIFEST = ROOT / "examples/c/MANIFEST.yaml"
DEFAULT_STAGE_ROOTS = {
    "hermetic": ROOT / "build/docs-assets/hermetic",
    "local": ROOT / "build/docs-assets/local",
    "publish": ROOT / "build/docs-assets/publish",
}
GALLERY_STAGE_RELATIVE = Path("gallery/v0.4")
WEBGPU_STAGE_RELATIVE = Path("webgpu-data")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--profile", choices=sorted(DEFAULT_STAGE_ROOTS), required=True)
    parser.add_argument("--stage-root", type=Path, required=True)
    parser.add_argument("--gallery-products", type=Path)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    return parser.parse_args()


def _contains(parent: Path, child: Path) -> bool:
    try:
        child.relative_to(parent)
    except ValueError:
        return False
    return True


def validate_separate_roots(stage_root: Path, gallery_products: Path | None) -> None:
    """Reject layouts that could overwrite verified products while replacing a stage."""
    stage = stage_root.resolve()
    if stage == Path(stage.anchor):
        raise ValueError(f"refusing broad documentation stage root: {stage}")
    if gallery_products is None:
        return
    products = gallery_products.resolve()
    if _contains(stage, products) or _contains(products, stage):
        raise ValueError(
            "documentation stage and verified gallery products must be separate: "
            f"stage={stage}, products={products}"
        )


def _placeholder_payload() -> bytes:
    from PIL import Image, ImageDraw

    image = Image.new("RGB", (640, 360), color=(32, 38, 48))
    draw = ImageDraw.Draw(image)
    draw.line((0, 0, 640, 360), fill=(80, 96, 120), width=8)
    draw.line((0, 360, 640, 0), fill=(80, 96, 120), width=8)
    draw.text((24, 24), "MEDIA EXCLUDED FROM HERMETIC BUILD", fill=(220, 226, 235))
    encoded = io.BytesIO()
    image.save(encoded, format="WEBP", quality=70, method=6)
    return encoded.getvalue()


def stage_hermetic_gallery_placeholders(output: Path, manifest_path: Path) -> int:
    """Create generated-only stand-ins without reading external gallery products."""
    from build_gallery import collect_examples, load_manifest

    payload = _placeholder_payload()
    examples = collect_examples(load_manifest(manifest_path))
    count = 0
    for example in examples:
        if not example.screenshot_expected:
            continue
        path = output / example.lane / f"{example.id}.webp"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(payload)
        count += 1
    return count


def required_gallery_products(manifest: dict) -> list[Path]:
    """Return every gallery product directly referenced by generated documentation."""
    import gallery_media

    required: list[Path] = []
    for entry in manifest.get("examples", []):
        if "screenshot" not in (entry.get("validation") or []):
            continue
        lane, example_id = gallery_media.entry_key(entry)
        if not lane or not example_id:
            raise ValueError("manifest screenshot entries require a lane and id")
        if gallery_media.preferred_preview_kind(entry) == "video-mp4":
            required.extend(
                [
                    Path(lane) / f"{example_id}.poster.webp",
                    Path(lane) / f"{example_id}.mp4",
                ]
            )
        else:
            required.append(Path(lane) / f"{example_id}.webp")
    return sorted(required)


def validate_publish_products(gallery_stage: Path, manifest_path: Path) -> None:
    """Reject a publish stage missing any manifest-required static or card product."""
    import gallery_media

    manifest = gallery_media.load_manifest(manifest_path)
    missing = [
        path
        for path in required_gallery_products(manifest)
        if not (gallery_stage / path).is_file() or (gallery_stage / path).stat().st_size == 0
    ]
    if missing:
        labels = ", ".join(path.as_posix() for path in missing)
        raise ValueError(f"publish stage is missing required gallery products: {labels}")


def _populate_stage(
    profile: str,
    stage_root: Path,
    gallery_products: Path | None,
    manifest_path: Path,
) -> None:
    gallery_stage = stage_root / GALLERY_STAGE_RELATIVE
    if profile == "hermetic":
        count = stage_hermetic_gallery_placeholders(gallery_stage, manifest_path)
        print(f"docs assets: staged {count} hermetic gallery placeholders")
        return

    if gallery_products is None:
        raise ValueError(f"{profile} profile requires --gallery-products")
    products = gallery_products.resolve()
    if not products.is_dir():
        raise FileNotFoundError(f"verified gallery product directory not found: {products}")
    ignored_products = (
        ("local-video-assets.json", "*.mp4")
        if profile == "local"
        else ("local-video-assets.json",)
    )
    shutil.copytree(products, gallery_stage, ignore=shutil.ignore_patterns(*ignored_products))

    import build_webgpu_data_bundles

    rc = build_webgpu_data_bundles.stage_bundles(
        output_dir=stage_root / WEBGPU_STAGE_RELATIVE,
        include_local=profile == "local",
    )
    if rc != 0:
        raise RuntimeError(f"WebGPU data staging failed with status {rc}")
    if profile == "local":
        (stage_root / ".gallery-video-assets.ready").touch()
    else:
        validate_publish_products(gallery_stage, manifest_path)
    print(f"docs assets: copied verified gallery products into {profile} stage")


def prepare_stage(
    profile: str,
    stage_root: Path,
    gallery_products: Path | None = None,
    manifest_path: Path = DEFAULT_MANIFEST,
) -> Path:
    """Prepare a complete profile stage, then replace the prior stage as one directory."""
    if profile not in DEFAULT_STAGE_ROOTS:
        raise ValueError(f"unknown documentation asset profile: {profile}")
    stage = stage_root.resolve()
    products = gallery_products.resolve() if gallery_products is not None else None
    validate_separate_roots(stage, products)
    stage.parent.mkdir(parents=True, exist_ok=True)
    temporary = Path(tempfile.mkdtemp(prefix=f".{stage.name}-{profile}-", dir=stage.parent))
    backup = stage.with_name(f".{stage.name}-previous-{os.getpid()}")
    try:
        _populate_stage(profile, temporary, products, manifest_path)
        if backup.exists():
            shutil.rmtree(backup)
        if stage.exists():
            os.replace(stage, backup)
        os.replace(temporary, stage)
        if backup.exists():
            shutil.rmtree(backup)
    except Exception:
        if not stage.exists() and backup.exists():
            os.replace(backup, stage)
        raise
    finally:
        if temporary.exists():
            shutil.rmtree(temporary)
    return stage


def main() -> int:
    args = parse_args()
    try:
        stage = prepare_stage(
            args.profile,
            args.stage_root,
            args.gallery_products,
            args.manifest,
        )
    except (FileNotFoundError, RuntimeError, ValueError) as exc:
        print(f"docs assets: {exc}")
        return 2
    print(f"docs assets: {args.profile} stage ready at {stage}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
