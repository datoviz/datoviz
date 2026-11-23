#!/usr/bin/env python3
"""Compare the real Python cortical producer with the canonical CPU-state receipt."""

import ctypes
import json
import math
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

import datoviz as dvz
from examples.python.gallery.showcases import cortical_activity as example


def main() -> None:
    receipt = ROOT / "build/example-state-parity/report.json"
    canonical = json.loads(receipt.read_text())["examples"]["cortical"]["native"]
    material_values = []
    original_material = dvz.dvz_visual_set_material

    def record_material(visual, description):
        material = description._obj.phong
        material_values.extend([material.ambient, material.diffuse, material.specular, material.shininess])
        return original_material(visual, description)

    dvz.dvz_visual_set_material = record_material
    scene = None
    try:
        scene, figure, panel, mesh, data, peak = example._build_scene()
        view = dvz.DvzCameraView()
        dvz.dvz_camera_get_view(dvz.dvz_panel_camera(panel), ctypes.byref(view))
        camera = [*view.eye, *view.up]
        assert len(canonical[3:9]) == len(camera) == 6
        assert len(material_values) == len(canonical[58:62]) == 4
        assert all(math.isclose(a, b, abs_tol=2e-6, rel_tol=2e-6)
                   for a, b in zip(camera, canonical[3:9]))
        assert all(math.isclose(a, b, abs_tol=2e-6, rel_tol=2e-6)
                   for a, b in zip(material_values, canonical[58:62]))
        light_count = int(canonical[62])
        count_offset = 63 + 4 * light_count
        canonical_counts = canonical[count_offset + 1:]
        assert len(data.positions) in canonical_counts
        assert len(data.positions) == len(example._normals(data.positions, data.indices))
        assert int(data.indices.max()) < len(data.positions)
        report = {
            "scope": "real Python direct-engine producer: camera, material, geometry counts; no GPU rendering",
            "camera": camera, "phong_material": material_values,
            "render_vertices": len(data.positions), "initial_detected_peak_ms": peak,
            "remaining_differences": [
                "Python starts at detected peak; canonical live C starts at 106 ms",
                "Python does not create the canonical colorbar and time readout",
                "Python does not apply the shared cortical presentation motion",
                "View-level arcball configuration and GUI controls are not exercised by this CPU check",
            ],
        }
        (receipt.parent / "python-report.json").write_text(json.dumps(report, indent=2) + "\n")
        print("Python cortical camera/material/geometry match canonical state; remaining differences recorded")
    finally:
        dvz.dvz_visual_set_material = original_material
        if scene:
            dvz.dvz_scene_destroy(scene)


if __name__ == "__main__":
    main()
