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


@pytest.mark.parametrize('base', ['', 'http://example.org/assets', 'https://user@example.org/assets',
    'https://example.org/assets?q=1', 'https://example.org/assets#tag', 'https:///assets',
    'https://example.org/has space'])
def test_homepage_publication_base_requires_clean_https(base, monkeypatch):
    monkeypatch.setenv(mkdocs_hooks.HOMEPAGE_ASSET_BASE_ENV, base)
    with pytest.raises(ValueError, match=mkdocs_hooks.HOMEPAGE_ASSET_BASE_ENV):
        mkdocs_hooks.homepage_asset_base_url()


def test_homepage_publication_urls_rewrite_media_attributes_only(monkeypatch):
    base = 'https://github.com/datoviz/assets/releases/download/homepage-sha256-test'
    monkeypatch.setenv(mkdocs_hooks.HOMEPAGE_ASSET_BASE_ENV, base + '/')
    prefix = '/assets/gallery/v0.4/profiles/homepage/showcases/'
    html = (f'<video poster="{prefix}cloud.poster.webp"><source data-src="{prefix}cloud.av1.mp4">'
            f'<source src="/docs{prefix}cloud.mp4"></video>'
            f'<img src="../assets/gallery/v0.4/profiles/homepage/showcases/cloud.poster.webp" '
            f'srcset="{prefix}cloud.poster.webp 1x, {prefix}cloud2.poster.webp 2x">'
            '<video poster="/assets/gallery/v0.4/showcases/other.poster.webp"></video>'
            '<img src="https://other.example/assets/gallery/v0.4/profiles/homepage/showcases/cloud.webp">'
            '<a href="/examples/">Examples</a>')
    result = mkdocs_hooks.on_post_page(html, {'site_url': 'https://example.org/docs/'})
    assert f'poster="{base}/cloud.poster.webp"' in result
    assert f'data-src="{base}/cloud.av1.mp4"' in result
    assert f'src="{base}/cloud.mp4"' in result
    assert f'srcset="{base}/cloud.poster.webp 1x, {base}/cloud2.poster.webp 2x"' in result
    assert 'poster="/docs/assets/gallery/v0.4/showcases/other.poster.webp"' in result
    assert 'src="https://other.example/assets/gallery/v0.4/profiles/homepage/showcases/cloud.webp"' in result
    assert 'href="/docs/examples/"' in result
    assert mkdocs_hooks.on_post_page(f'<img src="{prefix}cloud.webp">', {'site_url': 'https://example.org/'}) == f'<img src="{base}/cloud.webp">'


def test_homepage_publication_ignores_nested_or_empty_media_names(monkeypatch):
    monkeypatch.setenv(mkdocs_hooks.HOMEPAGE_ASSET_BASE_ENV, 'https://cdn.example.org/release')
    html = ('<img src="/assets/gallery/v0.4/profiles/homepage/showcases/">'
            '<img src="/assets/gallery/v0.4/profiles/homepage/showcases/nested/file.webp">')
    assert mkdocs_hooks.on_post_page(html, {'site_url': 'https://example.org/'}) == html


def test_homepage_publication_skips_file_registration_and_postbuild_copy(monkeypatch, tmp_path):
    from types import SimpleNamespace
    stage = tmp_path / 'stage'
    gallery = stage / 'gallery/v0.4'
    homepage = gallery / 'profiles/homepage/showcases'
    homepage.mkdir(parents=True)
    (homepage / 'cloud.av1.mp4').write_bytes(b'av1')
    (homepage / 'cloud.mp4').write_bytes(b'h264')
    (homepage / 'cloud.poster.webp').write_bytes(b'poster')
    ordinary = gallery / 'showcases/other.mp4'
    ordinary.parent.mkdir()
    ordinary.write_bytes(b'ordinary')
    monkeypatch.setattr(mkdocs_hooks, 'ROOT', tmp_path)
    monkeypatch.setenv(mkdocs_hooks.ASSET_STAGE_ENV, str(stage))
    monkeypatch.setattr(mkdocs_hooks.File, 'generated', lambda config, uri, **kwargs: SimpleNamespace(src_uri=uri))
    config = {'site_dir': str(tmp_path / 'site'), 'site_url': 'https://example.org/'}
    monkeypatch.setenv(mkdocs_hooks.HOMEPAGE_ASSET_BASE_ENV, 'https://cdn.example.org/release')
    files = mkdocs_hooks.on_files([], config)
    assert [item.src_uri for item in files] == ['assets/gallery/v0.4/showcases/other.mp4']
    # A previous private build must not leave homepage binary products behind.
    old = Path(config['site_dir']) / 'assets/gallery/v0.4/profiles/homepage/stale.mp4'
    old.parent.mkdir(parents=True)
    old.write_bytes(b'stale')
    mkdocs_hooks.on_post_build(config)
    assert not (Path(config['site_dir']) / 'assets/gallery/v0.4/profiles/homepage').exists()
    assert (Path(config['site_dir']) / 'assets/gallery/v0.4/showcases/other.mp4').read_bytes() == b'ordinary'
    monkeypatch.delenv(mkdocs_hooks.HOMEPAGE_ASSET_BASE_ENV)
    files = mkdocs_hooks.on_files([], config)
    assert len(files) == 4
    mkdocs_hooks.on_post_build(config)
    assert (Path(config['site_dir']) / 'assets/gallery/v0.4/profiles/homepage/showcases/cloud.av1.mp4').read_bytes() == b'av1'
    authored = '<source data-src="/assets/gallery/v0.4/profiles/homepage/showcases/cloud.av1.mp4">'
    assert mkdocs_hooks.on_post_page(authored, config) == authored
