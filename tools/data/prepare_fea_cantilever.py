"""Generate a compact deterministic cantilever-beam FEA toy bundle."""

from __future__ import annotations

import argparse
import hashlib
import io
import json
import zipfile
from pathlib import Path

import numpy as np


DEFAULT_OUTPUT = Path("data/examples/generated/fea_cantilever")
SEED = 20260517
GENERATED_AT = "2026-05-17T00:00:00Z"


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _write_json(path: Path, payload: dict) -> None:
    path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf8")


def _write_npz(path: Path, **arrays: np.ndarray) -> None:
    with zipfile.ZipFile(path, "w") as archive:
        for name, array in arrays.items():
            buffer = io.BytesIO()
            np.save(buffer, np.asarray(array), allow_pickle=False)
            info = zipfile.ZipInfo(f"{name}.npy", (2026, 5, 17, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info, buffer.getvalue(), compress_type=zipfile.ZIP_DEFLATED)


def _surface_triangles(nx: int, ny: int, nz: int) -> np.ndarray:
    def vid(i: int, j: int, k: int) -> int:
        return (i * ny + j) * nz + k

    triangles: list[tuple[int, int, int]] = []

    def add_quad(a: int, b: int, c: int, d: int) -> None:
        triangles.append((a, b, c))
        triangles.append((a, c, d))

    for i in range(nx - 1):
        for j in range(ny - 1):
            add_quad(vid(i, j, 0), vid(i + 1, j, 0), vid(i + 1, j + 1, 0), vid(i, j + 1, 0))
            add_quad(
                vid(i, j, nz - 1),
                vid(i, j + 1, nz - 1),
                vid(i + 1, j + 1, nz - 1),
                vid(i + 1, j, nz - 1),
            )

    for i in range(nx - 1):
        for k in range(nz - 1):
            add_quad(vid(i, 0, k), vid(i, 0, k + 1), vid(i + 1, 0, k + 1), vid(i + 1, 0, k))
            add_quad(
                vid(i, ny - 1, k),
                vid(i + 1, ny - 1, k),
                vid(i + 1, ny - 1, k + 1),
                vid(i, ny - 1, k + 1),
            )

    for j in range(ny - 1):
        for k in range(nz - 1):
            add_quad(vid(0, j, k), vid(0, j + 1, k), vid(0, j + 1, k + 1), vid(0, j, k + 1))
            add_quad(
                vid(nx - 1, j, k),
                vid(nx - 1, j, k + 1),
                vid(nx - 1, j + 1, k + 1),
                vid(nx - 1, j + 1, k),
            )

    return np.asarray(triangles, dtype=np.uint32)


def generate_bundle(output: Path, nx: int, ny: int, nz: int) -> None:
    rng = np.random.default_rng(SEED)
    output.mkdir(parents=True, exist_ok=True)

    length, width, height = 4.0, 0.7, 0.7
    x = np.linspace(0.0, length, nx, dtype=np.float32)
    y = np.linspace(-width / 2.0, width / 2.0, ny, dtype=np.float32)
    z = np.linspace(-height / 2.0, height / 2.0, nz, dtype=np.float32)
    xx, yy, zz = np.meshgrid(x, y, z, indexing="ij")
    nodes = np.column_stack([xx.ravel(), yy.ravel(), zz.ravel()]).astype(np.float32)

    xn = nodes[:, 0] / length
    bending = -(3.0 * xn**2 - xn**3) * 0.18
    twist = 0.015 * np.sin(np.pi * xn) * nodes[:, 1]
    poisson = -0.035 * xn * nodes[:, 2]
    displacement = np.column_stack([poisson, twist, bending]).astype(np.float32)
    displacement += rng.normal(0.0, 0.00025, displacement.shape).astype(np.float32)
    displacement[nodes[:, 0] == 0.0] = 0.0

    stress = (0.22 + 1.75 * (1.0 - xn) * np.abs(nodes[:, 2]) / (height / 2.0)).astype(np.float32)
    stress += (0.08 * np.sin(2.0 * np.pi * xn) ** 2).astype(np.float32)
    fixed = (nodes[:, 0] == 0.0).astype(np.uint8)
    load = np.array([0.0, 0.0, -1.0], dtype=np.float32)
    triangles = _surface_triangles(nx, ny, nz)

    data_path = output / "data.npz"
    _write_npz(
        data_path,
        nodes=nodes,
        displacement=displacement,
        von_mises=stress,
        fixed_mask=fixed,
        load_vector=load,
        surface_triangles=triangles,
    )

    manifest = {
        "name": "fea_cantilever",
        "kind": "synthetic_fea",
        "version": 1,
        "files": {"data.npz": _sha256(data_path)},
        "arrays": {
            "nodes": list(nodes.shape),
            "displacement": list(displacement.shape),
            "von_mises": list(stress.shape),
            "fixed_mask": list(fixed.shape),
            "load_vector": list(load.shape),
            "surface_triangles": list(triangles.shape),
        },
        "units": {"position": "m", "displacement": "m", "von_mises": "arbitrary"},
    }
    _write_json(output / "manifest.json", manifest)
    _write_json(
        output / "provenance.json",
        {
            "generated_at": GENERATED_AT,
            "generator": Path(__file__).as_posix(),
            "seed": SEED,
            "method": "Euler-Bernoulli-inspired analytic beam field on a regular node grid",
            "parameters": {"nx": nx, "ny": ny, "nz": nz},
        },
    )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--nx", type=int, default=33)
    parser.add_argument("--ny", type=int, default=7)
    parser.add_argument("--nz", type=int, default=7)
    args = parser.parse_args()
    generate_bundle(args.output, args.nx, args.ny, args.nz)


if __name__ == "__main__":
    main()
