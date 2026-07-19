import json
import tempfile
import unittest
from pathlib import Path

from tools.write_wasm_asset_manifest import ARTIFACT_NAMES, build_manifest, encoded_manifest


class WasmAssetManifestTests(unittest.TestCase):
    def test_manifest_is_deterministic_and_changes_with_output(self):
        with tempfile.TemporaryDirectory() as tmp:
            wasm_dir = Path(tmp)
            for index, name in enumerate(ARTIFACT_NAMES):
                (wasm_dir / name).write_bytes(bytes([index + 1]) * (index + 2))

            first = build_manifest(wasm_dir)
            second = build_manifest(wasm_dir)
            self.assertEqual(first, second)
            self.assertEqual(first["schema"], "datoviz.wasm-assets.v1")
            self.assertEqual(first["artifacts"][ARTIFACT_NAMES[2]]["bytes"], 4)
            json.loads(encoded_manifest(first))

            (wasm_dir / ARTIFACT_NAMES[1]).write_bytes(b"changed")
            changed = build_manifest(wasm_dir)
            self.assertNotEqual(first["version"], changed["version"])


if __name__ == "__main__":
    unittest.main()
