#!/usr/bin/env python3
"""Smoke-test raw ctypes from editable and wheel installs."""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import venv
from pathlib import Path


ROOT_DIR = Path(__file__).resolve().parents[2]
WORK_DIR = ROOT_DIR / 'build' / 'bindings' / 'package_smoke'

SMOKE_CODE = r'''
from pathlib import Path
import datoviz.raw as dvz
import datoviz._ctypes as impl

assert not hasattr(__import__("datoviz"), "dvz_time_monotonic_ns")
assert hasattr(dvz, "dvz_scene")
assert Path(impl.dvz._name).exists(), impl.dvz._name
t0 = dvz.dvz_time_monotonic_ns()
t1 = dvz.dvz_time_monotonic_ns()
assert isinstance(t0, int)
assert t1 >= t0
scene = dvz.dvz_scene()
assert bool(scene)
dvz.dvz_scene_destroy(scene)
print(f"raw ctypes package smoke: OK ({impl.dvz._name})")
'''


def _run(cmd: list[str], *, cwd: Path | None = None, env: dict[str, str] | None = None) -> None:
    subprocess.run(cmd, cwd=cwd or ROOT_DIR, env=env, check=True)


def _isolated_env() -> dict[str, str]:
    env = os.environ.copy()
    env['PIP_CONFIG_FILE'] = os.devnull
    env['PYTHONNOUSERSITE'] = '1'
    return env


def _venv_python(path: Path) -> Path:
    if sys.platform == 'win32':
        return path / 'Scripts' / 'python.exe'
    return path / 'bin' / 'python'


def _create_venv(path: Path) -> Path:
    if path.exists():
        shutil.rmtree(path)
    venv.EnvBuilder(with_pip=True, clear=True).create(path)
    python = _venv_python(path)
    _run(
        [str(python), '-m', 'pip', 'install', '--isolated', '--upgrade', 'pip', 'setuptools', 'wheel'],
        env=_isolated_env(),
    )
    return python


def _library_path() -> Path:
    names = {
        'darwin': 'libdatoviz.dylib',
        'linux': 'libdatoviz.so',
        'win32': 'libdatoviz.dll',
    }
    name = names.get(sys.platform)
    if name is None:
        raise RuntimeError(f'unsupported platform: {sys.platform}')
    for base in (ROOT_DIR / 'build' / 'src', ROOT_DIR / 'build'):
        path = base / name
        if path.exists():
            return path
    raise RuntimeError(f'{name} has not been built')


def _run_package_smoke(python: Path, *, cwd: Path, env: dict[str, str] | None = None) -> None:
    _run([str(python), '-c', SMOKE_CODE], cwd=cwd, env=env)


def editable_smoke() -> None:
    python = _create_venv(WORK_DIR / 'editable-venv')
    _run(
        [
            str(python),
            '-m',
            'pip',
            'install',
            '--isolated',
            '--no-build-isolation',
            '--no-deps',
            '-e',
            str(ROOT_DIR),
        ],
        env=_isolated_env(),
    )
    env = _isolated_env()
    env['DATOVIZ_LIBRARY'] = str(_library_path())
    run_dir = WORK_DIR / 'editable-run'
    run_dir.mkdir(parents=True, exist_ok=True)
    _run_package_smoke(python, cwd=run_dir, env=env)


def _copy_package_root(path: Path) -> None:
    if path.exists():
        shutil.rmtree(path)
    package_dir = path / 'datoviz'
    package_dir.mkdir(parents=True)

    for source in (ROOT_DIR / 'datoviz').glob('*.py'):
        shutil.copy2(source, package_dir / source.name)
    if not (package_dir / '_ctypes.py').exists():
        raise RuntimeError('datoviz/_ctypes.py is missing; run `just ctypes` first')

    for base in (ROOT_DIR / 'build' / 'src', ROOT_DIR / 'build'):
        if not base.exists():
            continue
        for pattern in ('libdatoviz*.dylib', 'libdatoviz*.so*', 'libdatoviz*.dll'):
            for source in base.glob(pattern):
                shutil.copy2(source, package_dir / source.name)

    shutil.copy2(ROOT_DIR / 'pyproject.toml', path / 'pyproject.toml')
    shutil.copy2(ROOT_DIR / 'README.md', path / 'README.md')


def wheel_smoke() -> None:
    python = _create_venv(WORK_DIR / 'wheel-venv')
    package_root = WORK_DIR / 'wheel-src'
    wheelhouse = WORK_DIR / 'wheelhouse'
    run_dir = WORK_DIR / 'wheel-run'
    _copy_package_root(package_root)
    if wheelhouse.exists():
        shutil.rmtree(wheelhouse)
    wheelhouse.mkdir(parents=True)
    run_dir.mkdir(parents=True, exist_ok=True)

    _run(
        [
            str(python),
            '-m',
            'pip',
            'wheel',
            '--isolated',
            '--no-build-isolation',
            '--no-deps',
            str(package_root),
            '-w',
            str(wheelhouse),
        ],
        env=_isolated_env(),
    )
    wheels = sorted(wheelhouse.glob('datoviz-*.whl'))
    if not wheels:
        raise RuntimeError('wheel build did not produce a datoviz wheel')
    _run(
        [str(python), '-m', 'pip', 'install', '--isolated', '--no-deps', str(wheels[-1])],
        env=_isolated_env(),
    )
    _run_package_smoke(python, cwd=run_dir, env=_isolated_env())


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=('editable', 'wheel'))
    args = parser.parse_args()

    WORK_DIR.mkdir(parents=True, exist_ok=True)
    if args.mode == 'editable':
        editable_smoke()
    else:
        wheel_smoke()
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
