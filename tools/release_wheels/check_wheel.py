#!/usr/bin/env python3
"""Install a Datoviz wheel in a clean venv and run installed-package smokes."""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def _wheel(path: str | None) -> Path:
    if path:
        return Path(path)
    wheels = sorted((ROOT / "dist").glob("datoviz-*.whl"))
    if not wheels:
        raise FileNotFoundError("no dist/datoviz-*.whl found")
    if len(wheels) > 1:
        raise RuntimeError(f"multiple wheels found; pass --wheel explicitly: {wheels}")
    return wheels[0]


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
    _run([str(exe)], cwd=work)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--wheel")
    parser.add_argument("--work-dir", type=Path)
    parser.add_argument("--render", action="store_true")
    parser.add_argument("--shaderc", action="store_true")
    parser.add_argument("--cmake-consumer", action="store_true")
    parser.add_argument("--qt-probe", choices=("skip", "optional", "required"), default="skip")
    parser.add_argument("--keep", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    wheel = _wheel(args.wheel).resolve()
    work = args.work_dir or Path(tempfile.mkdtemp(prefix="datoviz-wheel-"))
    work.mkdir(parents=True, exist_ok=True)
    venv = work / "venv"

    try:
        _create_venv(venv)
        python = _bin_dir(venv) / ("python.exe" if os.name == "nt" else "python")
        _pip_install(python, work, ["--upgrade", "pip", "wheel"])
        _pip_install(python, work, [str(wheel)])
        _python_smokes(python, work)
        if args.shaderc:
            _shaderc_smoke(python, work)
        if args.render:
            _render_smoke(python, work)
        if args.cmake_consumer:
            _cmake_consumer_smoke(python, work)
        if args.qt_probe != "skip":
            _qt_probe(python, work, required=args.qt_probe == "required")
    finally:
        if not args.keep and args.work_dir is None:
            shutil.rmtree(work, ignore_errors=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
