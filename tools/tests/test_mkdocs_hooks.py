import sys
from pathlib import Path
from unittest import mock

import pytest


sys.path.insert(0, str(Path(__file__).parents[1]))

pytest.importorskip("mkdocs")
pytest.importorskip("PIL")

import mkdocs_hooks


def test_hermetic_docs_do_not_stage_external_site_assets(monkeypatch, tmp_path):
    gallery = tmp_path / "build/gallery-webp/v0.4"
    webgpu = tmp_path / "build/webgpu-data"
    gallery.mkdir(parents=True)
    webgpu.mkdir(parents=True)
    monkeypatch.setattr(mkdocs_hooks, "ROOT", tmp_path)
    monkeypatch.delenv(mkdocs_hooks.SITE_ASSETS_ENV, raising=False)
    monkeypatch.delenv(mkdocs_hooks.GENERATED_ROOT_ENV, raising=False)

    with (
        mock.patch("build_gallery_webp.generate_gallery_webp") as generate_gallery,
        mock.patch("build_webgpu_data_bundles.stage_bundles") as stage_webgpu,
        mock.patch.object(mkdocs_hooks, "stage_hermetic_gallery_placeholders") as placeholders,
    ):
        mkdocs_hooks.prepare_optional_site_assets()

    assert not webgpu.exists()
    generate_gallery.assert_not_called()
    stage_webgpu.assert_not_called()
    placeholders.assert_called_once_with(gallery)


def test_hermetic_gallery_placeholders_cover_declared_media(tmp_path):
    mkdocs_hooks.stage_hermetic_gallery_placeholders(tmp_path)
    assert list(tmp_path.rglob("*.webp"))


def test_optional_site_assets_require_explicit_mode(monkeypatch, tmp_path):
    monkeypatch.setattr(mkdocs_hooks, "ROOT", tmp_path)
    monkeypatch.setenv(mkdocs_hooks.SITE_ASSETS_ENV, "1")
    monkeypatch.delenv(mkdocs_hooks.GENERATED_ROOT_ENV, raising=False)

    with (
        mock.patch("build_gallery_webp.generate_gallery_webp") as generate_gallery,
        mock.patch("build_webgpu_data_bundles.stage_bundles") as stage_webgpu,
    ):
        mkdocs_hooks.prepare_optional_site_assets()

    generate_gallery.assert_called_once_with(quiet_missing=False, animated_fallbacks=True)
    stage_webgpu.assert_called_once_with()


def test_generated_root_override_isolated_from_shared_build(monkeypatch, tmp_path):
    shared_gallery = tmp_path / "build/gallery-webp/v0.4"
    shared_gallery.mkdir(parents=True)
    marker = shared_gallery / "local-video-assets.json"
    marker.write_text("shared\n", encoding="utf8")
    monkeypatch.setattr(mkdocs_hooks, "ROOT", tmp_path)
    monkeypatch.delenv(mkdocs_hooks.SITE_ASSETS_ENV, raising=False)
    monkeypatch.setenv(mkdocs_hooks.GENERATED_ROOT_ENV, "build/docs-check-generated")

    with mock.patch.object(mkdocs_hooks, "stage_hermetic_gallery_placeholders") as placeholders:
        mkdocs_hooks.prepare_optional_site_assets()

    assert marker.read_text(encoding="utf8") == "shared\n"
    placeholders.assert_called_once_with(
        tmp_path / "build/docs-check-generated/gallery-webp/v0.4"
    )
