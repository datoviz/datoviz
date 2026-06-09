#!/usr/bin/env python3
"""Run available Datoviz example-data preparation scripts."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


SCRIPT_GROUPS: dict[str, list[str]] = {
    "existing": [
        "tools/data/prepare_lidar.py",
        "tools/data/prepare_protein_arcball.py",
        "tools/data/prepare_protein_bundle.py",
        "tools/data/prepare_existing_allen_ibl.py",
    ],
    "napari": [
        "tools/data/prepare_cells3d.py",
        "tools/data/prepare_bbbc038.py",
        "tools/data/prepare_napari_synthetic_spatial.py",
        "tools/data/prepare_cell_tracking_synthetic.py",
    ],
    "generated": [
        "tools/data/prepare_fea_cantilever.py",
        "tools/data/prepare_perovskite_phonon.py",
        "tools/data/prepare_cfd_vortex.py",
        "tools/data/prepare_tokamak.py",
        "tools/data/prepare_image_embedding_toy.py",
        "tools/data/prepare_embedding_atlas.py",
        "tools/data/prepare_lipid_brain_atlas.py",
    ],
    "external": [
        "tools/data/prepare_geo_public.py",
        "tools/data/prepare_physics_public.py",
        "tools/data/prepare_astronomy_public.py",
        "tools/data/prepare_bio_public.py",
    ],
}


POSTPROCESS_SCRIPTS = [
    "tools/data/normalize_manifests.py",
    "tools/data/validate_manifests.py",
]


def _run(script: str, keep_going: bool) -> bool:
    path = ROOT / script
    if not path.exists():
        print(f"skip missing {script}")
        return False

    print(f"run {script}")
    result = subprocess.run([sys.executable, script], cwd=ROOT, check=False)
    if result.returncode != 0:
        print(f"failed {script}: exit {result.returncode}", file=sys.stderr)
        if not keep_going:
            raise SystemExit(result.returncode)
        return False
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "groups",
        nargs="*",
        choices=sorted(SCRIPT_GROUPS),
        help="groups to run; default: all groups",
    )
    parser.add_argument("--keep-going", action="store_true", help="continue after script failures")
    args = parser.parse_args()

    groups = args.groups or list(SCRIPT_GROUPS)
    ok = 0
    failed_or_missing = 0
    for group in groups:
        for script in SCRIPT_GROUPS[group]:
            if _run(script, args.keep_going):
                ok += 1
            else:
                failed_or_missing += 1

    for script in POSTPROCESS_SCRIPTS:
        if _run(script, args.keep_going):
            ok += 1
        else:
            failed_or_missing += 1

    print(f"prepared: {ok}; failed_or_missing: {failed_or_missing}")
    return 0 if failed_or_missing == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
