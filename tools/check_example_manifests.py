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
IGNORED_SOURCE_TITLE_CHECKS = {"examples/c/runtime/multi_window.c"}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--examples", type=Path, default=DEFAULT_EXAMPLES)
    parser.add_argument("--capabilities", type=Path, default=DEFAULT_CAPABILITIES)
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
    for match in re.finditer(r"\{(?P<body>.*?)\}", text, re.S):
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

    for directory in PUBLIC_C_DIRS:
        for path in sorted(directory.glob("*.c")):
            if path.name == "CMakeLists.txt":
                continue
            source = path.relative_to(ROOT).as_posix()
            if source not in sources:
                print(f"public C example has no manifest row: {source}")
                ok = False
            elif source not in IGNORED_SOURCE_TITLE_CHECKS:
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


def path_to_tool(path: Path) -> str:
    if path.name == "examples.json":
        return "tools/build_examples_manifest.py"
    if path.name == "capabilities.json":
        return "tools/build_capabilities.py"
    return "tools/build_examples_manifest.py"


def main() -> int:
    args = parse_args()
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
    ok = all(_check_file(path, expected) for path, expected in checks) and ok
    if ok:
        print("generated example manifests are up to date")
        return 0
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
