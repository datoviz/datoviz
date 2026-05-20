#!/usr/bin/env python3
"""Report oversized and boilerplate-heavy scene spec documents."""

from __future__ import annotations

import argparse
from pathlib import Path


DEFAULT_PATTERNS = (
    "Agent Pickup",
    "API not finalized",
    "not assume the final",
    "FramePlan",
    "DRP2",
    "pressure",
    "download",
    "cache",
    "synthetic fallback",
)


def _count_lines(path: Path) -> int:
    with path.open("r", encoding="utf8") as f:
        return sum(1 for _ in f)


def _count_matches(text: str, patterns: tuple[str, ...]) -> int:
    lower = text.lower()
    return sum(lower.count(pattern.lower()) for pattern in patterns)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path("spec/scene"))
    parser.add_argument("--line-threshold", type=int, default=450)
    parser.add_argument("--top", type=int, default=40)
    args = parser.parse_args()

    rows = []
    for path in sorted(args.root.rglob("*.md")):
        text = path.read_text(encoding="utf8")
        lines = text.count("\n") + (0 if text.endswith("\n") else 1)
        fences = text.count("```")
        headings = sum(1 for line in text.splitlines() if line.startswith("#"))
        boilerplate = _count_matches(text, DEFAULT_PATTERNS)
        if lines >= args.line_threshold or boilerplate:
            rows.append((lines, boilerplate, fences, headings, path))

    rows.sort(reverse=True)
    print("lines boilerplate fences headings path")
    for lines, boilerplate, fences, headings, path in rows[: args.top]:
        print(f"{lines:5d} {boilerplate:11d} {fences:6d} {headings:8d} {path}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
