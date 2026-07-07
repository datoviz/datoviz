#!/usr/bin/env python3
"""Wavefront OBJ fixture loaded as retained mesh geometry."""

from __future__ import annotations

import ctypes
import tempfile
from pathlib import Path

import datoviz as dvz

from examples.python.gallery import common as ex


OBJ_FIXTURE = """# Compact low-poly bunny fixture.
o datoviz_low_poly_bunny
v 0.000 0.350 0.000
v 0.000 0.150 0.340
v 0.330 0.150 0.240
v 0.470 0.150 0.000
v 0.330 0.150 -0.240
v 0.000 0.150 -0.340
v -0.330 0.150 -0.240
v -0.470 0.150 0.000
v -0.330 0.150 0.240
v 0.000 -0.550 0.280
v 0.280 -0.550 0.200
v 0.400 -0.550 0.000
v 0.280 -0.550 -0.200
v 0.000 -0.550 -0.280
v -0.280 -0.550 -0.200
v -0.400 -0.550 0.000
v -0.280 -0.550 0.200
v 0.000 -0.780 0.000
v 0.000 0.780 0.220
v 0.000 0.200 0.220
v 0.000 0.490 0.500
v 0.270 0.490 0.220
v 0.000 0.490 -0.040
v -0.270 0.490 0.220
v -0.160 0.560 0.200
v -0.350 1.050 0.200
v -0.080 1.040 0.200
v -0.230 1.350 0.200
v -0.160 0.560 0.080
v -0.350 1.050 0.080
v -0.080 1.040 0.080
v -0.230 1.350 0.080
v 0.160 0.560 0.200
v 0.080 1.040 0.200
v 0.350 1.050 0.200
v 0.230 1.350 0.200
v 0.160 0.560 0.080
v 0.080 1.040 0.080
v 0.350 1.050 0.080
v 0.230 1.350 0.080
v 0.000 -0.200 -0.520
v 0.000 -0.500 -0.520
v 0.000 -0.350 -0.360
v 0.180 -0.350 -0.520
v 0.000 -0.350 -0.680
v -0.180 -0.350 -0.520
f 1 2 3
f 1 3 4
f 1 4 5
f 1 5 6
f 1 6 7
f 1 7 8
f 1 8 9
f 1 9 2
f 2 10 11 3
f 3 11 12 4
f 4 12 13 5
f 5 13 14 6
f 6 14 15 7
f 7 15 16 8
f 8 16 17 9
f 9 17 10 2
f 18 11 10
f 18 12 11
f 18 13 12
f 18 14 13
f 18 15 14
f 18 16 15
f 18 17 16
f 18 10 17
f 19 21 22
f 19 22 23
f 19 23 24
f 19 24 21
f 20 22 21
f 20 23 22
f 20 24 23
f 20 21 24
f 25 26 28 27
f 29 31 32 30
f 25 29 30 26
f 27 28 32 31
f 26 30 32 28
f 25 27 31 29
f 33 34 36 35
f 37 39 40 38
f 33 37 38 34
f 35 36 40 39
f 34 38 40 36
f 33 35 39 37
f 41 43 44
f 41 44 45
f 41 45 46
f 41 46 43
f 42 44 43
f 42 45 44
f 42 46 45
f 42 43 46
"""


def _add_geometry(scene, panel, geometry) -> None:
    if not geometry:
        raise RuntimeError("geometry creation failed")

    mesh = dvz.dvz_mesh(scene, 0)
    if not mesh:
        dvz.dvz_geometry_destroy(geometry)
        raise RuntimeError("dvz_mesh() failed")

    try:
        if dvz.dvz_mesh_set_geometry(mesh, geometry) != 0:
            raise RuntimeError("dvz_mesh_set_geometry() failed")
        ex.add_visual(panel, mesh)
    finally:
        dvz.dvz_geometry_destroy(geometry)


def _load_obj_geometry():
    path: Path | None = None
    try:
        with tempfile.NamedTemporaryFile("w", suffix=".obj", delete=False) as f:
            f.write(OBJ_FIXTURE)
            path = Path(f.name)

        desc = dvz.dvz_geometry_obj_desc()
        desc.color = ex.CYAN
        geometry = dvz.dvz_geometry_obj(str(path).encode(), ctypes.byref(desc))
        if not geometry:
            raise RuntimeError("dvz_geometry_obj() failed")
        return geometry
    finally:
        if path is not None:
            path.unlink(missing_ok=True)


def main() -> None:
    scene, figure, panel = ex.scene_panel()
    ex.manual_camera(panel)
    _add_geometry(scene, panel, _load_obj_geometry())

    def configure(view) -> None:
        arcball = dvz.dvz_view_arcball(view, panel, None)
        if not arcball:
            raise RuntimeError("dvz_view_arcball() failed")
        angles = (ctypes.c_float * 3)(0.0, 0.0, 0.0)
        if dvz.dvz_arcball_set(arcball, angles) != 0:
            raise RuntimeError("dvz_arcball_set() failed")

    ex.run_with_view(scene, figure, "OBJ Loading", configure)


if __name__ == "__main__":
    main()
