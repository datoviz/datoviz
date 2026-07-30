#!/usr/bin/env python3
"""Validate public Python binding guidance against generated-facade policy."""

from __future__ import annotations

import argparse
import re
from pathlib import Path

import yaml


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_POLICY = ROOT / "spec/bindings/ctypes.yml"
DEFAULT_REFERENCE = ROOT / "docs/reference/ctypes.md"
FACADE_HELPERS = {
    "dvz_sampled_field_from_array",
    "dvz_sampled_field_update_from_array",
    "dvz_view_capture_rgba",
}
REQUIRED_CONCEPTS = {
    "datoviz.raw": "exact-call surface",
    "callback": "callback lifetime",
    "GSP/VisPy2": "high-level plotting boundary",
    "NumPy": "NumPy-adapted surface",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--policy", type=Path, default=DEFAULT_POLICY)
    parser.add_argument("--reference", type=Path, default=DEFAULT_REFERENCE)
    return parser.parse_args()


def validate(policy_path: Path, reference_path: Path) -> list[str]:
    """Return missing adapted-call and binding-concept documentation."""
    policy = yaml.safe_load(policy_path.read_text(encoding="utf8")) or {}
    facade = policy.get("array_facade") or {}
    expected_functions = set(str(name) for name in facade) | FACADE_HELPERS
    text = reference_path.read_text(encoding="utf8")
    documented_functions = set(re.findall(r"`(dvz_[A-Za-z0-9_]+)\(\)`", text))
    errors = [
        f"Python API reference is missing adapted call {name}()"
        for name in sorted(expected_functions - documented_functions)
    ]
    errors.extend(
        f"Python API reference is missing {description} ({needle})"
        for needle, description in REQUIRED_CONCEPTS.items()
        if needle not in text
    )
    return errors


def main() -> int:
    args = parse_args()
    errors = validate(args.policy, args.reference)
    if errors:
        print("\n".join(errors))
        return 1
    policy = yaml.safe_load(args.policy.read_text(encoding="utf8")) or {}
    count = len(policy.get("array_facade") or {}) + len(FACADE_HELPERS)
    print(f"Python API documentation coverage: {count} adapted calls/helpers")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
