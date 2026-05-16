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


Vec3 = tuple[float, float, float]

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
    insertion_code: str
    x: float
    y: float
    z: float
    occupancy: float
    bfactor: float
    hetero: bool


@dataclass
class Residue:
    chain: str
    residue_index: int
    insertion_code: str
    name: str
    atoms: dict[str, Atom]
    ss: int = 0


SS_COIL = 0
SS_HELIX = 1
SS_SHEET = 2
SS_TURN = 3

SS_COLORS = {
    SS_COIL: (185, 185, 185, 255),
    SS_HELIX: (220, 75, 65, 255),
    SS_SHEET: (230, 190, 65, 255),
    SS_TURN: (85, 155, 225, 255),
}

RIBBON_CROSS_SECTION_COUNT = 16


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
            insertion_code=insertion_code,
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


def _residues_from_atoms(atoms: list[Atom]) -> list[Residue]:
    residues: dict[tuple[str, int, str], Residue] = {}
    for atom in atoms:
        key = (atom.chain, atom.residue_index, atom.insertion_code)
        residue = residues.get(key)
        if residue is None:
            residue = Residue(
                chain=atom.chain,
                residue_index=atom.residue_index,
                insertion_code=atom.insertion_code,
                name=atom.residue,
                atoms={},
            )
            residues[key] = residue
        residue.atoms[atom.name.strip().upper()] = atom
    return sorted(residues.values(), key=lambda r: (r.chain, r.residue_index, r.insertion_code))


def _ss_category(code: str) -> int:
    code = (code or " ").strip().upper()
    if code in {"H", "G", "I"}:
        return SS_HELIX
    if code in {"E", "B"}:
        return SS_SHEET
    if code in {"T", "S", "P"}:
        return SS_TURN
    return SS_COIL


def _assign_dssp(residues: list[Residue], pdb_text: str, enabled: bool) -> bool:
    if not enabled:
        return False
    try:
        import rs_dssp  # type: ignore
    except ImportError:
        print("warning: --dssp requested but rs-dssp is unavailable", file=sys.stderr)
        return False

    try:
        result = rs_dssp.assign_from_string(pdb_text, calculate_sasa=False)
    except Exception as exc:
        print(f"warning: DSSP assignment failed: {exc}", file=sys.stderr)
        return False

    lookup: dict[tuple[str, int], int] = {}
    for item in result.residues:
        try:
            chain = str(item.chain_id)
            seq_id = int(item.seq_id)
            code = str(item.structure)
        except (AttributeError, TypeError, ValueError):
            continue
        lookup[(chain, seq_id)] = _ss_category(code)

    assigned = 0
    for residue in residues:
        ss = lookup.get((residue.chain, residue.residue_index))
        if ss is None:
            continue
        residue.ss = ss
        assigned += 1
    return assigned > 0


def _center_and_radius(atoms: list[Atom]) -> tuple[Vec3, float, list[float], list[float]]:
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


def _v_add(a: Vec3, b: Vec3) -> Vec3:
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def _v_sub(a: Vec3, b: Vec3) -> Vec3:
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def _v_mul(a: Vec3, s: float) -> Vec3:
    return (a[0] * s, a[1] * s, a[2] * s)


def _v_dot(a: Vec3, b: Vec3) -> float:
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def _v_cross(a: Vec3, b: Vec3) -> Vec3:
    return (
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    )


def _v_len(a: Vec3) -> float:
    return math.sqrt(_v_dot(a, a))


def _v_norm(a: Vec3, fallback: Vec3 = (0.0, 0.0, 1.0)) -> Vec3:
    length = _v_len(a)
    if length <= 1e-12:
        return fallback
    return (a[0] / length, a[1] / length, a[2] / length)


def _atom_pos(atom: Atom) -> Vec3:
    return (atom.x, atom.y, atom.z)


def _catmull_rom(
    p0: Vec3,
    p1: Vec3,
    p2: Vec3,
    p3: Vec3,
    t: float,
) -> Vec3:
    t2 = t * t
    t3 = t2 * t
    return (
        0.5
        * (
            2 * p1[0]
            + (-p0[0] + p2[0]) * t
            + (2 * p0[0] - 5 * p1[0] + 4 * p2[0] - p3[0]) * t2
            + (-p0[0] + 3 * p1[0] - 3 * p2[0] + p3[0]) * t3
        ),
        0.5
        * (
            2 * p1[1]
            + (-p0[1] + p2[1]) * t
            + (2 * p0[1] - 5 * p1[1] + 4 * p2[1] - p3[1]) * t2
            + (-p0[1] + 3 * p1[1] - 3 * p2[1] + p3[1]) * t3
        ),
        0.5
        * (
            2 * p1[2]
            + (-p0[2] + p2[2]) * t
            + (2 * p0[2] - 5 * p1[2] + 4 * p2[2] - p3[2]) * t2
            + (-p0[2] + 3 * p1[2] - 3 * p2[2] + p3[2]) * t3
        ),
    )


def _catmull_rom_tangent(
    p0: Vec3,
    p1: Vec3,
    p2: Vec3,
    p3: Vec3,
    t: float,
) -> Vec3:
    t2 = t * t
    return (
        0.5
        * (
            (-p0[0] + p2[0])
            + 2 * (2 * p0[0] - 5 * p1[0] + 4 * p2[0] - p3[0]) * t
            + 3 * (-p0[0] + 3 * p1[0] - 3 * p2[0] + p3[0]) * t2
        ),
        0.5
        * (
            (-p0[1] + p2[1])
            + 2 * (2 * p0[1] - 5 * p1[1] + 4 * p2[1] - p3[1]) * t
            + 3 * (-p0[1] + 3 * p1[1] - 3 * p2[1] + p3[1]) * t2
        ),
        0.5
        * (
            (-p0[2] + p2[2])
            + 2 * (2 * p0[2] - 5 * p1[2] + 4 * p2[2] - p3[2]) * t
            + 3 * (-p0[2] + 3 * p1[2] - 3 * p2[2] + p3[2]) * t2
        ),
    )


def _residue_up(residue: Residue, tangent: Vec3, fallback: Vec3) -> Vec3:
    n = residue.atoms.get("N")
    ca = residue.atoms.get("CA")
    c = residue.atoms.get("C")
    up = fallback
    if n is not None and ca is not None and c is not None:
        n_to_ca = _v_sub(_atom_pos(n), _atom_pos(ca))
        c_to_ca = _v_sub(_atom_pos(c), _atom_pos(ca))
        up = _v_norm(_v_cross(n_to_ca, c_to_ca), up)
    up = _v_sub(up, _v_mul(tangent, _v_dot(up, tangent)))
    return _v_norm(up, fallback)


def _initial_ribbon_frame(residue: Residue, tangent: Vec3) -> tuple[Vec3, Vec3]:
    up = _residue_up(residue, tangent, (0.0, 0.0, 1.0))
    side = _v_norm(_v_cross(tangent, up), (1.0, 0.0, 0.0))
    up = _v_norm(_v_cross(side, tangent), up)
    return side, up


def _transport_ribbon_frame(
    tangent: Vec3, previous_side: Vec3, previous_up: Vec3
) -> tuple[Vec3, Vec3]:
    side = _v_sub(previous_side, _v_mul(tangent, _v_dot(previous_side, tangent)))
    if _v_len(side) <= 1e-8:
        side = _v_cross(tangent, previous_up)
    side = _v_norm(side, previous_side)
    if _v_dot(side, previous_side) < 0.0:
        side = _v_mul(side, -1.0)
    up = _v_norm(_v_cross(side, tangent), previous_up)
    return side, up


def _ribbon_section(
    p: Vec3, side: Vec3, up: Vec3, half_w: float, half_t: float, count: int
) -> list[tuple[Vec3, Vec3]]:
    section: list[tuple[Vec3, Vec3]] = []
    inv_w = 1.0 / max(half_w, 1e-6)
    inv_t = 1.0 / max(half_t, 1e-6)
    for i in range(count):
        angle = 2.0 * math.pi * i / float(count)
        c = math.cos(angle)
        s = math.sin(angle)
        pos = _v_add(_v_add(p, _v_mul(side, half_w * c)), _v_mul(up, half_t * s))
        normal = _v_norm(_v_add(_v_mul(side, c * inv_w), _v_mul(up, s * inv_t)))
        section.append((pos, normal))
    return section


def _append_ribbon_section(
    positions: list[float],
    normals: list[float],
    colors_chain: list[tuple[int, int, int, int]],
    colors_ss: list[tuple[int, int, int, int]],
    p: Vec3,
    side: Vec3,
    up: Vec3,
    half_w: float,
    half_t: float,
    chain_color: tuple[int, int, int, int],
    ss_color: tuple[int, int, int, int],
) -> None:
    for pos, normal in _ribbon_section(
        p, side, up, half_w, half_t, RIBBON_CROSS_SECTION_COUNT):
        positions.extend(pos)
        normals.extend(normal)
        colors_chain.append(chain_color)
        colors_ss.append(ss_color)


def _append_ribbon_section_indices(indices: list[int], previous: int, current: int) -> None:
    for i in range(RIBBON_CROSS_SECTION_COUNT):
        j = (i + 1) % RIBBON_CROSS_SECTION_COUNT
        q0 = previous + i
        q1 = previous + j
        q2 = current + j
        q3 = current + i
        indices.extend([q0, q1, q2, q0, q2, q3])


def _ribbon_shape(ss: int, radius: float) -> tuple[float, float]:
    scale = 1.0 / radius
    if ss == SS_HELIX:
        return 1.05 * scale, 0.22 * scale
    if ss == SS_SHEET:
        return 1.35 * scale, 0.16 * scale
    if ss == SS_TURN:
        return 0.62 * scale, 0.13 * scale
    return 0.46 * scale, 0.11 * scale


def _ribbon_mesh(
    residues: list[Residue],
    chains: dict[str, int],
    center: Vec3,
    radius: float,
    samples_per_segment: int,
) -> dict[str, list]:
    positions: list[float] = []
    normals: list[float] = []
    colors_chain: list[tuple[int, int, int, int]] = []
    colors_ss: list[tuple[int, int, int, int]] = []
    indices: list[int] = []
    residue_ss: list[int] = []

    by_chain: dict[str, list[Residue]] = {}
    for residue in residues:
        if "CA" not in residue.atoms:
            continue
        by_chain.setdefault(residue.chain, []).append(residue)
        residue_ss.append(residue.ss)

    vertex_count = 0
    for chain, chain_residues in sorted(by_chain.items()):
        if len(chain_residues) < 2:
            continue
        ca = [_atom_pos(residue.atoms["CA"]) for residue in chain_residues]
        p0 = ca[0]
        p1 = ca[0]
        p2 = ca[1]
        p3 = ca[min(2, len(ca) - 1)]
        tangent = _v_norm(_catmull_rom_tangent(p0, p1, p2, p3, 0.0), (0.0, 0.0, 1.0))
        side, up = _initial_ribbon_frame(chain_residues[0], tangent)
        section_count = 0
        for i in range(len(chain_residues) - 1):
            segment_samples = samples_per_segment
            for s in range(segment_samples):
                t = s / float(segment_samples)
                p0 = ca[max(i - 1, 0)]
                p1 = ca[i]
                p2 = ca[i + 1]
                p3 = ca[min(i + 2, len(ca) - 1)]
                p = _catmull_rom(p0, p1, p2, p3, t)
                tangent = _v_norm(_catmull_rom_tangent(p0, p1, p2, p3, t), (0.0, 0.0, 1.0))
                residue = chain_residues[i if t < 0.5 else i + 1]
                if section_count > 0:
                    side, up = _transport_ribbon_frame(tangent, side, up)
                width, thickness = _ribbon_shape(residue.ss, radius)
                p_norm = (
                    (p[0] - center[0]) / radius,
                    (p[1] - center[1]) / radius,
                    (p[2] - center[2]) / radius,
                )
                half_w = 0.5 * width
                half_t = 0.5 * thickness
                chain_color = CHAIN_PALETTE[chains[chain] % len(CHAIN_PALETTE)]
                ss_color = SS_COLORS.get(residue.ss, SS_COLORS[SS_COIL])
                _append_ribbon_section(
                    positions, normals, colors_chain, colors_ss, p_norm, side, up, half_w, half_t,
                    chain_color, ss_color)
                if section_count > 0:
                    a = vertex_count - RIBBON_CROSS_SECTION_COUNT
                    b = vertex_count
                    _append_ribbon_section_indices(indices, a, b)
                vertex_count += RIBBON_CROSS_SECTION_COUNT
                section_count += 1
        # Add an explicit final section at the last C-alpha.
        p = ca[-1]
        tangent = _v_norm(_v_sub(ca[-1], ca[-2]), (0.0, 0.0, 1.0))
        residue = chain_residues[-1]
        side, up = _transport_ribbon_frame(tangent, side, up)
        width, thickness = _ribbon_shape(residue.ss, radius)
        p_norm = (
            (p[0] - center[0]) / radius,
            (p[1] - center[1]) / radius,
            (p[2] - center[2]) / radius,
        )
        half_w = 0.5 * width
        half_t = 0.5 * thickness
        chain_color = CHAIN_PALETTE[chains[chain] % len(CHAIN_PALETTE)]
        ss_color = SS_COLORS.get(residue.ss, SS_COLORS[SS_COIL])
        _append_ribbon_section(
            positions, normals, colors_chain, colors_ss, p_norm, side, up, half_w, half_t,
            chain_color, ss_color)
        if section_count > 0:
            a = vertex_count - RIBBON_CROSS_SECTION_COUNT
            b = vertex_count
            _append_ribbon_section_indices(indices, a, b)
        vertex_count += RIBBON_CROSS_SECTION_COUNT

    return {
        "position": positions,
        "normal": normals,
        "color_chain": colors_chain,
        "color_ss": colors_ss,
        "index": indices,
        "residue_ss": residue_ss,
    }


def _write_f32(path: Path, values: list[float]) -> None:
    with path.open("wb") as f:
        for value in values:
            f.write(struct.pack("<f", float(value)))


def _write_u32(path: Path, values: list[int]) -> None:
    with path.open("wb") as f:
        for value in values:
            f.write(struct.pack("<I", int(value)))


def _write_u8(path: Path, values: list[int]) -> None:
    with path.open("wb") as f:
        f.write(bytes(max(0, min(255, int(value))) for value in values))


def _write_rgba8(path: Path, values: list[tuple[int, int, int, int]]) -> None:
    with path.open("wb") as f:
        for rgba in values:
            f.write(bytes(rgba))


def _export_bundle(
    pdb_id: str,
    atoms: list[Atom],
    bonds: list[tuple[int, int]],
    source: str,
    pdb_text: str,
    output_dir: Path,
    use_dssp: bool,
    ribbon_samples: int,
) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    center, radius, bbox_min, bbox_max = _center_and_radius(atoms)
    chains = _chain_map(atoms)
    residues = _residues_from_atoms(atoms)
    dssp_assigned = _assign_dssp(residues, pdb_text, use_dssp)
    ribbon = _ribbon_mesh(residues, chains, center, radius, max(1, ribbon_samples))

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
    _write_f32(output_dir / "ribbon_position.f32", ribbon["position"])
    _write_f32(output_dir / "ribbon_normal.f32", ribbon["normal"])
    _write_rgba8(output_dir / "ribbon_color_chain.rgba8", ribbon["color_chain"])
    _write_rgba8(output_dir / "ribbon_color_ss.rgba8", ribbon["color_ss"])
    _write_u32(output_dir / "ribbon_index.u32", ribbon["index"])
    _write_u8(output_dir / "residue_secondary_structure.u8", ribbon["residue_ss"])

    ribbon_vertex_count = len(ribbon["position"]) // 3
    ribbon_index_count = len(ribbon["index"])

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
        "residue_count": len(residues),
        "chain_count": len(chains),
        "chains": sorted(chains, key=chains.get),
        "has_surface": False,
        "has_ribbon": ribbon_vertex_count > 0 and ribbon_index_count > 0,
        "dssp_assigned": dssp_assigned,
        "secondary_structure_encoding": {
            "0": "coil",
            "1": "helix",
            "2": "sheet",
            "3": "turn",
        },
        "ribbon_samples_per_segment": max(1, ribbon_samples),
        "ribbon_cross_section_count": RIBBON_CROSS_SECTION_COUNT,
        "ribbon_vertex_count": ribbon_vertex_count,
        "ribbon_index_count": ribbon_index_count,
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
            "ribbon_position": "ribbon_position.f32",
            "ribbon_normal": "ribbon_normal.f32",
            "ribbon_color_chain": "ribbon_color_chain.rgba8",
            "ribbon_color_ss": "ribbon_color_ss.rgba8",
            "ribbon_index": "ribbon_index.u32",
            "residue_secondary_structure": "residue_secondary_structure.u8",
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
    parser.add_argument(
        "--dssp",
        action="store_true",
        help="assign secondary structure with optional rs-dssp Python package",
    )
    parser.add_argument(
        "--ribbon-samples",
        type=int,
        default=10,
        help="interpolated ribbon sections per residue segment",
    )
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
    _export_bundle(
        pdb_id, atoms, bonds, source, text, output_dir, args.dssp, args.ribbon_samples)
    print(
        f"wrote {output_dir} ({len(atoms)} atoms, {len(bonds)} inferred bonds, "
        f"{len(_chain_map(atoms))} chains)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
