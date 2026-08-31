"""Validation and installed-smoke entry points for Datoviz wheels."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import zipfile
from base64 import urlsafe_b64encode
from email.parser import Parser
from pathlib import Path

from .config import ROOT
from .manifest import read_manifest
from .repair import inspect_native_deps
from .tags import expected_tags, project_version, wheel_parts, wheel_tags


_MARKDOWN_IMAGE_URL_RE = re.compile(r"!\[[^\]]*\]\(\s*<?([^\s)>]+)>?")
_HTML_IMAGE_URL_RE = re.compile(r"<img\b[^>]*\bsrc\s*=\s*['\"]([^'\"]+)['\"]", re.IGNORECASE)


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
        wheel_version, _ = wheel_parts(wheel)
        tags = wheel_tags(wheel)
        if wheel_version != expected_version:
            errors.append(f"{wheel.name}: version {wheel_version!r} != expected {expected_version!r}")
        validate_wheel(wheel)
        if len(tags) != 1:
            errors.append(
                f"{wheel.name}: release artifact must have exactly one tag, found {sorted(tags)!r}"
            )
            continue
        tag = next(iter(tags))
        if tag in found:
            errors.append(f"duplicate wheel tag {tag}: {found[tag].name} and {wheel.name}")
        found[tag] = wheel
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
        filename_tags = wheel_tags(wheel)
        metadata_tags = {
            line.removeprefix("Tag: ")
            for line in wheel_meta.splitlines()
            if line.startswith("Tag: ")
        }
        if "Root-Is-Purelib: true\n" not in wheel_meta:
            raise RuntimeError(f"{wheel}: WHEEL does not declare Root-Is-Purelib: true")
        if metadata_tags != filename_tags:
            raise RuntimeError(
                f"{wheel}: WHEEL tags {sorted(metadata_tags)!r} do not match "
                f"filename tags {sorted(filename_tags)!r}"
            )
        metadata = zf.read(f"{dist_info}/METADATA").decode("utf8")
        _validate_description_image_urls(metadata)
        _validate_record(zf, f"{dist_info}/RECORD")
        _validate_payload_manifest(zf)
        _validate_license_payload(zf, dist_info)
        _validate_license_metadata(zf, dist_info, metadata)
        _validate_macos_runtime(zf, filename_tags)
        forbidden = [name for name in names if "__pycache__" in name or name.endswith(".pyc") or name.endswith(".DS_Store")]
        if forbidden:
            raise RuntimeError(f"{wheel}: forbidden payload entries: {forbidden[:5]}")


def _validate_license_payload(zf: zipfile.ZipFile, dist_info: str) -> None:
    """Require the project license and maintained third-party notice inventory in every wheel."""

    manifest_name = f"{dist_info}/licenses/licenses/THIRD_PARTY_LICENSES.txt"
    required = {
        f"{dist_info}/licenses/LICENSE",
        f"{dist_info}/licenses/licenses/THIRD_PARTY_NOTICES.md",
        manifest_name,
    }
    names = set(zf.namelist())
    missing = sorted(required - names)
    if missing:
        raise RuntimeError(f"wheel license payload is incomplete: {', '.join(missing)}")

    for raw in zf.read(manifest_name).decode("utf8").splitlines():
        text = raw.strip()
        if not text or text.startswith("#"):
            continue
        path = Path(text)
        if path.is_absolute() or ".." in path.parts:
            raise RuntimeError(f"wheel license manifest contains an unsafe path: {text}")
        required_name = f"{dist_info}/licenses/{path.as_posix()}"
        if required_name not in names:
            raise RuntimeError(f"wheel license payload is incomplete: {required_name}")


def _validate_license_metadata(zf: zipfile.ZipFile, dist_info: str, metadata: str) -> None:
    """Require Core Metadata License-File entries to identify every wheel license payload."""

    prefix = f"{dist_info}/licenses/"
    payloads = {name.removeprefix(prefix) for name in zf.namelist() if name.startswith(prefix)}
    license_files = Parser().parsestr(metadata).get_all("License-File", [])
    listed = set(license_files)
    if len(listed) != len(license_files):
        raise RuntimeError("wheel metadata contains duplicate License-File entries")
    if listed != payloads:
        missing = sorted(payloads - listed)
        unexpected = sorted(listed - payloads)
        details = []
        if missing:
            details.append(f"missing: {', '.join(missing)}")
        if unexpected:
            details.append(f"unexpected: {', '.join(unexpected)}")
        raise RuntimeError(
            "wheel License-File metadata does not match license payload: " + "; ".join(details)
        )


def _validate_description_image_urls(metadata: str) -> None:
    """Require PyPI-rendered description images to use stable absolute HTTPS URLs."""

    description = metadata.partition("\n\n")[2]
    urls = _MARKDOWN_IMAGE_URL_RE.findall(description)
    urls.extend(_HTML_IMAGE_URL_RE.findall(description))
    relative = sorted({url for url in urls if not url.startswith("https://")})
    if relative:
        raise RuntimeError(
            "wheel description image URLs must use absolute HTTPS URLs for PyPI rendering: "
            + ", ".join(relative)
        )


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
    release_build: bool = False,
    render: bool = False,
    window: bool = False,
    precompiled_shaders: bool = False,
    shaderc: bool = False,
    cmake_consumer: bool = False,
    examples: str = "skip",
    qt_probe: str = "skip",
    keep: bool = False,
) -> None:
    """Install a wheel in a clean venv and run installed-package smokes."""

    work = work_dir or Path(tempfile.mkdtemp(prefix="datoviz-wheel-"))
    if not work.is_absolute():
        work = ROOT / work
    work.mkdir(parents=True, exist_ok=True)
    venv = work / "venv"
    try:
        _create_venv(venv)
        python = _bin_dir(venv) / ("python.exe" if os.name == "nt" else "python")
        _pip_install(python, work, ["--upgrade", "pip", "wheel"])
        _pip_install(python, work, [str(wheel)])
        if release_build:
            _release_build_smoke(python, work)
            _release_silence_smoke(python, work)
        _python_smokes(python, work, require_precompiled_shaders=precompiled_shaders)
        if shaderc:
            _shaderc_smoke(python, work)
        if render:
            _render_smoke(python, work)
        if window:
            _window_smoke(python, work)
        if cmake_consumer:
            _cmake_consumer_smoke(python, work)
        if examples != "skip":
            _installed_example_smokes(python, work, profile=examples)
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
    parser.add_argument("--release-build", action="store_true")
    parser.add_argument("--render", action="store_true")
    parser.add_argument("--window", action="store_true")
    parser.add_argument("--precompiled-shaders", action="store_true")
    parser.add_argument("--shaderc", action="store_true")
    parser.add_argument("--cmake-consumer", action="store_true")
    parser.add_argument("--examples", choices=("skip", "basic", "render"), default="skip")
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
    if (
        args.release_build
        or args.render
        or args.window
        or args.shaderc
        or args.cmake_consumer
        or args.examples != "skip"
        or args.qt_probe != "skip"
    ):
        run_installed_checks(
            wheel,
            work_dir=args.work_dir,
            release_build=args.release_build,
            render=args.render,
            window=args.window,
            precompiled_shaders=args.precompiled_shaders,
            shaderc=args.shaderc,
            cmake_consumer=args.cmake_consumer,
            examples=args.examples,
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


def _validate_macos_runtime(zf: zipfile.ZipFile, tags: set[str]) -> None:
    """Validate the standalone Vulkan/MoltenVK payload of a macOS wheel."""

    if not any("macosx_" in tag for tag in tags):
        return

    required = {
        "datoviz/MoltenVK_icd.json",
        "datoviz/libMoltenVK.dylib",
        "datoviz/libvulkan.1.dylib",
    }
    missing = required - set(zf.namelist())
    if missing:
        raise RuntimeError(f"macOS wheel runtime is incomplete: {sorted(missing)}")

    manifest = json.loads(zf.read("datoviz/MoltenVK_icd.json"))
    icd = manifest.get("ICD", {})
    if icd.get("library_path") != "./libMoltenVK.dylib":
        raise RuntimeError("macOS MoltenVK manifest must use sibling ./libMoltenVK.dylib")
    if icd.get("is_portability_driver") is not True:
        raise RuntimeError("macOS MoltenVK manifest must declare a portability driver")


def _bin_dir(venv: Path) -> Path:
    return venv / ("Scripts" if os.name == "nt" else "bin")


def _run(cmd: list[str], *, cwd: Path, env: dict[str, str] | None = None) -> None:
    print("+", " ".join(cmd))
    subprocess.run(cmd, cwd=cwd, env=_venv_env() if env is None else env, check=True)


def _venv_env() -> dict[str, str]:
    env = os.environ.copy()
    for name in (
        "PYTHONPATH",
        "PYTHONHOME",
        "DATOVIZ_LIBRARY",
        "DATOVIZ_DEV_ROOT",
        "DVZ_WHEEL_RUNTIME_DIRS",
        "DVZ_SHADERC_RUNTIME_LIBRARY",
        "DVZ_VULKAN_LOADER_LIBRARY",
        "LD_LIBRARY_PATH",
        "DYLD_LIBRARY_PATH",
        "DYLD_FALLBACK_LIBRARY_PATH",
    ):
        env.pop(name, None)
    env["PIP_USER"] = "false"
    env["PYTHONNOUSERSITE"] = "1"
    return env


def _create_venv(venv: Path) -> None:
    try:
        subprocess.run(
            [sys.executable, "-m", "venv", "--clear", str(venv)],
            env=_venv_env(),
            check=True,
        )
        return
    except subprocess.CalledProcessError:
        uv = shutil.which("uv")
        if uv is None:
            raise
        subprocess.run(
            [uv, "venv", "--clear", "--python", sys.executable, str(venv)],
            env=_venv_env(),
            check=True,
        )


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


def _python_smokes(python: Path, work: Path, *, require_precompiled_shaders: bool = False) -> None:
    _installed_origin_smoke(python, work)
    _run([str(python), "-c", "import datoviz; print(datoviz.__file__)"], cwd=work)
    _run([str(python), "-c", "import datoviz.raw as dvz; print(dvz.__file__)"], cwd=work)
    _run(
        [str(python), "-c", "import datoviz.experimental.cuda as cuda; print(cuda.__file__)"],
        cwd=work,
    )
    _builtin_shader_resource_smoke(
        python, work, require_precompiled_shaders=require_precompiled_shaders
    )
    _run([str(python), "-m", "datoviz.cli", "--prefix"], cwd=work)
    _run([str(python), "-m", "datoviz.cli", "--cflags"], cwd=work)
    _run([str(python), "-m", "datoviz.cli", "--libs"], cwd=work)
    _run([str(python), "-m", "datoviz.cli", "--cmake-dir"], cwd=work)


def _installed_origin_smoke(python: Path, work: Path) -> None:
    """Require Python, native, and CMake payloads to resolve inside the test venv."""

    code = r'''
import sys
from pathlib import Path

import datoviz
import datoviz._ctypes as bindings
from datoviz import cli

prefix = Path(sys.prefix).resolve()
paths = {
    "package": Path(datoviz.__file__).resolve(),
    "bindings": Path(bindings.__file__).resolve(),
    "native library": Path(bindings.dvz._name).resolve(),
    "CMake metadata": (Path(cli.__file__).resolve().parent / "lib" / "cmake" / "datoviz"),
}
for label, path in paths.items():
    if not path.is_relative_to(prefix):
        raise SystemExit(f"installed-wheel isolation failure: {label} resolved outside {prefix}: {path}")
    if not path.exists():
        raise SystemExit(f"installed-wheel payload is missing: {label}: {path}")
    print(f"installed {label}: {path}")
'''
    _run([str(python), "-c", code], cwd=work)


def _release_build_smoke(python: Path, work: Path) -> None:
    """Reject an installed release artifact carrying a Debug native library."""

    version = subprocess.check_output(
        [
            str(python),
            "-c",
            "import datoviz.raw as dvz; print(dvz.dvz_version().decode())",
        ],
        cwd=work,
        env=_venv_env(),
        text=True,
    ).strip()
    print(f"installed native version: {version}")
    if "(DEBUG)" in version.upper():
        raise RuntimeError(f"release wheel contains a Debug native library: {version}")


def _release_silence_smoke(python: Path, work: Path) -> None:
    """Verify silent Release logging and the explicit environment opt-in."""

    marker = "datoviz-release-log-opt-in"
    code = rf'''
import ctypes
import datoviz._ctypes as bindings

scene = bindings.dvz_scene()
if not scene:
    raise SystemExit("unable to create a basic scene")
bindings.dvz_scene_destroy(scene)

bindings.dvz.log_log.argtypes = [
    ctypes.c_int,
    ctypes.c_char_p,
    ctypes.c_int,
    ctypes.c_char_p,
]
bindings.dvz.log_log.restype = None
bindings.dvz.log_log(2, b"release-smoke", 1, b"{marker}")
'''

    quiet_env = _venv_env()
    quiet_env.pop("DVZ_LOG_LEVEL", None)
    quiet = subprocess.run(
        [str(python), "-c", code],
        cwd=work,
        env=quiet_env,
        capture_output=True,
        text=True,
        check=False,
    )
    if quiet.returncode != 0:
        raise RuntimeError(f"release silence smoke failed:\n{quiet.stderr}")
    if quiet.stdout or quiet.stderr:
        raise RuntimeError(
            "release library emitted output without DVZ_LOG_LEVEL:\n"
            f"stdout={quiet.stdout!r}\nstderr={quiet.stderr!r}"
        )

    verbose_env = quiet_env.copy()
    verbose_env["DVZ_LOG_LEVEL"] = "info"
    verbose_env["DVZ_LOG_COLOR"] = "0"
    verbose = subprocess.run(
        [str(python), "-c", code],
        cwd=work,
        env=verbose_env,
        capture_output=True,
        text=True,
        check=False,
    )
    if verbose.returncode != 0:
        raise RuntimeError(f"release logging opt-in smoke failed:\n{verbose.stderr}")
    if marker not in verbose.stderr:
        raise RuntimeError(
            "DVZ_LOG_LEVEL=info did not enable Release logging:\n"
            f"stdout={verbose.stdout!r}\nstderr={verbose.stderr!r}"
        )


def _builtin_shader_resource_smoke(
    python: Path, work: Path, *, require_precompiled_shaders: bool = False
) -> None:
    code = r'''
import ctypes
import datoviz.raw as dvz

def check(function, key, required=True):
    size = ctypes.c_uint64(0)
    pointer = function(key, ctypes.byref(size))
    if required and (not pointer or size.value == 0):
        raise SystemExit(f"missing embedded shader resource: {key.decode()}")
    return bool(pointer and size.value > 0)

check(dvz.dvz_resource_glsl, b"point_vert")
has_spirv = check(
    dvz.dvz_resource_shader,
    b"point_vert",
    required=REQUIRE_PRECOMPILED_SHADERS,
)
status = "present" if has_spirv else "unavailable (runtime GLSL fallback)"
print(f"installed built-in shader resources: OK; SPIR-V {status}")
'''.replace("REQUIRE_PRECOMPILED_SHADERS", repr(require_precompiled_shaders))
    _run([str(python), "-c", code], cwd=work)


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

request = dvz.DvzShaderCompileRequest(
    stage=dvz.DVZ_SHADER_STAGE_VERTEX,
    profile=dvz.DVZ_SHADER_PROFILE_GRAPHICS,
    source=glsl,
    source_size=len(glsl),
    source_name=b"installed_shaderc_smoke.vert",
    entry_point=b"main",
)
result = dvz.DvzShaderCompileResult()
status = dvz.dvz_shader_compile(ctypes.byref(request), ctypes.byref(result))
if status != dvz.DVZ_SHADER_COMPILE_SUCCESS:
    diagnostic = result.diagnostics.decode(errors="replace") if result.diagnostics else "no diagnostics"
    dvz.dvz_shader_compile_result_destroy(ctypes.byref(result))
    raise SystemExit(f"dvz_shader_compile failed: {diagnostic}")
if result.spirv_size == 0 or result.spirv_size % 4 != 0:
    dvz.dvz_shader_compile_result_destroy(ctypes.byref(result))
    raise SystemExit(f"invalid SPIR-V size: {result.spirv_size}")
words = result.spirv
if words[0] != 0x07230203:
    dvz.dvz_shader_compile_result_destroy(ctypes.byref(result))
    raise SystemExit(f"invalid SPIR-V magic: {words[0]:08x}")
print(f"shaderc GLSL smoke produced {result.spirv_size} bytes")
dvz.dvz_shader_compile_result_destroy(ctypes.byref(result))
'''
    _run([str(python), "-c", code], cwd=work)


def _render_smoke(python: Path, work: Path) -> None:
    script = work / "datoviz-wheel-render-smoke.py"
    script.write_text(_PYTHON_RENDER_EXAMPLE, encoding="utf8")
    cmd = [str(python), str(script)]
    print("+", " ".join(cmd))
    result = subprocess.run(cmd, cwd=work, env=_venv_env(), check=False)
    if result.returncode == 77:
        print("check_wheel: render smoke skipped because no Vulkan runtime is available")
        return
    if result.returncode != 0:
        raise subprocess.CalledProcessError(result.returncode, cmd)
    path = work / "python_render_example.png"
    size = path.stat().st_size if path.exists() else 0
    if size == 0:
        raise RuntimeError(f"render smoke output is missing or too small: {path} ({size} bytes)")


def _window_smoke(python: Path, work: Path) -> None:
    """Create, render, and destroy a native installed-wheel window."""

    script = work / "datoviz-wheel-window-smoke.py"
    script.write_text(_PYTHON_WINDOW_EXAMPLE, encoding="utf8")
    cmd = [str(python), str(script)]
    env = _venv_env()
    if sys.platform == "darwin":
        for name in (
            "VULKAN_SDK",
            "VK_DRIVER_FILES",
            "VK_ICD_FILENAMES",
            "VK_LAYER_PATH",
            "DYLD_LIBRARY_PATH",
            "DYLD_FALLBACK_LIBRARY_PATH",
        ):
            env.pop(name, None)
    print("+", " ".join(cmd))
    subprocess.run(cmd, cwd=work, env=env, check=True)


def _qt_probe(python: Path, work: Path, *, required: bool) -> None:
    cmd = [str(python), "-m", "datoviz.qt"]
    print("+", " ".join(cmd))
    result = subprocess.run(cmd, cwd=work, env=_venv_env(), check=False)
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
        [str(python), "-m", "datoviz.cli", "--cmake-dir"],
        cwd=work,
        env=_venv_env(),
        text=True,
    ).strip()
    build = source / "build"
    generator = ["-GNinja"] if shutil.which("ninja") else []
    _run([cmake, "-S", str(source), "-B", str(build), f"-Ddatoviz_DIR={cmake_dir}", *generator], cwd=work)
    _run([cmake, "--build", str(build)], cwd=work)
    exe = build / ("datoviz_cmake_consumer.exe" if os.name == "nt" else "datoviz_cmake_consumer")
    env = _venv_env()
    if os.name == "nt":
        prefix = subprocess.check_output(
            [str(python), "-m", "datoviz.cli", "--prefix"],
            cwd=work,
            env=_venv_env(),
            text=True,
        ).strip()
        env["PATH"] = f"{prefix}{os.pathsep}{env.get('PATH', '')}"
    _run([str(exe)], cwd=work, env=env)


def _installed_example_smokes(python: Path, work: Path, *, profile: str) -> None:
    examples = work / "installed-examples"
    examples.mkdir(parents=True, exist_ok=True)
    _python_installed_example(python, examples, render=profile == "render")
    _c_installed_example(python, examples, render=profile == "render")


def _python_installed_example(python: Path, work: Path, *, render: bool) -> None:
    script = work / ("python_render_example.py" if render else "python_basic_example.py")
    if render:
        script.write_text(_PYTHON_RENDER_EXAMPLE, encoding="utf8")
    else:
        script.write_text(_PYTHON_BASIC_EXAMPLE, encoding="utf8")
    _run([str(python), str(script)], cwd=work)


def _c_installed_example(python: Path, work: Path, *, render: bool) -> None:
    cmake = shutil.which("cmake")
    if cmake is None:
        raise RuntimeError("cmake is required for installed C example smoke")
    source = work / ("c-render-example" if render else "c-basic-example")
    source.mkdir(parents=True, exist_ok=True)
    (source / "main.c").write_text(
        _C_RENDER_EXAMPLE if render else _C_BASIC_EXAMPLE,
        encoding="utf8",
    )
    (source / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.21)\n"
        "project(datoviz_installed_example C)\n"
        "find_package(datoviz REQUIRED)\n"
        "add_executable(datoviz_installed_example main.c)\n"
        "target_link_libraries(datoviz_installed_example PRIVATE datoviz::datoviz)\n",
        encoding="utf8",
    )
    cmake_dir = subprocess.check_output(
        [str(python), "-m", "datoviz.cli", "--cmake-dir"],
        cwd=work,
        env=_venv_env(),
        text=True,
    ).strip()
    build = source / "build"
    generator = ["-GNinja"] if shutil.which("ninja") else []
    _run(
        [cmake, "-S", str(source), "-B", str(build), f"-Ddatoviz_DIR={cmake_dir}", *generator],
        cwd=work,
    )
    _run([cmake, "--build", str(build)], cwd=work)
    exe = build / (
        "datoviz_installed_example.exe" if os.name == "nt" else "datoviz_installed_example"
    )
    env = _venv_env()
    if os.name == "nt":
        prefix = subprocess.check_output(
            [str(python), "-m", "datoviz.cli", "--prefix"],
            cwd=work,
            env=_venv_env(),
            text=True,
        ).strip()
        env["PATH"] = f"{prefix}{os.pathsep}{env.get('PATH', '')}"
    _run([str(exe)], cwd=work, env=env)


_PYTHON_BASIC_EXAMPLE = r'''
import datoviz as dvz
import datoviz.raw as raw

scene = dvz.dvz_scene()
if not scene:
    raise SystemExit("dvz_scene() failed")
try:
    figure = dvz.dvz_figure(scene, 64, 64, 0)
    if not figure:
        raise SystemExit("dvz_figure() failed")
    panel = dvz.dvz_panel_full(figure)
    if not panel:
        raise SystemExit("dvz_panel_full() failed")
    if not raw.dvz_scene:
        raise SystemExit("datoviz.raw did not expose dvz_scene")
    print("installed Python basic example: OK")
finally:
    dvz.dvz_scene_destroy(scene)
'''


_PYTHON_RENDER_EXAMPLE = r'''
import ctypes
from pathlib import Path

import datoviz.raw as dvz


def void_p(array):
    return ctypes.cast(array, ctypes.c_void_p)

scene = dvz.dvz_scene()
if not scene:
    raise SystemExit("dvz_scene() failed")

app = None
try:
    figure = dvz.dvz_figure(scene, 128, 128, 0)
    if not figure:
        raise SystemExit("dvz_figure() failed")
    panel = dvz.dvz_panel_full(figure)
    if not panel:
        raise SystemExit("dvz_panel_full() failed")
    dvz.dvz_panel_set_background_color(panel, dvz.DvzColor(13, 15, 20, 255))

    visual = dvz.dvz_point(scene, 0)
    if not visual:
        raise SystemExit("dvz_point() failed")
    positions = (ctypes.c_float * 9)(
        -0.55,
        -0.45,
        0.0,
        +0.55,
        -0.45,
        0.0,
        0.0,
        +0.50,
        0.0,
    )
    colors = (dvz.DvzColor * 3)(
        dvz.DvzColor(255, 80, 80, 255),
        dvz.DvzColor(80, 220, 120, 255),
        dvz.DvzColor(90, 150, 255, 255),
    )
    diameters = (ctypes.c_float * 3)(16.0, 16.0, 16.0)
    if dvz.dvz_visual_set_data(visual, b"position", void_p(positions), 3) != 0:
        raise SystemExit("dvz_visual_set_data(position) failed")
    if dvz.dvz_visual_set_data(visual, b"color", void_p(colors), 3) != 0:
        raise SystemExit("dvz_visual_set_data(color) failed")
    if dvz.dvz_visual_set_data(visual, b"diameter_px", void_p(diameters), 3) != 0:
        raise SystemExit("dvz_visual_set_data(diameter_px) failed")
    if dvz.dvz_panel_add_visual(panel, visual, None) != 0:
        raise SystemExit("dvz_panel_add_visual() failed")

    app = dvz.dvz_app(scene)
    if not app:
        print("installed Python render example: SKIP (dvz_app() failed)")
        raise SystemExit(77)
    view = dvz.dvz_view_offscreen(app, figure, 128, 128)
    if not view:
        print("installed Python render example: SKIP (dvz_view_offscreen() failed)")
        raise SystemExit(77)
    status = dvz.dvz_view_render_once(view)
    if status != dvz.DVZ_CANVAS_FRAME_READY:
        raise SystemExit(f"dvz_view_render_once() failed with status {status}")
    path = Path("python_render_example.png")
    if dvz.dvz_view_capture_png(view, str(path).encode()) != 0:
        raise SystemExit("dvz_view_capture_png() failed")
    if not path.exists() or path.stat().st_size == 0:
        raise SystemExit("PNG capture was not written")
    print("installed Python render example: OK")
finally:
    if app:
        dvz.dvz_app_destroy(app)
    dvz.dvz_scene_destroy(scene)
'''


_PYTHON_WINDOW_EXAMPLE = r'''
import datoviz as dvz


scene = dvz.dvz_scene()
if not scene:
    raise SystemExit("dvz_scene() failed")

app = None
try:
    figure = dvz.dvz_figure(scene, 320, 240, 0)
    if not figure:
        raise SystemExit("dvz_figure() failed")
    panel = dvz.dvz_panel_full(figure)
    if not panel:
        raise SystemExit("dvz_panel_full() failed")

    app = dvz.dvz_app(scene)
    if not app:
        raise SystemExit("dvz_app() failed")
    view = dvz.dvz_view_window(app, figure, 320, 240, b"Datoviz wheel window smoke")
    if not view:
        raise SystemExit("dvz_view_window() failed")
    if dvz.dvz_view_render_once(view) < 0:
        raise SystemExit("dvz_view_render_once() failed")
    print("installed Python native-window example: OK")
finally:
    if app:
        dvz.dvz_app_destroy(app)
    dvz.dvz_scene_destroy(scene)
'''


_C_BASIC_EXAMPLE = r'''
#include <datoviz.h>
#include <stdio.h>

int main(void)
{
    DvzScene* scene = dvz_scene();
    if (scene == NULL)
    {
        fprintf(stderr, "dvz_scene() failed\n");
        return 1;
    }
    DvzFigure* figure = dvz_figure(scene, 64, 64, 0);
    if (figure == NULL)
    {
        fprintf(stderr, "dvz_figure() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    DvzPanel* panel = dvz_panel_full(figure);
    if (panel == NULL)
    {
        fprintf(stderr, "dvz_panel_full() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    printf("installed C basic example: OK %s\n", dvz_version());
    dvz_scene_destroy(scene);
    return 0;
}
'''


_C_RENDER_EXAMPLE = r'''
#include <datoviz.h>
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    int ret = 1;
    DvzScene* scene = dvz_scene();
    DvzApp* app = NULL;
    if (scene == NULL)
    {
        fprintf(stderr, "dvz_scene() failed\n");
        return 1;
    }

    DvzFigure* figure = dvz_figure(scene, 128, 128, 0);
    DvzPanel* panel = figure != NULL ? dvz_panel_full(figure) : NULL;
    DvzVisual* visual = dvz_point(scene, 0);
    if (figure == NULL || panel == NULL || visual == NULL)
    {
        fprintf(stderr, "failed to create scene objects\n");
        goto cleanup;
    }
    dvz_panel_set_background_color(panel, (DvzColor){13, 15, 20, 255});

    vec3 positions[3] = {
        {-0.55f, -0.45f, 0.0f},
        {+0.55f, -0.45f, 0.0f},
        {0.0f, +0.50f, 0.0f},
    };
    DvzColor colors[3] = {
        {255, 80, 80, 255},
        {80, 220, 120, 255},
        {90, 150, 255, 255},
    };
    float diameters[3] = {16.0f, 16.0f, 16.0f};
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = positions, .item_count = 3},
        {.attr_name = "color", .data = colors, .item_count = 3},
        {.attr_name = "diameter_px", .data = diameters, .item_count = 3},
    };
    if (dvz_visual_set_data_many(visual, updates, 3) != 0 ||
        dvz_panel_add_visual(panel, visual, NULL) != 0)
    {
        fprintf(stderr, "failed to add point visual\n");
        goto cleanup;
    }

    app = dvz_app(scene);
    if (app == NULL)
    {
        printf("installed C render example: SKIP (dvz_app() failed)\n");
        ret = 0;
        goto cleanup;
    }
    DvzView* view = dvz_view_offscreen(app, figure, 128, 128);
    if (view == NULL)
    {
        printf("installed C render example: SKIP (dvz_view_offscreen() failed)\n");
        ret = 0;
        goto cleanup;
    }
    if (dvz_view_render_once(view) < 0)
    {
        fprintf(stderr, "dvz_view_render_once() failed\n");
        goto cleanup;
    }
    if (dvz_view_capture_png(view, "c_render_example.png") != 0)
    {
        fprintf(stderr, "dvz_view_capture_png() failed\n");
        goto cleanup;
    }
    printf("installed C render example: OK\n");
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
'''


if __name__ == "__main__":
    raise SystemExit(main())
