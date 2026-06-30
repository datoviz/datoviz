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


def test_panel_emit_routes_generated_visuals_from_attachment_policy() -> None:
    panel_emit = Path("src/scene/scene_emit/panel.c")
    assert panel_emit.exists(), "missing panel emitter source"
    text = panel_emit.read_text(encoding="utf-8")

    required_patterns = (
        "_scene_visual_generated_policy(",
        "attach->has_generated_role",
        "attach->generated_role",
        "_scene_visual_explicit_clip_rect(",
        "_scene_visual_explicit_viewport_rect(",
    )
    for pattern in required_patterns:
        assert pattern in text, f"{panel_emit} is missing attachment routing pattern {pattern!r}"

    forbidden_pointer_scans = (
        "background_visual",
        "border_visual",
        "axis->",
        "colorbar->",
        "legend->",
        "scalebar->",
        "bounds_visual",
        "_scene_visual_is_axis",
        "_scene_visual_is_colorbar",
        "_scene_visual_is_legend",
    )
    for pattern in forbidden_pointer_scans:
        assert pattern not in text, f"{panel_emit} contains generated-visual pointer scan {pattern!r}"
