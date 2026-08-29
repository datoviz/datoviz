import sys
from pathlib import Path
from unittest import mock

import pytest


sys.path.insert(0, str(Path(__file__).parents[1]))

pytest.importorskip("PIL")

import docs_asset_stage


def write_manifest(path: Path, preferred: str = "") -> Path:
    card = (
        f"\n        card:\n          preferred: {preferred}"
        if preferred
        else ""
    )
    path.write_text(
        "examples:\n"
        "  - id: example\n"
        "    category: showcase\n"
        "    validation: [screenshot]\n"
        "    media:\n"
        "      preview:\n"
        f"        kind: {'animated-webp' if preferred else 'static-webp'}{card}\n",
        encoding="utf8",
    )
    return path


def test_hermetic_stage_cannot_poison_verified_products(tmp_path):
    products = tmp_path / "verified/gallery-webp/v0.4"
    products.mkdir(parents=True)
    marker = products / "verified.webp"
    marker.write_bytes(b"verified")
    stage = tmp_path / "stages/hermetic"

    docs_asset_stage.prepare_stage("hermetic", stage)

    assert marker.read_bytes() == b"verified"
    assert list((stage / "gallery/v0.4").rglob("*.webp"))
    assert not (stage / "webgpu-data").exists()


@pytest.mark.parametrize("profile", ["local", "publish"])
def test_real_profile_copies_products_and_stages_webgpu(profile, tmp_path):
    products = tmp_path / "verified/gallery-webp/v0.4"
    source = products / "showcases/example.webp"
    source.parent.mkdir(parents=True)
    source.write_bytes(b"verified")
    stage = tmp_path / f"stages/{profile}"
    manifest = write_manifest(tmp_path / "manifest.yaml")

    def stage_webgpu(*, output_dir):
        output_dir.mkdir(parents=True)
        (output_dir / "bundle.json").write_text("{}\n", encoding="utf8")
        return 0

    with mock.patch("build_webgpu_data_bundles.stage_bundles", side_effect=stage_webgpu):
        docs_asset_stage.prepare_stage(profile, stage, products, manifest)

    assert (stage / "gallery/v0.4/showcases/example.webp").read_bytes() == b"verified"
    assert (stage / "webgpu-data/bundle.json").is_file()
    assert source.read_bytes() == b"verified"
    assert (stage / ".gallery-video-assets.ready").exists() == (profile == "local")


def test_replacing_stage_removes_stale_files_without_touching_products(tmp_path):
    products = tmp_path / "verified/gallery-webp/v0.4"
    products.mkdir(parents=True)
    (products / "current.webp").write_bytes(b"current")
    stage = tmp_path / "stages/local"
    stage.mkdir(parents=True)
    (stage / "stale.webp").write_bytes(b"stale")

    with mock.patch("build_webgpu_data_bundles.stage_bundles", return_value=0):
        docs_asset_stage.prepare_stage("local", stage, products)

    assert not (stage / "stale.webp").exists()
    assert (stage / "gallery/v0.4/current.webp").is_file()
    assert (products / "current.webp").is_file()


def test_stage_and_verified_products_must_not_overlap(tmp_path):
    products = tmp_path / "products"
    products.mkdir()

    with pytest.raises(ValueError, match="must be separate"):
        docs_asset_stage.prepare_stage("local", products / "stage", products)


def test_local_and_publish_stages_are_independent(tmp_path):
    products = tmp_path / "verified/gallery-webp/v0.4"
    products.mkdir(parents=True)
    media = products / "showcases/example.webp"
    media.parent.mkdir()
    media.write_bytes(b"first")
    local = tmp_path / "stages/local"
    publish = tmp_path / "stages/publish"
    manifest = write_manifest(tmp_path / "manifest.yaml")

    with mock.patch("build_webgpu_data_bundles.stage_bundles", return_value=0):
        docs_asset_stage.prepare_stage("local", local, products)
        media.write_bytes(b"second")
        docs_asset_stage.prepare_stage("publish", publish, products, manifest)

    assert (local / "gallery/v0.4/showcases/example.webp").read_bytes() == b"first"
    assert (publish / "gallery/v0.4/showcases/example.webp").read_bytes() == b"second"


def test_publish_stage_rejects_missing_video_and_poster(tmp_path):
    products = tmp_path / "verified/gallery-webp/v0.4"
    products.mkdir(parents=True)
    manifest = write_manifest(tmp_path / "manifest.yaml", preferred="video-mp4")

    with (
        mock.patch("build_webgpu_data_bundles.stage_bundles", return_value=0),
        pytest.raises(ValueError, match="example.mp4.*example.poster.webp"),
    ):
        docs_asset_stage.prepare_stage(
            "publish", tmp_path / "stages/publish", products, manifest
        )


def test_publish_stage_excludes_local_availability_manifest(tmp_path):
    products = tmp_path / "verified/gallery-webp/v0.4"
    products.mkdir(parents=True)
    (products / "showcases").mkdir()
    (products / "showcases/example.poster.webp").write_bytes(b"poster")
    (products / "showcases/example.mp4").write_bytes(b"video")
    (products / "local-video-assets.json").write_text("{}\n", encoding="utf8")
    manifest = write_manifest(tmp_path / "manifest.yaml", preferred="video-mp4")
    stage = tmp_path / "stages/publish"

    with mock.patch("build_webgpu_data_bundles.stage_bundles", return_value=0):
        docs_asset_stage.prepare_stage("publish", stage, products, manifest)

    assert not (stage / "gallery/v0.4/local-video-assets.json").exists()
    assert (stage / "gallery/v0.4/showcases/example.mp4").read_bytes() == b"video"


def test_local_stage_excludes_product_mp4_until_hydration(tmp_path):
    products = tmp_path / "verified/gallery-webp/v0.4"
    lane = products / "showcases"
    lane.mkdir(parents=True)
    (lane / "example.poster.webp").write_bytes(b"poster")
    (lane / "example.mp4").write_bytes(b"legacy-video")
    stage = tmp_path / "stages/local"

    with mock.patch("build_webgpu_data_bundles.stage_bundles", return_value=0):
        docs_asset_stage.prepare_stage("local", stage, products)

    staged_lane = stage / "gallery/v0.4/showcases"
    assert (staged_lane / "example.poster.webp").read_bytes() == b"poster"
    assert not (staged_lane / "example.mp4").exists()
    assert (stage / ".gallery-video-assets.ready").is_file()
