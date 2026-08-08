#!/usr/bin/env python3
"""Test native shader-tool configuration policy."""

from __future__ import annotations

import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


class ShaderCMakeTest(unittest.TestCase):
    """Validate missing and explicitly configured glslc behavior."""

    def _run_cmake(self, *args: str, hide_glslc: bool = False) -> subprocess.CompletedProcess[str]:
        cmake = shutil.which('cmake')
        self.assertIsNotNone(cmake)
        env = os.environ.copy()
        command = [cmake, '-S', str(ROOT)]

        with tempfile.TemporaryDirectory() as tmp:
            command.extend(['-B', tmp])
            if hide_glslc:
                ignored_dirs = []
                sdk = env.pop('VULKAN_SDK', '')
                env.pop('DVZ_GLSLC', None)
                candidates = env.get('PATH', '').split(os.pathsep)
                if sdk:
                    candidates.extend([str(Path(sdk) / 'bin'), str(Path(sdk) / 'Bin')])
                for candidate in candidates:
                    directory = Path(candidate)
                    if any((directory / name).is_file() for name in ('glslc', 'glslc.exe')):
                        resolved = str(directory.resolve())
                        if resolved not in ignored_dirs:
                            ignored_dirs.append(resolved)
                command.append(f'-DCMAKE_IGNORE_PATH={";".join(ignored_dirs)}')
            command.extend(args)
            return subprocess.run(  # noqa: S603
                command, capture_output=True, env=env, text=True, check=False
            )

    def test_invalid_explicit_glslc_path_fails_immediately(self) -> None:
        """An invalid explicit override reports the namespaced setting and exact path."""
        missing = '/definitely/missing/datoviz-glslc'
        result = self._run_cmake(f'-DDVZ_GLSLC_EXECUTABLE={missing}')
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Configured DVZ_GLSLC_EXECUTABLE does not exist', result.stderr)
        self.assertIn(missing, result.stderr)

    def test_scene_requires_a_build_or_runtime_compiler(self) -> None:
        """Scene configuration fails when neither glslc nor runtime shaderc can work."""
        result = self._run_cmake(
            '-DDVZ_BUILD_TESTING=OFF',
            '-DDVZ_BUILD_SCENE=ON',
            '-DDVZ_BUILD_APP=OFF',
            '-DDVZ_BUILD_GUI=OFF',
            '-DDVZ_ENABLE_SHADERC=OFF',
            hide_glslc=True,
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(
            'DVZ_BUILD_SCENE=ON requires either glslc for embedded SPIR-V or runtime shaderc',
            ' '.join(result.stderr.split()),
        )

    def test_native_test_fixtures_require_glslc(self) -> None:
        """Native fixture requirements are diagnosed before subdirectories configure."""
        result = self._run_cmake(
            '-DDVZ_BUILD_TESTING=ON',
            '-DDVZ_BUILD_SCENE=OFF',
            '-DDVZ_BUILD_APP=OFF',
            '-DDVZ_BUILD_GUI=OFF',
            '-DDVZ_ENABLE_SHADERC=OFF',
            hide_glslc=True,
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('glslc is required by native test fixtures', result.stderr)


if __name__ == '__main__':
    unittest.main()
