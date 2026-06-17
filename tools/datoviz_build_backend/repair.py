"""Platform repair and native dependency inspection helpers."""

from __future__ import annotations

import platform
import shutil
import subprocess
import sys
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
    repaired_dir = wheel.parent / ".repaired"
    if repaired_dir.exists():
        shutil.rmtree(repaired_dir)
    repaired_dir.mkdir(parents=True)
    subprocess.run([delocate, "-w", str(repaired_dir), str(wheel)], check=True)
    repaired = _single_wheel(repaired_dir)
    wheel.unlink()
    target = wheel.parent / repaired.name
    repaired.replace(target)
    shutil.rmtree(repaired_dir)
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
    return target


def _single_wheel(path: Path) -> Path:
    wheels = sorted(path.glob("datoviz-*.whl"))
    if len(wheels) != 1:
        raise RuntimeError(f"expected one wheel in {path}, found: {wheels}")
    return wheels[0]


def _run_optional(cmd: list[str]) -> None:
    try:
        subprocess.run(cmd, check=False)
    except FileNotFoundError:
        print(f"wheel inspection command not found: {cmd[0]}", file=sys.stderr)

