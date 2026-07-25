from pathlib import Path

import pytest

from tools.stage_docs_deployment import stage_deployment


def _site_repo(tmp_path: Path) -> Path:
    site_repo = tmp_path / "website"
    (site_repo / ".git").mkdir(parents=True)
    (site_repo / "CNAME").write_text("datoviz.org\n", encoding="utf8")
    (site_repo / "index.html").write_text(
        '<link rel="canonical" href="https://datoviz.org/">'
        '<a href="/guide/"><img src="/images/old.png"></a>',
        encoding="utf8",
    )
    (site_repo / "images").mkdir()
    (site_repo / "images" / "old.png").write_bytes(b"old")
    return site_repo


def _built_site(tmp_path: Path) -> Path:
    built = tmp_path / "built"
    built.mkdir()
    (built / "CNAME").write_text("datoviz.org\n", encoding="utf8")
    (built / "index.html").write_text("v0.4", encoding="utf8")
    (built / ".DS_Store").write_bytes(b"metadata")
    return built


def test_seed_legacy_archive(tmp_path: Path) -> None:
    site_repo = _site_repo(tmp_path)
    built = _built_site(tmp_path)
    output = tmp_path / "output"

    stage_deployment(built, site_repo, output, legacy_prefix="v0.3", seed_legacy=True)

    assert (output / "index.html").read_text(encoding="utf8") == "v0.4"
    assert (output / "CNAME").read_text(encoding="utf8") == "datoviz.org\n"
    assert (output / ".nojekyll").is_file()
    assert not (output / ".DS_Store").exists()
    assert not (output / "v0.3" / "CNAME").exists()
    legacy = (output / "v0.3" / "index.html").read_text(encoding="utf8")
    assert 'href="https://datoviz.org/v0.3/"' in legacy
    assert 'href="/v0.3/guide/"' in legacy
    assert 'src="/v0.3/images/old.png"' in legacy


def test_preserve_existing_legacy_archive(tmp_path: Path) -> None:
    site_repo = _site_repo(tmp_path)
    (site_repo / "v0.3").mkdir()
    (site_repo / "v0.3" / "index.html").write_text("preserved", encoding="utf8")
    output = tmp_path / "output"

    stage_deployment(
        _built_site(tmp_path), site_repo, output, legacy_prefix="v0.3", seed_legacy=False
    )

    assert (output / "v0.3" / "index.html").read_text(encoding="utf8") == "preserved"


def test_refuse_missing_legacy_without_seed(tmp_path: Path) -> None:
    with pytest.raises(ValueError, match="--seed-legacy"):
        stage_deployment(
            _built_site(tmp_path),
            _site_repo(tmp_path),
            tmp_path / "output",
            legacy_prefix="v0.3",
            seed_legacy=False,
        )


def test_refuse_missing_gallery_media(tmp_path: Path) -> None:
    built = _built_site(tmp_path)
    (built / "index.html").write_text(
        '<video poster="/assets/gallery/v0.4/showcases/example.poster.webp">'
        '<source data-src="/assets/gallery/v0.4/showcases/example.mp4">',
        encoding="utf8",
    )

    with pytest.raises(ValueError, match="example.mp4"):
        stage_deployment(
            built,
            _site_repo(tmp_path),
            tmp_path / "output",
            legacy_prefix="v0.3",
            seed_legacy=True,
        )


def test_refuse_missing_tutorial_media(tmp_path: Path) -> None:
    built = _built_site(tmp_path)
    (built / "index.html").write_text(
        '<img src="/assets/tutorials/vulkan/first-triangle.webp">',
        encoding="utf8",
    )

    with pytest.raises(ValueError, match="first-triangle.webp"):
        stage_deployment(
            built,
            _site_repo(tmp_path),
            tmp_path / "output",
            legacy_prefix="v0.3",
            seed_legacy=True,
        )
