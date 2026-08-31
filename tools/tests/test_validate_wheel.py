"""Tests for installed-wheel validation isolation."""

from __future__ import annotations

import os
import subprocess
import unittest
from pathlib import Path
from unittest import mock

from tools.datoviz_build_backend import validate


class InstalledWheelIsolationTests(unittest.TestCase):
    def test_venv_env_removes_checkout_overrides(self) -> None:
        poison = {
            "PYTHONPATH": "/tmp/source-python",
            "PYTHONHOME": "/tmp/source-home",
            "DATOVIZ_LIBRARY": "/tmp/source-libdatoviz.so",
            "DATOVIZ_DEV_ROOT": "/tmp/source-root",
            "DVZ_WHEEL_RUNTIME_DIRS": "/tmp/runtime",
            "DVZ_SHADERC_RUNTIME_LIBRARY": "/tmp/libshaderc.so",
            "DVZ_VULKAN_LOADER_LIBRARY": "/tmp/libvulkan.so",
            "LD_LIBRARY_PATH": "/tmp/linux-libs",
            "DYLD_LIBRARY_PATH": "/tmp/macos-libs",
            "DYLD_FALLBACK_LIBRARY_PATH": "/tmp/macos-fallback-libs",
            "PATH": "/tmp/bin",
        }
        with mock.patch.dict(os.environ, poison, clear=True):
            env = validate._venv_env()

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
            self.assertNotIn(name, env)
        self.assertEqual(env["PATH"], "/tmp/bin")
        self.assertEqual(env["PIP_USER"], "false")
        self.assertEqual(env["PYTHONNOUSERSITE"], "1")

    def test_run_uses_isolated_environment_by_default(self) -> None:
        expected = {"PATH": "/tmp/bin"}
        with (
            mock.patch.object(validate, "_venv_env", return_value=expected),
            mock.patch.object(subprocess, "run") as run,
        ):
            validate._run(["tool", "arg"], cwd=Path("/tmp"))

        run.assert_called_once_with(
            ["tool", "arg"], cwd=Path("/tmp"), env=expected, check=True
        )

    def test_create_venv_clears_reused_environment(self) -> None:
        with mock.patch.object(subprocess, "run") as run:
            validate._create_venv(Path("/tmp/reused-venv"))

        command = run.call_args.args[0]
        self.assertEqual(command[:4], [validate.sys.executable, "-m", "venv", "--clear"])
        self.assertEqual(command[-1], "/tmp/reused-venv")


if __name__ == "__main__":
    unittest.main()
