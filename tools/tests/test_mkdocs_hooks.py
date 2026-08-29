import sys
from pathlib import Path

import pytest


sys.path.insert(0, str(Path(__file__).parents[1]))

pytest.importorskip("mkdocs")

import mkdocs_hooks


def test_asset_stage_is_explicit(monkeypatch, tmp_path):
    monkeypatch.setattr(mkdocs_hooks, "ROOT", tmp_path)
    monkeypatch.delenv(mkdocs_hooks.ASSET_STAGE_ENV, raising=False)

    with pytest.raises(ValueError, match=mkdocs_hooks.ASSET_STAGE_ENV):
        mkdocs_hooks.asset_stage_root()


def test_asset_stage_must_be_prepared(monkeypatch, tmp_path):
    monkeypatch.setattr(mkdocs_hooks, "ROOT", tmp_path)
    monkeypatch.setenv(mkdocs_hooks.ASSET_STAGE_ENV, "build/docs-assets/local")

    with pytest.raises(FileNotFoundError, match="asset stage not found"):
        mkdocs_hooks.asset_stage_root()


def test_gallery_and_webgpu_paths_are_read_from_stage(monkeypatch, tmp_path):
    stage = tmp_path / "build/docs-assets/publish"
    stage.mkdir(parents=True)
    monkeypatch.setattr(mkdocs_hooks, "ROOT", tmp_path)
    monkeypatch.setenv(mkdocs_hooks.ASSET_STAGE_ENV, "build/docs-assets/publish")

    assert mkdocs_hooks.gallery_output_path() == stage / "gallery/v0.4"
    assert mkdocs_hooks.webgpu_output_path() == stage / "webgpu-data"


def test_mkdocs_hook_has_no_gallery_or_webgpu_preparation_entry_point():
    assert not hasattr(mkdocs_hooks, "prepare_optional_site_assets")
    assert not hasattr(mkdocs_hooks, "stage_hermetic_gallery_placeholders")
