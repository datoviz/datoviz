"""Generate a compact deterministic tokamak equilibrium toy bundle."""

from __future__ import annotations

import argparse
import hashlib
import io
import json
import zipfile
from pathlib import Path

import numpy as np


DEFAULT_OUTPUT = Path("data/examples/generated/tokamak")
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


def generate_bundle(output: Path, surfaces: int, theta_count: int, phi_count: int) -> None:
    rng = np.random.default_rng(SEED)
    output.mkdir(parents=True, exist_ok=True)

    major_radius = 2.7
    minor_radius = 0.85
    elongation = 1.55
    triangularity = 0.28

    rho = np.linspace(0.18, 1.0, surfaces, dtype=np.float32)
    theta = np.linspace(0.0, 2.0 * np.pi, theta_count, endpoint=False, dtype=np.float32)
    phi = np.linspace(0.0, 2.0 * np.pi, phi_count, endpoint=False, dtype=np.float32)
    rr, tt, pp = np.meshgrid(rho, theta, phi, indexing="ij")
    shaped_angle = tt + triangularity * np.sin(tt)
    minor = minor_radius * rr
    radial = major_radius + minor * np.cos(shaped_angle)
    z = elongation * minor * np.sin(tt)
    x = radial * np.cos(pp)
    y = radial * np.sin(pp)
    flux_surfaces = np.stack([x, y, z], axis=-1).astype(np.float32)

    q_profile = (0.9 + 2.7 * rho**2).astype(np.float32)
    temperature = (7.5 * (1.0 - rho**1.8) + 0.35).astype(np.float32)
    density = (1.1e20 * (1.0 - 0.82 * rho**2) + 0.08e20).astype(np.float32)

    line_count = 12
    steps = 240
    field_lines = np.empty((line_count, steps, 3), dtype=np.float32)
    for li in range(line_count):
        line_rho = np.float32(0.25 + 0.7 * li / max(1, line_count - 1))
        q = 0.9 + 2.7 * line_rho * line_rho
        phase = rng.uniform(0.0, 2.0 * np.pi)
        phi_line = np.linspace(0.0, 5.5 * np.pi, steps, dtype=np.float32)
        theta_line = q * phi_line + phase
        shaped = theta_line + triangularity * np.sin(theta_line)
        minor_line = minor_radius * line_rho
        radial_line = major_radius + minor_line * np.cos(shaped)
        field_lines[li, :, 0] = radial_line * np.cos(phi_line)
        field_lines[li, :, 1] = radial_line * np.sin(phi_line)
        field_lines[li, :, 2] = elongation * minor_line * np.sin(theta_line)

    data_path = output / "data.npz"
    _write_npz(
        data_path,
        rho=rho,
        theta=theta,
        phi=phi,
        flux_surfaces=flux_surfaces,
        q_profile=q_profile,
        temperature_keV=temperature,
        density_m3=density,
        field_lines=field_lines,
    )

    manifest = {
        "name": "tokamak",
        "kind": "synthetic_fusion_equilibrium",
        "version": 1,
        "files": {"data.npz": _sha256(data_path)},
        "arrays": {
            "rho": list(rho.shape),
            "theta": list(theta.shape),
            "phi": list(phi.shape),
            "flux_surfaces": list(flux_surfaces.shape),
            "q_profile": list(q_profile.shape),
            "temperature_keV": list(temperature.shape),
            "density_m3": list(density.shape),
            "field_lines": list(field_lines.shape),
        },
        "units": {"position": "m", "temperature": "keV", "density": "m^-3"},
    }
    _write_json(output / "manifest.json", manifest)
    _write_json(
        output / "provenance.json",
        {
            "generated_at": GENERATED_AT,
            "generator": Path(__file__).as_posix(),
            "seed": SEED,
            "method": "Analytic shaped torus with q-profile and helical field-line traces",
            "parameters": {
                "surfaces": surfaces,
                "theta_count": theta_count,
                "phi_count": phi_count,
            },
        },
    )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--surfaces", type=int, default=7)
    parser.add_argument("--theta-count", type=int, default=48)
    parser.add_argument("--phi-count", type=int, default=64)
    args = parser.parse_args()
    generate_bundle(args.output, args.surfaces, args.theta_count, args.phi_count)


if __name__ == "__main__":
    main()
