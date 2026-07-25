#!/usr/bin/env python3
"""Check the compiled RC3 Vulkan tutorial pilot for source/documentation drift."""

from __future__ import annotations

from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
EXAMPLE = ROOT / "examples" / "c" / "tutorial"
DOCS = ROOT / "docs" / "tutorials" / "vulkan"

CHAPTERS = {
    "first_triangle": ("first-triangle.md", "shaders", False),
    "shaders_and_pipeline": ("shaders-and-pipeline.md", "shaders/pipeline", False),
    "vertex_buffers": ("vertex-buffers.md", "shaders/vertex_buffer", True),
}


def _require(text: str, token: str, label: str, errors: list[str]) -> None:
    if token not in text:
        errors.append(f"{label}: missing {token!r}")


def main() -> int:
    errors: list[str] = []
    cmake_path = EXAMPLE / "CMakeLists.txt"
    source_path = EXAMPLE / "triangle.c"
    if not cmake_path.is_file():
        errors.append("missing standalone tutorial CMakeLists.txt")
    if not source_path.is_file():
        errors.append("missing canonical tutorial source triangle.c")
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1

    cmake = cmake_path.read_text()
    source = source_path.read_text()
    for token in (
        "find_package(datoviz CONFIG REQUIRED)",
        "datoviz::datoviz",
        "DVZ_TUTORIAL_SHADER_DIR",
        "DVZ_TUTORIAL_USE_VERTEX_BUFFER",
    ):
        _require(cmake, token, "examples/c/tutorial/CMakeLists.txt", errors)

    for token in (
        "dvz_read_text(",
        "dvz_shader_compile(",
        "dvz_shader_compile_result_destroy(",
        "dvz_commands_wrap_borrowed_recording(",
        "dvz_commands_unwrap(",
        "dvz_cmd_set_viewport_scissor(",
        "dvz_buffer_upload(",
        "dvz_graphics_vertex_binding(",
        "dvz_cmd_bind_vertex_buffers(",
        "--offscreen",
        "--live",
        "--shader-dir",
        "--vertex-buffer",
    ):
        _require(source, token, "examples/c/tutorial/triangle.c", errors)
    for forbidden in ("dvz_compile_glsl(", "VERT_GLSL", "FRAG_GLSL"):
        if forbidden in source:
            errors.append(
                "examples/c/tutorial/triangle.c: forbidden legacy or inline shader token "
                f"{forbidden!r}"
            )

    for target, (doc_name, shader_dir, uses_vertex_buffer) in CHAPTERS.items():
        cmake_label = "examples/c/tutorial/CMakeLists.txt"
        _require(cmake, f"add_tutorial_chapter({target} {shader_dir} ", cmake_label, errors)
        expected_flag = "1)" if uses_vertex_buffer else "0)"
        _require(
            cmake,
            f"add_tutorial_chapter({target} {shader_dir} {expected_flag}",
            cmake_label,
            errors,
        )

        doc_path = DOCS / doc_name
        if not doc_path.is_file():
            errors.append(f"missing tutorial chapter {doc_path.relative_to(ROOT)}")
            continue
        document = doc_path.read_text()
        for token in (target, "--live", "--offscreen", "--validate", "## Checkpoint", "## Exercise"):
            _require(document, token, str(doc_path.relative_to(ROOT)), errors)

        directory = EXAMPLE / shader_dir
        for suffix in ("vert", "frag"):
            shader_path = directory / f"vklite_triangle.{suffix}"
            if not shader_path.is_file():
                errors.append(f"missing shader {shader_path.relative_to(ROOT)}")
                continue
            shader = shader_path.read_text()
            _require(shader, "#version 450", str(shader_path.relative_to(ROOT)), errors)
            _require(shader, "void main()", str(shader_path.relative_to(ROOT)), errors)

    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    print("Vulkan tutorial pilot source/documentation synchronization: OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
