#!/usr/bin/env python3
"""Validate Doxygen documentation for Datoviz public C functions."""

from __future__ import annotations

import argparse
import json
import re
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_API = ROOT / "build/bindings/datoviz_api.json"
DEFAULT_INCLUDE = ROOT / "include/datoviz"

PARAM_RE = re.compile(
    r"^@param(?:\[(?P<direction>[^]]+)\])?\s+(?P<name>[A-Za-z_]\w*)"
    r"(?:\s+(?P<doc>.*))?$"
)
RETURN_RE = re.compile(r"^@returns?(?:\s+(?P<doc>.*))?$")
INLINE_RE = re.compile(
    r"(?P<doc>/\*\*.*?\*/\s*)?"
    r"static\s+inline\s+(?P<result>[^;{}]+?)\s+"
    r"(?P<name>dvz_[A-Za-z0-9_]+)\s*\((?P<params>[^()]*)\)\s*\{",
    re.DOTALL,
)

BORROWED_GROWABLE_ACCESSORS = {
    "dvz_drp2_stream_get": ("borrowed", "appends a command", "stream is destroyed"),
    "dvz_drp2_stream_label": ("borrowed", "set_label", "stream is destroyed"),
    "dvz_frame_plan_node_get": ("borrowed", "appends a node", "plan is destroyed"),
}


@dataclass(frozen=True)
class Function:
    name: str
    header: str
    line: int
    result: str
    parameters: tuple[str, ...]
    doc: str


@dataclass(frozen=True)
class ParsedDoc:
    summary: str
    params: tuple[tuple[str, str], ...]
    returns: tuple[str, ...]


def clean_doc(raw: str) -> list[str]:
    raw = raw.strip()
    if raw.startswith("/**"):
        raw = raw[3:]
    if raw.endswith("*/"):
        raw = raw[:-2]
    lines = []
    for line in raw.splitlines():
        line = line.strip()
        if line.startswith("*"):
            line = line[1:].strip()
        lines.append(line)
    return lines


def parse_doc(raw: str) -> ParsedDoc:
    summary_lines: list[str] = []
    params: list[tuple[str, str]] = []
    returns: list[str] = []
    current: tuple[str, int] | None = None

    for line in clean_doc(raw):
        match = PARAM_RE.match(line)
        if match:
            params.append((match.group("name"), (match.group("doc") or "").strip()))
            current = ("param", len(params) - 1)
            continue
        match = RETURN_RE.match(line)
        if match:
            returns.append((match.group("doc") or "").strip())
            current = ("return", len(returns) - 1)
            continue
        if line.startswith("@"):
            current = None
            continue
        if current is not None and line:
            kind, index = current
            if kind == "param":
                name, description = params[index]
                params[index] = (name, " ".join(filter(None, (description, line))))
            else:
                returns[index] = " ".join(filter(None, (returns[index], line)))
            continue
        if current is None and line:
            summary_lines.append(line)

    return ParsedDoc(" ".join(summary_lines).strip(), tuple(params), tuple(returns))


def load_exported(path: Path) -> list[Function]:
    api = json.loads(path.read_text(encoding="utf8"))
    functions = []
    for item in api.get("functions", []):
        location = item.get("location") or {}
        functions.append(
            Function(
                name=str(item.get("name", "")),
                header=str(location.get("file", "")),
                line=int(location.get("line") or 0),
                result=str((item.get("result") or {}).get("qualtype") or "void"),
                parameters=tuple(
                    str(param.get("name", "")) for param in item.get("parameters", [])
                ),
                doc=str(item.get("doc") or ""),
            )
        )
    return functions


def _inline_parameters(raw: str) -> tuple[str, ...]:
    raw = raw.strip()
    if not raw or raw == "void":
        return ()
    parameters = []
    for declaration in raw.split(","):
        declaration = declaration.strip()
        match = re.search(r"([A-Za-z_]\w*)\s*(?:\[[^]]*\])?$", declaration)
        parameters.append(match.group(1) if match else "")
    return tuple(parameters)


def load_inline(include_dir: Path) -> list[Function]:
    functions = []
    for path in sorted(include_dir.rglob("*.h")):
        source = path.read_text(encoding="utf8")
        for match in INLINE_RE.finditer(source):
            functions.append(
                Function(
                    name=match.group("name"),
                    header=path.relative_to(ROOT).as_posix(),
                    line=source.count("\n", 0, match.start("name")) + 1,
                    result=" ".join(match.group("result").split()),
                    parameters=_inline_parameters(match.group("params")),
                    doc=(match.group("doc") or "").strip(),
                )
            )
    return functions


def validate(function: Function) -> list[str]:
    prefix = f"{function.header}:{function.line}: {function.name}"
    if not function.doc:
        return [f"{prefix}: missing Doxygen docstring"]

    doc = parse_doc(function.doc)
    errors = []
    if not doc.summary:
        errors.append(f"{prefix}: missing summary")

    actual = list(function.parameters)
    documented = [name for name, _ in doc.params]
    for name in actual:
        if not name:
            errors.append(f"{prefix}: unnamed public parameter")
        elif name not in documented:
            errors.append(f"{prefix}: missing @param {name}")
    reported_duplicates: set[str] = set()
    for name in documented:
        if name not in actual:
            errors.append(f"{prefix}: stale @param {name}")
        if documented.count(name) > 1 and name not in reported_duplicates:
            errors.append(f"{prefix}: duplicate @param {name}")
            reported_duplicates.add(name)
    for name, description in doc.params:
        if not description:
            errors.append(f"{prefix}: empty @param {name} description")

    if function.result != "void":
        if not doc.returns:
            errors.append(f"{prefix}: missing @return")
        elif len(doc.returns) > 1:
            errors.append(f"{prefix}: duplicate @return")
        if any(not description for description in doc.returns):
            errors.append(f"{prefix}: empty @return description")
    elif doc.returns:
        errors.append(f"{prefix}: unexpected @return on void function")

    normalized_doc = " ".join(clean_doc(function.doc)).lower()
    for required in BORROWED_GROWABLE_ACCESSORS.get(function.name, ()):
        if required not in normalized_doc:
            errors.append(f"{prefix}: borrowed accessor contract must mention {required!r}")
    return errors


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--api", type=Path, default=DEFAULT_API)
    parser.add_argument("--include-dir", type=Path, default=DEFAULT_INCLUDE)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    functions = load_exported(args.api)
    exported_names = {function.name for function in functions}
    functions.extend(
        function for function in load_inline(args.include_dir) if function.name not in exported_names
    )
    errors = [error for function in functions for error in validate(function)]
    if errors:
        print("\n".join(errors))
        print(f"public API docstring validation failed with {len(errors)} error(s)")
        return 1
    print(f"validated docstrings for {len(functions)} public API functions")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
