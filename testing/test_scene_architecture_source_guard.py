from pathlib import Path


ROOTS = (Path("src"), Path("testing"), Path("examples"))

ALLOWED_UNTYPED_VISUAL_PATHS = {
    Path("src/scene/frame_plan/frame_plan.h"),
    Path("src/scene/frame_plan/passes.c"),
}

ALLOWED_UNTYPED_VISUAL_DIRS = {
    Path("src/scene/tests"),
}

SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".h", ".hpp"}


def _is_under(path: Path, parent: Path) -> bool:
    try:
        path.relative_to(parent)
    except ValueError:
        return False
    return True


def test_untyped_visual_compatibility_is_fixture_only() -> None:
    pattern = "dvz_frame_plan_render_allow_untyped_visuals("
    checked = False

    for root in ROOTS:
        if not root.exists():
            continue
        for path in sorted(root.rglob("*")):
            if path.suffix not in SOURCE_SUFFIXES:
                continue
            checked = True
            if path in ALLOWED_UNTYPED_VISUAL_PATHS:
                continue
            if any(_is_under(path, allowed) for allowed in ALLOWED_UNTYPED_VISUAL_DIRS):
                continue

            text = path.read_text(encoding="utf-8")
            assert pattern not in text, (
                f"{path} enables untyped visual compatibility outside explicit scene fixtures"
            )

    assert checked, "scene architecture source guard did not check any source files"
