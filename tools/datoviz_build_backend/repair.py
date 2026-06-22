"""Platform repair and native dependency inspection helpers."""

from __future__ import annotations

import base64
import csv
import hashlib
import io
import json
import platform
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path


def repair_wheel(wheel: Path, *, skip: bool = False) -> Path:
    """Repair a wheel with the platform tool when required."""

    if skip:
        return wheel
    system = platform.system()
    if system == "Darwin":
        return _repair_macos(wheel)
    if system == "Linux":
        return _repair_linux(wheel)
    if system == "Windows":
        return _repair_windows(wheel)
    return wheel


def inspect_native_deps(wheel: Path) -> None:
    """Print native dependency inspection using the host platform tool."""

    system = platform.system()
    if system == "Linux":
        _run_optional(["auditwheel", "show", str(wheel)])
    elif system == "Darwin":
        _run_optional(["delocate-listdeps", str(wheel)])
    elif system == "Windows":
        _run_optional([sys.executable, "-m", "delvewheel", "show", str(wheel)])


def _repair_macos(wheel: Path) -> Path:
    delocate = shutil.which("delocate-wheel")
    if delocate is None:
        raise RuntimeError("delocate-wheel is required to repair macOS wheels")
    platform_tag = _platform_tag_from_wheel_name(wheel)
    repaired_dir = wheel.parent / ".repaired"
    if repaired_dir.exists():
        shutil.rmtree(repaired_dir)
    repaired_dir.mkdir(parents=True)
    subprocess.run([delocate, "-w", str(repaired_dir), str(wheel)], check=True)
    repaired = _single_wheel(repaired_dir)
    wheel.unlink()
    target = wheel.parent / wheel.name
    repaired.replace(target)
    shutil.rmtree(repaired_dir)
    _retag_wheel(target, platform_tag)
    _refresh_manifest_for_repair(target)
    return target


def _repair_linux(wheel: Path) -> Path:
    auditwheel = shutil.which("auditwheel")
    if auditwheel is None:
        raise RuntimeError("auditwheel is required to repair Linux wheels")
    subprocess.run([auditwheel, "show", str(wheel)], check=True)
    repaired_dir = wheel.parent / ".repaired"
    if repaired_dir.exists():
        shutil.rmtree(repaired_dir)
    repaired_dir.mkdir(parents=True)
    subprocess.run([auditwheel, "repair", "-w", str(repaired_dir), str(wheel)], check=True)
    repaired = _single_wheel(repaired_dir)
    wheel.unlink()
    target = wheel.parent / repaired.name
    repaired.replace(target)
    shutil.rmtree(repaired_dir)
    _refresh_manifest_for_repair(target)
    return target


def _repair_windows(wheel: Path) -> Path:
    try:
        subprocess.run([sys.executable, "-m", "delvewheel", "show", str(wheel)], check=True)
    except ModuleNotFoundError as exc:  # pragma: no cover - depends on environment.
        raise RuntimeError("delvewheel is required to repair Windows wheels") from exc
    repaired_dir = wheel.parent / ".repaired"
    if repaired_dir.exists():
        shutil.rmtree(repaired_dir)
    repaired_dir.mkdir(parents=True)
    subprocess.run(
        [sys.executable, "-m", "delvewheel", "repair", "-w", str(repaired_dir), str(wheel)],
        check=True,
    )
    repaired = _single_wheel(repaired_dir)
    wheel.unlink()
    target = wheel.parent / repaired.name
    repaired.replace(target)
    shutil.rmtree(repaired_dir)
    _refresh_manifest_for_repair(target)
    return target


def _single_wheel(path: Path) -> Path:
    wheels = sorted(path.glob("datoviz-*.whl"))
    if len(wheels) != 1:
        raise RuntimeError(f"expected one wheel in {path}, found: {wheels}")
    return wheels[0]


def _platform_tag_from_wheel_name(wheel: Path) -> str:
    name = wheel.name
    if not name.endswith(".whl"):
        raise RuntimeError(f"expected wheel filename, got: {name}")
    parts = name[:-4].split("-")
    if len(parts) < 5:
        raise RuntimeError(f"cannot parse wheel platform tag from: {name}")
    return parts[-1]


def _retag_wheel(wheel: Path, platform_tag: str) -> None:
    """Restore the requested wheel platform tag after a repair tool rewrites it."""

    with zipfile.ZipFile(wheel) as zf:
        files = {
            info.filename: zf.read(info.filename)
            for info in zf.infolist()
            if not info.filename.endswith("/")
        }
    wheel_names = [name for name in files if name.endswith(".dist-info/WHEEL")]
    if len(wheel_names) != 1:
        raise RuntimeError(f"{wheel}: expected one WHEEL metadata file, found {wheel_names}")
    wheel_name = wheel_names[0]
    lines = files[wheel_name].decode("utf8").splitlines()
    lines = [f"Tag: py3-none-{platform_tag}" if line.startswith("Tag: ") else line for line in lines]
    if not any(line.startswith("Tag: ") for line in lines):
        lines.append(f"Tag: py3-none-{platform_tag}")
    files[wheel_name] = ("\n".join(lines) + "\n").encode("utf8")
    record_names = [name for name in files if name.endswith(".dist-info/RECORD")]
    if len(record_names) == 1:
        files[record_names[0]] = _record_bytes(files, record_names[0])
    _write_wheel_files(wheel, files)


def _run_optional(cmd: list[str]) -> None:
    try:
        subprocess.run(cmd, check=False)
    except FileNotFoundError:
        print(f"wheel inspection command not found: {cmd[0]}", file=sys.stderr)


def _refresh_manifest_for_repair(wheel: Path) -> None:
    """Record repair-added runtime files in the payload manifest and RECORD."""

    manifest_name = "datoviz/_wheel_payload.json"
    with zipfile.ZipFile(wheel) as zf:
        files = {
            info.filename: zf.read(info.filename)
            for info in zf.infolist()
            if not info.filename.endswith("/")
        }
    if manifest_name not in files:
        return

    manifest = json.loads(files[manifest_name].decode("utf8"))
    entries = manifest.setdefault("entries", [])
    known = {entry.get("wheel_path") for entry in entries}
    for name in sorted(files):
        if not name.startswith("datoviz/") or name in known:
            continue
        if not _looks_repair_added(name):
            continue
        entries.append(
            {
                "source": "",
                "wheel_path": name,
                "kind": "runtime",
                "required": True,
                "reason": "platform-repair",
                "repair_status": "added-by-repair",
            }
        )
        known.add(name)

    files[manifest_name] = (json.dumps(manifest, indent=2, sort_keys=True) + "\n").encode("utf8")
    dist_infos = sorted({name.split("/", 1)[0] for name in files if ".dist-info/" in name})
    if len(dist_infos) != 1:
        raise RuntimeError(f"{wheel}: expected one .dist-info directory, found {dist_infos}")
    record_name = f"{dist_infos[0]}/RECORD"
    files[record_name] = _record_bytes(files, record_name)

    _write_wheel_files(wheel, files)


def _looks_repair_added(name: str) -> bool:
    path = Path(name)
    return any(part in {".dylibs", ".libs"} for part in path.parts) and path.suffix.lower() in {
        ".dylib",
        ".so",
        ".dll",
    }


def _record_bytes(files: dict[str, bytes], record_name: str) -> bytes:
    rows: list[tuple[str, str, str]] = []
    for name, data in sorted(files.items()):
        if name == record_name:
            continue
        digest = base64.urlsafe_b64encode(hashlib.sha256(data).digest()).rstrip(b"=").decode("ascii")
        rows.append((name, f"sha256={digest}", str(len(data))))
    rows.append((record_name, "", ""))
    stream = io.StringIO(newline="")
    writer = csv.writer(stream)
    writer.writerows(rows)
    return stream.getvalue().encode("utf8")


def _write_wheel_files(wheel: Path, files: dict[str, bytes]) -> None:
    tmp = wheel.with_suffix(".tmp.whl")
    with zipfile.ZipFile(tmp, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        for name, data in sorted(files.items()):
            info = zipfile.ZipInfo(name)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o644 << 16
            zf.writestr(info, data)
    tmp.replace(wheel)
