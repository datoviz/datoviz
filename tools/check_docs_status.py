#!/usr/bin/env python3
"""Check mechanically derived public documentation status facts for drift."""

from __future__ import annotations

import re
import sys
from pathlib import Path

import build_gallery


ROOT = Path(__file__).resolve().parents[1]
REFERENCE = ROOT / "docs" / "reference"
VISUALS = REFERENCE / "visual-families"
MANIFEST = ROOT / "examples" / "c" / "MANIFEST.yaml"

STATUS_LABELS = {
    "supported",
    "experimental",
    "advanced/unstable",
    "deferred",
    "external/GSP",
}
WEBGPU_LABELS = {
    "webgpu-live": "live",
    "webgpu-planned": "planned",
    "webgpu-deferred": "deferred",
    "native-only": "native only",
}


def _error(errors: list[str], path: Path, line: int, message: str) -> None:
    errors.append(f"{path.relative_to(ROOT)}:{line}: {message}")


def _line(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def _table_rows(text: str, heading: str) -> list[tuple[int, list[str]]]:
    match = re.search(rf"^## {re.escape(heading)}\s*$", text, re.M)
    if match is None:
        return []
    section_start = match.end()
    next_heading = re.search(r"^## ", text[section_start:], re.M)
    section_end = section_start + next_heading.start() if next_heading else len(text)
    rows: list[tuple[int, list[str]]] = []
    for item in re.finditer(r"^\|(.+)\|\s*$", text[section_start:section_end], re.M):
        cells = [cell.strip() for cell in item.group(1).split("|")]
        if cells and all(re.fullmatch(r":?-+:?", cell) for cell in cells):
            continue
        rows.append((_line(text, section_start + item.start()), cells))
    return rows


def _check_webgpu_page(errors: list[str]) -> None:
    path = REFERENCE / "webgpu-subset.md"
    text = path.read_text(encoding="utf8")
    link = "[WebGPU matrix](../examples/webgpu-matrix.md)"
    if link not in text:
        _error(errors, path, 1, f"must link the generated matrix as {link}")
    for match in re.finditer(r"^\|\s*Browser status\s*\|\s*Count\s*\|", text, re.M | re.I):
        _error(errors, path, _line(text, match.start()), "must not copy manifest status counts")
    for match in re.finditer(
        r"^\|\s*(?:Live in browser|Planned|Deferred|Native only)\s*\|\s*\d+\s*\|",
        text,
        re.M | re.I,
    ):
        _error(errors, path, _line(text, match.start()), "hard-coded WebGPU status count")


def _visual_entries() -> dict[str, build_gallery.Example]:
    examples = build_gallery.collect_examples(build_gallery.load_manifest(MANIFEST))
    return {
        example.primary: example
        for example in examples
        if example.lane == "visuals" and example.primary
    }


def _check_visual_pages(errors: list[str]) -> None:
    entries = _visual_entries()
    index_status: dict[str, tuple[int, str]] = {}
    index_path = VISUALS / "index.md"
    for line_no, line in enumerate(index_path.read_text(encoding="utf8").splitlines(), 1):
        match = re.search(r"\| \[([^]]+)\]\(([^)]+)\.md\) \| ([^|]+) \|", line)
        if match:
            index_status[Path(match.group(2)).name] = (line_no, match.group(3).strip())

    for name, example in sorted(entries.items()):
        path = VISUALS / f"{name}.md"
        if not path.is_file():
            _error(errors, path, 1, f"missing page for manifest visual {example.id}")
            continue
        text = path.read_text(encoding="utf8")
        expected_status = example.status.strip()
        status = re.search(r"^Status:\s*([^.]+)\.\s*$", text, re.M)
        if status is None:
            _error(errors, path, 1, "missing single-line Status declaration")
        elif status.group(1).strip() != expected_status:
            _error(
                errors,
                path,
                _line(text, status.start()),
                f"Status {status.group(1).strip()!r} != manifest gallery status {expected_status!r}",
            )

        backend = re.search(r"^Backends:\s*native;\s*WebGPU ([^( .]+(?: [^( .]+)?)", text, re.M)
        expected_backend = WEBGPU_LABELS.get(example.webgpu_status)
        if expected_backend is None:
            _error(errors, path, 1, f"unknown manifest WebGPU status {example.webgpu_status!r}")
        elif backend is None:
            _error(errors, path, 1, "missing 'Backends: native; WebGPU <status>' declaration")
        elif backend.group(1).strip() != expected_backend:
            _error(
                errors,
                path,
                _line(text, backend.start()),
                f"WebGPU backend {backend.group(1).strip()!r} != manifest {expected_backend!r}",
            )

        indexed = index_status.get(name)
        if indexed is None:
            _error(errors, index_path, 1, f"missing index row for visual {name}")
        elif indexed[1] != expected_status:
            _error(
                errors,
                index_path,
                indexed[0],
                f"index status {indexed[1]!r} != manifest gallery status {expected_status!r}",
            )


def _check_status_tables(errors: list[str]) -> None:
    project = REFERENCE / "project-status.md"
    text = project.read_text(encoding="utf8")
    vocabulary: list[tuple[int, str]] = []
    for line_no, line in enumerate(text.splitlines(), 1):
        match = re.match(r"^\| (supported|experimental|advanced/unstable|deferred|external/GSP) \|", line)
        if match:
            vocabulary.append((line_no, match.group(1)))
    labels = [label for _, label in vocabulary]
    if set(labels) != STATUS_LABELS or len(labels) != len(STATUS_LABELS):
        _error(errors, project, 7, f"status vocabulary must contain each canonical label once: {sorted(STATUS_LABELS)}")

    for path, heading in ((project, "Current Broad Status"), (REFERENCE / "feature-status.md", "")):
        page = path.read_text(encoding="utf8")
        if heading:
            rows = _table_rows(page, heading)
        else:
            rows = []
            for match in re.finditer(r"^\|([^|]+)\|([^|]+)\|", page, re.M):
                cells = [cell.strip() for cell in match.group(0).strip("| ").split("|")]
                rows.append((_line(page, match.start()), cells))
        names: dict[str, int] = {}
        for line_no, cells in rows:
            if not cells or cells[0] in {"Area", "---"} or set(cells[0]) <= {"-", ":"}:
                continue
            name = re.sub(r"[`*_]", "", cells[0])
            if name in names:
                _error(errors, path, line_no, f"duplicate status area {name!r}; first declared at line {names[name]}")
            names[name] = line_no
            if len(cells) > 1:
                status = cells[1]
                if not any(label in status for label in STATUS_LABELS):
                    if not ("fixed" in status and "v0.3" in name):
                        _error(errors, path, line_no, f"status {status!r} uses no canonical vocabulary label")


def main() -> int:
    errors: list[str] = []
    _check_webgpu_page(errors)
    _check_visual_pages(errors)
    _check_status_tables(errors)
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    print("documentation status facts match manifest and authored status policy")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
