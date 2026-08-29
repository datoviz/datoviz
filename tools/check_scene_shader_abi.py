#!/usr/bin/env python3
"""Check the committed scene shader ABI source layout."""

from __future__ import annotations

import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
SCENE_SHADER_DIR = REPO_ROOT / "src" / "scene" / "shaders"
GLSL_DIR = SCENE_SHADER_DIR / "glsl"
WGSL_DIR = SCENE_SHADER_DIR / "wgsl"

MATERIAL_FIELDS = [
    "params",
    "model",
    "base_color_factor",
    "standard_params",
    "emissive_rim",
    "limb_params",
    "depth_cue",
    "depth_cue_color",
    "depth_cue_extra",
]

# WGSL variants intentionally supported by the current WebGPU-oriented scene slice.
SUPPORTED_WGSL_SHADER_PAIRS = [
    "fixture",
    "texture",
    "point",
    "point_cue",
    "point_style",
    "point_cue_style",
    "pixel",
    "pixel_cue",
    "marker",
    "segment",
    "path",
    "primitive",
    "primitive_lit",
    "image",
    "mesh_textured",
    "sphere",
]

COMMON_GLSL_VERTEX_SHADERS = [
    "point.vert",
    "pixel.vert",
    "point_cue.vert",
    "pixel_cue.vert",
    "point_style.vert",
    "point_cue_style.vert",
    "point_pick.vert",
    "pixel_pick.vert",
    "primitive.vert",
    "primitive_lit.vert",
    "image.vert",
    "marker.vert",
    "segment.vert",
    "sphere.vert",
    "sphere_gbuffer.vert",
    "volume_slice.vert",
]

COMMON_WGSL_VERTEX_SHADERS = [
    "point.vert.wgsl",
    "pixel.vert.wgsl",
    "point_cue.vert.wgsl",
    "pixel_cue.vert.wgsl",
    "point_style.vert.wgsl",
    "point_cue_style.vert.wgsl",
    "marker.vert.wgsl",
    "marker_item_state.vert.wgsl",
    "segment.vert.wgsl",
    "path.vert.wgsl",
    "primitive.vert.wgsl",
    "primitive_lit.vert.wgsl",
    "image.vert.wgsl",
    "mesh_textured.vert.wgsl",
    "sphere.vert.wgsl",
]


def _failures() -> list[str]:
    failures: list[str] = []
    for rel in ["common.glsl", "scene_material.glsl"]:
        if not (GLSL_DIR / rel).exists():
            failures.append(f"missing GLSL shared file: {rel}")
    for rel in ["common.wgsl", "scene_material.wgsl"]:
        if not (WGSL_DIR / rel).exists():
            failures.append(f"missing WGSL shared file: {rel}")

    glsl_common = (GLSL_DIR / "common.glsl").read_text(encoding="utf8")
    wgsl_common = (WGSL_DIR / "common.wgsl").read_text(encoding="utf8")
    if (
        "sceneClipToDeviceClip" not in glsl_common
        or "sceneClip.y = -sceneClip.y" not in glsl_common
        or "sceneClip.z = 0.5 * (sceneClip.z + sceneClip.w)" not in glsl_common
    ):
        failures.append("GLSL scene-to-device clip conversion must lower Y and depth explicitly")
    if (
        "scene_clip_to_device_clip" not in wgsl_common
        or "vec4f(scene_clip.xy, 0.5 * (scene_clip.z + scene_clip.w), scene_clip.w)"
        not in wgsl_common
        or "-scene_clip.y" in wgsl_common
    ):
        failures.append("WGSL scene-to-device clip conversion must lower depth without flipping Y")

    for path in GLSL_DIR.glob("*.vert"):
        if path.name in {"fixture.vert", "fullscreen.vert", "texture.vert"}:
            continue
        for line_number, line in enumerate(path.read_text(encoding="utf8").splitlines(), start=1):
            if "mvp.proj *" in line and "sceneClipToDeviceClip" not in line:
                failures.append(
                    f"raw GLSL scene clip bypasses backend conversion: {path.name}:{line_number}"
                )

    for path in WGSL_DIR.glob("*.vert.wgsl"):
        if path.name in {"fixture.vert.wgsl", "fullscreen.vert.wgsl", "texture.vert.wgsl"}:
            continue
        for line_number, line in enumerate(path.read_text(encoding="utf8").splitlines(), start=1):
            if "mvp.proj *" in line and "scene_clip_to_device_clip" not in line:
                failures.append(
                    f"raw WGSL scene clip bypasses backend conversion: {path.name}:{line_number}"
                )

    glsl_material = (GLSL_DIR / "scene_material.glsl").read_text(encoding="utf8")
    wgsl_material = (WGSL_DIR / "scene_material.wgsl").read_text(encoding="utf8")
    glsl_fields = [
        "params",
        "model",
        "baseColorFactor",
        "standardParams",
        "emissiveRim",
        "limbParams",
        "depthCue",
        "depthCueColor",
        "depthCueExtra",
    ]
    if any(
        glsl_material.find(field) >= glsl_material.find(next_field)
        for field, next_field in zip(glsl_fields, glsl_fields[1:])
    ):
        failures.append("GLSL SceneMaterial field order does not match the C payload")
    if any(
        wgsl_material.find(field) >= wgsl_material.find(next_field)
        for field, next_field in zip(MATERIAL_FIELDS, MATERIAL_FIELDS[1:])
    ):
        failures.append("WGSL SceneMaterial field order does not match the C payload")
    if "layout(set = 1, binding = 4) uniform ScenePanelLights" not in glsl_material:
        failures.append("GLSL ScenePanelLights is not declared at set 1 binding 4")
    if "@group(1) @binding(4) var<uniform> panel_lights: ScenePanelLights;" not in wgsl_material:
        failures.append("WGSL ScenePanelLights is not declared at group 1 binding 4")
    if "active_count: vec4u" not in wgsl_material:
        failures.append("WGSL ScenePanelLights active-count lane is missing or uses an invalid name")
    for text, language in [(glsl_material, "GLSL"), (wgsl_material, "WGSL")]:
        if "SceneLight" not in text or "ScenePanelLights" not in text:
            failures.append(f"{language} panel-light payload declarations are incomplete")

    for name in COMMON_GLSL_VERTEX_SHADERS:
        text = (GLSL_DIR / name).read_text(encoding="utf8")
        if '#include "common.glsl"' not in text:
            failures.append(f"missing common.glsl include: src/scene/shaders/glsl/{name}")

    for name in COMMON_WGSL_VERTEX_SHADERS:
        text = (WGSL_DIR / name).read_text(encoding="utf8")
        if '#include "common.wgsl"' not in text:
            failures.append(f"missing common.wgsl include: src/scene/shaders/wgsl/{name}")

    for stem in SUPPORTED_WGSL_SHADER_PAIRS:
        for stage in ["vert", "frag"]:
            if not (WGSL_DIR / f"{stem}.{stage}.wgsl").exists():
                failures.append(f"missing supported WGSL shader: {stem}.{stage}.wgsl")

    for stage in ["vert", "frag"]:
        name = f"primitive_query_u32.{stage}.wgsl"
        text = (WGSL_DIR / name).read_text(encoding="utf8")
        if "@location(0) @interpolate(flat) id: u32" not in text:
            failures.append(f"integer query id is not flat-interpolated: {name}")

    image_frag = (GLSL_DIR / "image.frag").read_text(encoding="utf8")
    if "uniform sampler2D tex" in image_frag:
        failures.append("image.frag still uses combined sampler2D binding")
    if "uniform texture2D tex" not in image_frag or "uniform sampler samp" not in image_frag:
        failures.append("image.frag does not use split texture/sampler bindings")
    return failures


def main() -> int:
    failures = _failures()
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        return 1
    print("scene shader ABI check passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
