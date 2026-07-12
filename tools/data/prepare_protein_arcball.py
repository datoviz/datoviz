#!/usr/bin/env python3
"""Prepare a C-compatible protein arcball bundle manifest."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

from common import artifact, command_argv, relpath, write_manifest, write_provenance


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_PDB_ID = "1UBQ"
DEFAULT_ROOT = ROOT / "data" / "examples" / "proteins" / "1ubq"


def prepare(pdb_id: str, bundle_root: Path, regenerate: bool) -> None:
    """Prepare a C-compatible protein bundle and manifest.

    @param pdb_id PDB identifier
    @param bundle_root output bundle root
    @param regenerate whether to run the protein preprocessor before writing metadata
    """
    prepared = bundle_root / "prepared"
    if regenerate:
        subprocess.check_call(
            [
                sys.executable,
                "tools/preprocess_protein.py",
                pdb_id,
                "--out",
                prepared.as_posix(),
            ],
            cwd=ROOT,
        )
    if not prepared.exists():
        raise FileNotFoundError(prepared)

    artifacts = []
    for path in sorted(prepared.iterdir()):
        if not path.is_file():
            continue
        suffix = path.suffix.lstrip(".") or "binary"
        role = path.stem
        artifacts.append(artifact(path, bundle_root, role, suffix))

    source_url = f"https://files.rcsb.org/download/{pdb_id.upper()}.pdb"
    license_name = "CC0-1.0"
    usage_url = "https://www.wwpdb.org/about/usage-policies"
    write_manifest(
        bundle_root,
        example_id=f"proteins/{pdb_id.lower()}",
        title=f"{pdb_id.upper()} Protein Arcball Bundle",
        status="committed",
        script=relpath(Path(__file__), ROOT),
        command=command_argv(
            relpath(Path(__file__), ROOT), [pdb_id, "--regenerate"] if regenerate else [pdb_id]
        ),
        source={
            "name": f"RCSB PDB {pdb_id.upper()}",
            "url": source_url,
            "license": license_name,
            "usage_url": usage_url,
        },
        artifacts=artifacts,
        validation={"artifact_count": len(artifacts)},
    )
    write_provenance(
        bundle_root,
        title=f"{pdb_id.upper()} Protein Arcball Bundle",
        source_lines=[f"Downloaded source: `{source_url}`."],
        processing_lines=[
            "Generated with `tools/preprocess_protein.py` into the C viewer bundle format.",
            "The prepared directory contains raw `.f32`, `.u32`, `.u8`, and `.rgba8` arrays.",
        ],
        license_lines=[
            "PDB archive data files are available under the CC0 1.0 Universal Public Domain "
            f"Dedication: {usage_url}.",
            "Attribution to the original structure authors is encouraged.",
        ],
    )
    print(f"wrote {relpath(bundle_root, ROOT)}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pdb_id", nargs="?", default=DEFAULT_PDB_ID)
    parser.add_argument("--output", type=Path, default=DEFAULT_ROOT)
    parser.add_argument("--regenerate", action="store_true")
    args = parser.parse_args()
    prepare(args.pdb_id, args.output, args.regenerate)


if __name__ == "__main__":
    main()
