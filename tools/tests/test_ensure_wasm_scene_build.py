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
