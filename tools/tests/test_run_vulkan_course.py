from __future__ import annotations

import os
from pathlib import Path
import subprocess
import shutil
import sys
import tempfile
import unittest

TOOLS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS))

import run_vulkan_course


@unittest.skipUnless(shutil.which("cmake") and shutil.which("ninja"), "requires CMake and Ninja")
class InstalledCourseBuildTests(unittest.TestCase):
    def _build_and_run(self, generator: str | None) -> None:
        with tempfile.TemporaryDirectory(prefix="course consumer with spaces ") as name:
            root = Path(name)
            source_dir = root / "source files é"
            source_dir.mkdir()
            source = source_dir / "step01.c"
            source.write_text("int main(void) { return 0; }\n")
            for step in run_vulkan_course.STEPS[1:]:
                (source_dir / f"{step.name}.c").write_text("int main(void) { return 0; }\n")

            package = root / "fake package"
            package.mkdir()
            (package / "datovizConfig.cmake").write_text(
                "add_library(datoviz::datoviz INTERFACE IMPORTED)\n"
            )

            old_sources = run_vulkan_course.SOURCES
            run_vulkan_course.SOURCES = source_dir
            try:
                discovery = [f"-DCMAKE_PREFIX_PATH={package}"]
                if generator:
                    discovery = ["-G", generator, *discovery]
                consumer = root / "consumer"
                consumer.mkdir()
                build = run_vulkan_course._installed_build(consumer, discovery)
            finally:
                run_vulkan_course.SOURCES = old_sources

            executable = build / "step01"
            if os.name == "nt":
                executable = executable.with_suffix(".exe")
            self.assertTrue(executable.is_file(), executable)
            subprocess.run([str(executable)], check=True)

    def test_single_config_generator_accepts_spaces(self) -> None:
        self._build_and_run("Ninja")

    def test_multi_config_generator_builds_release_location(self) -> None:
        self._build_and_run("Ninja Multi-Config")


if __name__ == "__main__":
    unittest.main()
