import sys
from pathlib import Path

import pytest


sys.path.insert(0, str(Path(__file__).parents[1]))

pytest.importorskip("mkdocs")

import mkdocs_hooks


def test_mounted_site_keeps_media_links_and_redirects_under_prefix():
    html = '''<picture><source srcset="/assets/animated.webp"><img src="/assets/course.webp"></picture><video poster='/assets/poster.webp'><source data-src="/assets/movie.mp4"></video><a href="/examples/">Examples</a><meta http-equiv="refresh" content="0; url=/examples/">'''
    actual = mkdocs_hooks.on_post_page(html, {'site_url': 'https://example.invalid/docs/'})
    assert actual == html.replace('"/assets/', '"/docs/assets/').replace("'/assets/", "'/docs/assets/").replace('"/examples/', '"/docs/examples/').replace('url=/examples/', 'url=/docs/examples/')


def test_mounted_site_preserves_relative_external_and_already_prefixed_urls():
    html = '''<a href="../examples/">Relative</a><a href="https://example.com/">External</a><img src="//cdn.example.com/image.webp"><a href="/docs/examples/">Mounted</a><code>&lt;img src=&quot;/example.webp&quot;&gt;</code>'''
    assert mkdocs_hooks.on_post_page(html, {'site_url': 'https://example.invalid/docs/'}) == html
    assert mkdocs_hooks.on_post_page(html, {'site_url': 'https://example.invalid/'}) == html


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
