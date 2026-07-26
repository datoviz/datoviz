#!/usr/bin/env python3
"""Check the compiled RC3 Vulkan tutorial pilot for source/documentation drift."""

from __future__ import annotations

from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
EXAMPLE = ROOT / "examples" / "c" / "tutorial"
DOCS = ROOT / "docs" / "tutorials" / "vulkan"

CHAPTERS = {
    "first_triangle": ("first-triangle.md", "first-triangle.webp", "shaders", False),
    "shaders_and_pipeline": (
        "shaders-and-pipeline.md",
        "shaders-and-pipeline.webp",
        "shaders/pipeline",
        False,
    ),
    "vertex_buffers": ("vertex-buffers.md", "vertex-buffers.webp", "shaders/vertex_buffer", True),
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

    for token in (
        "indexed_depth_spike",
        "DVZ_TUTORIAL_USE_INDEXED_DEPTH=1",
        'DVZ_TUTORIAL_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders/depth"',
    ):
        _require(cmake, token, "examples/c/tutorial/CMakeLists.txt", errors)
    for token in (
        "canvas_config.depth_format = VK_FORMAT_D32_SFLOAT",
        "frame->depth_valid",
        "frame->depth_view",
        "dvz_graphics_attachment_depth(",
        "dvz_graphics_depth(",
        "dvz_cmd_bind_index_buffer(",
        "dvz_cmd_draw_indexed(",
    ):
        _require(source, token, "examples/c/tutorial/triangle.c", errors)
    for suffix in ("vert", "frag"):
        depth_shader = EXAMPLE / "shaders" / "depth" / f"vklite_triangle.{suffix}"
        if not depth_shader.is_file():
            errors.append(f"missing depth spike shader {depth_shader.relative_to(ROOT)}")

    for token in (
        "texture_upload_spike",
        "DVZ_TUTORIAL_USE_TEXTURE_UPLOAD=1",
        'DVZ_TUTORIAL_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders/texture"',
    ):
        _require(cmake, token, "examples/c/tutorial/CMakeLists.txt", errors)
    for token in (
        "VK_FORMAT_R8G8B8A8_SRGB",
        "VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL",
        "VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL",
        "VK_ACCESS_2_SHADER_SAMPLED_READ_BIT",
        "dvz_barrier_image_stage(",
        "dvz_barrier_image_access(",
        "dvz_barrier_image_layout(",
        "dvz_cmd_copy_buffer_to_image(",
        "dvz_sampler_create(",
        "dvz_slots_binding(",
        "dvz_descriptors_image(",
        "dvz_cmd_bind_descriptors(",
    ):
        _require(source, token, "examples/c/tutorial/triangle.c", errors)
    for suffix in ("vert", "frag"):
        texture_shader = EXAMPLE / "shaders" / "texture" / f"vklite_triangle.{suffix}"
        if not texture_shader.is_file():
            errors.append(f"missing texture spike shader {texture_shader.relative_to(ROOT)}")

    for token in (
        "arcball_spike",
        "DVZ_TUTORIAL_USE_ARCBALL=1",
        'DVZ_TUTORIAL_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders/arcball"',
    ):
        _require(cmake, token, "examples/c/tutorial/CMakeLists.txt", errors)
    for token in (
        "dvz_canvas_input(",
        "dvz_arcball_create(",
        "dvz_arcball_connect(",
        "dvz_camera_resize(",
        "dvz_camera_mvp(",
        "dvz_arcball_mvp(",
        "dvz_slots_push(",
        "dvz_cmd_push_constants(",
        "dvz_arcball_disconnect(",
    ):
        _require(source, token, "examples/c/tutorial/triangle.c", errors)
    for suffix in ("vert", "frag"):
        arcball_shader = EXAMPLE / "shaders" / "arcball" / f"vklite_triangle.{suffix}"
        if not arcball_shader.is_file():
            errors.append(f"missing arcball spike shader {arcball_shader.relative_to(ROOT)}")

    for target, (doc_name, preview_name, shader_dir, uses_vertex_buffer) in CHAPTERS.items():
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
        _require(
            document,
            f"../../assets/tutorials/vulkan/{preview_name}",
            str(doc_path.relative_to(ROOT)),
            errors,
        )

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
