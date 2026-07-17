#!/usr/bin/env python3
"""Check generated public example JSON manifests for drift."""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path
from typing import Any
from urllib.parse import parse_qs, urlparse

import build_capabilities
import build_examples_manifest
import build_gallery


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_EXAMPLES = ROOT / "docs/examples/examples.json"
DEFAULT_CAPABILITIES = ROOT / "docs/examples/capabilities.json"
DEFAULT_MANIFEST = ROOT / "examples/c/MANIFEST.yaml"
DEFAULT_GALLERY_MEDIA = ROOT / "data/gallery/v0.4"
PUBLIC_C_DIRS = (
    ROOT / "examples/c/start",
    ROOT / "examples/c/features",
    ROOT / "examples/c/runtime",
    ROOT / "examples/c/visuals",
    ROOT / "examples/c/composites",
    ROOT / "examples/c/showcases",
    ROOT / "examples/c/advanced",
)
DATA_KINDS = {"synthetic", "simulated", "generated", "real", "prepared"}
WEBGPU_EFFECT_STATUSES = {"supported", "limited", "unavailable"}
WEBGPU_RENDERING_EFFECT_PATTERNS = {
    "ssao": re.compile(r"\b(?:dvz_panel_set_ssao|example_tuner_ssao)\b"),
    "msaa": re.compile(r"\b(?:dvz_panel_set_msaa|example_tuner_msaa)\b"),
    "edl": re.compile(r"\b(?:dvz_panel_set_edl|example_tuner_edl)\b"),
    "depth-cue": re.compile(r"\b(?:dvz_visual_set_depth_cue|example_tuner_depth_cue)\b"),
    "wboit": re.compile(r"\bDVZ_ALPHA_WBOIT\b"),
    "depth-peeling": re.compile(r"\bDVZ_ALPHA_DEPTH_PEEL\b"),
}
IGNORED_SOURCE_TITLE_CHECKS = {"examples/c/runtime/multi_window.c"}
GALLERY_MEDIA_SUFFIXES = {".gif", ".jpeg", ".jpg", ".mp4", ".png", ".webm", ".webp"}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--examples", type=Path, default=DEFAULT_EXAMPLES)
    parser.add_argument("--capabilities", type=Path, default=DEFAULT_CAPABILITIES)
    parser.add_argument("--gallery-media", type=Path, default=DEFAULT_GALLERY_MEDIA)
    return parser.parse_args()


def _canonical_json(payload: dict[str, Any]) -> str:
    return json.dumps(payload, indent=2) + "\n"


def _check_file(path: Path, expected: str) -> bool:
    if not path.exists():
        print(f"missing generated manifest: {path}")
        return False
    actual = path.read_text(encoding="utf8")
    if actual == expected:
        return True
    print(f"generated manifest drift: {path}")
    print(f"regenerate with: python3 {path_to_tool(path)}")
    return False


def _live_js_entries(path: Path) -> list[dict[str, str]]:
    text = path.read_text(encoding="utf8")
    out: list[dict[str, str]] = []
    for match in re.finditer(r"^  \{(?P<body>.*?)^  \},?$", text, re.S | re.M):
        body = match.group("body")
        id_match = re.search(r'\bid:\s*"([^"]+)"', body)
        scenario_match = re.search(r'\bscenarioId:\s*"([^"]+)"', body)
        if id_match is None and scenario_match is None:
            continue
        if id_match is None or scenario_match is None:
            print(f"incomplete LIVE_EXAMPLES entry: {body}")
            return []
        out.append({"id": id_match.group(1), "scenario_id": scenario_match.group(1)})
    return out


def _check_manifest_semantics(manifest_path: Path) -> bool:
    manifest = build_gallery.load_manifest(manifest_path)
    ok = True
    entries = manifest["examples"]
    sources = {str(entry.get("source")) for entry in entries if entry.get("source")}
    extra_sources = {
        str(extra.get("path"))
        for entry in entries
        for extra in entry.get("extra_sources", [])
        if extra.get("path")
    }
    documented_sources = sources | extra_sources
    by_source = {str(entry.get("source")): entry for entry in entries if entry.get("source")}

    for entry in entries:
        entry_id = str(entry.get("id", "<missing>"))
        if entry.get("stage") == "lab" or entry.get("lane") == "lab":
            print(f"lab example must live under non_public_examples, not examples: {entry_id}")
            ok = False
        data_kind = (entry.get("data") or {}).get("kind")
        if data_kind is not None and str(data_kind) not in DATA_KINDS:
            print(f"{entry_id}: unknown data.kind {data_kind!r}")
            ok = False

        webgpu = entry.get("webgpu") or {}
        rendering_effects = webgpu.get("rendering_effects") or []
        declared_effects = [str(effect.get("effect") or "") for effect in rendering_effects]
        if len(set(declared_effects)) != len(declared_effects):
            print(f"{entry_id}: duplicate webgpu.rendering_effects entries")
            ok = False
        for effect in rendering_effects:
            effect_name = str(effect.get("effect") or "")
            status = str(effect.get("status") or "")
            warning = str(effect.get("warning") or "")
            if effect_name not in WEBGPU_RENDERING_EFFECT_PATTERNS:
                print(f"{entry_id}: unknown WebGPU rendering effect {effect_name!r}")
                ok = False
            if status not in WEBGPU_EFFECT_STATUSES:
                print(f"{entry_id}: unknown WebGPU rendering-effect status {status!r}")
                ok = False
            if status != "supported" and not warning:
                print(f"{entry_id}: {effect_name} {status} status needs a warning")
                ok = False

        if webgpu.get("status") == "webgpu-live" and entry.get("source"):
            source_path = ROOT / str(entry["source"])
            source_text = source_path.read_text(encoding="utf8")
            detected_effects = {
                name
                for name, pattern in WEBGPU_RENDERING_EFFECT_PATTERNS.items()
                if pattern.search(source_text)
            }
            if detected_effects != set(declared_effects):
                print(
                    f"{entry_id}: webgpu.rendering_effects must account for native effects; "
                    f"detected={sorted(detected_effects)}, declared={sorted(declared_effects)}"
                )
                ok = False

    for directory in PUBLIC_C_DIRS:
        for path in sorted(directory.glob("*.c")):
            if path.name == "CMakeLists.txt":
                continue
            source = path.relative_to(ROOT).as_posix()
            if source not in documented_sources:
                print(f"public C example has no manifest row: {source}")
                ok = False
            elif source in sources and source not in IGNORED_SOURCE_TITLE_CHECKS:
                entry = by_source[source]
                text = path.read_text(encoding="utf8")
                titles = []
                for match in re.finditer(
                    r"return\s*\(DvzScenarioSpec\)\s*\{(?P<body>.*?)\};", text, re.S
                ):
                    title_match = re.search(r"\.title\s*=\s*\"([^\"]+)\"", match.group("body"))
                    if title_match is not None:
                        titles.append(title_match.group(1))
                if len(titles) == 1 and titles[0] != str(entry["title"]):
                    print(
                        f"{source}: scenario title {titles[0]!r} "
                        f"does not match manifest title {entry['title']!r}"
                    )
                    ok = False

    live_js = _live_js_entries(ROOT / "examples/webgpu/live_examples.js")
    live_by_route = {entry["id"]: entry["scenario_id"] for entry in live_js}
    for entry in entries:
        webgpu = entry.get("webgpu") or {}
        if webgpu.get("status") != "webgpu-live":
            continue
        route = str(webgpu.get("route") or "")
        route_ids = parse_qs(urlparse(route).query).get("id", [])
        if route_ids != [str(entry["id"])]:
            print(f"{entry['id']}: webgpu-live route must use id={entry['id']}: {route}")
            ok = False
            continue
        expected_scenario = live_by_route.get(route_ids[0])
        manifest_scenario = str(webgpu.get("scenario_id") or entry["id"])
        if expected_scenario is None:
            print(f"{entry['id']}: webgpu-live route missing from live_examples.js")
            ok = False
        elif manifest_scenario != expected_scenario:
            print(
                f"{entry['id']}: manifest scenario_id {manifest_scenario!r} "
                f"does not match live_examples.js {expected_scenario!r}"
            )
            ok = False
        if manifest_scenario != entry["id"] and "scenario_id" not in webgpu:
            print(f"{entry['id']}: route/scenario alias needs explicit webgpu.scenario_id")
            ok = False

    return ok


def _expected_entry_id(entry: dict[str, Any], source_key: str) -> str | None:
    lane = entry.get("lane")
    source = entry.get(source_key)
    if lane is None or source is None:
        return None
    return f"{lane}_{Path(str(source)).stem}"


def _check_example_id_paths(manifest_path: Path) -> bool:
    manifest = build_gallery.load_manifest(manifest_path)
    ok = True
    for entry in manifest["examples"]:
        entry_id = str(entry.get("id", "<missing>"))
        expected = _expected_entry_id(entry, "source")
        if expected is not None and entry_id != expected:
            print(f"{entry_id}: source stem should map to example id {expected!r}")
            ok = False

        python = entry.get("python") or {}
        python_entry = {**entry, "source": python.get("source")}
        expected_python = _expected_entry_id(python_entry, "source")
        if expected_python is not None and entry_id != expected_python:
            print(f"{entry_id}: Python source stem should map to example id {expected_python!r}")
            ok = False
    return ok


def _check_gallery_media_ids(manifest_path: Path, media_root: Path) -> bool:
    if not media_root.exists():
        return True

    manifest = build_gallery.load_manifest(manifest_path)
    by_id = {str(entry["id"]): str(entry["lane"]) for entry in manifest["examples"]}
    ok = True
    for path in sorted(media_root.rglob("*")):
        if not path.is_file() or path.suffix.lower() not in GALLERY_MEDIA_SUFFIXES:
            continue
        example_id = path.stem
        lane = path.parent.name
        expected_lane = by_id.get(example_id)
        if expected_lane is None:
            print(f"gallery media is not named after a manifest id: {path.relative_to(ROOT)}")
            ok = False
        elif lane != expected_lane:
            print(
                f"gallery media lane mismatch for {example_id}: "
                f"{path.relative_to(ROOT)} should be under {expected_lane}/"
            )
            ok = False
    return ok


def path_to_tool(path: Path) -> str:
    if path.name == "examples.json":
        return "tools/build_examples_manifest.py"
    if path.name == "capabilities.json":
        return "tools/build_capabilities.py"
    return "tools/build_examples_manifest.py"


def main() -> int:
    args = parse_args()
    examples = build_gallery.collect_examples(build_gallery.load_manifest(args.manifest))
    build_gallery.validate_navigation(build_gallery.EXAMPLE_NAVIGATION, examples)
    checks = [
        (
            args.examples,
            _canonical_json(build_examples_manifest.build_payload(args.manifest)),
        ),
        (
            args.capabilities,
            _canonical_json(build_capabilities.build_payload(args.manifest)),
        ),
    ]
    ok = _check_manifest_semantics(args.manifest)
    ok = _check_example_id_paths(args.manifest) and ok
    ok = _check_gallery_media_ids(args.manifest, args.gallery_media) and ok
    ok = all(_check_file(path, expected) for path, expected in checks) and ok
    if ok:
        print("generated example manifests are up to date")
        return 0
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
