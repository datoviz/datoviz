#!/usr/bin/env python3
"""Check the committed scene shader ABI source layout."""

from __future__ import annotations

import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
SCENE_SHADER_DIR = REPO_ROOT / "src" / "scene" / "shaders"
GLSL_DIR = SCENE_SHADER_DIR / "glsl"
WGSL_DIR = SCENE_SHADER_DIR / "wgsl"

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
    "primitive",
    "primitive_lit",
    "image",
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
    "primitive.vert.wgsl",
    "primitive_lit.vert.wgsl",
    "image.vert.wgsl",
]


def _failures() -> list[str]:
    failures: list[str] = []
    for rel in ["common.glsl", "scene_material.glsl"]:
        if not (GLSL_DIR / rel).exists():
            failures.append(f"missing GLSL shared file: {rel}")
    for rel in ["common.wgsl", "scene_material.wgsl"]:
        if not (WGSL_DIR / rel).exists():
            failures.append(f"missing WGSL shared file: {rel}")

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
