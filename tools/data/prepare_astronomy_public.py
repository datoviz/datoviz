#!/usr/bin/env python3
"""Document public astronomy data blockers for planned examples."""

from __future__ import annotations

import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
OUT_DIR = ROOT / "data" / "examples" / "external_public" / "astronomy"


def prepare(out_dir: Path) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    blockers = """# Astronomy Public Data Blockers

Planned source:

- Gaia DR3 `gaiadr3.gaia_source` through ESA Gaia Archive TAP.

Why no prepared cache is generated here by default:

- The planned Pleiades stress-test query is up to 250,000 rows, which is a comparatively large
  network result for repository example data.
- The runtime example should not depend on TAP availability or astronomy package parsing.
- A future narrow preparation step should either cap the row count much lower for a committed
  sample or publish a separately downloadable cache with explicit size/version checks.

Reference endpoints:

- https://gea.esac.esa.int/archive/
- https://gea.esac.esa.int/tap-server/tap
- https://gea.esac.esa.int/archive/documentation/GDR3/
"""
    (out_dir / "BLOCKERS.md").write_text(blockers, encoding="utf-8")

    metadata = {
        "dataset": "gaia_dr3",
        "status": "blocked",
        "reason": "The planned Gaia cache is a large TAP query; do not commit it by default.",
        "source": "ESA Gaia Archive TAP",
        "source_urls": [
            "https://gea.esac.esa.int/archive/",
            "https://gea.esac.esa.int/tap-server/tap",
            "https://gea.esac.esa.int/archive/documentation/GDR3/",
        ],
    }
    (out_dir / "metadata.json").write_text(
        json.dumps(metadata, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=Path, default=OUT_DIR)
    args = parser.parse_args()
    prepare(args.out)


if __name__ == "__main__":
    main()
