"""Generate a compact deterministic CFD vortex toy bundle."""

from __future__ import annotations

import argparse
import hashlib
import io
import json
import zipfile
from pathlib import Path

import numpy as np


DEFAULT_OUTPUT = Path("data/examples/generated/cfd_vortex")
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


def generate_bundle(output: Path, nx: int, ny: int, frames: int) -> None:
    rng = np.random.default_rng(SEED)
    output.mkdir(parents=True, exist_ok=True)

    x = np.linspace(-2.0, 2.0, nx, dtype=np.float32)
    y = np.linspace(-1.3, 1.3, ny, dtype=np.float32)
    xx, yy = np.meshgrid(x, y, indexing="xy")
    times = np.linspace(0.0, 2.2, frames, dtype=np.float32)
    velocity = np.empty((frames, ny, nx, 2), dtype=np.float32)
    pressure = np.empty((frames, ny, nx), dtype=np.float32)
    vorticity = np.empty((frames, ny, nx), dtype=np.float32)

    gamma = 2.4
    core = 0.18
    free_stream = 0.45
    for ti, time in enumerate(times):
        centers = [
            (-0.8 + 0.25 * time, 0.32 * np.sin(2.0 * time), gamma),
            (0.55 - 0.18 * time, -0.25 * np.cos(1.6 * time), -0.72 * gamma),
        ]
        u = np.full_like(xx, free_stream)
        v = np.zeros_like(xx)
        vort = np.zeros_like(xx)
        for cx, cy, strength in centers:
            dx = xx - cx
            dy = yy - cy
            r2 = dx * dx + dy * dy + core * core
            swirl = strength * (1.0 - np.exp(-r2 / (2.0 * core * core))) / (2.0 * np.pi * r2)
            u += -swirl * dy
            v += swirl * dx
            vort += strength * np.exp(-r2 / (2.0 * core * core)) / (2.0 * np.pi * core * core)
        u += 0.025 * rng.normal(size=u.shape)
        v += 0.025 * rng.normal(size=v.shape)
        velocity[ti, :, :, 0] = u
        velocity[ti, :, :, 1] = v
        pressure[ti] = (1.0 - 0.5 * (u * u + v * v)).astype(np.float32)
        vorticity[ti] = vort.astype(np.float32)

    particle_xy = rng.uniform([-1.8, -1.1], [1.8, 1.1], size=(160, 2)).astype(np.float32)
    data_path = output / "data.npz"
    _write_npz(
        data_path,
        x=x,
        y=y,
        time=times,
        velocity=velocity,
        pressure=pressure,
        vorticity=vorticity,
        particle_xy=particle_xy,
    )

    manifest = {
        "name": "cfd_vortex",
        "kind": "synthetic_cfd",
        "version": 1,
        "files": {"data.npz": _sha256(data_path)},
        "arrays": {
            "x": list(x.shape),
            "y": list(y.shape),
            "time": list(times.shape),
            "velocity": list(velocity.shape),
            "pressure": list(pressure.shape),
            "vorticity": list(vorticity.shape),
            "particle_xy": list(particle_xy.shape),
        },
        "units": {"x": "m", "y": "m", "time": "s", "velocity": "m/s"},
    }
    _write_json(output / "manifest.json", manifest)
    _write_json(
        output / "provenance.json",
        {
            "generated_at": GENERATED_AT,
            "generator": Path(__file__).as_posix(),
            "seed": SEED,
            "method": "Two analytic Lamb-Oseen-like vortices with deterministic sample noise",
            "parameters": {"nx": nx, "ny": ny, "frames": frames},
        },
    )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--nx", type=int, default=96)
    parser.add_argument("--ny", type=int, default=64)
    parser.add_argument("--frames", type=int, default=12)
    args = parser.parse_args()
    generate_bundle(args.output, args.nx, args.ny, args.frames)


if __name__ == "__main__":
    main()
