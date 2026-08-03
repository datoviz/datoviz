import re
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


def test_panel_render_policy_lives_in_planner() -> None:
    panel_emit = Path("src/scene/scene_emit/panel.c")
    planner_header = Path("src/scene/scene_emit/panel_render_plan.h")
    planner_source = Path("src/scene/scene_emit/panel_render_plan.c")
    assert panel_emit.exists(), "missing panel emitter source"
    assert planner_header.exists(), "missing panel render planner header"
    assert planner_source.exists(), "missing panel render planner source"

    panel_text = panel_emit.read_text(encoding="utf-8")
    planner_header_text = planner_header.read_text(encoding="utf-8")
    planner_source_text = planner_source.read_text(encoding="utf-8")

    assert "DvzPanelRenderPlan" in planner_header_text
    assert "_scene_panel_render_plan_build(" in planner_source_text

    forbidden_panel_policy = (
        "_scene_visual_pass_caps_from_visual(",
        "_scene_technique_gbuffer_enabled(",
        "_scene_technique_ao_state(",
        "_scene_technique_msaa_state(",
        "_scene_technique_edl_state(",
        "_scene_panel_has_visible_scene_occlusion_target(",
        "_scene_panel_has_visible_volume_occlusion_target(",
    )
    for pattern in forbidden_panel_policy:
        assert pattern not in panel_text, f"{panel_emit} owns planner policy {pattern!r}"


def test_effects_use_typed_products_without_legacy_scratch_fallbacks() -> None:
    composition = Path("src/scene/scene_emit/composition.c")
    assert composition.exists(), "missing composition source"
    text = composition.read_text(encoding="utf-8")

    forbidden_scratch_allocation = re.compile(
        r"\bSCRATCH\s*\(\s*DVZ_SCENE_SCRATCH_"
        r"(?:EDL|WBOIT|PEEL|SCENE_OCCLUSION|VOLUME_OCCLUSION)"
    )
    match = forbidden_scratch_allocation.search(text)
    assert match is None, (
        f"{composition} still allocates an effect-specific scratch at {match.start()}"
    )

    assert "legacy_transition" not in text
    assert "_composition_mark_unrealized" not in text


def test_runtime_does_not_infer_composition_identity_from_names() -> None:
    runtime_text = "\n".join(
        path.read_text(encoding="utf-8") for path in Path("src/scene/runtime").glob("*.c")
    )
    forbidden_name_comparison = re.compile(
        r"(?:strcmp|strstr)\s*\([^;\n]*(?:work_label|pipeline_key|shader_key|builtin_variant|suffix)"
    )
    match = forbidden_name_comparison.search(runtime_text)
    assert match is None, "runtime still infers typed composition identity from a name"
    assert "_scene_render_role_work_label" not in runtime_text
