#!/usr/bin/env python3
"""Download a PDB structure and export Datoviz-friendly protein arrays."""

from __future__ import annotations

import argparse
import json
import math
import struct
import sys
import urllib.request
from dataclasses import dataclass
from pathlib import Path


PDB_URL = "https://files.rcsb.org/download/{pdb_id}.pdb"
WATER_NAMES = {"HOH", "WAT", "H2O"}

VDW_RADII = {
    "H": 1.20,
    "C": 1.70,
    "N": 1.55,
    "O": 1.52,
    "F": 1.47,
    "P": 1.80,
    "S": 1.80,
    "CL": 1.75,
    "BR": 1.85,
    "I": 1.98,
    "FE": 1.80,
    "MG": 1.73,
    "ZN": 1.39,
    "CA": 1.94,
}

COVALENT_RADII = {
    "H": 0.31,
    "C": 0.76,
    "N": 0.71,
    "O": 0.66,
    "F": 0.57,
    "P": 1.07,
    "S": 1.05,
    "CL": 1.02,
    "BR": 1.20,
    "I": 1.39,
    "FE": 1.24,
    "MG": 1.30,
    "ZN": 1.22,
    "CA": 1.74,
}

ELEMENT_COLORS = {
    "H": (240, 240, 240, 255),
    "C": (80, 90, 100, 255),
    "N": (70, 115, 220, 255),
    "O": (220, 65, 65, 255),
    "F": (80, 200, 120, 255),
    "P": (230, 150, 60, 255),
    "S": (230, 210, 80, 255),
    "CL": (80, 200, 120, 255),
    "BR": (150, 70, 45, 255),
    "I": (110, 60, 160, 255),
    "FE": (200, 90, 45, 255),
    "MG": (120, 210, 120, 255),
    "ZN": (120, 120, 180, 255),
    "CA": (140, 180, 230, 255),
}

CHAIN_PALETTE = [
    (75, 135, 210, 255),
    (220, 120, 75, 255),
    (95, 175, 100, 255),
    (190, 95, 175, 255),
    (210, 180, 75, 255),
    (90, 180, 190, 255),
    (190, 140, 85, 255),
    (145, 145, 145, 255),
]


@dataclass
class Atom:
    serial: int
    name: str
    element: str
    residue: str
    chain: str
    residue_index: int
    x: float
    y: float
    z: float
    occupancy: float
    bfactor: float
    hetero: bool


def _default_cache_dir(pdb_id: str) -> Path:
    return Path.home() / ".cache" / "datoviz" / "proteins" / pdb_id.lower()


def _element_from_atom_name(name: str) -> str:
    stripped = "".join(ch for ch in name.strip().upper() if ch.isalpha())
    if not stripped:
        return "C"
    if len(stripped) >= 2 and stripped[:2] in VDW_RADII:
        return stripped[:2]
    return stripped[0]


def _parse_float(text: str, default: float = 0.0) -> float:
    try:
        return float(text.strip())
    except ValueError:
        return default


def _parse_int(text: str, default: int = 0) -> int:
    try:
        return int(text.strip())
    except ValueError:
        return default


def _download_pdb(pdb_id: str) -> str:
    url = PDB_URL.format(pdb_id=pdb_id.upper())
    with urllib.request.urlopen(url, timeout=30) as response:
        return response.read().decode("utf-8", errors="replace")


def _load_source(pdb_id: str, input_path: Path | None) -> tuple[str, str]:
    if input_path is not None:
        return input_path.read_text(encoding="utf-8"), str(input_path)
    return _download_pdb(pdb_id), PDB_URL.format(pdb_id=pdb_id.upper())


def _parse_pdb(text: str, include_hetero: bool, keep_waters: bool) -> list[Atom]:
    selected: dict[tuple[str, int, str, str, str], tuple[str, Atom]] = {}
    for line in text.splitlines():
        record = line[0:6].strip()
        if record not in {"ATOM", "HETATM"}:
            continue
        hetero = record == "HETATM"
        if hetero and not include_hetero:
            continue

        residue = line[17:20].strip().upper()
        if residue in WATER_NAMES and not keep_waters:
            continue

        name = line[12:16].strip()
        altloc = line[16:17].strip()
        chain = line[21:22].strip() or "_"
        residue_index = _parse_int(line[22:26])
        insertion_code = line[26:27].strip()
        element = line[76:78].strip().upper() or _element_from_atom_name(name)
        x = _parse_float(line[30:38])
        y = _parse_float(line[38:46])
        z = _parse_float(line[46:54])
        occupancy = _parse_float(line[54:60], 1.0)
        bfactor = _parse_float(line[60:66])
        atom = Atom(
            serial=_parse_int(line[6:11]),
            name=name,
            element=element,
            residue=residue,
            chain=chain,
            residue_index=residue_index,
            x=x,
            y=y,
            z=z,
            occupancy=occupancy,
            bfactor=bfactor,
            hetero=hetero,
        )

        key = (chain, residue_index, insertion_code, name, element)
        previous = selected.get(key)
        if previous is None:
            selected[key] = (altloc, atom)
            continue
        prev_altloc, prev_atom = previous
        prefer_blank = altloc == "" and prev_altloc != ""
        prefer_a = altloc == "A" and prev_altloc not in {"", "A"}
        prefer_occ = occupancy > prev_atom.occupancy and prev_altloc not in {"", "A"}
        if prefer_blank or prefer_a or prefer_occ:
            selected[key] = (altloc, atom)

    atoms = [atom for _, atom in selected.values()]
    atoms.sort(key=lambda atom: atom.serial)
    return atoms


def _center_and_radius(atoms: list[Atom]) -> tuple[tuple[float, float, float], float, list[float], list[float]]:
    xs = [atom.x for atom in atoms]
    ys = [atom.y for atom in atoms]
    zs = [atom.z for atom in atoms]
    bbox_min = [min(xs), min(ys), min(zs)]
    bbox_max = [max(xs), max(ys), max(zs)]
    center = (
        0.5 * (bbox_min[0] + bbox_max[0]),
        0.5 * (bbox_min[1] + bbox_max[1]),
        0.5 * (bbox_min[2] + bbox_max[2]),
    )
    radius = 0.0
    for atom in atoms:
        dx = atom.x - center[0]
        dy = atom.y - center[1]
        dz = atom.z - center[2]
        radius = max(radius, math.sqrt(dx * dx + dy * dy + dz * dz))
    return center, max(radius, 1.0), bbox_min, bbox_max


def _chain_map(atoms: list[Atom]) -> dict[str, int]:
    chains = sorted({atom.chain for atom in atoms})
    return {chain: i for i, chain in enumerate(chains)}


def _infer_bonds(atoms: list[Atom], max_distance: float, scale: float) -> list[tuple[int, int]]:
    bonds: list[tuple[int, int]] = []
    max_dist2 = max_distance * max_distance
    for i, atom_i in enumerate(atoms):
        ri = COVALENT_RADII.get(atom_i.element, 0.77)
        for j in range(i + 1, len(atoms)):
            atom_j = atoms[j]
            dx = atom_i.x - atom_j.x
            dy = atom_i.y - atom_j.y
            dz = atom_i.z - atom_j.z
            d2 = dx * dx + dy * dy + dz * dz
            if d2 < 0.16 or d2 > max_dist2:
                continue
            rj = COVALENT_RADII.get(atom_j.element, 0.77)
            cutoff = scale * (ri + rj)
            if d2 <= cutoff * cutoff:
                bonds.append((i, j))
    return bonds


def _write_f32(path: Path, values: list[float]) -> None:
    with path.open("wb") as f:
        for value in values:
            f.write(struct.pack("<f", float(value)))


def _write_u32(path: Path, values: list[int]) -> None:
    with path.open("wb") as f:
        for value in values:
            f.write(struct.pack("<I", int(value)))


def _write_rgba8(path: Path, values: list[tuple[int, int, int, int]]) -> None:
    with path.open("wb") as f:
        for rgba in values:
            f.write(bytes(rgba))


def _export_bundle(
    pdb_id: str,
    atoms: list[Atom],
    bonds: list[tuple[int, int]],
    source: str,
    output_dir: Path,
) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    center, radius, bbox_min, bbox_max = _center_and_radius(atoms)
    chains = _chain_map(atoms)

    positions: list[float] = []
    radius_vdw: list[float] = []
    radius_ball: list[float] = []
    colors_element: list[tuple[int, int, int, int]] = []
    colors_chain: list[tuple[int, int, int, int]] = []
    atom_chain: list[int] = []
    atom_residue: list[int] = []
    bfactor_values = [atom.bfactor for atom in atoms]
    bmin = min(bfactor_values) if bfactor_values else 0.0
    bmax = max(bfactor_values) if bfactor_values else 1.0
    bspan = max(bmax - bmin, 1.0)
    colors_bfactor: list[tuple[int, int, int, int]] = []

    for atom in atoms:
        positions.extend(
            [
                (atom.x - center[0]) / radius,
                (atom.y - center[1]) / radius,
                (atom.z - center[2]) / radius,
            ]
        )
        vdw = VDW_RADII.get(atom.element, 1.70) / radius
        radius_vdw.append(vdw)
        radius_ball.append(0.33 * vdw)
        colors_element.append(ELEMENT_COLORS.get(atom.element, (180, 180, 180, 255)))
        chain_index = chains[atom.chain]
        colors_chain.append(CHAIN_PALETTE[chain_index % len(CHAIN_PALETTE)])
        atom_chain.append(chain_index)
        atom_residue.append(max(atom.residue_index, 0))
        t = (atom.bfactor - bmin) / bspan
        colors_bfactor.append((int(60 + 180 * t), int(130 - 70 * t), int(220 - 150 * t), 255))

    bond_index: list[int] = []
    for i, j in bonds:
        bond_index.extend([i, j])

    _write_f32(output_dir / "atom_position.f32", positions)
    _write_f32(output_dir / "atom_radius_vdw.f32", radius_vdw)
    _write_f32(output_dir / "atom_radius_ball.f32", radius_ball)
    _write_rgba8(output_dir / "atom_color_element.rgba8", colors_element)
    _write_rgba8(output_dir / "atom_color_chain.rgba8", colors_chain)
    _write_rgba8(output_dir / "atom_color_bfactor.rgba8", colors_bfactor)
    _write_u32(output_dir / "atom_chain.u32", atom_chain)
    _write_u32(output_dir / "atom_residue.u32", atom_residue)
    _write_u32(output_dir / "bond_index.u32", bond_index)

    metadata = {
        "format": "datoviz.protein.bundle.v1",
        "pdb_id": pdb_id.upper(),
        "source": source,
        "normalized": True,
        "normalization": "position = (angstrom - center) / radius",
        "center": list(center),
        "radius": radius,
        "bbox_min": bbox_min,
        "bbox_max": bbox_max,
        "atom_count": len(atoms),
        "bond_count": len(bonds),
        "chain_count": len(chains),
        "chains": sorted(chains, key=chains.get),
        "has_surface": False,
        "has_ribbon": False,
        "files": {
            "atom_position": "atom_position.f32",
            "atom_radius_vdw": "atom_radius_vdw.f32",
            "atom_radius_ball": "atom_radius_ball.f32",
            "atom_color_element": "atom_color_element.rgba8",
            "atom_color_chain": "atom_color_chain.rgba8",
            "atom_color_bfactor": "atom_color_bfactor.rgba8",
            "atom_chain": "atom_chain.u32",
            "atom_residue": "atom_residue.u32",
            "bond_index": "bond_index.u32",
        },
    }
    (output_dir / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n")


def _parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pdb_id", help="PDB identifier, for example 1UBQ or 1CRN")
    parser.add_argument("--input", type=Path, help="existing PDB file; skips download")
    parser.add_argument(
        "--out",
        type=Path,
        help="output bundle directory; default: ~/.cache/datoviz/proteins/<pdb_id>",
    )
    parser.add_argument("--include-hetero", action="store_true", help="include HETATM records")
    parser.add_argument("--keep-waters", action="store_true", help="keep water molecules")
    parser.add_argument("--bond-scale", type=float, default=1.25, help="covalent radius scale")
    parser.add_argument(
        "--max-bond-distance",
        type=float,
        default=2.25,
        help="absolute bond cutoff in Angstroms",
    )
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = _parse_args(argv)
    pdb_id = args.pdb_id.strip().upper()
    if len(pdb_id) != 4 or not pdb_id.isalnum():
        print(f"invalid PDB id: {args.pdb_id!r}", file=sys.stderr)
        return 2

    output_dir = args.out if args.out is not None else _default_cache_dir(pdb_id)
    text, source = _load_source(pdb_id, args.input)
    atoms = _parse_pdb(text, args.include_hetero, args.keep_waters)
    if not atoms:
        print("no atoms found after filtering", file=sys.stderr)
        return 1

    bonds = _infer_bonds(atoms, args.max_bond_distance, args.bond_scale)
    _export_bundle(pdb_id, atoms, bonds, source, output_dir)
    print(
        f"wrote {output_dir} ({len(atoms)} atoms, {len(bonds)} inferred bonds, "
        f"{len(_chain_map(atoms))} chains)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
