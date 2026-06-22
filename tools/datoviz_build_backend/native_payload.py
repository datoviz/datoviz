"""Collect Datoviz native payloads for release wheels."""

from __future__ import annotations

import os
import platform
import shutil
import subprocess
from pathlib import Path

from .config import ReleaseWheelConfig, runtime_roots
from .manifest import PayloadEntry, write_manifest


def copy_file(src: Path, dst: Path) -> None:
    """Copy one required file."""

    if not src.exists():
        raise FileNotFoundError(src)
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)


def copy_tree(src: Path, dst: Path) -> None:
    """Copy a tree while excluding bytecode caches."""

    if not src.exists():
        raise FileNotFoundError(src)
    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(src, dst, ignore=shutil.ignore_patterns("__pycache__", "*.pyc"))


def stage_payload(config: ReleaseWheelConfig, *, clean: bool = False) -> list[PayloadEntry]:
    """Stage a Datoviz release wheel tree and return manifest entries."""

    stage = config.stage_dir
    package_dir = stage / "datoviz"
    if clean and stage.exists():
        shutil.rmtree(stage)
    stage.mkdir(parents=True, exist_ok=True)

    entries: list[PayloadEntry] = []
    entries.extend(_stage_python(config.root, package_dir))
    entries.extend(_stage_native(config, package_dir))
    entries.extend(_stage_c_integration(config.root, package_dir))

    manifest_path = stage / config.payload_manifest
    manifest_entry = PayloadEntry(
        source=os.fspath(manifest_path),
        wheel_path=config.payload_manifest,
        kind="metadata",
        required=True,
        reason="payload-manifest",
    )
    write_manifest([*entries, manifest_entry], manifest_path)
    entries.append(manifest_entry)
    return entries


def _stage_python(root: Path, package_dir: Path) -> list[PayloadEntry]:
    entries: list[PayloadEntry] = []
    package_dir.mkdir(parents=True, exist_ok=True)
    for src in sorted((root / "datoviz").glob("*.py")):
        dst = package_dir / src.name
        copy_file(src, dst)
        entries.append(_entry(src, f"datoviz/{src.name}", "python", "python-package"))
    for subpackage in ("experimental",):
        src_dir = root / "datoviz" / subpackage
        if not src_dir.exists():
            continue
        dst_dir = package_dir / subpackage
        copy_tree(src_dir, dst_dir)
        for dst in sorted(path for path in dst_dir.rglob("*") if path.is_file()):
            rel = dst.relative_to(package_dir).as_posix()
            src = src_dir / Path(rel).relative_to(subpackage)
            entries.append(_entry(src, f"datoviz/{rel}", "python", "python-package"))
    return entries


def _find_windows_bash() -> str | None:
    candidates = [
        r"C:\Program Files\Git\bin\bash.exe",
        r"C:\Program Files\Git\usr\bin\bash.exe",
        shutil.which("bash"),
    ]
    for candidate in candidates:
        if candidate is None:
            continue
        path = Path(candidate)
        if path.name.lower() != "bash.exe":
            continue
        if path.parent.as_posix().lower().endswith("/windows/system32"):
            continue
        if path.exists():
            return str(path)
    return None


def _git_bash_path(path: Path) -> str:
    path = path.resolve()
    drive = path.drive.rstrip(":").lower()
    parts = path.parts[1:]
    if drive:
        return "/" + "/".join([drive, *parts])
    return path.as_posix()


def _stage_c_integration(root: Path, package_dir: Path) -> list[PayloadEntry]:
    script = root / "tools" / "copy_wheel_c_integration.sh"
    if not script.exists():
        raise FileNotFoundError(script)
    cmd = [str(script), str(package_dir)]
    if os.name == "nt":
        bash = _find_windows_bash()
        if bash is None:
            raise RuntimeError("Git Bash is required to stage C integration files on Windows")
        cmd = [bash, _git_bash_path(script), _git_bash_path(package_dir)]
        env = os.environ.copy()
        git_root = Path(bash).parents[1] if Path(bash).parent.name.lower() == "bin" else Path(bash).parents[1]
        git_usr_bin = git_root / "usr" / "bin"
        env["PATH"] = f"{git_usr_bin}{os.pathsep}{Path(bash).parent}{os.pathsep}{env.get('PATH', '')}"
        subprocess.run(cmd, cwd=root, check=True, env=env)
    else:
        subprocess.run(cmd, cwd=root, check=True)

    entries: list[PayloadEntry] = []
    for dst in sorted((package_dir / "include").rglob("*")):
        if dst.is_file():
            entries.append(_entry(dst, _wheel_path(package_dir, dst), "header", "cmake-consumer"))
    for dst in sorted((package_dir / "lib" / "cmake" / "datoviz").glob("*.cmake")):
        entries.append(_entry(dst, _wheel_path(package_dir, dst), "cmake", "cmake-consumer"))
    return entries


def _stage_native(config: ReleaseWheelConfig, package_dir: Path) -> list[PayloadEntry]:
    system = platform.system()
    entries: list[PayloadEntry] = []
    if system == "Linux":
        lib = _first_existing(
            ["src/libdatoviz.so", "libdatoviz.so", "**/libdatoviz.so"], config.native_build_dir
        )
        dst = package_dir / lib.name
        copy_file(lib, dst)
        entries.append(_entry(lib, f"datoviz/{dst.name}", "libdatoviz", "core-runtime"))
        entries.extend(
            _copy_runtime_matches(
                config, ["libshaderc*.so*"], package_dir, required=config.require_shaderc
            )
        )
    elif system == "Darwin":
        lib = _first_existing(
            ["src/libdatoviz.dylib", "libdatoviz.dylib", "**/libdatoviz.dylib"],
            config.native_build_dir,
        )
        dst = package_dir / lib.name
        copy_file(lib, dst)
        entries.append(_entry(lib, f"datoviz/{dst.name}", "libdatoviz", "core-runtime"))
        entries.extend(
            _copy_runtime_matches(
                config,
                [
                    "libvulkan*.dylib",
                    "libshaderc*.dylib",
                    "libMoltenVK.dylib",
                    "MoltenVK_icd.json",
                ],
                package_dir,
                required=config.require_shaderc,
            )
        )
    elif system == "Windows":
        copied = _copy_matches(
            config.root,
            [
                "build/src/*.dll",
                "build/src/*.dll.a",
                "build/*.dll",
                "build/*.dll.a",
                "build/vcpkg_installed/*-windows/bin/*.dll",
                "build/vcpkg_installed/*-windows/debug/bin/*.dll",
            ],
            package_dir,
        )
        if not any(path.name.lower() in {"datoviz.dll", "libdatoviz.dll"} for path in copied):
            raise FileNotFoundError("no datoviz DLL was copied from the Windows build outputs")
        for dst in copied:
            kind = (
                "libdatoviz"
                if dst.name.lower() in {"datoviz.dll", "libdatoviz.dll", "libdatoviz.dll.a"}
                else "runtime"
            )
            entries.append(_entry(dst, f"datoviz/{dst.name}", kind, "core-runtime"))
        gcc = shutil.which("gcc")
        if gcc is not None:
            mingw = Path(gcc).resolve().parent
            for name in ("libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll"):
                src = mingw / name
                if src.exists():
                    dst = package_dir / name
                    copy_file(src, dst)
                    entries.append(_entry(src, f"datoviz/{name}", "runtime", "mingw-runtime"))
    else:
        raise RuntimeError(f"unsupported platform: {system}")

    if config.include_qtbridge:
        suffixes = {
            "Linux": ["build/qtbridge/libdatoviz_qtbridge.so"],
            "Darwin": ["build/qtbridge/libdatoviz_qtbridge.dylib"],
            "Windows": ["build/qtbridge/*.dll"],
        }[system]
        copied = _copy_matches(config.root, suffixes, package_dir)
        if not copied:
            raise FileNotFoundError("Qt bridge requested but no datoviz_qtbridge library was found")
        for dst in copied:
            entries.append(_entry(dst, f"datoviz/{dst.name}", "qtbridge", "qtbridge"))
    return entries


def _first_existing(patterns: list[str], build_dir: Path) -> Path:
    for pattern in patterns:
        matches = sorted(build_dir.glob(pattern))
        if matches:
            return matches[0]
    raise FileNotFoundError(f"none of these build artifacts were found: {patterns}")


def _copy_matches(root: Path, patterns: list[str], dst: Path) -> list[Path]:
    copied: list[Path] = []
    seen_names: set[str] = set()
    for pattern in patterns:
        for src in sorted(root.glob(pattern)):
            if src.is_file():
                key = src.name.lower()
                if key in seen_names:
                    continue
                target = dst / src.name
                copy_file(src, target)
                copied.append(target)
                seen_names.add(key)
    return copied


def _copy_runtime_matches(
    config: ReleaseWheelConfig, patterns: list[str], dst: Path, *, required: bool = False
) -> list[PayloadEntry]:
    copied: list[PayloadEntry] = []
    seen: set[Path] = set()
    for root in runtime_roots(config.native_build_dir, config.runtime_dirs_env):
        for pattern in patterns:
            for src in sorted(root.glob(pattern)):
                resolved = src.resolve()
                if src.is_file() and resolved not in seen:
                    target = dst / src.name
                    copy_file(src, target)
                    copied.append(_entry(src, f"datoviz/{src.name}", "runtime", _runtime_reason(src)))
                    seen.add(resolved)
    if required and not copied:
        roots = ", ".join(os.fspath(root) for root in runtime_roots(config.native_build_dir, config.runtime_dirs_env))
        raise FileNotFoundError(
            f"no runtime files matched {patterns}; searched build dir and "
            f"{config.runtime_dirs_env}: {roots}"
        )
    return copied


def _runtime_reason(path: Path) -> str:
    name = path.name.lower()
    if "shaderc" in name:
        return "shaderc-runtime"
    if "moltenvk" in name:
        return "moltenvk"
    if "vulkan" in name:
        return "vulkan-loader"
    return "runtime"


def _wheel_path(package_dir: Path, dst: Path) -> str:
    return f"datoviz/{dst.relative_to(package_dir).as_posix()}"


def _entry(src: Path, wheel_path: str, kind: str, reason: str) -> PayloadEntry:
    return PayloadEntry(
        source=os.fspath(src),
        wheel_path=wheel_path,
        kind=kind,
        required=True,
        reason=reason,
    )
