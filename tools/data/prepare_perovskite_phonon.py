"""Generate a compact deterministic perovskite phonon toy bundle."""

from __future__ import annotations

import argparse
import hashlib
import io
import json
import zipfile
from pathlib import Path

import numpy as np


DEFAULT_OUTPUT = Path("data/examples/generated/perovskite_phonon")
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


def generate_bundle(output: Path, cells: int, q_count: int) -> None:
    rng = np.random.default_rng(SEED)
    output.mkdir(parents=True, exist_ok=True)

    basis = np.array(
        [
            [0.0, 0.0, 0.0],
            [0.5, 0.5, 0.5],
            [0.5, 0.5, 0.0],
            [0.5, 0.0, 0.5],
            [0.0, 0.5, 0.5],
        ],
        dtype=np.float32,
    )
    species_codes = np.array([0, 1, 2, 2, 2], dtype=np.uint8)
    masses = np.array([207.2, 47.9, 16.0], dtype=np.float32)

    positions = []
    species = []
    cell_index = []
    for i in range(cells):
        for j in range(cells):
            for k in range(cells):
                origin = np.array([i, j, k], dtype=np.float32)
                positions.append(origin + basis)
                species.append(species_codes)
                cell_index.append(np.tile(origin.astype(np.uint16), (len(basis), 1)))
    positions_arr = np.concatenate(positions).astype(np.float32)
    species_arr = np.concatenate(species).astype(np.uint8)
    cell_index_arr = np.concatenate(cell_index).astype(np.uint16)

    q_path = np.column_stack(
        [
            np.linspace(0.0, 0.5, q_count, dtype=np.float32),
            np.zeros(q_count, dtype=np.float32),
            np.linspace(0.0, 0.5, q_count, dtype=np.float32),
        ]
    )
    modes = np.arange(6, dtype=np.float32)[None, :]
    q_norm = np.linalg.norm(q_path, axis=1, keepdims=True)
    frequencies = 1.2 + 3.1 * modes + 2.4 * np.sin(np.pi * q_norm * (modes + 1.0)) ** 2
    frequencies[:, :3] = 0.2 + 5.0 * q_norm * np.array([[0.75, 0.95, 1.15]], dtype=np.float32)
    frequencies = frequencies.astype(np.float32)

    phase = 2.0 * np.pi * (positions_arr @ np.array([0.35, 0.15, 0.25], dtype=np.float32))
    eigenvectors = np.empty((q_count, 6, positions_arr.shape[0], 3), dtype=np.float32)
    for qi in range(q_count):
        for mi in range(6):
            axis = np.roll(np.array([1.0, 0.35, 0.15], dtype=np.float32), mi % 3)
            amp = 0.035 / np.sqrt(masses[species_arr])
            wave = np.sin(phase + qi * 0.37 + mi * 0.61)
            jitter = rng.normal(0.0, 0.015, (positions_arr.shape[0], 3)).astype(np.float32)
            eigenvectors[qi, mi] = (amp[:, None] * wave[:, None] * axis[None, :] + 0.002 * jitter)

    bonds = []
    for a_idx, pos in enumerate(positions_arr):
        if species_arr[a_idx] != 1:
            continue
        delta = positions_arr - pos
        wrapped = delta - np.rint(delta / cells) * cells
        dist = np.linalg.norm(wrapped, axis=1)
        near_oxygen = np.where((species_arr == 2) & (dist < 0.53))[0]
        bonds.extend((a_idx, int(o_idx)) for o_idx in near_oxygen)
    bonds_arr = np.asarray(bonds, dtype=np.uint32)

    data_path = output / "data.npz"
    _write_npz(
        data_path,
        positions=positions_arr,
        species=species_arr,
        cell_index=cell_index_arr,
        bonds=bonds_arr,
        q_path=q_path,
        frequencies=frequencies,
        eigenvectors=eigenvectors,
    )

    manifest = {
        "name": "perovskite_phonon",
        "kind": "synthetic_lattice_dynamics",
        "version": 1,
        "files": {"data.npz": _sha256(data_path)},
        "arrays": {
            "positions": list(positions_arr.shape),
            "species": list(species_arr.shape),
            "cell_index": list(cell_index_arr.shape),
            "bonds": list(bonds_arr.shape),
            "q_path": list(q_path.shape),
            "frequencies": list(frequencies.shape),
            "eigenvectors": list(eigenvectors.shape),
        },
        "species": {"0": "A_site", "1": "B_site", "2": "oxygen"},
    }
    _write_json(output / "manifest.json", manifest)
    _write_json(
        output / "provenance.json",
        {
            "generated_at": GENERATED_AT,
            "generator": Path(__file__).as_posix(),
            "seed": SEED,
            "method": "Analytic ABO3 lattice with toy acoustic and optical phonon branches",
            "parameters": {"cells": cells, "q_count": q_count},
        },
    )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--cells", type=int, default=3)
    parser.add_argument("--q-count", type=int, default=24)
    args = parser.parse_args()
    generate_bundle(args.output, args.cells, args.q_count)


if __name__ == "__main__":
    main()
