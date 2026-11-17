"""Named homepage captures preserve the default gallery and scientific cadence."""

import argparse
import copy
from dataclasses import replace
from pathlib import Path
import sys

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import gallery_frames
import gallery_media
import compare_gallery_media as compare


def homepage_entry():
    return next(entry for entry in gallery_media.load_manifest()["examples"]
                if entry["id"] == "showcases_textured_planet")


def test_resolution_preserves_default_and_scientific_cadence():
    original = homepage_entry()
    unchanged = copy.deepcopy(original)
    default = gallery_media.resolve_media_entry(original)
    effective = gallery_media.resolve_media_entry(original, "homepage")
    assert original == unchanged
    assert default["media"]["preview"]["frames"] == 480
    assert "profiles" not in effective["media"]
    assert effective["media"]["preview"]["frames"] == 864
    assert effective["media"]["preview"]["time_scale"] == 8.0
    previews = gallery_frames.collect_previews(gallery_media.DEFAULT_MANIFEST, Path("build/examples/c"), set(), set(), "homepage")
    assert {p.id for p in previews} == gallery_media.HOMEPAGE_IDS
    assert all((p.fps, p.sample_stride) == (60, 1) for p in previews)
    assert {p.id: (p.frames, p.presentation_period) for p in previews} == {
        "showcases_protein": (864, 14.4), "showcases_point_cloud": (576, 9.6),
        "showcases_cortical_activity": (864, 14.4), "showcases_textured_planet": (864, 14.4)}
    assert {p.id: p.presentation_amplitude_deg for p in previews}["showcases_protein"] == 12


@pytest.mark.parametrize("field,value", [("unknown", 1), ("time_scale", float("nan")), ("fps", float("inf")), ("frames", 1.5), ("sample_stride", 0)])
def test_profile_rejects_invalid_overrides(field, value):
    entry = homepage_entry()
    entry["media"]["profiles"]["homepage"][field] = value
    with pytest.raises(ValueError):
        gallery_media.resolve_media_entry(entry, "homepage")


@pytest.mark.parametrize("presentation", [{"kind": "orbit"}, {"kind": "sway", "amplitude_deg": float("inf")}, {"kind": "sway", "period": 3}])
def test_profile_rejects_invalid_presentation(presentation):
    entry = homepage_entry()
    entry["media"]["profiles"]["homepage"]["presentation"] = presentation
    with pytest.raises(ValueError):
        gallery_media.resolve_media_entry(entry, "homepage")


def test_effective_hash_ignores_unused_profiles_and_editorial_metadata():
    first = homepage_entry()
    second = copy.deepcopy(first)
    second["title"] = "Editorial change"
    second["media"]["profiles"]["unused"] = {"fps": 3}
    def fingerprint(entry, profile):
        return gallery_frames.capture_manifest_hash(gallery_media.resolve_media_entry(entry, profile))
    assert fingerprint(first, "homepage") == fingerprint(second, "homepage")
    second["media"]["profiles"]["homepage"]["frames"] = 900
    assert fingerprint(first, "") == fingerprint(second, "")
    assert fingerprint(first, "homepage") != fingerprint(second, "homepage")


def test_profile_paths_are_isolated_and_reject_default_tree_overrides(tmp_path):
    args = argparse.Namespace(profile="homepage", output=Path("default"))
    gallery_media.configure_profile_paths(args, {"output": (Path("default"), "assets/profiles/homepage")})
    assert args.output == gallery_media.HOMEPAGE_PRODUCTS_ROOT / "assets/profiles/homepage"
    args.output = tmp_path
    with pytest.raises(ValueError):
        gallery_media.configure_profile_paths(args, {"output": (Path("default"), "assets/profiles/homepage")})


def test_capture_forwards_presentation_and_unchanged_time_scale(tmp_path, monkeypatch):
    preview = gallery_frames.collect_previews(gallery_media.DEFAULT_MANIFEST, tmp_path, {"showcases_textured_planet"}, set(), "homepage")[0]
    calls = []
    monkeypatch.setattr(gallery_frames.subprocess, "run", lambda cmd, **kwargs: calls.append(cmd))
    gallery_frames.capture_sequence(preview, tmp_path / "frames")
    cmd = calls[0]
    assert cmd[cmd.index("--preview-time-scale") + 1] == "8"
    assert cmd[cmd.index("--preview-presentation") + 1] == "default"
    assert cmd[cmd.index("--preview-presentation-period") + 1] == "14.4"


def test_renderer_content_and_profile_identity_change_frame_key(tmp_path, monkeypatch):
    renderer = tmp_path / "libdatoviz.so"
    renderer.write_bytes(b"first")
    preview = gallery_frames.collect_previews(gallery_media.DEFAULT_MANIFEST, tmp_path, {"showcases_protein"}, set(), "homepage")[0]
    monkeypatch.setattr(gallery_frames, "renderer_library_paths", lambda unused: (renderer,))
    def fingerprint(item):
        return gallery_frames.input_hash_for(item, tmp_path / "manifest", tmp_path / "frames")
    first = fingerprint(preview)
    assert first != fingerprint(replace(preview, profile=""))
    renderer.write_bytes(b"second")
    assert first != fingerprint(preview)


def test_encoding_profile_uses_resolved_card_without_changing_quality():
    preview = gallery_frames.collect_previews(gallery_media.DEFAULT_MANIFEST, Path("build/examples/c"), {"showcases_textured_planet"}, set(), "homepage")[0]
    args = argparse.Namespace(profile="homepage", manifest=gallery_media.DEFAULT_MANIFEST,
                              preferred_kind="animated-webp", step=2, webp_quality=40,
                              mp4_crf=32, webm_crf=38, fps=0, no_manifest_card=False)
    profile = compare.profile_for(preview, args)
    assert (profile.fps, profile.step, profile.mp4_crf) == (60, 1, 26)


@pytest.mark.parametrize("field", ["fps", "mp4_crf", "webm_crf", "webp_quality", "sample_step"])
@pytest.mark.parametrize("value", [float("inf"), float("nan"), 1.5])
def test_card_rejects_nonfinite_and_fractional_encoding_fields(field, value):
    entry = homepage_entry()
    entry["media"]["preview"]["card"][field] = value
    with pytest.raises(ValueError, match="finite integer"):
        gallery_media.validate_animation_media_policy(entry)


@pytest.mark.parametrize("link", ["@rpath/libdatoviz.dylib", "@loader_path/libdatoviz.dylib", "@executable_path/libdatoviz.dylib"])
def test_darwin_renderer_resolution_ignores_system_libraries(tmp_path, monkeypatch, link):
    build_src = tmp_path / "build/src"
    build_src.mkdir(parents=True)
    executable = build_src / "example"
    executable.touch()
    renderer = build_src / "libdatoviz.dylib"
    renderer.write_bytes(b"renderer")
    calls = []
    def run(command, **kwargs):
        calls.append(command)
        return type("Result", (), {"stdout": f"{executable}:\n\\t{link} (compatibility version 0)\n /usr/lib/libSystem.B.dylib (compatibility version 1)\n".replace("\\t", "\t")})()
    monkeypatch.setattr(gallery_frames, "ROOT", tmp_path)
    monkeypatch.setattr(gallery_frames.sys, "platform", "darwin")
    monkeypatch.setattr(gallery_frames.subprocess, "run", run)
    assert gallery_frames.renderer_library_paths(executable) == (renderer.resolve(),)
    assert calls == [["otool", "-L", str(executable)]]


def test_darwin_renderer_fallback_without_otool(tmp_path, monkeypatch):
    renderer = tmp_path / "build/src/libdatoviz.dylib"
    renderer.parent.mkdir(parents=True)
    renderer.write_bytes(b"renderer")
    executable = tmp_path / "example"
    executable.touch()
    monkeypatch.setattr(gallery_frames, "ROOT", tmp_path)
    monkeypatch.setattr(gallery_frames.sys, "platform", "darwin")
    def unavailable(*args, **kwargs):
        raise FileNotFoundError("otool")
    monkeypatch.setattr(gallery_frames.subprocess, "run", unavailable)
    assert gallery_frames.renderer_library_paths(executable) == (renderer.resolve(),)


def test_profile_card_overrides_are_effective_without_changing_capture_hash():
    entry = homepage_entry()
    before = gallery_media.resolve_media_entry(entry, 'homepage')
    entry['media']['profiles']['homepage']['card'] = {
        'mp4_crf': 24, 'mp4_max_crf': 28, 'video_budget_bytes': 4_000_000}
    after = gallery_media.resolve_media_entry(entry, 'homepage')
    assert after['media']['preview']['card']['mp4_crf'] == 24
    assert after['media']['preview']['card']['mp4_max_crf'] == 28
    assert after['media']['preview']['card']['video_budget_bytes'] == 4_000_000
    assert gallery_frames.capture_manifest_hash(before) == gallery_frames.capture_manifest_hash(after)
    default_before = gallery_media.resolve_media_entry(homepage_entry())
    default_after = gallery_media.resolve_media_entry(entry)
    assert default_before == default_after


@pytest.mark.parametrize('overrides', [
    {'mp4_max_crf': 41}, {'mp4_max_crf': -1}, {'mp4_crf': 29, 'mp4_max_crf': 28},
    {'mp4_max_crf': float('inf')}, {'mp4_crf': float('nan')},
    {'video_budget_bytes': 0}, {'video_budget_bytes': -1},
    {'video_budget_bytes': 1.5}, {'video_budget_bytes': float('inf')},
    {'video_budget_bytes': True}, {'unknown': 1}])
def test_profile_card_override_bounds_are_explicit(overrides):
    entry = homepage_entry()
    entry['media']['profiles']['homepage']['card'] = overrides
    with pytest.raises(ValueError):
        gallery_media.resolve_media_entry(entry, 'homepage')


@pytest.mark.parametrize('required', ['/absolute/input', '../input', 'data/../input', 42])
def test_required_input_rejects_nonportable_paths(required):
    entry = homepage_entry()
    entry['media']['profiles']['homepage']['required_inputs'] = [required]
    with pytest.raises(ValueError):
        gallery_media.resolve_media_entry(entry, 'homepage')


def test_required_input_blocks_fallback_and_fingerprints_content(tmp_path, monkeypatch):
    import yaml
    entry = homepage_entry()
    entry['media']['profiles']['homepage']['required_inputs'] = ['inputs/dense.bin']
    manifest = tmp_path / 'manifest.yaml'
    manifest.write_text(yaml.safe_dump({'examples': [entry]}))
    monkeypatch.setattr(gallery_frames, 'ROOT', tmp_path)
    with pytest.raises(FileNotFoundError, match='required homepage input missing'):
        gallery_frames.collect_previews(manifest, tmp_path, set(), set(), 'homepage')
    required = tmp_path / 'inputs/dense.bin'
    required.parent.mkdir()
    required.write_bytes(b'dense data first')
    assert gallery_frames.collect_previews(manifest, tmp_path, set(), set(), 'homepage')
    effective = gallery_media.resolve_media_entry(entry, 'homepage')
    before = gallery_media.prepared_dataset_fingerprint(effective, tmp_path)
    required.write_bytes(b'dense data second')
    assert before != gallery_media.prepared_dataset_fingerprint(effective, tmp_path)



def test_default_presentation_override_and_custom_card_policy_are_resolved():
    entry = homepage_entry()
    entry['media']['profiles']['homepage']['presentation'] = {'kind': 'default'}
    entry['media']['profiles']['homepage']['card'] = {
        'mp4_crf': 24, 'mp4_max_crf': 28, 'video_budget_bytes': 4_000_000}
    effective = gallery_media.resolve_media_entry(entry, 'homepage')
    assert effective['media']['preview']['presentation']['kind'] == 'default'
    args = argparse.Namespace(profile='homepage', preferred_kind='animated-webp', step=2,
        webp_quality=40, mp4_crf=32, webm_crf=38, fps=0, no_manifest_card=False)
    preview = type('Preview', (), {'lane': 'showcases', 'id': entry['id'], 'fps': 60})()
    profile = compare.profile_for(preview, args, {('showcases', entry['id']): entry})
    assert (profile.mp4_crf, profile.mp4_max_crf, profile.video_budget_bytes) == (24, 28, 4_000_000)
    args.profile = ''
    default = compare.profile_for(preview, args, {('showcases', entry['id']): entry})
    assert (default.mp4_crf, default.mp4_max_crf, default.video_budget_bytes) == (26, 40, 1_000_000)


def test_profile_poster_quality_override():
    entry = homepage_entry()
    entry["media"]["profiles"]["homepage"]["card"] = {"webp_quality": 85}
    assert gallery_media.resolve_media_entry(entry, "homepage")["media"]["preview"]["card"]["webp_quality"] == 85
