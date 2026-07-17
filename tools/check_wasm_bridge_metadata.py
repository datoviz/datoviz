#!/usr/bin/env python3
"""Check WASM scenario/browser metadata against the example manifest."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path
from urllib.parse import parse_qs, urlparse

import yaml


ROOT = Path(__file__).resolve().parents[1]
MANIFEST_PATH = ROOT / "examples" / "c" / "MANIFEST.yaml"
LIVE_EXAMPLES_PATH = ROOT / "examples" / "webgpu" / "live_examples.js"
SCENARIO_C_PATH = ROOT / "src" / "wasm" / "scene_api_scenario.c"
SCENARIO_INTERNAL_PATH = ROOT / "src" / "wasm" / "scene_api_internal.h"
SMOKE_PATH = ROOT / "tools" / "wasm_scene_smoke.mjs"


def _fail(message: str) -> int:
    print(f"ERROR: {message}", file=sys.stderr)
    return 1


def _manifest_live_entries(path: Path) -> list[dict[str, str]]:
    manifest = yaml.safe_load(path.read_text(encoding="utf8")) or {}
    out: list[dict[str, str]] = []
    for entry in manifest.get("examples", []):
        webgpu = entry.get("webgpu") or {}
        route = str(
            webgpu.get("route")
            if webgpu.get("status") == "webgpu-live"
            else webgpu.get("local_route") or ""
        )
        if not route:
            continue
        query = parse_qs(urlparse(route).query)
        route_ids = query.get("id", [])
        if len(route_ids) != 1:
            raise ValueError(f"{entry.get('id')}: webgpu-live route needs one id query: {route}")
        out.append(
            {
                "id": str(entry["id"]),
                "title": str(entry["title"]),
                "route_id": route_ids[0],
                "scenario_id": str(webgpu.get("scenario_id") or entry["id"]),
            }
        )
    return out


def _live_js_entries(path: Path) -> list[dict[str, str]]:
    text = path.read_text(encoding="utf8")
    array_match = re.search(r"export\s+const\s+LIVE_EXAMPLES\s*=\s*\[(?P<body>.*)\];", text, re.S)
    if array_match is None:
        raise ValueError("LIVE_EXAMPLES array not found")

    bodies: list[str] = []
    array_body = array_match.group("body")
    depth = 0
    start = -1
    quote = ""
    escaped = False
    for index, char in enumerate(array_body):
        if quote:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == quote:
                quote = ""
            continue
        if char in {'"', "'", "`"}:
            quote = char
        elif char == "{":
            if depth == 0:
                start = index + 1
            depth += 1
        elif char == "}":
            depth -= 1
            if depth < 0:
                raise ValueError("unbalanced LIVE_EXAMPLES object braces")
            if depth == 0:
                bodies.append(array_body[start:index])
    if depth != 0 or quote:
        raise ValueError("unterminated LIVE_EXAMPLES object or string")

    out: list[dict[str, str]] = []
    for body in bodies:
        id_match = re.search(r'\bid:\s*"([^"]+)"', body)
        label_match = re.search(r'\blabel:\s*"([^"]+)"', body)
        scenario_match = re.search(r'\bscenarioId:\s*"([^"]+)"', body)
        if id_match is None and label_match is None and scenario_match is None:
            continue
        if id_match is None or label_match is None or scenario_match is None:
            raise ValueError(f"incomplete LIVE_EXAMPLES entry: {body}")
        out.append(
            {
                "id": id_match.group(1),
                "label": label_match.group(1),
                "scenario_id": scenario_match.group(1),
            }
        )
    return out


def _scenario_case_count(path: Path) -> int:
    text = path.read_text(encoding="utf8")
    return len(re.findall(r"^\s*case\s+\d+\s*:", text, re.M))


def _scenario_count_constant(path: Path) -> int:
    text = path.read_text(encoding="utf8")
    match = re.search(r"#define\s+DVZ_WASM_API_SCENARIO_COUNT\s+(\d+)", text)
    if match is None:
        raise ValueError("DVZ_WASM_API_SCENARIO_COUNT not found")
    return int(match.group(1))


def _smoke_expected_scenarios(path: Path) -> list[str]:
    text = path.read_text(encoding="utf8")
    match = re.search(r"const expectedScenarioIds = \[(?P<body>.*?)\];", text, re.S)
    if match is None:
        raise ValueError("expectedScenarioIds array not found")
    return re.findall(r'"([^"]+)"', match.group("body"))


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, default=MANIFEST_PATH)
    args = parser.parse_args(argv)

    manifest_live = _manifest_live_entries(args.manifest)
    live_js = _live_js_entries(LIVE_EXAMPLES_PATH)
    c_count = _scenario_count_constant(SCENARIO_INTERNAL_PATH)
    c_cases = _scenario_case_count(SCENARIO_C_PATH)
    smoke_ids = _smoke_expected_scenarios(SMOKE_PATH)

    manifest_route_ids = [entry["route_id"] for entry in manifest_live]
    manifest_scenario_ids = [entry["scenario_id"] for entry in manifest_live]
    live_ids = [entry["id"] for entry in live_js]
    live_scenario_ids = [entry["scenario_id"] for entry in live_js]

    bad_route_ids = [
        entry for entry in manifest_live if entry["route_id"] != entry["id"]
    ]
    if bad_route_ids:
        return _fail(f"webgpu-live route ids must match example ids: {bad_route_ids}")
    if len(set(manifest_route_ids)) != len(manifest_route_ids):
        return _fail("duplicate webgpu-live route ids in examples/c/MANIFEST.yaml")
    if set(manifest_route_ids) != set(live_ids):
        return _fail("examples/webgpu/live_examples.js ids do not match manifest webgpu-live routes")
    if set(manifest_scenario_ids) != set(live_scenario_ids):
        return _fail(
            "examples/webgpu/live_examples.js scenario ids do not match manifest webgpu.scenario_id"
        )
    if c_count != c_cases:
        return _fail(f"DVZ_WASM_API_SCENARIO_COUNT={c_count}, but scenario switch has {c_cases} cases")
    if c_count != len(smoke_ids):
        return _fail(f"smoke expectedScenarioIds has {len(smoke_ids)} ids, but C scenario count is {c_count}")
    missing_scenarios = sorted(set(live_scenario_ids) - set(smoke_ids))
    if missing_scenarios:
        return _fail(f"browser live scenarios missing from smoke/C registry constants: {missing_scenarios}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
