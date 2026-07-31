#!/usr/bin/env python3
"""Test the sourced Vulkan SDK environment helper."""

from __future__ import annotations

import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / 'tools/vulkan-env.sh'


class VulkanEnvironmentTest(unittest.TestCase):
    """Validate default and strict macOS environment selection."""

    def _source_macos_environment(self, sanitize: bool) -> tuple[dict[str, str], str]:
        with tempfile.TemporaryDirectory() as tmp:
            temp_dir = Path(tmp)
            sdk_dir = temp_dir / 'VulkanSDK/1.4.0/macOS'
            (sdk_dir / 'bin').mkdir(parents=True)
            (sdk_dir / 'lib').mkdir()
            (sdk_dir / 'share/vulkan/explicit_layer.d').mkdir(parents=True)
            icd_path = sdk_dir / 'share/vulkan/icd.d/MoltenVK_icd.json'
            icd_path.parent.mkdir(parents=True)
            icd_path.write_text('{}\n', encoding='utf-8')

            fake_bin = temp_dir / 'fake-bin'
            fake_bin.mkdir()
            uname = fake_bin / 'uname'
            uname.write_text('#!/usr/bin/env sh\nprintf "Darwin\\n"\n', encoding='utf-8')
            uname.chmod(0o755)

            env = os.environ.copy()
            env.update(
                {
                    'PATH': f'{fake_bin}:{env["PATH"]}',
                    'VULKAN_SDK': str(sdk_dir),
                    'VK_ICD_FILENAMES': '/stale/icd.json',
                    'VK_DRIVER_FILES': '/stale/driver.json',
                    'VK_ADD_DRIVER_FILES': '/stale/add-driver.json',
                    'VK_LOADER_DRIVERS_SELECT': 'stale-select',
                    'VK_LOADER_DRIVERS_DISABLE': 'stale-disable',
                    'VK_ROOT': '/stale/vulkan-root',
                    'DYLD_LIBRARY_PATH': '/stale/dyld',
                    'DYLD_FALLBACK_LIBRARY_PATH': '/stale/fallback',
                }
            )
            if sanitize:
                env['DVZ_VULKAN_SANITIZE_ENV'] = '1'
            else:
                env.pop('DVZ_VULKAN_SANITIZE_ENV', None)

            bash = shutil.which('bash')
            self.assertIsNotNone(bash)
            result = subprocess.run(  # noqa: S603
                [bash, '-c', 'source "$1"; env -0', 'bash', str(SCRIPT)],
                check=True,
                capture_output=True,
                env=env,
            )

            resolved = {}
            for entry in result.stdout.decode().split('\0'):
                if entry:
                    name, value = entry.split('=', 1)
                    resolved[name] = value
            return resolved, result.stderr.decode()

    def test_default_mode_preserves_existing_selection_environment(self) -> None:
        """Default mode merges SDK paths without removing inherited selectors."""
        env, stderr = self._source_macos_environment(sanitize=False)
        sdk_dir = Path(env['VULKAN_SDK'])

        self.assertEqual(env['VK_DRIVER_FILES'], '/stale/driver.json')
        self.assertEqual(env['VK_ADD_DRIVER_FILES'], '/stale/add-driver.json')
        self.assertEqual(env['VK_LOADER_DRIVERS_SELECT'], 'stale-select')
        self.assertEqual(env['VK_LOADER_DRIVERS_DISABLE'], 'stale-disable')
        self.assertEqual(env['VK_ROOT'], '/stale/vulkan-root')
        self.assertEqual(env['DYLD_LIBRARY_PATH'], f'{sdk_dir}/lib:/stale/dyld')
        self.assertEqual(
            env['DYLD_FALLBACK_LIBRARY_PATH'], f'{sdk_dir}/lib:/stale/fallback'
        )
        self.assertNotIn('sanitizing conflicting', stderr)

    def test_strict_mode_selects_only_the_active_sdk(self) -> None:
        """Strict mode removes stale selectors and direct library paths."""
        env, stderr = self._source_macos_environment(sanitize=True)
        sdk_dir = Path(env['VULKAN_SDK'])

        self.assertEqual(
            env['VK_ICD_FILENAMES'],
            str(sdk_dir / 'share/vulkan/icd.d/MoltenVK_icd.json'),
        )
        self.assertEqual(
            env['VK_LAYER_PATH'], str(sdk_dir / 'share/vulkan/explicit_layer.d')
        )
        self.assertEqual(env['DYLD_FALLBACK_LIBRARY_PATH'], str(sdk_dir / 'lib'))
        self.assertNotIn('DYLD_LIBRARY_PATH', env)
        for name in (
            'VK_DRIVER_FILES',
            'VK_ADD_DRIVER_FILES',
            'VK_LOADER_DRIVERS_SELECT',
            'VK_LOADER_DRIVERS_DISABLE',
            'VK_ROOT',
        ):
            self.assertNotIn(name, env)
        self.assertIn('sanitizing conflicting macOS Vulkan environment', stderr)


if __name__ == '__main__':
    unittest.main()
