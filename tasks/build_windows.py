#!/usr/bin/env python3
"""Configure and build Datoviz with MSVC and Ninja on Windows."""

import argparse
import importlib.util
import os
import shlex
import shutil
import subprocess
import sys
from pathlib import Path


IS_WINDOWS = os.name == 'nt'


def environment_value(environment, name):
    """Read Windows environment keys without depending on their capitalization."""
    return next((value for key, value in environment.items() if key.upper() == name.upper()), '')


def initialize_msvc(environment):
    """Return an x64 developer environment without exposing its captured contents."""
    environment = dict(environment)
    if shutil.which('cl', path=environment_value(environment, 'PATH')):
        return environment
    program_files = environment_value(environment, 'ProgramFiles(x86)')
    vswhere = Path(program_files) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    if not program_files or not vswhere.is_file():
        raise RuntimeError('MSVC cl.exe is unavailable and vswhere.exe was not found. '
                           'Install Visual Studio C++ build tools or use an x64 developer prompt.')
    result = subprocess.run(
        [str(vswhere), '-latest', '-products', '*', '-requires',
         'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath',
         '-utf8'],
        env=environment, capture_output=True, text=True, encoding='utf-8', check=False)
    installation = result.stdout.strip()
    if result.returncode or not installation:
        raise RuntimeError('vswhere could not find Visual Studio with the C++ x64 build tools.')
    developer_command = Path(installation) / 'Common7/Tools/VsDevCmd.bat'
    if not developer_command.is_file():
        raise RuntimeError(f'Visual Studio developer command was not found: {developer_command}')
    command = f'call "{developer_command}" -no_logo -arch=x64 -host_arch=x64 >nul && set'
    comspec = environment_value(environment, 'ComSpec') or 'cmd.exe'
    # cmd.exe needs its own quoting; list2cmdline escapes inner quotes for C runtimes instead.
    command_line = f'"{comspec}" /d /u /s /c "{command}"'
    result = subprocess.run(
        command_line,
        env=environment, capture_output=True, text=True, encoding='utf-16-le', check=False)
    if result.returncode:
        raise RuntimeError('VsDevCmd.bat failed to initialize the x64 MSVC environment.')
    for line in result.stdout.splitlines():
        key, separator, value = line.partition('=')
        if separator and key:
            # A normal dict is case-sensitive even when it models the Windows environment.
            for previous in list(environment):
                if previous.upper() == key.upper():
                    del environment[previous]
            environment[key] = value
    if not shutil.which('cl', path=environment_value(environment, 'PATH')):
        raise RuntimeError('VsDevCmd.bat completed but cl.exe is still unavailable.')
    return environment


def find_vcpkg(environment):
    """Locate the explicitly selected or Visual Studio bundled vcpkg toolchain."""
    configured = environment_value(environment, 'VCPKG_ROOT')
    installation = environment_value(environment, 'VSINSTALLDIR')
    if configured:
        root = Path(configured)
    elif installation:
        root = Path(installation) / 'VC/vcpkg'
    else:
        raise RuntimeError('Set VCPKG_ROOT to a vcpkg checkout, or initialize a Visual Studio '
                           'environment containing VSINSTALLDIR and bundled VC/vcpkg.')
    toolchain = root / 'scripts/buildsystems/vcpkg.cmake'
    if not toolchain.is_file():
        raise RuntimeError(f'vcpkg toolchain was not found: {toolchain}. Set VCPKG_ROOT '
                           'to a checkout containing scripts/buildsystems/vcpkg.cmake.')
    return root.resolve(), toolchain.resolve()


def read_cache(path):
    """Read CMake cache entries, preserving values containing equals signs."""
    values = {}
    if path.is_file():
        for line in path.read_text(encoding='utf-8').splitlines():
            if line.startswith(('#', '//')) or '=' not in line:
                continue
            key, value = line.split('=', 1)
            values[key.split(':', 1)[0]] = value
    return values


def guard_cache(build_directory, root):
    """Reject incompatible caches without cleaning or changing their contents."""
    cache_path = build_directory / 'CMakeCache.txt'
    if not cache_path.is_file():
        return
    cache = read_cache(cache_path)
    reasons = []
    if cache.get('CMAKE_GENERATOR') != 'Ninja':
        reasons.append('generator is not single-configuration Ninja')
    compilers = [value for key, value in cache.items()
                 if key in {'CMAKE_C_COMPILER', 'CMAKE_CXX_COMPILER'}]
    if not compilers or any(Path(value.replace('\\', '/')).name.lower() not in {'cl', 'cl.exe'}
                            for value in compilers):
        reasons.append('compiler is not MSVC cl.exe')
    source = cache.get('CMAKE_HOME_DIRECTORY')
    if not source or os.path.normcase(str(Path(source).resolve())) != os.path.normcase(str(root.resolve())):
        reasons.append('source directory does not match this checkout')
    if reasons:
        raise RuntimeError(f'Refusing existing cache {cache_path}: {"; ".join(reasons)}. '
                           'Preserve any needed artifacts, then move the incompatible build '
                           'directory aside before retrying. No files were removed.')


def cmake_arguments(value):
    """Parse extra CMake options using POSIX quoting, without shell evaluation."""
    arguments = shlex.split(value, posix=True)
    for argument in arguments:
        if (argument.startswith(('-B', '-S', '-G', '--preset', '--build', '--install'))
                or argument in {'--fresh', '--workflow'}):
            raise RuntimeError(f'DVZ_CMAKE_ARGS cannot override the build layout or action: {argument}')
    return arguments


def stage_runtime(build_directory, configuration):
    """Copy configured vcpkg DLLs and Datoviz beside native executables and the library."""
    cache = read_cache(build_directory / 'CMakeCache.txt')
    installed = cache.get('VCPKG_INSTALLED_DIR') or cache.get('_VCPKG_INSTALLED_DIR')
    if not installed:
        raise RuntimeError('CMakeCache.txt does not record VCPKG_INSTALLED_DIR; '
                           'runtime dependency staging cannot locate installed packages.')
    installed_root = Path(installed)
    if not installed_root.is_absolute():
        installed_root = build_directory / installed_root
    triplet = cache.get('VCPKG_TARGET_TRIPLET', 'x64-windows')
    runtime_directory = installed_root / triplet
    if configuration == 'Debug':
        runtime_directory /= 'debug'
    runtime_directory /= 'bin'
    if not runtime_directory.is_dir():
        raise RuntimeError(f'vcpkg runtime directory was not found: {runtime_directory}')
    dependencies = sorted((path for path in runtime_directory.iterdir()
                           if path.is_file() and path.suffix.lower() == '.dll'), key=lambda p: p.name)
    library = build_directory / 'src/datoviz.dll'
    if not library.is_file():
        raise RuntimeError(f'Built Datoviz DLL was not found: {library}')
    executables = list((build_directory / 'testing').glob('*.exe'))
    executables.extend((build_directory / 'examples').rglob('*.exe'))
    destinations = {library.parent, *(path.parent for path in executables if path.is_file())}
    copies = 0
    for destination in sorted(destinations):
        sources = dependencies if destination == library.parent else [*dependencies, library]
        for source in sources:
            target = destination / source.name
            if source.resolve() != target.resolve():
                shutil.copy2(source, target)
                copies += 1
    return copies


def main(argv=None):
    """Configure the shared Ninja build and optionally build and stage its DLLs."""
    parser = argparse.ArgumentParser(description=__doc__, epilog=(
        'DVZ_CMAKE_ARGS uses POSIX-style quoting, like the Linux recipes. Use forward slashes '
        'in Windows paths, and quote values containing spaces. DVZ_BUILD_JOBS sets parallelism.'))
    parser.add_argument('configuration', choices=('Debug', 'Release'))
    parser.add_argument('--configure-only', action='store_true')
    args = parser.parse_args(argv)
    try:
        if not IS_WINDOWS:
            raise RuntimeError('This helper requires Windows with MSVC and Ninja.')
        if importlib.util.find_spec('yaml') is None:
            raise RuntimeError(f'The selected Python interpreter ({sys.executable}) needs PyYAML. '
                               'Install it with this interpreter using -m pip install PyYAML.')
        environment = initialize_msvc(os.environ)
        vcpkg_root, toolchain = find_vcpkg(environment)
        environment['VCPKG_ROOT'] = str(vcpkg_root)
        root = Path(__file__).resolve().parents[1]
        build_directory = root / 'build'
        guard_cache(build_directory, root)
        extras = cmake_arguments(environment_value(environment, 'DVZ_CMAKE_ARGS'))
        jobs = environment_value(environment, 'DVZ_BUILD_JOBS')
        if jobs and (not jobs.isdigit() or int(jobs) < 1):
            raise RuntimeError('DVZ_BUILD_JOBS must be a positive integer.')
        configure = [
            'cmake', '--preset', 'msvc-ninja', *extras,
            f'-DCMAKE_BUILD_TYPE={args.configuration}', f'-DPython3_EXECUTABLE={sys.executable}',
            f'-DCMAKE_TOOLCHAIN_FILE={toolchain}', '-DVCPKG_TARGET_TRIPLET=x64-windows',
        ]
        subprocess.run(configure, cwd=root, env=environment, check=True)
        guard_cache(build_directory, root)
        if args.configure_only:
            return 0
        build = ['cmake', '--build', str(build_directory)]
        if jobs:
            build.extend(['--parallel', jobs])
        subprocess.run(build, cwd=root, env=environment, check=True)
        copies = stage_runtime(build_directory, args.configuration)
        print(f'Built {args.configuration}; staged {copies} runtime DLL copies.')
        return 0
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as exc:
        print(f'Windows build failed: {exc}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
