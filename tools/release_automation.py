#!/usr/bin/env python3
"""Local Datoviz release-candidate planning, state, and reports."""

from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
import os
import platform
import re
import shutil
import socket
import subprocess
import sys
import tarfile
import tempfile
import time
from pathlib import Path
from typing import Any

try:
    import tomllib
except ModuleNotFoundError:  # pragma: no cover - Python 3.10 fallback.
    import tomli as tomllib  # type: ignore[no-redef]


ROOT = Path(__file__).resolve().parents[1]
STATE_SCHEMA = "datoviz.release-state.v1"
DEFAULT_DIST_DIR = ROOT / "dist"

PREFLIGHT_PROFILES: dict[str, list[list[str]]] = {
    "none": [],
    "light": [
        ["git", "diff", "--check"],
        ["just", "wheel-matrix"],
        ["just", "check-example-manifests"],
        ["just", "docs-api-check"],
    ],
    "rc1": [
        ["git", "diff", "--check"],
        ["just", "build"],
        ["just", "test"],
        ["just", "spec-check"],
        ["just", "ctypes"],
        ["just", "wheel-matrix"],
        ["just", "check-example-manifests"],
        ["just", "docs-api-check"],
    ],
}

APPROVAL_GATES = [
    {
        "id": "version_identity",
        "description": "Package, citation, C runtime, tag, notes, and artifacts agree.",
        "status": "manual",
    },
    {
        "id": "machine_validation",
        "description": "Required physical-machine validation evidence is present.",
        "status": "missing",
    },
    {
        "id": "release_notes_review",
        "description": "Maintainer reviewed release notes and known issues.",
        "status": "manual",
    },
    {
        "id": "testpypi_rehearsal",
        "description": "Candidate artifacts uploaded to TestPyPI and installed from there.",
        "status": "missing",
    },
    {
        "id": "publication_approval",
        "description": "Maintainer explicitly approved tag/upload/publish actions.",
        "status": "manual",
    },
]

MACHINE_CLASSES = [
    {
        "class": "macos-arm64",
        "required_for_rc": True,
        "profiles": ["rc", "manual"],
        "proof": "arm64 wheel install, offscreen render, live app smoke, optional Qt/PyQt",
    },
    {
        "class": "macos-x86_64",
        "required_for_rc": "if-available",
        "profiles": ["rc"],
        "proof": "x86_64 wheel install, native dependency inventory, import/render smoke",
    },
    {
        "class": "linux-x86_64-vulkan",
        "required_for_rc": True,
        "profiles": ["rc", "full"],
        "proof": "manylinux wheel install, Vulkan validation, C/Python examples",
    },
    {
        "class": "linux-aarch64",
        "required_for_rc": "artifact-required",
        "profiles": ["quick"],
        "proof": "wheel inventory and native execution when a host exists",
    },
    {
        "class": "windows-amd64",
        "required_for_rc": True,
        "profiles": ["rc"],
        "proof": "wheel install, datoviz.raw, CMake consumer, Python smoke",
    },
    {
        "class": "windows-arm64",
        "required_for_rc": "artifact-required",
        "profiles": ["quick"],
        "proof": "wheel inventory and native execution when a host exists",
    },
]

VALIDATION_PROFILES: dict[str, dict[str, Any]] = {
    "quick": {
        "description": "Wheel inventory plus installed import and CLI smoke.",
        "commands": [
            {"kind": "wheel-inspect", "args": []},
            {"kind": "wheel-check", "args": ["--qt-probe", "optional"]},
        ],
    },
    "rc": {
        "description": "Required installed wheel smoke with CMake consumer and optional Qt probe.",
        "commands": [
            {"kind": "wheel-inspect", "args": ["--native-deps"]},
            {"kind": "wheel-check", "args": ["--cmake-consumer", "--qt-probe", "optional"]},
        ],
    },
    "full": {
        "description": "Installed wheel smoke with CMake, shaderc, render, and optional Qt probe.",
        "commands": [
            {"kind": "wheel-inspect", "args": ["--native-deps"]},
            {
                "kind": "wheel-check",
                "args": ["--shaderc", "--cmake-consumer", "--render", "--qt-probe", "optional"],
            },
        ],
    },
}


def utc_now() -> str:
    return dt.datetime.now(dt.UTC).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def relpath(path: Path) -> str:
    try:
        return path.resolve().relative_to(ROOT).as_posix()
    except ValueError:
        return os.fspath(path)


def state_dir(version: str) -> Path:
    return ROOT / "build" / "release" / version


def state_path(version: str) -> Path:
    return state_dir(version) / "release-state.json"


def run_text(args: list[str], *, check: bool = False) -> str:
    result = subprocess.run(args, cwd=ROOT, text=True, capture_output=True, check=False)
    if check and result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or result.stdout.strip())
    return result.stdout.strip()


def git_value(args: list[str]) -> str:
    try:
        return run_text(["git", *args], check=True)
    except Exception:
        return ""


def read_pyproject_version() -> str:
    with (ROOT / "pyproject.toml").open("rb") as f:
        data = tomllib.load(f)
    return str(data.get("project", {}).get("version", ""))


def read_citation_version() -> str:
    path = ROOT / "CITATION.cff"
    if not path.is_file():
        return ""
    for line in path.read_text(encoding="utf8").splitlines():
        if line.startswith("version:"):
            return line.split(":", 1)[1].strip()
    return ""


def read_c_header_version() -> str:
    path = ROOT / "include" / "datoviz" / "common" / "version.h"
    if not path.is_file():
        return ""
    macros: dict[str, str] = {}
    for line in path.read_text(encoding="utf8").splitlines():
        match = re.match(r"#define\s+(DVZ_VERSION_(?:MAJOR|MINOR|PATCH|DEVEL))\s+(.+)", line)
        if match:
            macros[match.group(1)] = match.group(2).strip()
    major = macros.get("DVZ_VERSION_MAJOR")
    minor = macros.get("DVZ_VERSION_MINOR")
    patch = macros.get("DVZ_VERSION_PATCH")
    devel = macros.get("DVZ_VERSION_DEVEL", "")
    if not (major and minor and patch):
        return ""
    return f"{major}.{minor}.{patch}{devel}"


def collect_identity(version: str) -> dict[str, Any]:
    identity = {
        "requested": version,
        "tag": f"v{version}",
        "pyproject": read_pyproject_version(),
        "citation": read_citation_version(),
        "c_header": read_c_header_version(),
    }
    mismatches = []
    for key in ("pyproject", "citation"):
        if identity[key] and identity[key] != version:
            mismatches.append(f"{key}={identity[key]} differs from requested {version}")
    if identity["c_header"] and identity["c_header"] != version:
        mismatches.append(f"c_header={identity['c_header']} differs from requested {version}")
    identity["status"] = "pass" if not mismatches else "manual"
    identity["mismatches"] = mismatches
    return identity


def file_checksums(path: Path) -> dict[str, Any]:
    sha256 = hashlib.sha256()
    sha512 = hashlib.sha512()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            sha256.update(chunk)
            sha512.update(chunk)
    return {
        "bytes": path.stat().st_size,
        "sha256": sha256.hexdigest(),
        "sha512": sha512.hexdigest(),
    }


def artifact_record(path: Path, kind: str, *, validated: bool = False) -> dict[str, Any]:
    record = {
        "kind": kind,
        "path": relpath(path),
        "name": path.name,
        "validated": validated,
    }
    record.update(file_checksums(path))
    return record


def command_exists(name: str) -> str | None:
    found = shutil.which(name)
    return found if found else None


def command_output(args: list[str], *, timeout: int = 10) -> dict[str, Any]:
    try:
        result = subprocess.run(
            args,
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
            timeout=timeout,
        )
    except FileNotFoundError:
        return {"argv": args, "status": "missing"}
    except subprocess.TimeoutExpired as exc:
        return {
            "argv": args,
            "status": "timeout",
            "stdout": exc.stdout or "",
            "stderr": exc.stderr or "",
        }
    return {
        "argv": args,
        "status": "pass" if result.returncode == 0 else "fail",
        "returncode": result.returncode,
        "stdout": result.stdout.strip(),
        "stderr": result.stderr.strip(),
    }


def environment_metadata(version: str, machine_id: str, wheel: Path | None) -> dict[str, Any]:
    return {
        "schema": "datoviz.release-environment.v1",
        "version": version,
        "machine_id": machine_id,
        "created_at_utc": utc_now(),
        "hostname": socket.gethostname(),
        "platform": {
            "system": platform.system(),
            "release": platform.release(),
            "version": platform.version(),
            "machine": platform.machine(),
            "processor": platform.processor(),
            "platform": platform.platform(),
        },
        "python": {
            "version": platform.python_version(),
            "implementation": platform.python_implementation(),
            "executable": sys.executable,
        },
        "git": {
            "commit": git_value(["rev-parse", "HEAD"]),
            "branch": git_value(["branch", "--show-current"]),
            "dirty": bool(git_value(["status", "--porcelain"])),
        },
        "commands": {
            "cmake": command_exists("cmake"),
            "ninja": command_exists("ninja"),
            "vulkaninfo": command_exists("vulkaninfo"),
            "glslangValidator": command_exists("glslangValidator"),
        },
        "vulkaninfo": command_output(["vulkaninfo", "--summary"], timeout=15)
        if command_exists("vulkaninfo")
        else {"status": "missing"},
        "environment": {
            key: os.environ.get(key, "")
            for key in (
                "VULKAN_SDK",
                "VK_ICD_FILENAMES",
                "VK_LAYER_PATH",
                "DYLD_FALLBACK_LIBRARY_PATH",
                "LD_LIBRARY_PATH",
                "PATH",
            )
        },
        "wheel": artifact_record(wheel, "wheel") if wheel and wheel.is_file() else None,
    }


def run_command(
    argv: list[str],
    *,
    log_dir: Path,
    index: int,
    dry_run: bool,
) -> dict[str, Any]:
    label = re.sub(r"[^A-Za-z0-9_.-]+", "_", "_".join(argv[:3])).strip("_") or "command"
    log_path = log_dir / f"{index:02d}_{label}.log"
    record: dict[str, Any] = {
        "argv": argv,
        "started_at_utc": utc_now(),
        "log": relpath(log_path),
        "dry_run": dry_run,
    }
    print("+ " + " ".join(argv), flush=True)
    if dry_run:
        record.update({"returncode": None, "status": "dry-run", "duration_seconds": 0.0})
        return record

    start = time.monotonic()
    result = subprocess.run(argv, cwd=ROOT, text=True, capture_output=True, check=False)
    duration = time.monotonic() - start
    log_path.write_text(
        "COMMAND: " + " ".join(argv) + "\n\n"
        + "STDOUT:\n"
        + result.stdout
        + "\nSTDERR:\n"
        + result.stderr,
        encoding="utf8",
    )
    record.update(
        {
            "finished_at_utc": utc_now(),
            "returncode": result.returncode,
            "status": "pass" if result.returncode == 0 else "fail",
            "duration_seconds": round(duration, 3),
        }
    )
    if result.returncode != 0:
        print(f"failed: {' '.join(argv)}", file=sys.stderr)
    return record


def validation_command(entry: dict[str, Any], wheel: Path, work_dir: Path | None) -> list[str]:
    kind = entry["kind"]
    args = list(entry.get("args", []))
    if kind == "wheel-inspect":
        return [
            sys.executable,
            "tools/release_wheels/inspect_wheel.py",
            "--wheel",
            os.fspath(wheel),
            *args,
        ]
    if kind == "wheel-check":
        cmd = [
            sys.executable,
            "tools/release_wheels/check_wheel.py",
            "--wheel",
            os.fspath(wheel),
            *args,
        ]
        if work_dir is not None:
            cmd.extend(["--work-dir", os.fspath(work_dir)])
        return cmd
    raise ValueError(f"unknown validation command kind: {kind}")


def load_state(version: str) -> dict[str, Any]:
    path = state_path(version)
    if not path.is_file():
        raise FileNotFoundError(f"release state not found: {relpath(path)}")
    return json.loads(path.read_text(encoding="utf8"))


def save_state(version: str, state: dict[str, Any]) -> None:
    path = state_path(version)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(state, indent=2, sort_keys=True) + "\n", encoding="utf8")


def state_or_new(version: str) -> dict[str, Any]:
    path = state_path(version)
    if path.is_file():
        return load_state(version)
    return {
        "schema": STATE_SCHEMA,
        "version": version,
        "tag": f"v{version}",
        "created_at_utc": utc_now(),
        "commit": git_value(["rev-parse", "HEAD"]),
        "branch": git_value(["branch", "--show-current"]),
        "identity": collect_identity(version),
        "profile": "validation-pack",
        "commands": [],
        "artifacts": [],
        "evidence": [],
        "gates": APPROVAL_GATES,
        "status": "pack-only",
    }


def discover_wheels(dist_dir: Path) -> list[dict[str, Any]]:
    if not dist_dir.is_absolute():
        dist_dir = ROOT / dist_dir
    records = []
    for wheel in sorted(dist_dir.glob("datoviz-*.whl")):
        records.append(artifact_record(wheel, "wheel"))
    return records


def discover_release_notes(version: str) -> list[dict[str, Any]]:
    candidates = [
        ROOT / "docs" / "releases" / f"v{version}.md",
        ROOT / "docs" / "releases" / f"{version}.md",
    ]
    return [artifact_record(path, "release-notes") for path in candidates if path.is_file()]


def discover_evidence(version: str) -> list[dict[str, Any]]:
    evidence_root = state_dir(version) / "evidence"
    if not evidence_root.is_dir():
        return []
    records = []
    for path in sorted(evidence_root.glob("*/evidence.json")):
        try:
            evidence = json.loads(path.read_text(encoding="utf8"))
        except json.JSONDecodeError:
            evidence = {"status": "fail", "error": "invalid JSON"}
        records.append({"path": relpath(path), **evidence})
    return records


def find_evidence_dirs(path: Path) -> list[Path]:
    if path.is_file():
        return []
    if (path / "evidence.json").is_file():
        return [path]
    evidence_root = path / "evidence"
    if evidence_root.is_dir():
        return sorted(parent for parent in evidence_root.iterdir() if (parent / "evidence.json").is_file())
    return sorted(parent for parent in path.rglob("*") if parent.is_dir() and (parent / "evidence.json").is_file())


def read_evidence_dir(path: Path) -> dict[str, Any]:
    evidence_path = path / "evidence.json"
    if not evidence_path.is_file():
        raise FileNotFoundError(f"missing evidence.json in {path}")
    try:
        evidence = json.loads(evidence_path.read_text(encoding="utf8"))
    except json.JSONDecodeError as exc:
        raise ValueError(f"invalid evidence JSON: {evidence_path}") from exc
    if not isinstance(evidence, dict):
        raise ValueError(f"evidence JSON must be an object: {evidence_path}")
    return evidence


def validate_evidence_for_ingest(
    evidence: dict[str, Any], version: str, *, force: bool = False
) -> tuple[str, str]:
    schema = evidence.get("schema")
    if schema != "datoviz.release-evidence.v1" and not force:
        raise ValueError(f"unsupported evidence schema {schema!r}")
    evidence_version = str(evidence.get("version", ""))
    if evidence_version != version and not force:
        raise ValueError(f"evidence version {evidence_version!r} does not match {version!r}")
    machine_id = str(evidence.get("machine_id", "")).strip()
    if not machine_id:
        raise ValueError("evidence is missing machine_id")
    profile = str(evidence.get("profile", "")).strip()
    if not profile:
        raise ValueError(f"evidence for {machine_id} is missing profile")
    return machine_id, profile


def copy_evidence_dir(src: Path, dst: Path, *, replace: bool = False) -> None:
    if dst.exists():
        if not replace:
            raise FileExistsError(f"evidence already exists: {relpath(dst)}")
        shutil.rmtree(dst)
    shutil.copytree(src, dst)


def safe_extract_tar(archive: Path, dst: Path) -> None:
    dst_resolved = dst.resolve()
    with tarfile.open(archive) as tar:
        for member in tar.getmembers():
            target = (dst / member.name).resolve()
            if not target.is_relative_to(dst_resolved):
                raise ValueError(f"unsafe tar member path: {member.name}")
        try:
            tar.extractall(dst, filter="data")
        except TypeError:  # pragma: no cover - Python versions before tar filters.
            tar.extractall(dst)


def class_from_environment(environment: dict[str, Any]) -> str:
    platform_info = environment.get("platform") or {}
    system = str(platform_info.get("system", "")).lower()
    machine = str(platform_info.get("machine", "")).lower()
    vulkan = environment.get("vulkaninfo") or {}
    vulkan_ready = vulkan.get("status") == "pass"

    if system == "darwin":
        if machine in {"arm64", "aarch64"}:
            return "macos-arm64"
        if machine in {"x86_64", "amd64"}:
            return "macos-x86_64"
        return "macos-unknown"
    if system == "linux":
        if machine in {"x86_64", "amd64"}:
            return "linux-x86_64-vulkan" if vulkan_ready else "linux-x86_64"
        if machine in {"aarch64", "arm64"}:
            return "linux-aarch64"
        return "linux-unknown"
    if system == "windows":
        if machine in {"amd64", "x86_64"}:
            return "windows-amd64"
        if machine in {"arm64", "aarch64"}:
            return "windows-arm64"
        return "windows-unknown"
    return "unknown"


def evidence_class(item: dict[str, Any]) -> str:
    env_ref = item.get("environment")
    if isinstance(env_ref, str):
        env_path = ROOT / env_ref
        if env_path.is_file():
            try:
                return class_from_environment(json.loads(env_path.read_text(encoding="utf8")))
            except (json.JSONDecodeError, OSError):
                pass
    machine = str(item.get("machine_id", "")).lower()
    for machine_class in MACHINE_CLASSES:
        class_name = str(machine_class["class"])
        if class_name in machine:
            return class_name
    return "unknown"


def machine_matrix(evidence: list[dict[str, Any]]) -> list[dict[str, Any]]:
    rows = []
    evidence_by_class: dict[str, list[dict[str, Any]]] = {}
    for item in evidence:
        evidence_by_class.setdefault(evidence_class(item), []).append(item)

    for machine in MACHINE_CLASSES:
        class_name = str(machine["class"])
        items = evidence_by_class.get(class_name, [])
        passing = [item for item in items if item.get("status") == "pass"]
        failing = [item for item in items if item.get("status") == "fail"]
        if passing:
            status = "pass"
        elif failing:
            status = "fail"
        elif machine["required_for_rc"] is True:
            status = "missing"
        else:
            status = "optional-missing"
        rows.append(
            {
                "class": class_name,
                "required_for_rc": machine["required_for_rc"],
                "status": status,
                "profiles": sorted({str(item.get("profile", "")) for item in items if item.get("profile")}),
                "machines": sorted({str(item.get("machine_id", "")) for item in items if item.get("machine_id")}),
                "proof": machine["proof"],
            }
        )
    return rows


def update_state_evidence(version: str) -> None:
    path = state_path(version)
    if not path.is_file():
        return
    state = load_state(version)
    state["evidence"] = discover_evidence(version)
    state["updated_at_utc"] = utc_now()
    save_state(version, state)


def initial_state(version: str, args: argparse.Namespace) -> dict[str, Any]:
    return {
        "schema": STATE_SCHEMA,
        "version": version,
        "tag": f"v{version}",
        "created_at_utc": utc_now(),
        "commit": git_value(["rev-parse", "HEAD"]),
        "branch": git_value(["branch", "--show-current"]),
        "identity": collect_identity(version),
        "profile": args.profile,
        "commands": [],
        "artifacts": [],
        "evidence": [],
        "gates": APPROVAL_GATES,
    }


def command_plan(version: str) -> str:
    lines = [
        f"Release automation plan for {version}",
        "",
        "Local commands:",
        f"  just release-plan {version}",
        f"  just release-candidate {version}",
        f"  just release-validation-pack {version} --wheel path/to/wheel.whl",
        f"  just release-report {version}",
        "",
        "Candidate phase:",
        "  - run selected local preflight profile",
        "  - create source bundle under build/release/<version>/artifacts/",
        "  - collect wheel metadata and checksums from dist/",
        "  - write build/release/<version>/release-state.json",
        "",
        "Machine validation phase:",
        f"  just release-machine-validate {version} --wheel path/to/wheel.whl --profile rc",
        "",
        "Target machine classes:",
    ]
    for machine in MACHINE_CLASSES:
        required = machine["required_for_rc"]
        profiles = ", ".join(machine["profiles"])
        lines.append(f"  - {machine['class']}: required={required}, profiles={profiles}")
    lines.extend(
        [
            "",
            "Approval gates:",
            "  - version identity review",
            "  - physical-machine validation acceptance",
            "  - release notes and known issues approval",
            "  - TestPyPI rehearsal approval",
            "  - publication approval for tag/upload/publish commands",
            "",
            "This first automation slice does not tag, upload, publish, push, or mutate GitHub.",
        ]
    )
    return "\n".join(lines)


def copy_pack_file(src: Path, dst_root: Path, rel: Path | None = None) -> None:
    rel = rel or src.relative_to(ROOT)
    dst = dst_root / rel
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)


def write_pack_scripts(pack_root: Path, version: str, wheels: list[Path]) -> None:
    if not wheels:
        raise ValueError("validation pack requires at least one wheel")
    wheel_rel = Path("wheels") / wheels[0].name

    validate_sh = f"""#!/usr/bin/env sh
set -eu

profile="${{1:-rc}}"
case "$profile" in
    quick|rc|full) ;;
    *)
        echo "usage: $0 [quick|rc|full]" >&2
        exit 2
        ;;
esac

python_bin="${{PYTHON:-python3}}"
machine_id="${{DATOVIZ_RELEASE_MACHINE_ID:-}}"
if [ -z "$machine_id" ]; then
    machine_id="$("$python_bin" - <<'PY'
import platform
import socket
host = socket.gethostname().split('.')[0] or 'machine'
system = platform.system().lower() or 'unknown'
arch = platform.machine().lower() or 'unknown'
print(f"{{host}}-{{system}}-{{arch}}")
PY
)"
fi

exec "$python_bin" tools/release_automation.py machine-validate "{version}" \\
    --wheel "{wheel_rel.as_posix()}" \\
    --profile "$profile" \\
    --machine-id "$machine_id" \\
    --output-dir "evidence/$machine_id"
"""
    path = pack_root / "validate.sh"
    path.write_text(validate_sh, encoding="utf8")
    path.chmod(0o755)

    for profile in ("quick", "rc", "full"):
        wrapper = f"""#!/usr/bin/env sh
set -eu
exec "$(dirname "$0")/validate.sh" {profile}
"""
        wrapper_path = pack_root / f"validate-{profile}.sh"
        wrapper_path.write_text(wrapper, encoding="utf8")
        wrapper_path.chmod(0o755)

    validate_ps1 = f"""param(
    [ValidateSet("quick", "rc", "full")]
    [string]$Profile = "rc",
    [string]$MachineId = $env:DATOVIZ_RELEASE_MACHINE_ID,
    [string]$Python = "python"
)

if (-not $MachineId) {{
    $hostName = ([System.Net.Dns]::GetHostName()).Split(".")[0]
    $system = [System.Environment]::OSVersion.Platform.ToString().ToLowerInvariant()
    $arch = [System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString().ToLowerInvariant()
    $MachineId = "$hostName-$system-$arch"
}}

& $Python tools/release_automation.py machine-validate "{version}" `
    --wheel "{wheel_rel.as_posix()}" `
    --profile $Profile `
    --machine-id $MachineId `
    --output-dir "evidence/$MachineId"
exit $LASTEXITCODE
"""
    (pack_root / "validate.ps1").write_text(validate_ps1, encoding="utf8")


def write_pack_readme(
    pack_root: Path, version: str, wheels: list[Path], pack_manifest: dict[str, Any]
) -> None:
    wheel_list = "\n".join(f"- `{wheel.name}`" for wheel in wheels)
    readme = f"""# Datoviz {version} Validation Pack

This pack validates Datoviz release artifacts on a physical machine and writes evidence under
`evidence/<machine-id>/`.

## Artifacts

{wheel_list}

## macOS/Linux

```sh
./validate-quick.sh
./validate-rc.sh
./validate-full.sh
```

or:

```sh
./validate.sh rc
```

Set `PYTHON=/path/to/python` to choose a Python interpreter. Set
`DATOVIZ_RELEASE_MACHINE_ID=<name>` to override the generated machine id.

## Windows PowerShell

```powershell
./validate.ps1 -Profile quick
./validate.ps1 -Profile rc
./validate.ps1 -Profile full
```

## Return Evidence

After validation, copy back:

```text
evidence/<machine-id>/
```

The pack manifest is `release-pack.json`. The selected artifact checksums are:

```json
{json.dumps(pack_manifest["artifacts"], indent=2, sort_keys=True)}
```
"""
    (pack_root / "README.md").write_text(readme, encoding="utf8")


def make_tarball(source_dir: Path, archive: Path) -> None:
    if archive.exists():
        archive.unlink()
    with tarfile.open(archive, "w:gz") as tar:
        tar.add(source_dir, arcname=source_dir.name)


def validation_pack(args: argparse.Namespace) -> int:
    version = args.version
    out_dir = args.output_dir or state_dir(version) / "validation-pack"
    pack_root = out_dir / f"datoviz-{version}-validation"
    archive = out_dir / f"datoviz-{version}-validation.tar.gz"

    wheels = args.wheel or sorted(DEFAULT_DIST_DIR.glob(f"datoviz-{version}-*.whl"))
    if not wheels:
        raise RuntimeError("no wheel selected; pass --wheel or build a matching wheel in dist/")

    resolved_wheels = []
    for wheel in wheels:
        resolved = wheel if wheel.is_absolute() else ROOT / wheel
        if not resolved.is_file():
            raise FileNotFoundError(f"wheel not found: {relpath(resolved)}")
        if version not in resolved.name:
            raise RuntimeError(f"wheel name does not contain release version {version}: {resolved.name}")
        resolved_wheels.append(resolved)

    if args.dry_run:
        print(f"Would create validation pack at {relpath(archive)}")
        for wheel in resolved_wheels:
            print(f"Would include wheel: {relpath(wheel)}")
        return 0

    if pack_root.exists():
        shutil.rmtree(pack_root)
    pack_root.mkdir(parents=True, exist_ok=True)
    (pack_root / "wheels").mkdir(parents=True, exist_ok=True)

    copied_wheels = []
    for wheel in resolved_wheels:
        dst = pack_root / "wheels" / wheel.name
        shutil.copy2(wheel, dst)
        copied_wheels.append(dst)

    copy_pack_file(ROOT / "tools" / "release_automation.py", pack_root)
    for rel in (
        Path("tools/release_wheels/check_wheel.py"),
        Path("tools/release_wheels/inspect_wheel.py"),
        Path("tools/datoviz_build_backend/__init__.py"),
        Path("tools/datoviz_build_backend/config.py"),
        Path("tools/datoviz_build_backend/manifest.py"),
        Path("tools/datoviz_build_backend/repair.py"),
        Path("tools/datoviz_build_backend/tags.py"),
        Path("tools/datoviz_build_backend/validate.py"),
    ):
        copy_pack_file(ROOT / rel, pack_root, rel)
    copy_pack_file(ROOT / "pyproject.toml", pack_root)

    pack_manifest = {
        "schema": "datoviz.release-validation-pack.v1",
        "version": version,
        "tag": f"v{version}",
        "created_at_utc": utc_now(),
        "commit": git_value(["rev-parse", "HEAD"]),
        "branch": git_value(["branch", "--show-current"]),
        "profiles": sorted(VALIDATION_PROFILES),
        "artifacts": [artifact_record(path, "wheel") for path in copied_wheels],
        "commands": {
            "unix": ["./validate-quick.sh", "./validate-rc.sh", "./validate-full.sh"],
            "powershell": [
                "./validate.ps1 -Profile quick",
                "./validate.ps1 -Profile rc",
                "./validate.ps1 -Profile full",
            ],
        },
    }
    (pack_root / "release-pack.json").write_text(
        json.dumps(pack_manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf8",
    )
    write_pack_scripts(pack_root, version, copied_wheels)
    write_pack_readme(pack_root, version, copied_wheels, pack_manifest)
    make_tarball(pack_root, archive)

    state = state_or_new(version)
    artifacts = [
        artifact for artifact in state.get("artifacts", []) if artifact.get("path") != relpath(archive)
    ]
    artifacts.append(artifact_record(archive, "validation-pack"))
    state["artifacts"] = artifacts
    state["updated_at_utc"] = utc_now()
    save_state(version, state)

    print(relpath(archive))
    print(f"Wrote {relpath(state_path(version))}")
    return 0


def ingest_evidence(args: argparse.Namespace) -> int:
    version = args.version
    source = args.path if args.path.is_absolute() else ROOT / args.path
    target_root = state_dir(version) / "evidence"

    imported: list[dict[str, str]] = []

    def ingest_from_dir(root: Path) -> None:
        dirs = find_evidence_dirs(root)
        if not dirs:
            raise FileNotFoundError(f"no evidence.json found under {root}")
        for evidence_dir in dirs:
            evidence = read_evidence_dir(evidence_dir)
            machine_id, profile = validate_evidence_for_ingest(
                evidence, version, force=args.force
            )
            dst = target_root / machine_id
            copy_evidence_dir(evidence_dir, dst, replace=args.replace)
            imported.append(
                {
                    "machine_id": machine_id,
                    "profile": profile,
                    "source": relpath(evidence_dir),
                    "destination": relpath(dst),
                }
            )

    if source.is_file() and tarfile.is_tarfile(source):
        with tempfile.TemporaryDirectory(prefix="datoviz-evidence-") as tmp:
            tmp_path = Path(tmp)
            safe_extract_tar(source, tmp_path)
            ingest_from_dir(tmp_path)
    elif source.is_dir():
        ingest_from_dir(source)
    else:
        raise FileNotFoundError(f"evidence path not found or unsupported: {source}")

    update_state_evidence(version)
    if not state_path(version).is_file():
        state = state_or_new(version)
        state["evidence"] = discover_evidence(version)
        state["updated_at_utc"] = utc_now()
        save_state(version, state)

    for item in imported:
        print(
            f"ingested {item['machine_id']} profile={item['profile']} "
            f"-> {item['destination']}"
        )
    return 0


def machine_validate(args: argparse.Namespace) -> int:
    version = args.version
    profile = VALIDATION_PROFILES[args.profile]
    machine_id = args.machine_id or f"{platform.system().lower()}-{platform.machine().lower()}"
    evidence_dir = args.output_dir or state_dir(version) / "evidence" / machine_id
    log_dir = evidence_dir / "logs"
    work_dir = args.work_dir
    if work_dir is not None and not work_dir.is_absolute():
        work_dir = ROOT / work_dir

    wheel = args.wheel
    if wheel is None:
        wheels = sorted(DEFAULT_DIST_DIR.glob(f"datoviz-{version}-*.whl"))
        if len(wheels) == 1:
            wheel = wheels[0]
        else:
            wheels_text = ", ".join(relpath(path) for path in wheels) or "none"
            raise RuntimeError(
                "pass --wheel explicitly; "
                f"found {len(wheels)} wheel(s) matching version {version}: {wheels_text}"
            )
    if not wheel.is_absolute():
        wheel = ROOT / wheel

    if args.dry_run:
        print(f"Dry run for machine validation {version} on {machine_id} ({args.profile})")
    else:
        log_dir.mkdir(parents=True, exist_ok=True)
        evidence_dir.mkdir(parents=True, exist_ok=True)

    environment = environment_metadata(version, machine_id, wheel)
    if not args.dry_run:
        (evidence_dir / "environment.json").write_text(
            json.dumps(environment, indent=2, sort_keys=True) + "\n",
            encoding="utf8",
        )

    evidence: dict[str, Any] = {
        "schema": "datoviz.release-evidence.v1",
        "version": version,
        "machine_id": machine_id,
        "profile": args.profile,
        "profile_description": profile["description"],
        "started_at_utc": utc_now(),
        "artifact_checksums": {
            "wheel": artifact_record(wheel, "wheel") if wheel.is_file() else None,
        },
        "environment": relpath(evidence_dir / "environment.json"),
        "results": [],
        "captures": [],
        "skips": [],
        "failures": [],
    }

    if not wheel.is_file():
        failure = f"wheel not found: {relpath(wheel)}"
        evidence["failures"].append({"status": "fail", "message": failure})
        print(failure, file=sys.stderr)
    else:
        for index, entry in enumerate(profile["commands"], start=1):
            argv = validation_command(entry, wheel, work_dir)
            record = run_command(argv, log_dir=log_dir, index=index, dry_run=args.dry_run)
            evidence["results"].append(record)
            if record.get("returncode") not in (0, None):
                evidence["failures"].append(
                    {
                        "status": "fail",
                        "argv": record.get("argv"),
                        "log": record.get("log"),
                        "returncode": record.get("returncode"),
                    }
                )
                if not args.keep_going:
                    break

    evidence["finished_at_utc"] = utc_now()
    if args.dry_run:
        evidence["status"] = "dry-run"
        print("Dry run did not write evidence.")
        return 0
    evidence["status"] = "fail" if evidence["failures"] else "pass"

    failures_lines = []
    if evidence["failures"]:
        failures_lines.append(f"# Failures for {version} on {machine_id}\n")
        for failure in evidence["failures"]:
            if "message" in failure:
                failures_lines.append(f"- {failure['message']}")
            else:
                argv = " ".join(failure.get("argv", []))
                failures_lines.append(
                    f"- `{argv}` returned {failure.get('returncode')} "
                    f"(log: `{failure.get('log')}`)"
                )
    else:
        failures_lines.append(f"# Failures for {version} on {machine_id}\n")
        failures_lines.append("No failures recorded.")

    (evidence_dir / "failures.md").write_text("\n".join(failures_lines) + "\n", encoding="utf8")
    (evidence_dir / "evidence.json").write_text(
        json.dumps(evidence, indent=2, sort_keys=True) + "\n",
        encoding="utf8",
    )
    update_state_evidence(version)
    print(f"Wrote {relpath(evidence_dir / 'evidence.json')}")
    return 1 if evidence["status"] == "fail" else 0


def candidate(args: argparse.Namespace) -> int:
    version = args.version
    sdir = state_dir(version)
    log_dir = sdir / "logs"
    artifact_dir = args.artifact_dir or sdir / "artifacts"

    if args.dry_run:
        print(f"Dry run for release candidate {version}")
    else:
        log_dir.mkdir(parents=True, exist_ok=True)
        artifact_dir.mkdir(parents=True, exist_ok=True)

    state = initial_state(version, args)
    commands = PREFLIGHT_PROFILES[args.profile]
    failures = 0

    for index, argv in enumerate(commands, start=1):
        record = run_command(argv, log_dir=log_dir, index=index, dry_run=args.dry_run)
        state["commands"].append(record)
        if record.get("returncode") not in (0, None):
            failures += 1
            break

    if not args.skip_source:
        source_cmd = [
            sys.executable,
            "tools/release_source_bundle.py",
            version,
            "--output-dir",
            relpath(artifact_dir),
        ]
        record = run_command(
            source_cmd,
            log_dir=log_dir,
            index=len(state["commands"]) + 1,
            dry_run=args.dry_run,
        )
        state["commands"].append(record)
        if record.get("returncode") not in (0, None):
            failures += 1
        elif not args.dry_run:
            archive = artifact_dir / f"datoviz-{version}-source.tar.gz"
            if archive.is_file():
                state["artifacts"].append(artifact_record(archive, "source-bundle"))

    if not args.dry_run:
        state["artifacts"].extend(discover_wheels(args.dist_dir))
        state["artifacts"].extend(discover_release_notes(version))
        state["evidence"] = discover_evidence(version)
        state["updated_at_utc"] = utc_now()
        state["status"] = "fail" if failures else "candidate"
        save_state(version, state)
        print(f"Wrote {relpath(state_path(version))}")
    else:
        print("Dry run did not write release state.")

    return 1 if failures else 0


def report(args: argparse.Namespace) -> int:
    version = args.version
    state = load_state(version)
    artifacts = state.get("artifacts", [])
    commands = state.get("commands", [])
    evidence = discover_evidence(version) or state.get("evidence", [])
    failed_evidence = [item for item in evidence if item.get("status") == "fail"]
    matrix = machine_matrix(evidence)
    missing_required = [
        row for row in matrix if row["required_for_rc"] is True and row["status"] == "missing"
    ]

    changed = []
    missing = []
    version_mismatch = []
    for artifact in artifacts:
        path_text = artifact.get("path", "")
        path = ROOT / path_text
        if not path.is_file():
            missing.append(path_text)
            continue
        if artifact.get("kind") in {"wheel", "source-bundle"} and version not in artifact.get("name", ""):
            version_mismatch.append(path_text)
        current = file_checksums(path)
        if current.get("sha256") != artifact.get("sha256"):
            changed.append(path_text)

    lines = [
        f"# Datoviz Release Report: {version}",
        "",
        f"- State: `{relpath(state_path(version))}`",
        f"- Commit: `{state.get('commit', '')}`",
        f"- Branch: `{state.get('branch', '')}`",
        f"- Status: `{state.get('status', 'unknown')}`",
        "",
        "## Identity",
    ]
    identity = state.get("identity", {})
    for key in ("requested", "tag", "pyproject", "citation", "c_header", "status"):
        lines.append(f"- {key}: `{identity.get(key, '')}`")
    for mismatch in identity.get("mismatches", []):
        lines.append(f"- mismatch: {mismatch}")

    lines.extend(["", "## Commands"])
    if commands:
        for record in commands:
            argv = " ".join(record.get("argv", []))
            lines.append(f"- `{record.get('status', 'unknown')}` `{argv}`")
    else:
        lines.append("- missing: no command records")

    lines.extend(["", "## Artifacts"])
    if artifacts:
        for artifact in artifacts:
            name = artifact.get("name", "")
            kind = artifact.get("kind", "")
            size = artifact.get("bytes", 0)
            digest = str(artifact.get("sha256", ""))[:16]
            artifact_path = artifact.get("path")
            if artifact_path in missing:
                status = "missing"
            elif artifact_path in changed:
                status = "changed"
            elif artifact_path in version_mismatch:
                status = "version-mismatch"
            else:
                status = "recorded"
            lines.append(f"- {status}: `{name}` ({kind}, {size} bytes, sha256 {digest}...)")
    else:
        lines.append("- missing: no artifacts recorded")

    lines.extend(["", "## Evidence"])
    if evidence:
        for item in evidence:
            machine = item.get("machine_id", item.get("path", "unknown"))
            profile = item.get("profile", "unknown")
            status = item.get("status", "unknown")
            failures = item.get("failures") or []
            suffix = f", failures={len(failures)}" if failures else ""
            machine_class = evidence_class(item)
            lines.append(f"- `{status}` {machine} class={machine_class} profile={profile}{suffix}")
    else:
        lines.append("- missing: no physical-machine evidence ingested")

    lines.extend(["", "## Machine Matrix"])
    for row in matrix:
        profiles = ", ".join(row["profiles"]) if row["profiles"] else "-"
        machines = ", ".join(row["machines"]) if row["machines"] else "-"
        lines.append(
            f"- `{row['status']}` {row['class']} required={row['required_for_rc']} "
            f"profiles={profiles} machines={machines}"
        )

    lines.extend(["", "## Gates"])
    for gate in state.get("gates", APPROVAL_GATES):
        lines.append(f"- `{gate.get('status', 'manual')}` {gate.get('id')}: {gate.get('description')}")

    if missing or changed or version_mismatch:
        lines.extend(["", "## Artifact Drift"])
        for path in missing:
            lines.append(f"- missing: `{path}`")
        for path in changed:
            lines.append(f"- changed checksum: `{path}`")
        for path in version_mismatch:
            lines.append(f"- version mismatch: `{path}`")

    if failed_evidence:
        lines.extend(["", "## Evidence Failures"])
        for item in failed_evidence:
            machine = item.get("machine_id", item.get("path", "unknown"))
            for failure in item.get("failures") or []:
                if "message" in failure:
                    lines.append(f"- {machine}: {failure['message']}")
                else:
                    argv = " ".join(failure.get("argv", []))
                    lines.append(f"- {machine}: `{argv}` returned {failure.get('returncode')}")

    if missing_required:
        lines.extend(["", "## Missing Required Machines"])
        for row in missing_required:
            lines.append(f"- {row['class']}: {row['proof']}")

    output = "\n".join(lines)
    if args.output:
        output_path = args.output
        if not output_path.is_absolute():
            output_path = ROOT / output_path
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(output + "\n", encoding="utf8")
        print(relpath(output_path))
    else:
        print(output)
    strict_matrix_fail = args.strict_matrix and bool(missing_required)
    return 1 if missing or changed or version_mismatch or failed_evidence or strict_matrix_fail else 0


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    plan_parser = subparsers.add_parser("plan", help="Print the release plan for a version")
    plan_parser.add_argument("version")

    candidate_parser = subparsers.add_parser(
        "candidate", help="Create local release-candidate state"
    )
    candidate_parser.add_argument("version")
    candidate_parser.add_argument(
        "--profile",
        choices=sorted(PREFLIGHT_PROFILES),
        default="rc1",
        help="local preflight profile to run before source bundle/artifact collection",
    )
    candidate_parser.add_argument("--dry-run", action="store_true")
    candidate_parser.add_argument("--skip-source", action="store_true")
    candidate_parser.add_argument("--dist-dir", type=Path, default=DEFAULT_DIST_DIR)
    candidate_parser.add_argument("--artifact-dir", type=Path)

    pack_parser = subparsers.add_parser(
        "validation-pack", help="Create a portable validation pack for physical machines"
    )
    pack_parser.add_argument("version")
    pack_parser.add_argument("--wheel", type=Path, action="append", default=[])
    pack_parser.add_argument("--output-dir", type=Path)
    pack_parser.add_argument("--dry-run", action="store_true")

    ingest_parser = subparsers.add_parser(
        "ingest-evidence", help="Copy returned machine evidence into release state"
    )
    ingest_parser.add_argument("version")
    ingest_parser.add_argument("path", type=Path)
    ingest_parser.add_argument("--replace", action="store_true")
    ingest_parser.add_argument("--force", action="store_true")

    machine_parser = subparsers.add_parser(
        "machine-validate", help="Validate release artifacts on this machine and write evidence"
    )
    machine_parser.add_argument("version")
    machine_parser.add_argument("--wheel", type=Path)
    machine_parser.add_argument(
        "--profile",
        choices=sorted(VALIDATION_PROFILES),
        default="quick",
        help="machine validation profile",
    )
    machine_parser.add_argument("--machine-id", default="")
    machine_parser.add_argument("--output-dir", type=Path)
    machine_parser.add_argument("--work-dir", type=Path)
    machine_parser.add_argument("--dry-run", action="store_true")
    machine_parser.add_argument("--keep-going", action="store_true")

    report_parser = subparsers.add_parser("report", help="Print a release-candidate report")
    report_parser.add_argument("version")
    report_parser.add_argument("--output", type=Path)
    report_parser.add_argument(
        "--strict-matrix",
        action="store_true",
        help="return nonzero when required machine classes are missing",
    )

    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    if args.command == "plan":
        print(command_plan(args.version))
        return 0
    if args.command == "candidate":
        return candidate(args)
    if args.command == "validation-pack":
        return validation_pack(args)
    if args.command == "ingest-evidence":
        return ingest_evidence(args)
    if args.command == "machine-validate":
        return machine_validate(args)
    if args.command == "report":
        return report(args)
    raise AssertionError(args.command)


if __name__ == "__main__":
    raise SystemExit(main())
