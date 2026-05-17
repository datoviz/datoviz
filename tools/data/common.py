#!/usr/bin/env python3
"""Shared helpers for Datoviz example-data preparation scripts."""

from __future__ import annotations

import hashlib
import json
import os
import platform
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


SCHEMA = "datoviz.example-data.v1"
REPO_ROOT = Path(__file__).resolve().parents[2]
DATA_ROOT = REPO_ROOT / "data"
EXAMPLES_ROOT = DATA_ROOT / "examples"
CACHE_ROOT = REPO_ROOT / ".cache" / "datoviz" / "examples"


def utc_now() -> str:
    """Return a compact UTC timestamp suitable for manifests."""
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def relpath(path: Path, base: Path) -> str:
    """Return a POSIX relative path."""
    return path.resolve().relative_to(base.resolve()).as_posix()


def git_revision(path: Path | None = None) -> str:
    """Return a best-effort git revision for the repository containing path."""
    cwd = path or REPO_ROOT
    try:
        rev = subprocess.check_output(
            ["git", "rev-parse", "--short", "HEAD"],
            cwd=cwd,
            stderr=subprocess.DEVNULL,
            text=True,
        ).strip()
        dirty = subprocess.check_output(
            ["git", "status", "--porcelain"],
            cwd=cwd,
            stderr=subprocess.DEVNULL,
            text=True,
        )
        return f"git:{rev}{'-dirty' if dirty else ''}"
    except (OSError, subprocess.CalledProcessError):
        return "unknown"


def sha256_file(path: Path) -> str:
    """Return the SHA-256 hash of a file."""
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def artifact(path: Path, base: Path, role: str, fmt: str, **metadata: Any) -> dict[str, Any]:
    """Build one manifest artifact entry for an existing file."""
    entry: dict[str, Any] = {
        "role": role,
        "path": relpath(path, base),
        "format": fmt,
        "bytes": path.stat().st_size,
        "sha256": sha256_file(path),
    }
    entry.update({k: v for k, v in metadata.items() if v is not None})
    return entry


def ensure_bundle(example_id: str) -> tuple[Path, Path]:
    """Create and return the bundle root and prepared directory for an example id."""
    root = EXAMPLES_ROOT / example_id
    prepared = root / "prepared"
    prepared.mkdir(parents=True, exist_ok=True)
    return root, prepared


def write_json(path: Path, value: Any) -> None:
    """Write stable pretty JSON."""
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf8")


def write_manifest(
    bundle_root: Path,
    *,
    example_id: str,
    title: str,
    status: str,
    script: str,
    command: list[str],
    source: dict[str, Any],
    artifacts: list[dict[str, Any]],
    validation: dict[str, Any] | None = None,
    extra: dict[str, Any] | None = None,
) -> dict[str, Any]:
    """Write a Datoviz example-data manifest."""
    manifest: dict[str, Any] = {
        "schema": SCHEMA,
        "id": example_id,
        "title": title,
        "status": status,
        "producer": {
            "script": script,
            "script_version": git_revision(REPO_ROOT),
            "command": " ".join(command),
        },
        "source": source,
        "processing": {
            "created_at": utc_now(),
            "python": sys.version.split()[0],
            "platform": platform.platform(),
        },
        "artifacts": artifacts,
        "validation": validation or {},
    }
    if extra:
        manifest.update(extra)
    write_json(bundle_root / "manifest.json", manifest)
    return manifest


def write_provenance(
    bundle_root: Path,
    *,
    title: str,
    source_lines: list[str],
    processing_lines: list[str],
    license_lines: list[str] | None = None,
    notes: list[str] | None = None,
) -> None:
    """Write a compact human-readable provenance note."""
    lines: list[str] = [
        f"# {title}",
        "",
        "## Source",
        "",
    ]
    lines.extend(f"- {line}" for line in source_lines)
    lines.extend(["", "## Processing", ""])
    lines.extend(f"- {line}" for line in processing_lines)
    if license_lines:
        lines.extend(["", "## License And Attribution", ""])
        lines.extend(f"- {line}" for line in license_lines)
    if notes:
        lines.extend(["", "## Notes", ""])
        lines.extend(f"- {line}" for line in notes)
    lines.append("")
    (bundle_root / "PROVENANCE.md").write_text("\n".join(lines), encoding="utf8")


def command_argv(script: str, args: list[str] | None = None) -> list[str]:
    """Return a normalized command vector for manifest records."""
    return [Path(sys.executable).name, script, *(args or [])]


def env_flag(name: str) -> bool:
    """Return whether an environment flag is truthy."""
    value = os.environ.get(name, "")
    return value.lower() in {"1", "true", "yes", "on"}
