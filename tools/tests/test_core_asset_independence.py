import sys
from pathlib import Path


sys.path.insert(0, str(Path(__file__).parents[1]))

import build_examples_manifest
import build_gallery


ROOT = Path(__file__).parents[2]
CORE_WORKFLOWS = (
    ROOT / ".github/workflows/test.yml",
    ROOT / ".github/workflows/wheels.yml",
    ROOT / ".github/workflows/wheels-macos-x86_64.yml",
)


def test_core_workflows_do_not_hydrate_data_or_lfs():
    forbidden = (
        "submodules: recursive",
        "git lfs",
        "lfs: true",
        "materialize_lfs_assets",
        "data/assets/",
        "data/examples/",
    )
    for path in CORE_WORKFLOWS:
        text = path.read_text(encoding="utf8")
        for token in forbidden:
            assert token not in text, f"{path.relative_to(ROOT)} contains {token!r}"


def test_fileio_tests_use_hermetic_fixtures():
    path = ROOT / "src/fileio/tests/test_fileio.c"
    text = path.read_text(encoding="utf8")
    assert '"data/' not in text
    assert "tst_skip(suite, \"NPY fixture missing\")" not in text
    assert "tst_skip(suite, \"earth JPEG fixture missing\")" not in text


def test_obsolete_lfs_materializer_is_absent():
    assert not (ROOT / "tools/materialize_lfs_assets.sh").exists()


def test_generated_gallery_contract_does_not_require_capture_files(tmp_path):
    manifest = build_gallery.load_manifest(build_gallery.DEFAULT_MANIFEST)
    example = next(
        example
        for example in build_gallery.collect_examples(manifest)
        if example.screenshot_expected
    )
    block = build_gallery.media_block(
        "index.md",
        example,
        tmp_path / "absent-gallery",
        build_gallery.DEFAULT_IMAGE_URL_BASE,
    )
    assert "Screenshot pending" not in block
    assert build_gallery.image_url(
        "index.md", example, build_gallery.DEFAULT_IMAGE_URL_BASE, "webp"
    ) in block
    assert build_examples_manifest._media_fields(example)["screenshot"]["status"] == "available"
