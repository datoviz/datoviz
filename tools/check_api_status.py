#!/usr/bin/env python3
"""Validate the v0.4 API status manifest and dependent policies."""

from __future__ import annotations

import argparse
import fnmatch
import re
from pathlib import Path
from typing import Any

import yaml


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_STATUS = ROOT / "spec/api/status.yml"
DEFAULT_C_API_POLICY = ROOT / "spec/api/C_API_REFERENCE_POLICY.yaml"
DEFAULT_CTYPES_POLICY = ROOT / "spec/bindings/ctypes.yml"

TIERS = {"stable", "experimental", "advanced", "internal"}
UMBRELLAS = {"default", "advanced", "none"}
INCLUDE_RE = re.compile(r'^\s*#\s*include\s+[<"](?P<path>[^>"]+)[>"]')


def _as_list(value: Any) -> list[str]:
    if value is None:
        return []
    if isinstance(value, str):
        return [value]
    if isinstance(value, list) and all(isinstance(item, str) for item in value):
        return list(value)
    raise ValueError(f"expected string/list/null, got {value!r}")


def _load_yaml(path: Path) -> dict[str, Any]:
    data = yaml.safe_load(path.read_text(encoding="utf8")) or {}
    if not isinstance(data, dict):
        raise SystemExit(f"{path}: expected a mapping")
    return data


def _header_path(path: str) -> str:
    if path.startswith("datoviz/"):
        return f"include/{path}"
    if path == "datoviz.h":
        return "include/datoviz.h"
    if path.startswith("include/"):
        return path
    return path


def _matches(pattern: str, header: str) -> bool:
    if pattern.endswith("/**"):
        prefix = pattern[:-3]
        return header.startswith(prefix + "/")
    return fnmatch.fnmatch(header, pattern)


def _entry_matches(entry: dict[str, Any], header: str) -> bool:
    return any(_matches(pattern, header) for pattern in _as_list(entry.get("headers")))


def _entry_docs_groups(entry: dict[str, Any]) -> set[str]:
    return set(_as_list(entry.get("docs_group")))


def _load_entries(path: Path) -> list[dict[str, Any]]:
    data = _load_yaml(path)
    entries = data.get("modules")
    if not isinstance(entries, list):
        raise SystemExit(f"{path}: missing modules list")
    seen_modules: set[str] = set()
    for index, entry in enumerate(entries):
        if not isinstance(entry, dict):
            raise SystemExit(f"{path}: modules[{index}] must be a mapping")
        module = entry.get("module")
        if not isinstance(module, str) or not module:
            raise SystemExit(f"{path}: modules[{index}] missing module")
        if module in seen_modules:
            raise SystemExit(f"{path}: duplicate module {module}")
        seen_modules.add(module)
        headers = _as_list(entry.get("headers"))
        if not headers:
            raise SystemExit(f"{path}: module {module} has no headers")
        tier = entry.get("tier")
        if tier not in TIERS:
            raise SystemExit(f"{path}: module {module} has invalid tier {tier!r}")
        umbrella = entry.get("umbrella")
        if umbrella not in UMBRELLAS:
            raise SystemExit(f"{path}: module {module} has invalid umbrella {umbrella!r}")
        if not isinstance(entry.get("raw_binding"), bool):
            raise SystemExit(f"{path}: module {module} raw_binding must be boolean")
        if not isinstance(entry.get("python_top_level"), bool):
            raise SystemExit(f"{path}: module {module} python_top_level must be boolean")
        rationale = entry.get("rationale")
        if tier != "stable" and not isinstance(rationale, str):
            raise SystemExit(f"{path}: module {module} must explain non-stable tier")
    return entries


def _installed_headers() -> list[str]:
    return sorted(path.relative_to(ROOT).as_posix() for path in (ROOT / "include/datoviz").rglob("*.h"))


def _matching_entries(entries: list[dict[str, Any]], header: str) -> list[dict[str, Any]]:
    return [entry for entry in entries if _entry_matches(entry, header)]


def _check_header_coverage(entries: list[dict[str, Any]]) -> None:
    errors: list[str] = []
    for header in _installed_headers():
        matches = _matching_entries(entries, header)
        if not matches:
            errors.append(f"unclassified installed header: {header}")
        elif len(matches) > 1:
            modules = ", ".join(str(entry["module"]) for entry in matches)
            errors.append(f"header has multiple API status owners: {header} ({modules})")
    if errors:
        raise SystemExit("\n".join(errors))


def _direct_includes(path: Path) -> list[str]:
    out: list[str] = []
    for line in path.read_text(encoding="utf8").splitlines():
        match = INCLUDE_RE.match(line)
        if not match:
            continue
        include = match.group("path")
        if include.startswith("datoviz/"):
            out.append(_header_path(include))
        elif include.endswith(".h") and not include.startswith(("<", "/")):
            out.append(f"include/datoviz/{include}")
    return out


def _owner(entries: list[dict[str, Any]], header: str) -> dict[str, Any]:
    matches = _matching_entries(entries, header)
    if len(matches) != 1:
        raise SystemExit(f"{header}: expected one status owner, found {len(matches)}")
    return matches[0]


def _check_umbrella(entries: list[dict[str, Any]], header: str, allowed: set[str]) -> None:
    path = ROOT / header
    errors: list[str] = []
    for include in _direct_includes(path):
        owner = _owner(entries, include)
        umbrella = str(owner.get("umbrella"))
        if umbrella not in allowed:
            errors.append(
                f"{header} includes {include} from {owner['module']} with umbrella={umbrella}"
            )
    if errors:
        raise SystemExit("\n".join(errors))


def _check_c_api_policy(entries: list[dict[str, Any]], path: Path) -> None:
    policy = _load_yaml(path)
    pages = policy.get("pages") or {}
    if not isinstance(pages, dict):
        raise SystemExit(f"{path}: pages must be a mapping")
    errors: list[str] = []
    for page_key, page in pages.items():
        if not isinstance(page, dict):
            errors.append(f"{path}: page {page_key} must be a mapping")
            continue
        if "status" in page:
            errors.append(
                f"{path}: page {page_key} has a local status; use spec/api/status.yml"
            )
        for pattern in _as_list(page.get("headers")):
            normalized = _header_path(pattern)
            matched = [entry for entry in entries if any(_matches(normalized, h) for h in _as_list(entry.get("headers")))]
            matched.extend(entry for entry in entries if _entry_matches(entry, normalized))
            matched = list({str(entry["module"]): entry for entry in matched}.values())
            if not matched:
                errors.append(f"{path}: page {page_key} header pattern is not covered: {pattern}")
                continue
            if not any(page_key in _entry_docs_groups(entry) for entry in matched):
                modules = ", ".join(str(entry["module"]) for entry in matched)
                errors.append(
                    f"{path}: page {page_key} header pattern {pattern} not linked by docs_group "
                    f"({modules})"
                )
    if errors:
        raise SystemExit("\n".join(errors))


def _policy_header_list(path: Path) -> list[str]:
    data = _load_yaml(path)
    headers = ((data.get("headers") or {}).get("include") or [])
    if not isinstance(headers, list) or not all(isinstance(item, str) for item in headers):
        raise SystemExit(f"{path}: headers.include must be a list of strings")
    return [_header_path(header) for header in headers]


def _check_ctypes_policy(entries: list[dict[str, Any]], path: Path) -> None:
    errors: list[str] = []
    for header in _policy_header_list(path):
        owner = _owner(entries, header)
        if not owner.get("raw_binding"):
            errors.append(f"{path}: {header} is included for ctypes but {owner['module']} raw_binding=false")
    if errors:
        raise SystemExit("\n".join(errors))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--status", type=Path, default=DEFAULT_STATUS)
    parser.add_argument("--c-api-policy", type=Path, default=DEFAULT_C_API_POLICY)
    parser.add_argument("--ctypes-policy", type=Path, default=DEFAULT_CTYPES_POLICY)
    args = parser.parse_args()

    entries = _load_entries(args.status)
    _check_header_coverage(entries)
    _check_umbrella(entries, "include/datoviz/datoviz.h", {"default", "none"})
    _check_umbrella(entries, "include/datoviz/advanced.h", {"advanced", "none"})
    _check_c_api_policy(entries, args.c_api_policy)
    _check_ctypes_policy(entries, args.ctypes_policy)
    print(f"API status manifest OK: {len(entries)} modules, {len(_installed_headers())} headers")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
