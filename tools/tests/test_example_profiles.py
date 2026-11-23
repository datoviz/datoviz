"""Manifest propagation and actual native/WASM shared presentation semantics."""

import copy
import json
from pathlib import Path
import subprocess
import sys

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import example_profiles as profiles
import gallery_frames
import gallery_media


ROOT = profiles.ROOT


def cortical():
    return next(entry for entry in gallery_media.load_manifest()["examples"]
                if entry["id"] == "showcases_cortical_activity")


def test_one_manifest_change_updates_native_browser_and_capture(tmp_path, monkeypatch):
    entry = cortical()
    original = profiles.generated_files({"examples": [entry]})
    entry["media"]["profiles"]["homepage"]["presentation"]["amplitude_deg"] = 17
    updated = profiles.generated_files({"examples": [entry]})
    assert all(updated[path] != original[path] for path in original)
    assert profiles.live_profiles({"examples": [entry]})[entry["id"]]["amplitudeDeg"] == 17
    monkeypatch.setattr(gallery_media, "load_manifest", lambda *_: {"examples": [entry]})
    monkeypatch.setattr(gallery_frames, "ROOT", tmp_path)
    preview = gallery_frames.collect_previews(tmp_path / "manifest", tmp_path, set(), set(), "homepage")[0]
    commands = []
    monkeypatch.setattr(gallery_frames.subprocess, "run", lambda cmd, **_: commands.append(cmd))
    gallery_frames.capture_sequence(preview, tmp_path / "frames")
    command = commands[0]
    assert command[command.index("--preview-presentation-amplitude-deg") + 1] == "17"
    assert command[command.index("--preview-presentation-period") + 1] == "14.4"


def test_generated_outputs_current_and_unselected_profiles_do_not_change_live_defaults():
    manifest = gallery_media.load_manifest()
    for path, content in profiles.generated_files(manifest).items():
        assert path.read_text() == content
    changed = copy.deepcopy(manifest)
    protein = next(entry for entry in changed["examples"] if entry["id"] == "showcases_protein")
    protein["media"]["profiles"]["homepage"]["presentation"]["amplitude_deg"] = 20
    assert profiles.live_profiles(changed) == profiles.live_profiles(manifest)


def test_native_and_real_wasm_execute_same_presentation_state(tmp_path):
    source = ROOT / "tools/tests/fixtures/scenario_profile.c"
    includes = ["-I" + str(ROOT / "include"), "-I" + str(ROOT / "examples/c")]
    sources = [str(source), str(ROOT / "examples/c/runner/scenario_helpers.c")]
    common = [*sources, *includes, "-ffunction-sections", "-fdata-sections", "-lm"]
    native = tmp_path / "native-profile"
    wasm = tmp_path / "wasm-profile.mjs"
    subprocess.run(["cc", *common, "-Wl,--gc-sections", "-o", str(native)], check=True)
    subprocess.run(["emcc", *common, "-sENVIRONMENT=node", "-sWASM_ASYNC_COMPILATION=0",
                    "-o", str(wasm)], check=True)
    native_state = json.loads(subprocess.check_output([str(native)], text=True))
    runner = f"import createModule from {json.dumps(wasm.as_uri())}; await createModule();"
    wasm_state = json.loads(subprocess.check_output(
        ["node", "--input-type=module", "-e", runner], text=True))
    assert native_state == wasm_state
    assert native_state["amplitude"] == 12
    assert native_state["period"] == 14.4
    assert native_state["quarter"] == pytest.approx(0.209439510239, abs=1e-11)
    assert native_state["live"] == native_state["quarter"]
    assert native_state["scientific_time"] == 123
    assert native_state["invalid"] == -1
    assert native_state["default"] == 0
