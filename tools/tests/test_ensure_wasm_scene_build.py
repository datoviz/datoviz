import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

import ensure_wasm_scene_build as wasm_build  # noqa: E402


def test_wasm_inputs_include_linked_examples_and_exclude_tutorial() -> None:
    inputs = set(wasm_build.iter_inputs())
    cmake = (ROOT / "src/wasm/CMakeLists.txt").read_text()
    linked_examples = {
        (ROOT / path).resolve()
        for path in re.findall(r"\$\{PROJECT_SOURCE_DIR\}/(examples/c/[^\s)]+)", cmake)
    }

    assert (ROOT / "examples/c/features/controller_arcball.c").resolve() in inputs
    assert (ROOT / "examples/c/example_common.h").resolve() in inputs
    assert linked_examples <= inputs
    assert (ROOT / "examples/c/tutorial/triangle.c").resolve() not in inputs


def test_wasm_input_hash_ignores_order_and_tracks_contents(
    tmp_path: Path, monkeypatch
) -> None:
    monkeypatch.setattr(wasm_build, "ROOT", tmp_path)
    first = tmp_path / "first.c"
    second = tmp_path / "second.h"
    first.write_text("first\n")
    second.write_text("second\n")

    digest = wasm_build.input_hash([first, second])
    assert wasm_build.input_hash([second, first]) == digest

    second.write_text("changed\n")
    assert wasm_build.input_hash([first, second]) != digest


def test_wasm_inputs_track_embedded_wgsl_changes(tmp_path: Path, monkeypatch) -> None:
    shader_root = ROOT / "src/scene/shaders/wgsl"
    embedded_shaders = set(shader_root.glob("*.wgsl"))
    assert embedded_shaders
    assert embedded_shaders <= set(wasm_build.iter_inputs())

    monkeypatch.setattr(wasm_build, "ROOT", tmp_path)
    monkeypatch.setattr(wasm_build, "INPUT_ROOTS", (tmp_path / "src",))
    monkeypatch.setattr(wasm_build, "WASM_EXAMPLE_INPUT_ROOTS", ())
    monkeypatch.setattr(wasm_build, "WASM_EXAMPLE_SHARED_INPUTS", ())
    shader = tmp_path / "src/scene/shaders/wgsl/scene_material.wgsl"
    shader.parent.mkdir(parents=True)
    shader.write_text("fn shade() -> f32 { return 1.0; }\n")
    digest = wasm_build.input_hash(wasm_build.iter_inputs())
    shader.write_text("fn shade() -> f32 { return 0.5; }\n")
    assert wasm_build.input_hash(wasm_build.iter_inputs()) != digest


def test_resolved_asset_and_compiler_header_changes_invalidate_provenance(tmp_path, monkeypatch):
    monkeypatch.setattr(wasm_build, "ROOT", tmp_path)
    monkeypatch.setattr(wasm_build, "iter_inputs", lambda: [])
    build_dir = tmp_path / "build-wasm-scene"
    (build_dir / "wasm").mkdir(parents=True)
    asset = tmp_path / "asset.bin"
    font = tmp_path / "font.ttf"
    header = tmp_path / "external.h"
    for path in (asset, font, header):
        path.write_bytes(b"original")
    (build_dir / "datoviz-wasm-resolved-inputs.txt").write_text(f"{asset}\n{font}\n")
    (build_dir / "example.o.d").write_text(f"example.o: {header}\n")
    (build_dir / "compile_commands.json").write_text("[]")
    first = wasm_build.provenance(build_dir)["input_hash"]
    for path in (asset, font, header):
        path.write_bytes(b"modified")
        updated = wasm_build.provenance(build_dir)["input_hash"]
        assert updated != first
        first = updated


def test_cmake_preloads_have_actual_relink_dependencies():
    cmake = (ROOT / "src/wasm/CMakeLists.txt").read_text()
    helper = (ROOT / "cmake/DvzWasmAssets.cmake").read_text()
    assert "APPEND PROPERTY LINK_DEPENDS ${inputs}" in helper
    assert 'GLOB_RECURSE inputs CONFIGURE_DEPENDS' in helper
    assert "SHELL:--preload-file" not in cmake


def test_preload_byte_change_relinks_and_repackages_actual_wasm(tmp_path):
    import subprocess
    source = tmp_path / "source"
    source.mkdir()
    (source / "main.c").write_text("int main(void) { return 0; }\n")
    payload = source / "payload.bin"
    payload.write_bytes(b"original payload")
    (source / "CMakeLists.txt").write_text(
        'cmake_minimum_required(VERSION 3.20)\nproject(preload_regression C)\n'
        'add_executable(datoviz_wasm_scene main.c)\n'
        'set_target_properties(datoviz_wasm_scene PROPERTIES SUFFIX ".js")\n'
        f'include("{ROOT / "cmake/DvzWasmAssets.cmake"}")\n'
        'dvz_wasm_preload("${CMAKE_CURRENT_SOURCE_DIR}/payload.bin" "/payload.bin")\n')
    build = tmp_path / "build"
    subprocess.run(["emcmake", "cmake", "-S", str(source), "-B", str(build)], check=True,
                   capture_output=True)
    subprocess.run(["cmake", "--build", str(build)], check=True, capture_output=True)
    data = build / "datoviz_wasm_scene.data"
    assert data.read_bytes() == b"original payload"
    payload.write_bytes(b"different bytes and length")
    subprocess.run(["cmake", "--build", str(build)], check=True, capture_output=True)
    assert data.read_bytes() == b"different bytes and length"


def test_complete_runtime_outputs_must_match_successful_receipt(tmp_path):
    from write_wasm_asset_manifest import ARTIFACT_NAMES, build_manifest
    for index, name in enumerate(ARTIFACT_NAMES):
        (tmp_path / name).write_bytes(bytes([index]))
    stamp = {"outputs": build_manifest(tmp_path)["artifacts"]}
    assert wasm_build.outputs_match(tmp_path, stamp)
    (tmp_path / ARTIFACT_NAMES[1]).write_bytes(b"altered runtime")
    assert not wasm_build.outputs_match(tmp_path, stamp)
    (tmp_path / ARTIFACT_NAMES[1]).unlink()
    assert not wasm_build.outputs_match(tmp_path, stamp)
