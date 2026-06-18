"""Validation and installed-smoke entry points for Datoviz wheels."""

from __future__ import annotations

import argparse
import csv
import hashlib
import os
import shutil
import subprocess
import sys
import tempfile
import zipfile
from base64 import urlsafe_b64encode
from pathlib import Path

from .config import ROOT
from .manifest import read_manifest
from .repair import inspect_native_deps
from .tags import expected_tags, project_version, wheel_parts


def resolve_wheel(path: str | None = None, *, dist_dir: Path = ROOT / "dist") -> Path:
    """Resolve a wheel path or the single wheel in dist."""

    if path:
        return Path(path)
    wheels = sorted(dist_dir.glob("datoviz-*.whl"))
    if not wheels:
        raise FileNotFoundError(f"no datoviz-*.whl found in {dist_dir}")
    if len(wheels) > 1:
        raise RuntimeError(f"multiple wheels found; pass --wheel explicitly: {wheels}")
    return wheels[0]


def validate_dist(
    dist_dir: Path, *, version: str | None = None, platform_tags: list[str] | None = None
) -> None:
    """Validate wheel filenames in a distribution directory."""

    wheels = sorted(dist_dir.glob("datoviz-*.whl"))
    if not wheels:
        raise RuntimeError(f"no datoviz wheels found in {dist_dir}")
    expected_version = version or project_version()
    expected = set(expected_tags(platform_tags or []))
    found: dict[str, Path] = {}
    errors: list[str] = []
    for wheel in wheels:
        wheel_version, tag = wheel_parts(wheel)
        if wheel_version != expected_version:
            errors.append(f"{wheel.name}: version {wheel_version!r} != expected {expected_version!r}")
        if tag in found:
            errors.append(f"duplicate wheel tag {tag}: {found[tag].name} and {wheel.name}")
        found[tag] = wheel
        validate_wheel(wheel)
    actual = set(found)
    for tag in sorted(expected - actual):
        errors.append(f"missing wheel tag: {tag}")
    for tag in sorted(actual - expected):
        errors.append(f"unexpected wheel tag: {tag}")
    if errors:
        raise RuntimeError("wheel artifact validation failed:\n" + "\n".join(f"- {e}" for e in errors))
    print(f"Validated {len(wheels)} wheel artifact(s) in {dist_dir}")
    for tag in expected_tags(platform_tags or []):
        print(f"{tag}: {found[tag].name}")


def validate_wheel(wheel: Path) -> None:
    """Validate core wheel metadata, RECORD hashes, and payload manifest."""

    with zipfile.ZipFile(wheel) as zf:
        names = set(zf.namelist())
        dist_infos = sorted({name.split("/", 1)[0] for name in names if ".dist-info/" in name})
        if len(dist_infos) != 1:
            raise RuntimeError(f"{wheel}: expected one .dist-info directory, found {dist_infos}")
        dist_info = dist_infos[0]
        wheel_meta = zf.read(f"{dist_info}/WHEEL").decode("utf8")
        version, tag = wheel_parts(wheel)
        del version
        if "Root-Is-Purelib: true\n" not in wheel_meta:
            raise RuntimeError(f"{wheel}: WHEEL does not declare Root-Is-Purelib: true")
        if f"Tag: {tag}\n" not in wheel_meta:
            raise RuntimeError(f"{wheel}: WHEEL tag does not match filename tag {tag}")
        _validate_record(zf, f"{dist_info}/RECORD")
        _validate_payload_manifest(zf)
        forbidden = [name for name in names if "__pycache__" in name or name.endswith(".pyc") or name.endswith(".DS_Store")]
        if forbidden:
            raise RuntimeError(f"{wheel}: forbidden payload entries: {forbidden[:5]}")


def inspect_wheel(wheel: Path, *, native_deps: bool = False) -> None:
    """Print wheel contents and optional native dependency information."""

    print(wheel)
    with zipfile.ZipFile(wheel) as zf:
        for name in sorted(zf.namelist()):
            if name.startswith("datoviz/"):
                print(name)
    if native_deps:
        inspect_native_deps(wheel)


def run_installed_checks(
    wheel: Path,
    *,
    work_dir: Path | None = None,
    render: bool = False,
    shaderc: bool = False,
    cmake_consumer: bool = False,
    qt_probe: str = "skip",
    keep: bool = False,
) -> None:
    """Install a wheel in a clean venv and run installed-package smokes."""

    work = work_dir or Path(tempfile.mkdtemp(prefix="datoviz-wheel-"))
    work.mkdir(parents=True, exist_ok=True)
    venv = work / "venv"
    try:
        _create_venv(venv)
        python = _bin_dir(venv) / ("python.exe" if os.name == "nt" else "python")
        _pip_install(python, work, ["--upgrade", "pip", "wheel"])
        _pip_install(python, work, [str(wheel)])
        _python_smokes(python, work)
        if shaderc:
            _shaderc_smoke(python, work)
        if render:
            _render_smoke(python, work)
        if cmake_consumer:
            _cmake_consumer_smoke(python, work)
        if qt_probe != "skip":
            _qt_probe(python, work, required=qt_probe == "required")
    finally:
        if not keep and work_dir is None:
            shutil.rmtree(work, ignore_errors=True)


def main(argv: list[str] | None = None) -> int:
    """CLI for validation and installed smokes."""

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--wheel")
    parser.add_argument("--dist-dir", type=Path, default=ROOT / "dist")
    parser.add_argument("--validate-dist", action="store_true")
    parser.add_argument("--version")
    parser.add_argument("--platform-tag", action="append", default=[])
    parser.add_argument("--inspect", action="store_true")
    parser.add_argument("--native-deps", action="store_true")
    parser.add_argument("--work-dir", type=Path)
    parser.add_argument("--render", action="store_true")
    parser.add_argument("--shaderc", action="store_true")
    parser.add_argument("--cmake-consumer", action="store_true")
    parser.add_argument("--qt-probe", choices=("skip", "optional", "required"), default="skip")
    parser.add_argument("--keep", action="store_true")
    args = parser.parse_args(argv)

    if args.validate_dist:
        validate_dist(args.dist_dir, version=args.version, platform_tags=args.platform_tag)
        return 0
    wheel = resolve_wheel(args.wheel, dist_dir=args.dist_dir)
    validate_wheel(wheel)
    if args.inspect or args.native_deps:
        inspect_wheel(wheel, native_deps=args.native_deps)
    if args.render or args.shaderc or args.cmake_consumer or args.qt_probe != "skip":
        run_installed_checks(
            wheel,
            work_dir=args.work_dir,
            render=args.render,
            shaderc=args.shaderc,
            cmake_consumer=args.cmake_consumer,
            qt_probe=args.qt_probe,
            keep=args.keep,
        )
    return 0


def _validate_record(zf: zipfile.ZipFile, record_name: str) -> None:
    rows = csv.reader(zf.read(record_name).decode("utf8").splitlines())
    names = {name for name in zf.namelist() if not name.endswith("/")}
    recorded: set[str] = set()
    for row in rows:
        if len(row) != 3:
            raise RuntimeError(f"invalid RECORD row: {row}")
        name, digest, size = row
        recorded.add(name)
        if name == record_name:
            if digest or size:
                raise RuntimeError("RECORD row for RECORD must have empty digest and size")
            continue
        data = zf.read(name)
        expected = "sha256=" + urlsafe_b64encode(hashlib.sha256(data).digest()).rstrip(b"=").decode("ascii")
        if digest != expected:
            raise RuntimeError(f"RECORD digest mismatch for {name}")
        if size != str(len(data)):
            raise RuntimeError(f"RECORD size mismatch for {name}")
    missing = names - recorded
    if missing:
        raise RuntimeError(f"RECORD missing entries: {sorted(missing)[:5]}")


def _validate_payload_manifest(zf: zipfile.ZipFile) -> None:
    manifest_name = "datoviz/_wheel_payload.json"
    if manifest_name not in zf.namelist():
        raise RuntimeError("wheel payload manifest is missing")
    with tempfile.TemporaryDirectory(prefix="datoviz-manifest-") as tmp:
        path = Path(tmp) / "_wheel_payload.json"
        path.write_bytes(zf.read(manifest_name))
        entries = read_manifest(path)
    payload_names = {
        name for name in zf.namelist() if name.startswith("datoviz/") and not name.endswith("/")
    }
    manifest_names = {entry.wheel_path for entry in entries}
    missing = payload_names - manifest_names
    if missing:
        raise RuntimeError(f"payload manifest missing entries: {sorted(missing)[:5]}")


def _bin_dir(venv: Path) -> Path:
    return venv / ("Scripts" if os.name == "nt" else "bin")


def _run(cmd: list[str], *, cwd: Path, env: dict[str, str] | None = None) -> None:
    print("+", " ".join(cmd))
    subprocess.run(cmd, cwd=cwd, env=env, check=True)


def _venv_env() -> dict[str, str]:
    env = os.environ.copy()
    env["PIP_USER"] = "false"
    env["PYTHONNOUSERSITE"] = "1"
    return env


def _create_venv(venv: Path) -> None:
    try:
        subprocess.run([sys.executable, "-m", "venv", str(venv)], check=True)
        return
    except subprocess.CalledProcessError:
        uv = shutil.which("uv")
        if uv is None:
            raise
        subprocess.run([uv, "venv", "--python", sys.executable, str(venv)], check=True)


def _has_pip(python: Path, cwd: Path) -> bool:
    return subprocess.run(
        [str(python), "-m", "pip", "--version"],
        cwd=cwd,
        env=_venv_env(),
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        check=False,
    ).returncode == 0


def _pip_install(python: Path, cwd: Path, args: list[str]) -> None:
    if _has_pip(python, cwd):
        _run([str(python), "-m", "pip", "install", *args], cwd=cwd, env=_venv_env())
        return
    uv = shutil.which("uv")
    if uv is None:
        raise RuntimeError(f"{python} has no pip and uv is not available")
    _run([uv, "pip", "install", "--python", str(python), *args], cwd=cwd)


def _python_smokes(python: Path, work: Path) -> None:
    _run([str(python), "-c", "import datoviz; print(datoviz.__file__)"], cwd=work)
    _run([str(python), "-c", "import datoviz.raw as dvz; print(dvz.__file__)"], cwd=work)
    _run([str(python), "-m", "datoviz.cli", "--prefix"], cwd=work)
    _run([str(python), "-m", "datoviz.cli", "--cflags"], cwd=work)
    _run([str(python), "-m", "datoviz.cli", "--libs"], cwd=work)
    _run([str(python), "-m", "datoviz.cli", "--cmake-dir"], cwd=work)


def _shaderc_smoke(python: Path, work: Path) -> None:
    code = r'''
import ctypes
import datoviz.raw as dvz

glsl = b"""#version 450
layout(location = 0) out vec4 out_color;
vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);
void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    out_color = vec4(1.0);
}
"""

size = ctypes.c_uint64(0)
ptr = dvz.dvz_compile_glsl(b"VERTEX", glsl, ctypes.byref(size))
if not ptr:
    raise SystemExit("dvz_compile_glsl returned NULL")
if size.value == 0 or size.value % 4 != 0:
    raise SystemExit(f"invalid SPIR-V size: {size.value}")
words = ctypes.cast(ptr, ctypes.POINTER(ctypes.c_uint32))
if words[0] != 0x07230203:
    raise SystemExit(f"invalid SPIR-V magic: {words[0]:08x}")
print(f"shaderc GLSL smoke produced {size.value} bytes")
'''
    _run([str(python), "-c", code], cwd=work)


def _render_smoke(python: Path, work: Path) -> None:
    path = work / "datoviz-wheel-smoke.png"
    env = os.environ.copy()
    env["DVZ_CAPTURE_PNG"] = str(path)
    _run([str(python), "-c", "import datoviz; datoviz.demo()"], cwd=work, env=env)
    size = path.stat().st_size if path.exists() else 0
    if size <= 100_000:
        raise RuntimeError(f"render smoke output is missing or too small: {path} ({size} bytes)")


def _qt_probe(python: Path, work: Path, *, required: bool) -> None:
    cmd = [str(python), "-m", "datoviz.qt"]
    print("+", " ".join(cmd))
    result = subprocess.run(cmd, cwd=work, check=False)
    if required and result.returncode != 0:
        raise RuntimeError("required Qt probe failed")
    if not required and result.returncode != 0:
        print("check_wheel: optional Qt probe failed as expected for this environment", file=sys.stderr)


def _cmake_consumer_smoke(python: Path, work: Path) -> None:
    cmake = shutil.which("cmake")
    if cmake is None:
        raise RuntimeError("cmake is required for --cmake-consumer")
    source = work / "cmake-consumer"
    source.mkdir(parents=True, exist_ok=True)
    (source / "main.c").write_text(
        "#include <datoviz.h>\n"
        "#include <stdio.h>\n"
        "int main(void) { printf(\"%s\\n\", dvz_version()); return 0; }\n",
        encoding="utf8",
    )
    (source / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.21)\n"
        "project(datoviz_wheel_consumer C)\n"
        "find_package(datoviz REQUIRED)\n"
        "add_executable(datoviz_cmake_consumer main.c)\n"
        "target_link_libraries(datoviz_cmake_consumer PRIVATE datoviz::datoviz)\n",
        encoding="utf8",
    )
    cmake_dir = subprocess.check_output(
        [str(python), "-m", "datoviz.cli", "--cmake-dir"], cwd=work, text=True
    ).strip()
    build = source / "build"
    generator = ["-GNinja"] if shutil.which("ninja") else []
    _run([cmake, "-S", str(source), "-B", str(build), f"-Ddatoviz_DIR={cmake_dir}", *generator], cwd=work)
    _run([cmake, "--build", str(build)], cwd=work)
    exe = build / ("datoviz_cmake_consumer.exe" if os.name == "nt" else "datoviz_cmake_consumer")
    env = os.environ.copy()
    if os.name == "nt":
        prefix = subprocess.check_output(
            [str(python), "-m", "datoviz.cli", "--prefix"], cwd=work, text=True
        ).strip()
        env["PATH"] = f"{prefix}{os.pathsep}{env.get('PATH', '')}"
    _run([str(exe)], cwd=work, env=env)


if __name__ == "__main__":
    raise SystemExit(main())
