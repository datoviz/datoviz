from pathlib import Path


ROOTS = (Path("src"), Path("testing"), Path("examples"))

SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".h", ".hpp"}


def _is_under(path: Path, parent: Path) -> bool:
    try:
        path.relative_to(parent)
    except ValueError:
        return False
    return True


def test_untyped_visual_compatibility_is_fixture_only() -> None:
    patterns = (
        "dvz_frame_plan_render_allow_untyped_visual" "_compat(",
        "allow_untyped_visual" "_compat",
        "_scene_untyped" "_compat",
    )
    checked = False

    for root in ROOTS:
        if not root.exists():
            continue
        for path in sorted(root.rglob("*")):
            if path.suffix not in SOURCE_SUFFIXES:
                continue
            checked = True

            text = path.read_text(encoding="utf-8")
            for pattern in patterns:
                assert pattern not in text, f"{path} enables untyped visual compatibility"

    assert checked, "scene architecture source guard did not check any source files"
