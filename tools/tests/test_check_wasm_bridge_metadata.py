#!/usr/bin/env python3
"""Tests for WASM scenario and browser metadata validation."""

from __future__ import annotations

from pathlib import Path
import sys
import tempfile
import unittest


TOOLS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS))

import check_wasm_bridge_metadata as metadata  # noqa: E402


class WasmBridgeMetadataTests(unittest.TestCase):
    def test_live_entries_ignore_nested_data_bundle_objects(self) -> None:
        source = '''export const LIVE_EXAMPLES = [
  {
    id: "showcases_planet",
    label: "Planet",
    scenarioId: "showcases_planet",
    dataBundles: [
      { id: "planet", url: "bundle.json", virtualRoot: "data/planet", required: true },
    ],
  },
];
'''
        with tempfile.TemporaryDirectory(dir=metadata.ROOT) as tmp:
            path = Path(tmp) / "live_examples.js"
            path.write_text(source, encoding="utf8")
            entries = metadata._live_js_entries(path)
        self.assertEqual(
            entries,
            [
                {
                    "id": "showcases_planet",
                    "label": "Planet",
                    "scenario_id": "showcases_planet",
                    "effect_limitations": [],
                }
            ],
        )

    def test_live_entries_parse_effect_limitations(self) -> None:
        source = '''export const LIVE_EXAMPLES = [
  {
    id: "showcases_protein",
    label: "Protein",
    scenarioId: "showcases_protein",
    effectLimitations: [
      {
        effect: "ssao",
        status: "unavailable",
        warning: "The browser route omits SSAO.",
      },
    ],
  },
];
'''
        with tempfile.TemporaryDirectory(dir=metadata.ROOT) as tmp:
            path = Path(tmp) / "live_examples.js"
            path.write_text(source, encoding="utf8")
            entries = metadata._live_js_entries(path)
        self.assertEqual(
            entries[0]["effect_limitations"],
            [
                {
                    "effect": "ssao",
                    "status": "unavailable",
                    "warning": "The browser route omits SSAO.",
                }
            ],
        )

    def test_manifest_live_entries_exclude_local_only_routes(self) -> None:
        source = '''examples:
  - id: public
    title: Public
    webgpu:
      status: webgpu-live
      route: examples/webgpu/live.html?id=public
  - id: local
    title: Local
    webgpu:
      status: native-only
      local_route: examples/webgpu/live.html?id=local
'''
        with tempfile.TemporaryDirectory(dir=metadata.ROOT) as tmp:
            path = Path(tmp) / "MANIFEST.yaml"
            path.write_text(source, encoding="utf8")
            entries = metadata._manifest_live_entries(path)
        self.assertEqual([entry["id"] for entry in entries], ["public"])


if __name__ == "__main__":
    unittest.main()
